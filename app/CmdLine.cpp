//File: CmdLine.cpp
//Brief: A CmdLine parses the command line for exactly 1 configuration file and converts it into
//       a list of materials, a list of aabbs, and a list of CameraModels.  With any other number
//       of arguments, -h, --help, or a YAML file it can't parse, it throws an exception with
//       usage information.
//
//       All knowledge of yaml-cpp is centralized here.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//c++ includes
#include <exception>

//TODO: Remove me
#include <iostream>

//app includes
#include "app/CmdLine.h"

//camera includes
#include "camera/YAMLIntegration.h"

//Usage of all skyline applications.
//TODO: Customize the program description for each application.
#define USAGE "Usage: oneCell <configuration.yaml>\n\n"\
              "oneCell: A demonstration of the skyline city rendering engine.\n"\
              "         Reads in a city geometry\n"\
              "         from a YAML file and renders it without any bounding\n"\
              "         volume hierarchy.\n\n"\
              "\tThe configuration file should be written in YAML <= 1.2 with\n"\
              "\tthe following 3 maps in any order:\n"\
              "\t1) materials: A map of materials to use with buildings.  Each\n"\
              "\t              material may have a color and an emission.\n"\
              "\t2) geometry: A map of axis-aligned bounding boxes modelling\n"\
              "\t             buildings.  Each box may have a position and a width,\n"\
              "\t             and it must have a material whose name appears in\n"\
              "\t             the map of materials mentioned above.\n"\
              "\t3) cameras: A map of camera configurations.  Each camera may\n"\
              "\t            have a position and a focal plane position.\n\n"\
              "\tYou must also specify a node called \"skybox\" after which all\n"\
              "\trays will be killed\n\n"\
              "\tThis message will be printed if the YAML file cannot be parsed,\n"\
              "\tthere is not exactly 1 argument on the command line, or the sole\n"\
              "\tcommand line argument is -h or --help.\n\n"\
              "\tReturn values:\n"\
              "\t0: Command line was parsed successfully, and there were no error\n"\
              "\t   during rendering.\n"\
              "\t1: Command line was not parsed correctly.  Rendering did not start.\n"\
              "\t2: Could not begin rendering.\n"\
              "\t3: An error occurred during rendering.\n"

namespace app
{
  YAML::Node CmdLine::load(const int argc, const char** argv)
  {
    //argv[0] is always the path to this application.  So, the number of command line arguments is really argc - 1, and
    //I'm going to ignore argv[0].
    if(argc != 2) throw exception("Got " + std::to_string(argc-1) + " command line arguments, but expected excatly 1");
    if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) throw exception("");

    return load(argv[1]);
  }

  YAML::Node CmdLine::load(const std::string& fileName)
  {
    //The document into which I will try to load a YAML file
    YAML::Node document;

    try
    {
      //Try to load the configuration file from the current directory.  If that fails, try to load it
      //from the installation directory of this library as an example.
      YAML::Node document;
      try
      {
        document = YAML::LoadFile(fileName);
      }
      catch(YAML::Exception& /*e*/)
      {
        document = YAML::LoadFile(std::string(INSTALL_DIR) + "/include/examples/" + fileName);
      }

      //Map the YAML configuration file to a geometry to render using a few keywords
      const auto& matMap = document["materials"];
      std::map<std::string, int> matNameToIndex;

      for(const auto& mat: matMap)
      {
        matNameToIndex[mat.first.as<std::string>()] = materials.size();
        //Default color is color of my first 3D rendered triangle from learnopengl.com
        materials.push_back(material{mat.second["color"].as<cl::float3>(cl::float3{1., 0.64453125, 0.}).data,
                                     mat.second["emission"].as<cl::float3>(cl::float3()).data});
      }

      //TODO: This would be a great time to fill out metadata like box names for some GUI.
      const auto& boxMap = document["geometry"];
      for(const auto& box: boxMap)
      {
        const auto material = matNameToIndex.find(box.second["material"].as<std::string>());
        if(material == matNameToIndex.end()) throw exception("Failed to look up a material named " + box.second["material"].as<std::string>()
                                                             + " for a box named " + box.first.as<std::string>());
        boxes.push_back(aabb{box.second["width"].as<cl::float3>().data, box.second["center"].as<cl::float3>().data,
                             material->second});
      }

      const auto skyMaterial = matNameToIndex.find(document["skybox"]["material"].as<std::string>());
      if(skyMaterial == matNameToIndex.end()) throw exception("Failed to lookup material for the skybox named " + document["skybox"]["material"].as<std::string>());
      skybox = {document["skybox"]["width"].as<cl::float3>().data, document["skybox"]["center"].as<cl::float3>().data,
                skyMaterial->second};

      //TODO: This would be a great time to associate CameraModels with metadata like a name for some GUI.
      const auto& cameraMap = document["cameras"];
      for(const auto& camera: cameraMap)
      {
        cameras.emplace_back(camera.first.as<std::string>(), eng::CameraModel(camera.second["position"].as<cl::float3>().data, camera.second["focal"].as<cl::float3>().data));
      }
    }
    catch(const YAML::Exception& e)
    {
      throw exception(e.what());
    }

    return document;
  }

  YAML::Node write(const std::string& fileName)
  {
    YAML::Node newFile(fileName);
    return newFile;
  }

  CmdLine::exception::exception(const std::string& why): std::runtime_error(why + "\n\n" + USAGE)
  {
  }
}
