//File: LoadIntoCL.h
//Brief: Functions to simplify loading headers into an OpenCL kernel.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef APP_LOADINTOCL_H
#define APP_LOADINTOCL_H

//c++ includes
#include <initializer_list>

namespace cl
{
  class Context;
  class Device;
}

class GLFWwindow;

namespace app
{
  //Put together an OpenCL Program from a kernel that uses 0 or
  //more include files.
  cl::Program constructSource(cl::Context& ctx, const std::string kernelName, const std::initializer_list<std::string> includes);

  //Choose the best OpenCL-capable GPU for rendering.
  std::pair<cl::Context, cl::Device> chooseDevice(GLFWwindow* window);
}

#endif //APP_LOADINTOCL_H
