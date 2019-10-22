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

      //Map the YAML configuration file to a geometry to render using a few keywords
      const auto& matMap = document["materials"];

      for(const auto& mat: matMap)
      {
        nameToMaterialIndex[mat.first.as<std::string>()] = fMaterials.size();
        //Default color is color of my first 3D rendered triangle from learnopengl.com
        fMaterials.push_back(material{mat.second["color"].as<cl::float3>(cl::float3{1., 0.64453125, 0.}).data,
                                     mat.second["emission"].as<cl::float3>(cl::float3()).data});
      }

      //TODO: This would be a great time to fill out metadata like box names for some GUI.
      const auto& boxMap = document["geometry"];
      for(const auto& box: boxMap)
      {
        boxNames.push_back(box.first.as<std::string>());
        const auto material = nameToMaterialIndex.find(box.second["material"].as<std::string>());
        if(material == nameToMaterialIndex.end()) throw exception("Failed to look up a material named " + box.second["material"].as<std::string>()
                                                             + " for a box named " + box.first.as<std::string>());
        fBoxes.push_back(aabb{box.second["width"].as<cl::float3>().data, box.second["center"].as<cl::float3>().data,
                             material->second});
      }

      const auto skyMaterial = nameToMaterialIndex.find(document["skybox"]["material"].as<std::string>());
      if(skyMaterial == nameToMaterialIndex.end()) throw exception("Failed to lookup material for the skybox named " + document["skybox"]["material"].as<std::string>());
      fSkybox = {document["skybox"]["width"].as<cl::float3>().data, document["skybox"]["center"].as<cl::float3>().data,
                skyMaterial->second};

      fFloorY = fSkybox.center.y - fSkybox.width.y/2.;

      //Load textures
      //TODO: Read textures from YAML file instead of materials.  Replace materials entirely with textures.
      //TODO: Don't require 6 images for every texture.  Maybe I can get away with treating the skybox differently from
      //      buildings which only need a texture for the top and a texture for the sides.  I might want a third texture
      //      for facades.
      //TODO: If I switch from materials with colors to materials with textures in builder, oneCell has to be updated too.
      //      I could probably kludge things back together by creating a texture of a solid color so that the old YAML files
      //      work, but oneCell will have to use textures too.
      int width, height, channels = 3;
      auto pixels = stbi_load(INSTALL_DIR "/include/examples/testSkyTexture.jpg", &width, &height, &channels, STBI_rgb);
      assert(pixels != nullptr && "Failed to load image from /examples/testSkyTexture.jpg");

      fSkyTextures.reset(new gl::TextureArray<GL_RGB32F, GL_UNSIGNED_BYTE>(width, height, 6));
      const unsigned int format = GL_RGB;
      fSkyTextures->insert(0, format, pixels); //+x
      fSkyTextures->insert(1, format, pixels); //-x
      fSkyTextures->insert(2, format, pixels); //+y
      fSkyTextures->insert(3, format, pixels); //-y
      fSkyTextures->insert(4, format, pixels); //+z
      fSkyTextures->insert(5, format, pixels); //-z

      stbi_image_free(pixels);

      //TODO: Read textures from YAML file instead of materials.  Replace materials entirely with textures.
      //TODO: I'm only supporting 2 textures per building for now.  Eventually, I might want separate textures
      //      for facades.
      const unsigned int buildingFormat = GL_RGBA;
      channels = 4;
      pixels = stbi_load(INSTALL_DIR "/include/examples/testBuildingSides.png", &width, &height, &channels, STBI_rgb_alpha);
      assert(pixels != nullptr && "Failed to load image from /include/examples/testBuildingSides.png");
      fBuildingTextures.reset(new gl::TextureArray<GL_RGBA32F, GL_UNSIGNED_BYTE>(width, height, 6));

      //Sides
      fBuildingTextures->insert(0, buildingFormat, pixels);
      fBuildingTextures->insert(1, buildingFormat, pixels);
      fBuildingTextures->insert(4, buildingFormat, pixels);
      fBuildingTextures->insert(5, buildingFormat, pixels);
      stbi_image_free(pixels);

      //Top/bottom
      pixels = stbi_load(INSTALL_DIR "/include/examples/testBuildingRoof.png", &width, &height, &channels, STBI_rgb_alpha);
      assert(pixels != nullptr && "Failed to load image from /include/examples/testBuildingRoof.png");
      fBuildingTextures->insert(2, buildingFormat, pixels);
      fBuildingTextures->insert(3, buildingFormat, pixels);
      stbi_image_free(pixels);

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

    auto mats = newFile["materials"];
    for(const auto& inMemory: nameToMaterialIndex)
    {
      auto inFile = mats[inMemory.first];
      inFile["color"] = cl::float3(fMaterials[inMemory.second].color);
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

    //TODO: Write skybox textures

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

    glBindTexture(GL_TEXTURE_2D_ARRAY, fSkyTextures->name);
    fDevSkyTextures = cl::ImageGL(ctx, CL_MEM_READ_ONLY, GL_TEXTURE_2D_ARRAY, 0, fSkyTextures->name);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glBindTexture(GL_TEXTURE_2D_ARRAY, fBuildingTextures->name);
    fDevBuildingTextures = cl::ImageGL(ctx, CL_MEM_READ_ONLY, GL_TEXTURE_2D_ARRAY, 0, fBuildingTextures->name);
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
      fBoxes.push_back(aabb{cl::float3{0.1, 0.1, 0.1}, cl::float3{intersection.x, fFloorY, intersection.z},
                            fBoxes.empty()?fSkybox.material:fBoxes.back().material});
      found = std::prev(fBoxes.end());
    }

    return std::make_unique<selected>(selected{*found, fMaterials[found->material], boxNames[std::distance(fBoxes.begin(), found)]});
  }
}
