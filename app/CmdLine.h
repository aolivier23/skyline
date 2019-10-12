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

      //User perspectives on the current scene
      std::vector<std::pair<std::string, eng::CameraModel>> cameras;

      //Application handles to data on the GPU
      inline const cl::Buffer& materials() const { return fDevMaterials; }
      inline const cl::Buffer& boxes() const { return fDevBoxes; }
      inline const cl::Buffer& skybox() const { return fDevSkybox; }
      inline size_t nBoxes() const { return fBoxes.size(); }

      //Custom exception class to explain why the command line couldn't be parsed.
      class exception: public std::runtime_error
      {
        public:
          exception(const std::string& why);
          virtual ~exception() = default;
      };

      //Upload host-side state to the GPU
      void sendToGPU(cl::Context& ctx);

    private:
      //Serialized data that's matched to metadata.  This is a copy of
      //the data on the GPU.
      std::vector<material> fMaterials;
      std::vector<aabb> fBoxes;
      aabb fSkybox;

      //Metadata with references to GPU-ready data
      std::map<std::string, int> nameToMaterialIndex;
      std::map<std::string, int> nameToBoxIndex;

      //Data on the GPU
      cl::Buffer fDevMaterials;
      cl::Buffer fDevBoxes;
      cl::Buffer fDevSkybox;
  };
}
