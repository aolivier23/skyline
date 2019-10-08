//File: CmdLine.h
//Brief: A CmdLine parses the command line for exactly 1 configuration file and converts it into
//       a list of materials, a list of aabbs, and a list of CameraModels.  With any other number
//       of arguments, -h, --help, or a YAML file it can't parse, it throws an exception with
//       usage information.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//c++ includes
#include <exception>
#include <vector>

//OpenCL c++ API includes
#include <CL/cl.hpp>

//serial includes
#define NOT_ON_DEVICE
#include "serial/vector.h"
#include "serial/aabb.h"
#include "serial/material.h"

//camera includes
#include "camera/CameraModel.h"

namespace app
{
  class CmdLine
  {
    public:
      CmdLine(const int argc, const char** argv);

      std::vector<material> materials;
      std::vector<aabb> boxes;
      aabb skybox;
      std::vector<eng::CameraModel> cameras;

      //Custom exception class to explain why the command line couldn't be parsed.
      class exception: public std::runtime_error
      {
        public:
          exception(const std::string& why);
          virtual ~exception() = default;
      };
  };
}
