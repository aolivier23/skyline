//File: LoadIntoCL.cpp
//Brief: Functions to simplify loading headers into an OpenCL kernel.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//c++ includes
#include <fstream>
#include <algorithm>

#ifndef NDEBUG
#include <iostream>
#endif //NDEBUG

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>
#include <CL/cl_gl.h>

//GLFW includes
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include "GLFW/glfw3native.h"

//app includes
#include "app/LoadIntoCL.h"

namespace app
{
  //Put together an OpenCL Program from a kernel that uses 0 or
  //more include files.
  cl::Program constructSource(cl::Context& ctx, const std::string kernelName, const std::initializer_list<std::string> includes)
  {
    std::string source;
    std::istreambuf_iterator<char> end;

    for(const auto& file: includes)
    {
      std::ifstream fileStream(std::string(INSTALL_DIR) + "/include/" + file);
      std::istreambuf_iterator<char> begin(fileStream);
      source.append(begin, end);
    }

    std::ifstream fileStream(std::string(INSTALL_DIR) + "/include/" + kernelName);
    std::istreambuf_iterator<char> begin(fileStream);
    source.append(begin, end);

    return cl::Program(ctx, source);
  }

  //Choose the best OpenCL-capable GPU for rendering.
  std::pair<cl::Context, cl::Device> chooseDevice(GLFWwindow* window)
  { 
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
                                            #ifndef NDEBUG
                                            std::cout << "Can't create an OpenCL context that shares with OpenGL on "
                                                      << dev.getInfo<CL_DEVICE_NAME>() << "\n";
                                            #endif //NDEBUG
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

    //TODO: Throw an exception if I didn't find a good platform
    if(platform == platforms.end()) throw std::runtime_error("Couldn't find an OpenCL-capable GPU with the CL_GL_SHARING_EXT extension.");

    #ifndef NDEBUG
    std::cout << "Chose to use device named " << chosen.getInfo<CL_DEVICE_NAME>() << "\n";
    #endif //NDEBUG

    return std::make_pair(ctx, chosen);
  }
}
