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

//yaml-cpp includes
#include "yaml-cpp/yaml.h"

namespace app
{
  class CmdLine
  {
    public:
      //Create an empty application state.
      CmdLine() = default;

      //Serialization and de-serialization
      YAML::Node load(const int argc, const char** argv);
      YAML::Node load(const std::string& fileName);

      YAML::Node write(const std::string& fileName);

      //Serialized data to upload to the GPU
      //TODO: Deny the user direct access to these objects so
      //      that metadata can't get out of sync.  Maybe write
      //      a function like uploadToGPU() instead?  I'll also
      //      need const accessors and functions to add new
      //      materials and boxes.
      std::vector<material> materials;
      std::vector<aabb> boxes;
      aabb skybox;

      //Metadata with references to GPU-ready data
      std::vector<std::pair<std::string, eng::CameraModel>> cameras;
      std::map<std::string, int> nameToMaterialIndex;
      std::map<std::string, int> nameToBoxIndex;

      //Custom exception class to explain why the command line couldn't be parsed.
      class exception: public std::runtime_error
      {
        public:
          exception(const std::string& why);
          virtual ~exception() = default;
      };
  };
}
