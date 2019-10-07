//File: oneCell.cpp
//Brief: Render a room with a few boxes via path tracing.
//       Proof of concept for skyline.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>
#include <CL/cl_gl.h>

//GLAD includes
#include "glad/include/glad/glad.h"

//GLFW includes
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include "GLFW/glfw3native.h"

//gl includes
#include "gl/Framebuffer.h"

//camera includes
#include "camera/WithCamera.h"
#include "camera/CameraModel.h"
#include "camera/FPSController.h"

//serial includes
#define NOT_ON_DEVICE
#include "serial/vector.h"
#include "serial/aabb.h"
#include "serial/material.h"

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
    GL_SETUP_ERROR,
    CL_SETUP_ERROR,
    CL_COMPILE_ERROR,
    CL_RUN_ERROR
  };

  class changeWithWindowSize: public eng::WithCamera
  {
    public:
      changeWithWindowSize(GLFWwindow* window, cl::Context& ctx,
                           std::unique_ptr<eng::CameraController>&& camera): WithCamera(window, ctx, std::move(camera))
      {
        std::vector<size_t> hostSeeds(fWidth*fHeight);
        for(size_t id = 1; id <= fWidth*fHeight; ++id) hostSeeds[id] = id;
        seeds = cl::Buffer(ctx, hostSeeds.begin(), hostSeeds.end(), false);
  
        nIterations = 0;
      }
  
      cl::Buffer seeds;
      size_t nIterations; //TODO: Store this in the a component of every texture entry?
  
      void userResize(const int width, const int height)
      {
        std::vector<size_t> hostSeeds(width*height);
        for(size_t id = 1; id <= width*height; ++id) hostSeeds[id] = id;
        seeds = cl::Buffer(fContext, hostSeeds.begin(), hostSeeds.end(), false);
        
        nIterations = 0;
      };

    private:
      virtual void onCameraChange() override
      {
        nIterations = 10; //Weight the Scene starts with when the camera moves.  Basically, making this number
                          //larger causes the scene to blur more rather than be disrupted by bad sampling.
      }
  };
}

int main(const int argc, const char** argv)
{
  //Set up OpenGL context via GLFW
  if(!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW for window system with OpenGL context!  There's nothing I"
              << " can do to recover, so returning with an error.\n";
    return GL_SETUP_ERROR;
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
    return GL_SETUP_ERROR;
  }

  glfwMakeContextCurrent(window);

  //Initialize glad to get opengl extensions.  
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if(!gladLoadGL())
  {
    std::cerr << "Failed to load OpenGL functions with GLAD.  I won't be able to render anything, so"
              << " returning an error.\n";
    return GL_SETUP_ERROR;
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
    return CL_SETUP_ERROR;
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

  std::ifstream materialHeader(INSTALL_DIR "/include/serial/material.h");
  std::istreambuf_iterator<char> materialBegin(materialHeader);
  source.append(materialBegin, fileEnd);
  
  std::ifstream random(INSTALL_DIR "/include/kernels/linearCongruential.cl");
  std::istreambuf_iterator<char> randomBegin(random);
  source.append(randomBegin, fileEnd);

  std::ifstream main(INSTALL_DIR "/include/kernels/pathTrace.cl");
  std::istreambuf_iterator<char> mainBegin(main);
  source.append(mainBegin, fileEnd);

  #ifndef NDEBUG
  std::cout << "Kernel source is:\n" << source << "\n";
  #endif

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
      return CL_COMPILE_ERROR;
    }
    
    std::cerr << "Got unrecognized error when building OpenCL program: " << e.err() << ": " << e.what() << "\n";
    return CL_COMPILE_ERROR;
  }

  auto pathTrace = cl::make_kernel<cl::ImageGL, cl::Sampler, cl::Image2D, cl::Buffer, size_t, cl::Buffer, cl::Buffer, cl_float3, cl_float3, cl_float3, cl_float3, size_t, cl::Buffer, size_t>(cl::Kernel(program, "pathTrace"));

  //Set up viewport
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  //Set up "engine" that manages user interaction and exposes result through
  //public member functions.
  double initX = 0., initY = 0.;
  glfwGetCursorPos(window, &initX, &initY);
  auto model = std::make_unique<eng::CameraModel>(cl::float3{0., 0.2, 0.9}, cl::float3{0., 0.2, 0.0});
  ::changeWithWindowSize change(window, ctx,
                                std::make_unique<eng::FPSController>(std::move(model), 0.05, 0.02, initX, initY));

  //Set up geometry to send to the GPU
  //TODO: Replace this with a YAML file
  std::vector<material> boxMaterials;
  material lightMaterial;
  lightMaterial.color = cl_float3{0., 0., 0.};
  lightMaterial.emission = cl_float3{9., 8., 6.};
  boxMaterials.push_back(lightMaterial);

  material boxMaterial;
  boxMaterial.color = cl_float3{145./256., 145./256., 120./256.};
  boxMaterial.emission = cl_float3{0., 0., 0.};
  boxMaterials.push_back(boxMaterial);

  material wallMaterial;
  wallMaterial.color = cl_float3{1., 165./256., 0.};
  wallMaterial.emission = cl_float3{0., 0., 0.};
  boxMaterials.push_back(wallMaterial);

  std::vector<aabb> boxes;
  aabb lightBox;
  lightBox.width = cl_float3{0.5, 0.5, 0.5};
  lightBox.center = cl_float3{0., 1.05, -0.5};
  lightBox.material = 0;
  boxes.push_back(lightBox);

  aabb myFirstBox;
  myFirstBox.width = cl_float3{0.1, 0.2, 0.3};
  myFirstBox.center = cl_float3{-0.5, -1. + myFirstBox.width.y, -0.3};
  myFirstBox.material = 1;
  boxes.push_back(myFirstBox);

  aabb anotherBox;
  anotherBox.width = cl_float3{0.3, 0.6, 0.2};
  anotherBox.center = cl_float3{0.6, -1. + anotherBox.width.y, -0.1};
  anotherBox.material = 1;
  boxes.push_back(anotherBox);

  std::vector<aabb> skybox = {aabb{cl_float3{2., 2., 2.}, cl_float3{0., 0., 0.}, 2, 0, 0, 0}};

  cl::Buffer geometry(ctx, boxes.begin(), boxes.end(), false);
  cl::Buffer materials(ctx, boxMaterials.begin(), boxMaterials.end(), false);
  cl::Buffer skyboxes(ctx, skybox.begin(), skybox.end(), false); //If I want skybox to be in the global address space, it has to be a buffer
  cl::Sampler sampler(ctx, false, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST);

  //Render loop that calls OpenCL kernel
  while(!glfwWindowShouldClose(window))
  {
    try
    {
      std::vector<cl::Memory> mem = {*(change.glImage)};
      queue.enqueueAcquireGLObjects(&mem);
      pathTrace(cl::EnqueueArgs(queue, cl::NDRange(change.fWidth, change.fHeight)), *(change.glImage), sampler,
                    *(change.clImage), geometry, boxes.size(), materials, skyboxes, change.camera().position(),
                    change.camera().focalPlane(), change.camera().up(), change.camera().right(), 4, change.seeds,
                    ++change.nIterations);
      queue.finish();
      queue.enqueueReleaseGLObjects(&mem);
    }
    catch(const cl::Error& e)
    {
      std::cerr << "Caught an OpenCL error while running kernel for drawing:\n" << e.err() << ": " << e.what() << "\n";
      return CL_RUN_ERROR;
    }

    change.render(queue);
    glfwSwapBuffers(window);
    glfwPollEvents(); //TODO: This could cause the old OpenCL buffer to be thrown away.  Then, I'll be copying uninitialized memory
                      //      on the next frame.
  }

  glfwTerminate();
  return 0;
}