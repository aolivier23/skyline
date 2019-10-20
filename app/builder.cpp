//File: builder.cpp
//Brief: Edit a skyline geometry file with a GUI.  Also lets the user
//       explore the scene with the skyline engine and serialize the
//       current geometry into a file for skyline to consume.  See
//       CmdLine.cpp for usage.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>
#include <CL/cl_gl.h>

//GLAD includes
#include "glad/include/glad/glad.h"

//app includes
#include "app/CmdLine.h"
#include "app/GUI.h"

//algorithms borrowed from OpenCL kernel
#include "kernels/generateRay.cl"

//camera includes
#include "camera/FPSController.h"

//engine includes
#include "engine/WithRandomSeeds.h"

//imgui includes
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

//GLFW includes
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include "GLFW/glfw3native.h"

//gl includes
#include "gl/Framebuffer.h"

//c++ includes
#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory> //std::unique_ptr

namespace
{
  //Error codes returned to the operating system
  enum errorCode
  {
    SUCCESS = 0,
    CMD_LINE_ERROR,
    SETUP_ERROR,
    RENDER_ERROR
  };
}

int main(const int argc, const char** argv)
{
  //Set up OpenGL context via GLFW
  if(!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW for window system with OpenGL context!  There's nothing I"
              << " can do to recover, so returning with an error.\n";
    return SETUP_ERROR;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  #if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  #endif
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  auto window = glfwCreateWindow(800, 600, "HelloCL", nullptr, nullptr);
  if(window == nullptr)
  {
    std::cerr << "I managed to initialize GLFW, but I couldn't create a window with an OpenGL context."
              << "  There's nothing I can do, so returning with an error.\n";
    return SETUP_ERROR;
  }

  glfwMakeContextCurrent(window);

  //Initialize glad to get opengl extensions.  
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if(!gladLoadGL())
  {
    std::cerr << "Failed to load OpenGL functions with GLAD.  I won't be able to render anything, so"
              << " returning an error.\n";
    return SETUP_ERROR;
  }

  //Parse the command line for a configuration file.
  //This de-serializes the geometry to render and camera configurations.
  app::CmdLine params;

  try
  {
    params.load(argc, argv);
  }
  catch(const app::CmdLine::exception& e)
  {
    std::cerr << e.what();
    return CMD_LINE_ERROR;
  }

  //Get the first GPU device among all platforms that matches the current OpenGL context
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  std::vector<cl::Device> devices;

  cl::Device chosen;
  cl::Context ctx;
  auto platform = platforms.begin();
  for(; platform != platforms.end() /*&& devices.empty()*/; ++platform)
  {
    try
    {
      platform->getDevices(CL_DEVICE_TYPE_GPU, &devices);
    }
    catch(const cl::Error& e)
    {
      if(e.err() != CL_DEVICE_NOT_FOUND) throw e;
    }

    //FIXME: This part needs to change based on backend GLFW is using
    cl_context_properties prop[] = {
                                     CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetGLXContext(window),
                                     CL_GLX_DISPLAY_KHR, (cl_context_properties)glfwGetX11Display(),
                                     CL_CONTEXT_PLATFORM, (cl_context_properties)(*platform)(),
                                     0
                                   };

    //Check if this device has a current OpenGL context
    const auto found = std::find_if(devices.begin(), devices.end(), 
                                    [&ctx, &prop](const cl::Device& dev) 
                                    { 
                                      //Hack to exclude my Intel card which has weird drivers atm
                                      if(dev.getInfo<CL_DEVICE_VENDOR>().find("Intel") != std::string::npos) return false;


                                      const auto exts = dev.getInfo<CL_DEVICE_EXTENSIONS>();
                                      if(exts.find("cl_khr_gl_sharing") != std::string::npos)
                                      {
                                        cl_int status;
                                        ctx = cl::Context(dev, prop, NULL, NULL, &status);
                                        if(status != CL_SUCCESS)
                                        {
                                          std::cout << "Can't create an OpenCL context that shares with OpenGL on "
                                                    << dev.getInfo<CL_DEVICE_NAME>() << "\n";
                                          return false; //Device not compatible with OpenGL
                                        }
                                        return true; //Sucessfully created context
                                      }
                                      return false; //Device not compatible with sharing extension
                                    });
    //Learned to do this from https://github.com/9prady9/CLGLInterop/blob/master/examples/partsim.cpp
    if(found != devices.end())
    {
      chosen = *found;
      break; //platform is also implicitly set by breaking here
    }
  }

  if(platform == platforms.end())
  {
    std::cerr << "Couldn't find an OpenCL-capable GPU with the CL_GL_SHARING_EXT extension.\n";
    return SETUP_ERROR;
  }

  std::cout << "Chose to use device named " << chosen.getInfo<CL_DEVICE_NAME>() << "\n";

  cl::CommandQueue queue(ctx, chosen);

  //Create the OpenCL kernel from installed kernels
  std::ifstream macros(INSTALL_DIR "/include/serial/vector.h");
  std::istreambuf_iterator<char> macroBegin(macros), fileEnd;
  std::string source(macroBegin, fileEnd);

  std::ifstream rayHeader(INSTALL_DIR "/include/serial/ray.h");
  std::istreambuf_iterator<char> rayBegin(rayHeader);
  source.append(rayBegin, fileEnd);

  std::ifstream header(INSTALL_DIR "/include/serial/aabb.h");
  std::istreambuf_iterator<char> headerBegin(header);
  source.append(headerBegin, fileEnd);

  std::ifstream functions(INSTALL_DIR "/include/serial/aabb.cpp");
  std::istreambuf_iterator<char> functionsBegin(functions);
  source.append(functionsBegin, fileEnd);

  std::ifstream materialHeader(INSTALL_DIR "/include/serial/material.h");
  std::istreambuf_iterator<char> materialBegin(materialHeader);
  source.append(materialBegin, fileEnd);
  
  std::ifstream random(INSTALL_DIR "/include/kernels/linearCongruential.cl");
  std::istreambuf_iterator<char> randomBegin(random);
  source.append(randomBegin, fileEnd);

  std::ifstream generator(INSTALL_DIR "/include/kernels/generateRay.cl");
  std::istreambuf_iterator<char> generatorBegin(generator);
  source.append(generatorBegin, fileEnd);

  //std::ifstream main(INSTALL_DIR "/include/kernels/pathTrace.cl");
  std::ifstream main(INSTALL_DIR "/include/kernels/reuseFirstBounce.cl");
  std::istreambuf_iterator<char> mainBegin(main);
  source.append(mainBegin, fileEnd);

  /*#ifndef NDEBUG
  std::cout << "Kernel source is:\n" << source << "\n";
  #endif*/

  cl::Program program(ctx, source);

  //Build OpenCL program
  try
  {
    program.build();
  }
  catch(const cl::Error& e)
  {
    const auto status = program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(chosen);
    if(status == CL_BUILD_ERROR)
    {
      const auto name = chosen.getInfo<CL_DEVICE_NAME>();
      const auto log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(chosen);
      std::cerr << "The program for device " << name << " failed because:\n" << log << "\n";
      return SETUP_ERROR;
    }
    
    std::cerr << "Got unrecognized error when building OpenCL program: " << e.err() << ": " << e.what() << "\n";
    return SETUP_ERROR;
  }

  auto pathTrace = cl::make_kernel<cl::ImageGL, cl::Sampler, cl::Image2D, cl::Buffer, size_t, cl::Buffer, cl::Buffer, cl_float3, cl_float3, cl_float3, cl_float3, int, cl::Buffer, int, int, cl::ImageGL, cl::Sampler>(cl::Kernel(program, "pathTrace"));

  //Set up viewport
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  //Set up "engine" that manages user interaction and exposes result through
  //public member functions.
  double initX = 0., initY = 0.;
  glfwGetCursorPos(window, &initX, &initY);
  eng::WithRandomSeeds change(window, ctx,
                              std::make_unique<eng::FPSController>(params.cameras.front().second, 0.05, 0.02, initX, initY));

  //Set up Dear ImGui
  //N.B.: This has to happen after changeWithWindowSize is constructed to override the GLFW keyboard handler.
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 420");

  //Set up geometry to send to the GPU.  It was read in from the command line in a file.
  params.sendToGPU(ctx);
  cl::Sampler sampler(ctx, false, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
              textureSampler(ctx, true, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR);

  //Selection state
  std::unique_ptr<app::CmdLine::selected> selection;

  //Render loop that calls OpenCL kernel
  while(!glfwWindowShouldClose(window))
  {
    //Set up Dear ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //Run the skyline engine
    try
    {
      std::vector<cl::Memory> mem = {*(change.glImage), params.textures()};
      queue.enqueueAcquireGLObjects(&mem);
      pathTrace(cl::EnqueueArgs(queue, cl::NDRange(change.fWidth, change.fHeight)), *(change.glImage), sampler,
                    *(change.clImage), params.boxes(), params.nBoxes(), params.materials(), params.skybox(), change.camera().position().data,
                    change.camera().focalPlane().data, change.camera().up().data, change.camera().right().data, change.nBounces(), change.seeds(),
                    ++change.nIterations(), change.nSamples(), params.textures(), textureSampler);

      if(!io.WantCaptureMouse)
      {
        if(ImGui::IsMouseDoubleClicked(0))
        {
          const auto pos = ImGui::GetMousePos();
          //GLFW's pixels have the reverse convention of OpenGL textures in the y direction.  So, I have
          //to flip pos.y before using it with generateRay().
          const auto fromCamera = generateRay(cl::int2{pos.x, abs(pos.y - change.fHeight)}, change.camera().exactPosition(),
                                              change.camera().focalPlane(), change.camera().up(),
                                              change.camera().right(), change.fWidth, change.fHeight);

          //TODO: CTRL + click for multi-selection
          selection = std::move(params.select(fromCamera));

          std::cout << "Selected a box named " << selection->name << "\n";
          params.sendToGPU(ctx);
          change.onCameraChange();
        }
        else app::handleCamera(change, io);
      }

      //Draw GUI while kernel is running
      if(ImGui::BeginMainMenuBar())
      {
        if(app::drawFile(params))
        {
          params.sendToGPU(ctx);
          change.onCameraChange();
        }
        //TODO: edit menu with materials and skybox options
        app::drawCameras(params, change);
        app::drawMetrics(io);
        app::drawHelp();
        if(app::drawEngine(change)) change.onCameraChange();
        ImGui::EndMainMenuBar();

        if(selection)
        {
          if(app::editBox(selection))
          {
            //Only update GPU data if something changed.
            params.sendToGPU(ctx);
            change.onCameraChange();
          }

          //TODO: Material editor logic.  Maybe good enough to just pass change and app into editBox()?  Make sure to put it in the if above.
        }
      }

      queue.finish();
      queue.enqueueReleaseGLObjects(&mem);
    }
    catch(const cl::Error& e)
    {
      std::cerr << "Caught an OpenCL error while running kernel for drawing:\n" << e.err() << ": " << e.what() << "\n";
      return RENDER_ERROR;
    }

    change.render(queue);

    //Render DearImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents(); //TODO: This could cause the old OpenCL buffer to be thrown away.  Then, I'll be copying uninitialized memory
                      //      on the next frame.
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return SUCCESS;
}
