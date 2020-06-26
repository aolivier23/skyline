//File: Geometry.h
//Brief: A Geometry parses the command line for exactly 1 configuration file and converts it into
//       a list of materials, a list of aabbs, and a list of CameraModels.  With any other number
//       of arguments, -h, --help, or a YAML file it can't parse, it throws an exception with
//       usage information.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef APP_CMDLINE_H
#define APP_CMDLINE_H

//c++ includes
#include <exception>
#include <vector>
#include <tuple>

//OpenCL c++ API includes
#include <CL/cl.hpp>

//serial includes
#define NOT_ON_DEVICE
#include "serial/vector.h"
#include "serial/ray.h"
#include "serial/material.h"
#include "serial/aabb.h"
#include "serial/sphere.h"
#include "serial/grid.h"
#include "serial/gridCell.h"

//camera includes
#include "camera/CameraModel.h"

//gl includes
#include "gl/TextureArray.cpp"

//yaml-cpp includes
#include "yaml-cpp/yaml.h"

namespace app
{
  //TODO: Rename me to something like Geometry and put command line parsing and usage information into a separate function-only header.
  class Geometry
  {
    public:
      //Create an empty application state.
      //TODO: Initialize with default.yaml geometry
      Geometry() = default;

      //Serialization and de-serialization
      YAML::Node load(const int argc, const char** argv);
      YAML::Node load(const std::string& fileName);

      YAML::Node write(const std::string& fileName);

      //User perspectives on the current scene
      std::vector<std::pair<std::string, eng::CameraModel>> cameras;

      //Application handles to data on the GPU
      inline const cl::Buffer& materials() const { return fDevMaterials; }
      inline const cl::Buffer& boxes() const { return fDevBoxes; }
      inline const cl::LocalSpaceArg& localBoxes() const { return fDevLocalBoxes; }
      inline sphere& sky() { return fSky; }
      inline sphere& sun() { return fSun; }
      inline cl::float2& groundTexNorm() { return fGroundTexNorm; }
      inline const std::string& groundFile() const { return groundTextureFile; }
      inline const std::string& skyFile() const { return skyTextureFile; }
      inline cl::float3& sunEmission() { return fSunEmission; }
      inline const cl::ImageGL& textures() const { return fDevTextures; }
      inline size_t nBoxes() const { return fBoxes.size(); }
      inline grid& gridSize() { return fGridSize; }
      inline const cl::Buffer& gridCells() const { return fDevGridCells; }
      inline const cl::LocalSpaceArg& localGridCells() const { return fDevLocalGridCells; }
      inline const cl::Buffer& gridIndices() const { return fDevGridIndices; }
      inline const cl::LocalSpaceArg& localGridIndices() const { return fDevLocalGridIndices; }
      inline const int nGridIndices() const { return fBoxIndices.size(); }

      //Custom exception class to explain why the command line couldn't be parsed.
      //TODO: Derive from app::exception?
      class exception: public std::runtime_error
      {
        public:
          exception(const std::string& why);
          virtual ~exception() = default;
      };

      //References to the data for a selected box
      struct selected
      {
        aabb& box;
        material& mat;
        std::string& name;
        std::vector<cl::int2> gridCells;
      };

      //Upload host-side state to the GPU
      void sendToGPU(cl::Context& ctx);

      //TODO: Check whether the mouse is over the sun
      //bool isOverSun(const ray fromCamera);

      //Select the closest aabb intersected by a ray.  If there is
      //no such box, create a new one where this ray intersects fSkybox.
      std::unique_ptr<selected> select(const ray fromCamera);

      //Read-only access to list of materials.  Useful for a GUI
      //to select a new material.
      //TODO: I'll need some other interface to add a new material.
      inline const std::map<std::string, int>& listMaterials() const
      {
        return nameToMaterialIndex;
      }

    private:
      //Serialized data that's matched to metadata.  This is a copy of
      //the data on the GPU.
      std::vector<material> fMaterials;
      std::vector<aabb> fBoxes; //Buildings
      sphere fSky; //A dome over the city on which to render the sky
      sphere fSun; //The sun positioned in the sky
      grid fGridSize; //Parameters for the grid acceleration structure
      cl::float3 fSunEmission; //Color of light emitted by the sun
      cl::float2 fGroundTexNorm; //Convert a position on the ground to
                                 //texture coordinates.  Should be the
                                 //size of the ground.
      std::unique_ptr<gl::TextureArray<GL_RGBA32F, GL_UNSIGNED_BYTE>> fTextures;
      std::vector<gridCell> fGridCells; //Grid cells contain the boxes in fBoxes through a mapping defined in fBoxIndices.
      std::vector<int> fBoxIndices; //List of boxes in each of fGridCells.  These are indices into fBoxes.

      //Metadata with references to GPU-ready data
      std::string skyTextureFile;
      std::string groundTextureFile;
      std::map<std::string, int> nameToMaterialIndex;
      std::vector<std::string> boxNames; //I chose to use parallel arrays because I need fBoxes to be tightly packed
                                         //for upload to the GPU.
      std::vector<std::string> textureNames; //Names of files where I got textures that are now on the GPU.  Index in this array is index in
                                             //the texture array on the GPU.

      //Data to help create new boxes
      float fFloorY; //Height of the bottom of fSkybox in global coordinates

      //Data on the GPU
      cl::Buffer fDevMaterials;
      cl::Buffer fDevBoxes;
      cl::LocalSpaceArg fDevLocalBoxes;
      cl::ImageGL fDevTextures;
      cl::Buffer fDevGridCells;
      cl::LocalSpaceArg fDevLocalGridCells;
      cl::Buffer fDevGridIndices; //N.B.: fGridIndices are necessary so that each gridCell can refer to a contiguous range of elements
                               //      and multiple gridCells can refer to a given box.
      cl::LocalSpaceArg fDevLocalGridIndices;

      //Helper functions
      //Group a collection of boxes into 2D gridCells.  Returns the grid's size, the gridCells, and
      //the indices into the box collection that are sorted to be compatible with the container of
      //gridCells.
      std::tuple<grid, std::vector<gridCell>, std::vector<int>> buildGrid(const std::vector<aabb>& boxes, const cl::int2 nCells);

      //Calculate the boundaries of a geometry of boxes.
      std::pair<cl::float3, cl::float3> calcGridLimits(const std::vector<aabb>& boxes) const;
  };
}

#endif //APP_CMDLINE_H
