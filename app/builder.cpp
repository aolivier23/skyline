//File: builder.cpp
//Brief: Edit a skyline geometry file with a GUI.  Also lets the user
//       explore the scene with the skyline engine and serialize the
//       current geometry into a file for skyline to consume.  See
//       Geometry.cpp for usage.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//TODO: Remove redundant includes
//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>
#include <CL/cl_gl.h>

//GLAD includes
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "glad/include/glad/glad.h"

//app includes
#include "app/Geometry.h"
#include "app/GUI.h"
#include "app/LoadIntoCL.h"

//algorithms borrowed from OpenCL kernel
#include "serial/camera.cpp"

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
  try //Look for OpenCL errors in the whole program and print error codes for lookup
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
    auto window = glfwCreateWindow(1500, 1000, "Skyline Builder", glfwGetPrimaryMonitor(), nullptr); //nullptr, nullptr); //800, 600, "Skyline Builder", nullptr, nullptr);
    if(window == nullptr)
    {
      std::cerr << "I managed to initialize GLFW, but I couldn't create a window with an OpenGL context."
                << "  There's nothing I can do, so returning with an error.\n";
      return SETUP_ERROR;
    }
    //glfwMaximizeWindow(window);

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
    app::Geometry geom;

    try
    {
      geom.load(argc, argv);
    }
    catch(const app::Geometry::exception& e)
    {
      std::cerr << e.what();
      return CMD_LINE_ERROR;
    }

    //Get the first GPU device among all platforms that matches the current OpenGL context
    auto [ctx, chosen] = app::chooseDevice(window);
    cl::CommandQueue queue(ctx, chosen);

    //Create the OpenCL kernel from installed kernels
    auto program = app::constructSource(ctx, "kernels/skyline.cl",
                                        {"serial/vector.h",
                                         "serial/ray.h",
                                         "serial/material.h",
                                         "serial/aabb.h",
                                         "serial/aabb.cpp",
                                         "serial/sphere.h",
                                         "serial/sphere.cpp",
                                         "serial/groundPlane.h",
                                         "serial/groundPlane.cpp",
                                         "serial/grid.h",
                                         "serial/grid.cpp",
                                         "serial/gridCell.h",
                                         "serial/gridCell.cpp",
                                         "kernels/linearCongruential.cl",
                                         "serial/camera.h",
                                         "serial/camera.cpp"
                                        });

    //Build OpenCL program
    try
    {
      //TODO: Make this optional debug information?  Do this only if NVIDIA compiler options supported?
      program.build("-cl-nv-verbose -cl-nv-maxrregcount=64"); //This is how I get the NVIDIA compiler to print out resources my kernel uses
                                                              //TODO: Limiting the registers per thread to 64 makes my kernel a little faster?!
                                                              //      Setting 65536, the number of registers per block = compute unit, places no limit
                                                              //      and currently gets me stuck at 83.
      std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(chosen);
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

    auto pathTrace = cl::make_kernel<cl::ImageGL, cl::Sampler, cl::Image2D, cl::Buffer, cl::Buffer, cl::LocalSpaceArg, int, cl::Buffer, grid, cl::Buffer, sphere, sphere, cl_float3, cl_float2, camera, int, cl::Buffer, int, int, cl::ImageGL, cl::Sampler>(cl::Kernel(program, "pathTrace"));

    //Set up viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    //Set up "engine" that manages user interaction and exposes result through
    //public member functions.
    double initX = 0., initY = 0.;
    glfwGetCursorPos(window, &initX, &initY);
    eng::WithRandomSeeds change(window, ctx,
                                std::make_unique<eng::FPSController>(geom.cameras.front().second, 0.05, 0.02, initX, initY));

    //Set up Dear ImGui
    //N.B.: This has to happen after changeWithWindowSize is constructed to override the GLFW keyboard handler.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();

    //Control what the GUI looks like.
    ImGui::StyleColorsDark(); //Window background color
    ImFontConfig biggerFont;
    biggerFont.SizePixels = 18; //Font size in pixels
    io.Fonts->AddFontDefault(&biggerFont);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 420");

    //Set up geometry to send to the GPU.  It was read in from the command line in a file.
    geom.sendToGPU(ctx);
    cl::Sampler sampler(ctx, false, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
                textureSampler(ctx, true, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR);

    //Selection state
    std::unique_ptr<app::Geometry::selected> selection;

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
        std::vector<cl::Memory> mem = {*(change.glImage), geom.textures()};
        queue.enqueueAcquireGLObjects(&mem);
        std::cout << "Using " << std::gcd(change.fWidth, change.fHeight) << " work groups, which is the GCD of " << change.fWidth << " and " << change.fHeight << ".\n";
        pathTrace(cl::EnqueueArgs(queue, cl::NDRange(change.fWidth, change.fHeight), cl::NDRange(std::gcd(change.fWidth, change.fHeight), 1)),
                  *(change.glImage), sampler, *(change.clImage),
                  geom.boxes(), geom.gridIndices(), geom.localGridIndices(), geom.nGridIndices(), geom.gridCells(),
                  geom.gridSize(), geom.materials(),
                  geom.sky(), geom.sun(), geom.sunEmission().data,
                  geom.groundTexNorm().data, change.camera().state(),
                  change.nBounces(), change.seeds(), ++change.nIterations(),
                  change.nSamples(), geom.textures(), textureSampler);

        if(!io.WantCaptureMouse)
        {
          if(ImGui::IsMouseDoubleClicked(0))
          {
            const auto pos = ImGui::GetMousePos();
            //GLFW's pixels have the reverse convention of OpenGL textures in the y direction.  So, I have
            //to flip pos.y before using it with generateRay().
            size_t seed = 0; //I don't care about what random subpixel jitter I apply here
            const auto fromCamera = generateRay(change.camera().state(), cl::int2{pos.x, abs(pos.y - change.fHeight)},
                                                change.fWidth, change.fHeight, &seed);

            //TODO: Check whether I'm clicking on the sun
            //TODO: CTRL + click for multi-selection
            selection = std::move(geom.select(fromCamera));

            geom.sendToGPU(ctx);
            change.onCameraChange();
          }
          else app::handleCamera(change, io);
        }

        //Draw GUI while kernel is running
        if(ImGui::BeginMainMenuBar())
        {
          if(app::drawFile(geom))
          {
            geom.sendToGPU(ctx);
            change.onCameraChange();
          }
          //TODO: edit menu with materials and skybox options
          app::drawCameras(geom, change);
          app::drawMetrics(io);
          app::drawHelp();

          if(app::drawGrid(geom)) geom.sendToGPU(ctx);
          if(app::drawBackground(geom)) change.onCameraChange();
          if(app::drawEngine(change)) change.onCameraChange();
          ImGui::EndMainMenuBar();

          if(selection)
          {
            if(app::editBox(selection, geom))
            {
              //Only update GPU data if something changed.
              geom.sendToGPU(ctx);
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
  }
  catch(const cl::Error& e)
  {
    std::cerr << "Caught an OpenCL error somewhere outside of program building and kernel running:\n" << e.err() << ": " << e.what() << "\n";
    return SETUP_ERROR;
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return SUCCESS;
}
