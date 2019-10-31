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
#include <fstream>
#include <iostream>

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>

//app includes
#include "app/CmdLine.h"

//serial includes
#include "serial/aabb.cpp"

//camera includes
#include "algebra/YAMLIntegration.h"

//stb includes
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

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

//Helper functions to make code more readable
namespace
{
  unsigned char findOrCreate(const std::string& toFind, std::vector<std::string>& existingNames)
  {
    auto found = std::find(existingNames.begin(), existingNames.end(), toFind);
    if(found == existingNames.end())
    {
      existingNames.push_back(toFind);
      found = std::prev(existingNames.end());
    }

    return std::distance(existingNames.begin(), found);
  }
}

namespace app
{
  //TODO: Move this overload of load() and USAGE to individual applications when CmdLine -> Geometry.
  YAML::Node CmdLine::load(const int argc, const char** argv)
  {
    //argv[0] is always the path to this application.  So, the number of command line arguments is really argc - 1, and
    //I'm going to ignore argv[0].
    if(argc != 2) throw exception("Got " + std::to_string(argc-1) + " command line arguments, but expected excatly 1");
    if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) throw exception("");

    return load(argv[1]);
  }

  //TODO: Only print usage information in version that takes argc and argv.
  //      Let exceptions propagate in this version.
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

      //A temporary place to map texture names to indices
      //TODO: Make this cache permanent once I've decided on the format the GUI needs
      std::vector<std::string> texturesToCreate;

      //Map the YAML configuration file to a geometry to render using a few keywords
      const auto& matMap = document["materials"];

      //TODO: Look for a material called "default" for builder?
      for(const auto& mat: matMap)
      {
        nameToMaterialIndex[mat.first.as<std::string>()] = fMaterials.size();
        //Default color is color of my first 3D rendered triangle from learnopengl.com
        //TODO: As an alternative, read color and generate a texture of pure color.
        fMaterials.push_back(material{mat.second["emission"].as<cl::float3>(cl::float3()).data,
                             {
                               ::findOrCreate(mat.second["left"].as<std::string>(), texturesToCreate),
                               ::findOrCreate(mat.second["right"].as<std::string>(), texturesToCreate),
                               ::findOrCreate(mat.second["top"].as<std::string>(), texturesToCreate),
                               ::findOrCreate(mat.second["bottom"].as<std::string>(), texturesToCreate),
                               ::findOrCreate(mat.second["front"].as<std::string>(), texturesToCreate),
                               ::findOrCreate(mat.second["back"].as<std::string>(), texturesToCreate),
                               0,
                               0
                             }});
      }

      //TODO: This would be a great time to fill out metadata like box names for some GUI.
      const auto& boxMap = document["geometry"];
      for(const auto& box: boxMap)
      {
        boxNames.push_back(box.first.as<std::string>());
        const auto material = nameToMaterialIndex.find(box.second["material"].as<std::string>());
        if(material == nameToMaterialIndex.end()) throw exception("Failed to look up a material named " + box.second["material"].as<std::string>()
                                                             + " for a box named " + box.first.as<std::string>());

        aabb newBox;
        newBox.width = box.second["width"].as<cl::float3>().data;
        newBox.center = box.second["center"].as<cl::float3>().data;
        newBox.texNorm = box.second["texNorm"].as<cl::float3>(box.second["width"].as<cl::float3>()).data;
        newBox.material = material->second;

        fBoxes.push_back(newBox);
      }

      const auto skyMaterial = nameToMaterialIndex.find(document["skybox"]["material"].as<std::string>());
      if(skyMaterial == nameToMaterialIndex.end()) throw exception("Failed to lookup material for the skybox named " + document["skybox"]["material"].as<std::string>());
      fSkybox = {document["skybox"]["width"].as<cl::float3>().data, document["skybox"]["center"].as<cl::float3>().data,
                 document["skybox"]["texNorm"].as<cl::float3>(document["skybox"]["width"].as<cl::float3>()).data, skyMaterial->second};

      fFloorY = fSkybox.center.y - fSkybox.width.y/2.;

      //Load textures
      //TODO: oneCell has to be updated to use a kernel compatible with textures. 
      //TODO: Update example sky texture
      const unsigned int buildingFormat = GL_RGBA;
      int channels = 4;
      int width, height;

      //I have to load a texture to get its size before allocating memory on the GPU.
      //Or, I could get a constant size for textures somewhere else.
      auto pixels = stbi_load(texturesToCreate[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
      if(!pixels)
      {
        std::cerr << "Couldn't find " << texturesToCreate[0] << ", so trying to load "
                  << std::string(INSTALL_DIR) + "/include/examples/" + texturesToCreate[0] << " instead...\n";
        pixels = stbi_load((std::string(INSTALL_DIR) + "/include/examples/" + texturesToCreate[0]).c_str(), &width, &height, &channels, STBI_rgb_alpha);
      }
      if(!pixels) throw exception("Failed to load a texture from " + texturesToCreate[0]);

      fTextures.reset(new gl::TextureArray<GL_RGBA32F, GL_UNSIGNED_BYTE>(width, height, texturesToCreate.size()));
      fTextures->insert(0, buildingFormat, pixels);
      stbi_image_free(pixels);

      for(size_t whichFile = 1; whichFile < texturesToCreate.size(); ++whichFile)
      {
        pixels = stbi_load(texturesToCreate[whichFile].c_str(), &width, &height, &channels, STBI_rgb_alpha);

        //If we failed to load pixels, try to load from the examples that ship with skyline.
        //TODO: Look for texture search directories in the YAML file.
        if(!pixels)
        {
          std::cerr << "Couldn't find " << texturesToCreate[whichFile] << ", so trying to load "
                    << std::string(INSTALL_DIR) + "/include/examples/" + texturesToCreate[whichFile] << " instead...\n";
          pixels = stbi_load((std::string(INSTALL_DIR) + "/include/examples/" + texturesToCreate[whichFile]).c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }
        if(!pixels) throw exception("Failed to load a texture from " + texturesToCreate[whichFile]);
        if(!fTextures->checkDimensions(width, height))
        {
          throw exception(texturesToCreate[whichFile] + " has different dimensions from the first texture in this file.  "
                          "All textures must have the same dimensions.");
        }

        fTextures->insert(whichFile, buildingFormat, pixels);
        stbi_image_free(pixels);
      }

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

  YAML::Node CmdLine::write(const std::string& fileName)
  {
    //"Invert" the mapping in nameToMaterialIndex
    std::vector<std::string> materialIndexToName(nameToMaterialIndex.size());
    for(const auto& name: nameToMaterialIndex) materialIndexToName[name.second] = name.first;

    //Serialize the application state.
    YAML::Node newFile;

    //TODO: Update write() to serialize texture names
    auto mats = newFile["materials"];
    for(const auto& inMemory: nameToMaterialIndex)
    {
      auto inFile = mats[inMemory.first];
      //inFile["color"] = cl::float3(fMaterials[inMemory.second].color);
      inFile["emission"] = cl::float3(fMaterials[inMemory.second].emission);
    }

    auto geom = newFile["geometry"];
    for(size_t whichBox = 0; whichBox < fBoxes.size(); ++whichBox)
    {
      auto inFile = geom[boxNames[whichBox]];
      inFile["width"] = cl::float3(fBoxes[whichBox].width);
      inFile["center"] = cl::float3(fBoxes[whichBox].center);
      inFile["material"] = materialIndexToName[fBoxes[whichBox].material];
    }

    auto sky = newFile["skybox"];
    sky["width"] = cl::float3(fSkybox.width);
    sky["center"] = cl::float3(fSkybox.center);
    sky["material"] = materialIndexToName[fSkybox.material];

    auto cams = newFile["cameras"];
    for(const auto& inMemory: cameras)
    {
      auto onFile = cams[inMemory.first];
      onFile["position"] = inMemory.second.exactPosition();
      onFile["focal"] = cl::float3(inMemory.second.focalPlane());
    }

    //Write to a YAML file.
    std::ofstream output(fileName);
    output << newFile;

    return newFile;
  }

  CmdLine::exception::exception(const std::string& why): std::runtime_error(why + "\n\n" + USAGE)
  {
  }

  void CmdLine::sendToGPU(cl::Context& ctx)
  {
    fDevBoxes = cl::Buffer(ctx, fBoxes.begin(), fBoxes.end(), false);
    fDevMaterials = cl::Buffer(ctx, fMaterials.begin(), fMaterials.end(), false);
    fDevSkybox = cl::Buffer(ctx, &fSkybox, &fSkybox + 1, false);

    glBindTexture(GL_TEXTURE_2D_ARRAY, fTextures->name);
    fDevTextures = cl::ImageGL(ctx, CL_MEM_READ_ONLY, GL_TEXTURE_2D_ARRAY, 0, fTextures->name);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  }

  //Returning std::unique_ptr<> because I couldn't get std::optional<> to do what I want.
  std::unique_ptr<CmdLine::selected> CmdLine::select(const ray fromCamera)
  {
    auto found = fBoxes.end();
    float closest = std::numeric_limits<float>::max();
    for(auto box = fBoxes.begin(); box != fBoxes.end(); ++box)
    {
      const auto dist = aabb_intersect(&*box, fromCamera);
      if(dist > 0 && dist < closest)
      {
        closest = dist;
        found = box;
      }
    }

    //If I couldn't find a box, create one.
    //I'm assuming that there are no boxes outside fSkybox.
    if(found == fBoxes.end())
    {
      const auto distToSkybox = aabb_intersect(&fSkybox, fromCamera);
      assert(distToSkybox > 0 && "The user clicked on something outside the skybox!");
      const auto intersection = fromCamera.position + fromCamera.direction * distToSkybox;
      boxNames.push_back("defaultBox");
      fBoxes.push_back(aabb{cl::float3{0.1, 0.1, 0.1},
                            cl::float3{intersection.x, fFloorY, intersection.z},
                            cl::float3{0.1, 0.1, 0.1},
                            fBoxes.empty()?fSkybox.material:fBoxes.back().material});
      found = std::prev(fBoxes.end());
    }

    return std::make_unique<selected>(selected{*found, fMaterials[found->material], boxNames[std::distance(fBoxes.begin(), found)]});
  }
}
