//File: Geometry.cpp
//Brief: A Geometry parses the command line for exactly 1 configuration file and converts it into
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
#include "app/Geometry.h"

//serial includes
#include "serial/aabb.cpp"
#include "serial/sphere.cpp"
#include "serial/groundPlane.cpp"

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

  //List the corners of an aabb
  std::array<cl::float3, 8> corners(const aabb& box)
  {
    return {
             box.center + box.width*0.5f,
             box.center + cl::float3{ box.width.x, -box.width.y,  box.width.z}*0.5f,
             box.center + cl::float3{ box.width.x,  box.width.y, -box.width.z}*0.5f,
             box.center + cl::float3{-box.width.x,  box.width.y,  box.width.z}*0.5f,
             box.center + cl::float3{-box.width.x,  box.width.y, -box.width.z}*0.5f,
             box.center + cl::float3{-box.width.x, -box.width.y,  box.width.z}*0.5f,
             box.center + cl::float3{ box.width.x, -box.width.y, -box.width.z}*0.5f,
             box.center - box.width*0.5f
           };
  }

  //Construct a vector of the minimum component in each dimension of two vectors
  cl::float3 vecMin(const cl::float3 lhs, const cl::float3 rhs)
  {
    cl::float3 result;
    result.x = std::min(lhs.x, rhs.x);
    result.y = std::min(lhs.y, rhs.y);
    result.z = std::min(lhs.z, rhs.z);

    return result;
  }

  //Construct a vector of the maximum component in each dimension of two vectors
  cl::float3 vecMax(const cl::float3 lhs, const cl::float3 rhs)
  {
    cl::float3 result;
    result.x = std::max(lhs.x, rhs.x);
    result.y = std::max(lhs.y, rhs.y);
    result.z = std::max(lhs.z, rhs.z);

    return result;
  }
}

namespace app
{
  //TODO: Move this overload of load() and USAGE to individual applications when Geometry -> Geometry.
  YAML::Node Geometry::load(const int argc, const char** argv)
  {
    //argv[0] is always the path to this application.  So, the number of command line arguments is really argc - 1, and
    //I'm going to ignore argv[0].
    if(argc != 2) throw exception("Got " + std::to_string(argc-1) + " command line arguments, but expected exactly 1");
    if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) throw exception("");

    return load(argv[1]);
  }

  //TODO: Only print usage information in version that takes argc and argv.
  //      Let exceptions propagate in this version.
  YAML::Node Geometry::load(const std::string& fileName)
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

      //Read the color of the sun
      auto sun = document["sun"];
      fSunEmission = sun["color"].as<cl::float3>(cl::float3{10.f, 8.f, 6.f});
      //The sun's center will end up in fSky eventually in sendToGPU()
      fSun.center = sun["center"].as<cl::float3>(cl::float3{0.f, 1.f, 0.f});
      fSun.radius = sun["radius"].as<float>(0.1);

      //The sky texture and ground textures must be loaded before
      //any other textures.
      ::findOrCreate(document["sky"].as<std::string>(), textureNames);
      skyTextureFile = document["sky"].as<std::string>();
      ::findOrCreate(document["ground"]["file"].as<std::string>(), textureNames);
      groundTextureFile = document["ground"]["file"].as<std::string>();
      fGroundTexNorm = document["ground"]["texNorm"].as<cl::float2>(cl::float2{1.f, 1.f});

      //Configure the skybox.  It will be automatically adjusted to
      //fit everything if it turns out to be too small in sendToGPU().
      fSky.center = cl::float3{0.f, 0.f, 0.f};
      fSky.radius = document["horizon"].as<float>(10.f);

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
                               ::findOrCreate(mat.second["left"].as<std::string>(), textureNames),
                               ::findOrCreate(mat.second["right"].as<std::string>(), textureNames),
                               ::findOrCreate(mat.second["top"].as<std::string>(), textureNames),
                               ::findOrCreate(mat.second["bottom"].as<std::string>(), textureNames),
                               ::findOrCreate(mat.second["front"].as<std::string>(), textureNames),
                               ::findOrCreate(mat.second["back"].as<std::string>(), textureNames),
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
        newBox.texNorm = box.second["texNorm"].as<cl::float3>(newBox.width).data;
        newBox.material = material->second;

        fBoxes.push_back(newBox);
      }

      fFloorY = 0;  //The sky dome is always centered at (0, 0, 0)

      //Load textures
      //TODO: oneCell has to be updated to use a kernel compatible with textures. 
      const unsigned int buildingFormat = GL_RGBA;
      int channels = 4;
      int width, height;

      //I have to load a texture to get its size before allocating memory on the GPU.
      //Or, I could get a constant size for textures somewhere else.
      auto pixels = stbi_load(textureNames[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
      if(!pixels)
      {
        std::cerr << "Couldn't find " << textureNames[0] << ", so trying to load "
                  << std::string(INSTALL_DIR) + "/include/examples/" + textureNames[0] << " instead...\n";
        pixels = stbi_load((std::string(INSTALL_DIR) + "/include/examples/" + textureNames[0]).c_str(), &width, &height, &channels, STBI_rgb_alpha);
      }
      if(!pixels) throw exception("Failed to load a texture from " + textureNames[0]);
      #ifndef NDEBUG
      std::cout << "First image has a size of " << width << " x " << height << ".\n";
      #endif

      fTextures.reset(new gl::TextureArray<GL_RGBA32F, GL_UNSIGNED_BYTE>(width, height, textureNames.size()));
      fTextures->insert(0, buildingFormat, pixels);
      stbi_image_free(pixels);

      for(size_t whichFile = 1; whichFile < textureNames.size(); ++whichFile)
      {
        pixels = stbi_load(textureNames[whichFile].c_str(), &width, &height, &channels, STBI_rgb_alpha);

        //If we failed to load pixels, try to load from the examples that ship with skyline.
        //TODO: Look for texture search directories in the YAML file.
        if(!pixels)
        {
          std::cerr << "Couldn't find " << textureNames[whichFile] << ", so trying to load "
                    << std::string(INSTALL_DIR) + "/include/examples/" + textureNames[whichFile] << " instead...\n";
          pixels = stbi_load((std::string(INSTALL_DIR) + "/include/examples/" + textureNames[whichFile]).c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }
        if(!pixels) throw exception("Failed to load a texture from " + textureNames[whichFile]);
        if(!fTextures->checkDimensions(width, height))
        {
          throw exception(textureNames[whichFile] + " has different dimensions of " + std::to_string(width) + " x " + std::to_string(height) + " from the first texture in this file.  "
                          "All textures must have the same dimensions.");
        }

        fTextures->insert(whichFile, buildingFormat, pixels);
        stbi_image_free(pixels);
      }

      const auto& cameraMap = document["cameras"];
      for(const auto& camera: cameraMap)
      {
        cameras.emplace_back(camera.first.as<std::string>(), eng::CameraModel(camera.second["position"].as<cl::float3>().data, camera.second["focal"].as<cl::float3>().data, camera.second["size"].as<float>(1.f)));
      }

      fGridSize.max = document["grid"].as<cl::int2>(cl::int2{1, 1});
    }
    catch(const YAML::Exception& e)
    {
      throw exception(e.what());
    }

    return document;
  }

  YAML::Node Geometry::write(const std::string& fileName)
  {
    //"Invert" the mapping in nameToMaterialIndex
    std::vector<std::string> materialIndexToName(nameToMaterialIndex.size());
    for(const auto& name: nameToMaterialIndex) materialIndexToName[name.second] = name.first;

    //Serialize the application state.
    YAML::Node newFile;

    //Write sky and ground textures
    newFile["sky"] = skyTextureFile;
    newFile["ground"]["file"] = groundTextureFile;
    newFile["ground"]["texNorm"] = fGroundTexNorm;
    auto sun = newFile["sun"];
    sun["color"] = fSunEmission;
    sun["center"] = fSun.center;
    sun["radius"] = fSun.radius;

    //Write out materials
    auto mats = newFile["materials"];
    for(const auto& inMemory: nameToMaterialIndex)
    {
      auto inFile = mats[inMemory.first];
      //inFile["color"] = cl::float3(fMaterials[inMemory.second].color);
      inFile["emission"] = cl::float3(fMaterials[inMemory.second].emission);
      inFile["left"] = textureNames[fMaterials[inMemory.second].textures.data.s[0]];
      inFile["right"] = textureNames[fMaterials[inMemory.second].textures.data.s[1]];
      inFile["top"] = textureNames[fMaterials[inMemory.second].textures.data.s[2]];
      inFile["bottom"] = textureNames[fMaterials[inMemory.second].textures.data.s[3]];
      inFile["front"] = textureNames[fMaterials[inMemory.second].textures.data.s[4]];
      inFile["back"] = textureNames[fMaterials[inMemory.second].textures.data.s[5]];
    }

    //Write building geometry
    auto geom = newFile["geometry"];
    for(size_t whichBox = 0; whichBox < fBoxes.size(); ++whichBox)
    {
      //Ensure that each box gets a unique name.  Repeated names
      //are OK, but not encouraged, interactively, but they get
      //overwritten when writing the YAML file here.  I could
      //imagine the user manually making file names the same, so
      //this seems to be my opportunity to fix the problem with
      //one point of maintanence.

      //Adapt repeated box names so that they all get to the YAML file.
      while(geom[boxNames[whichBox]])
      {
        boxNames[whichBox] += "_copy";
      }
      auto inFile = geom[boxNames[whichBox]];

      inFile["width"] = cl::float3(fBoxes[whichBox].width);
      inFile["center"] = cl::float3(fBoxes[whichBox].center);
      inFile["material"] = materialIndexToName[fBoxes[whichBox].material];
      inFile["texNorm"] = cl::float3(fBoxes[whichBox].texNorm);
    }

    //Save camera states
    auto cams = newFile["cameras"];
    for(const auto& inMemory: cameras)
    {
      auto onFile = cams[inMemory.first];
      onFile["position"] = inMemory.second.exactPosition();
      onFile["focal"] = cl::float3(inMemory.second.focalPlane());
    }

    //Save grid state
    newFile["grid"] = fGridSize.max;

    //Write to a YAML file.
    std::ofstream output(fileName);
    output << newFile;

    return newFile;
  }

  Geometry::exception::exception(const std::string& why): std::runtime_error(why + "\n\n" + USAGE)
  {
  }

  //Calculate the (x, z) limits for a grid acceleration structure
  std::pair<cl::float3, cl::float3> Geometry::calcGridLimits(const std::vector<aabb>& boxes) const
  {
    cl::float3 min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}, max = -min;

    for(const auto& box: boxes)
    {
      for(const auto corner: ::corners(box))
      {
        min = ::vecMin(min, corner);
        max = ::vecMax(max, corner);
      }
    }

    const auto epsilon = cl::float3{1.f, 1.f, 1.f} * 0.1f; //Make the grid a little bigger than it needs to be so that all volumes unambiguously fit inside it.

    return std::make_pair(min - epsilon, max + epsilon);
  }

  //Group a collection of boxes into 2D gridCells.  Returns the grid's size, the gridCells, and
  //the indices into the box collection that are sorted to be compatible with the container of
  //gridCells.
  //TODO: When I'm ready for a grid optimizer, I need to split this up into:
  //      1) Get grid min/max and origin
  //      2) Decide number of grid cells along each axis
  //      3) Put boxes into grid cells
  std::tuple<grid, std::vector<gridCell>, std::vector<int>> Geometry::buildGrid(const std::vector<aabb>& boxes, const cl::int2 nCells)
  {
    grid size;
    std::vector<int> boxIndices;

    //First, choose size: origin, max, and cellSize.  I could choose origin and max * cellSize
    //by looking at a 2D bounding area for boxes.  cellSize should be optimized somehow to maximize
    //performance.  Maybe I could start with a minimum or maximum number of boxes per cell.
    //Eventually, this could be a good opportunity for real-time optimization.
    auto [min, max] = calcGridLimits(boxes);

    size.origin = {min.x, min.z};
    size.max = {nCells.x, nCells.y};
    size.cellSize = {(max.x - min.x)/size.max.x, (max.z - min.z)/size.max.y};

    //Next, group boxes into cells.  The std::set<> makes sure each box can be in each cell only once.
    //TODO: Do I even need a std::set<> anymore?  I should now loop over each cell once
    std::vector<std::set<int>> boxesInEachCell(size.max.x * size.max.y);
    for(int whichBox = 0; whichBox < (int)boxes.size(); ++whichBox)
    {
      //TODO: New algorithm for placing whichBox in cell(s).  Beware: it will end badly with boxes that aren't aligned
      //      with the x and z axes!
      cl::float3 boxMin = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                 boxMax = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};
      for(const auto corner: corners(boxes[whichBox]))
      {
        boxMin = ::vecMin(boxMin, corner);
        boxMax = ::vecMax(boxMax, corner);
      }

      const cl::float3 distToBoxMin = boxMin - min,
                       distToBoxMax = boxMax - min;
      
      for(int xCell = distToBoxMin.x/size.cellSize.x; xCell < distToBoxMax.x/size.cellSize.x; ++xCell)
      {
        for(int yCell = distToBoxMin.z/size.cellSize.y; yCell < distToBoxMax.z/size.cellSize.y; ++yCell)
        {
          const int whichCell = xCell + yCell*size.max.x; //See comments about size.max.y above

          //Check for logic errors only in debug builds
          assert(xCell < size.max.x && "Got a corner outside of size.max.x!  Grid construction failed.");
          assert(yCell < size.max.y && "Got a corner outside of size.max.y!  Grid construction failed.");
          assert(whichCell < (int)boxesInEachCell.size() && "Corner is inside of size.max, but I got an invalid cell somehow!  Grid construction failed.");
          assert(whichCell >= 0 && "Corner is before the first grid cell somehow!  Grid construction failed.");

          boxesInEachCell[whichCell].insert(whichBox);
        }
      }
    }

    //Then, put boxes into contiguous memory.  Create a record of where this memory is in
    //each gridCell so that I can find it later on the GPU.
    std::vector<gridCell> cells(boxesInEachCell.size());
    for(size_t whichCell = 0; whichCell < boxesInEachCell.size(); ++whichCell)
    {
      cells[whichCell].begin = boxIndices.size();
      boxIndices.insert(boxIndices.end(), boxesInEachCell[whichCell].begin(), boxesInEachCell[whichCell].end());
      cells[whichCell].end = boxIndices.size();
    }

    return std::make_tuple(size, cells, boxIndices);
  }

  void Geometry::sendToGPU(cl::Context& ctx)
  {
    //Update fSky and fGroundTexNorm to include all buildings.
    //If this ever becomes slow, I can think of lots of ways to speed it up.
    //I could imagine an acceleration structure with buildings on the edge
    //of the skybox that could help with lots of other things too.  I think
    //this loop would be limited by I/O into L2 cache if I tried to do SIMD
    //math.
    //TODO: Keep the apparent size of the sun the same when fSky grows?
    for(const auto& box: fBoxes)
    {
      const auto corners = ::corners(box);
      for(const auto& corner: corners) fSky.radius = std::max(fSky.radius, corner.mag());
    }

    //Force the ground plane to cover the equator of fSky
    //TODO: Do I want to stretch the ground plane like this automatically?
    //fGroundTexNorm = {1.f, 1.f}; //{fSky.radius, fSky.radius};

    //Force the sun to have a center on fSky
    const auto sunDir = fSun.center.norm();
    fSun.center = sunDir * fSky.radius;

    std::tie(fGridSize, fGridCells, fBoxIndices) = buildGrid(fBoxes, fGridSize.max); //TODO: Provide number of grid cells which could come from a user interface

    //Synchronize GPU data with the CPU
    //fGridSize = size;
    fDevBoxes = cl::Buffer(ctx, fBoxes.begin(), fBoxes.end(), false);
    fDevMaterials = cl::Buffer(ctx, fMaterials.begin(), fMaterials.end(), false);
    fDevGridIndices = cl::Buffer(ctx, fBoxIndices.begin(), fBoxIndices.end(), false);
    fDevGridCells = cl::Buffer(ctx, fGridCells.begin(), fGridCells.end(), false);

    glBindTexture(GL_TEXTURE_2D_ARRAY, fTextures->name);
    fDevTextures = cl::ImageGL(ctx, CL_MEM_READ_ONLY, GL_TEXTURE_2D_ARRAY, 0, fTextures->name);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  }

  //Returning std::unique_ptr<> because I couldn't get std::optional<> to do what I want.
  std::unique_ptr<Geometry::selected> Geometry::select(const ray fromCamera)
  {
    //First, intersect the sky and the ground.  Any box farther than the closer
    //of these two is not visible.
    const float groundDist = groundPlane_intersect(fromCamera),
                skyDist = sphere_intersect(fSky, fromCamera);
    float closest = std::numeric_limits<float>::max();
    if(groundDist < 0)
    {
      if(skyDist > 0) closest = skyDist;
    }
    else if(skyDist > 0) closest = std::min(groundDist, skyDist);
    else closest = groundDist;

    //Find the closest box that is closer than sky/ground if any.
    size_t found = fBoxes.size();
    for(size_t whichBox = 0; whichBox != fBoxes.size(); ++whichBox)
    {
      const auto dist = aabb_intersect(&fBoxes[whichBox], fromCamera);
      if(dist > 0 && dist < closest)
      {
        closest = dist;
        found = whichBox;
      }
    }

    //If I couldn't find a box, create one.
    //I'm assuming that there are no boxes outside fSky.
    if(found == fBoxes.size())
    {
      const auto intersection = fromCamera.position + fromCamera.direction * closest;
      boxNames.push_back("defaultBox");
      fBoxes.push_back(aabb{cl::float3{0.1, 0.1, 0.1},
                            cl::float3{intersection.x, fFloorY, intersection.z},
                            cl::float3{0.1, 0.1, 0.1},
                            fBoxes.empty()?SKY_TEXTURE:fBoxes.back().material});
    }

    //Look up the grid index of this box
    std::vector<cl::int2> cellsWithThisBox;
    for(int xCell = 0; xCell < fGridSize.max.x; ++xCell)
    {
      for(int yCell = 0; yCell < fGridSize.max.y; ++yCell)
      {
        const int whichCell = xCell + yCell * fGridSize.max.x;
        const auto begin = fBoxIndices.begin() + fGridCells[whichCell].begin,
                   end = fBoxIndices.begin() + fGridCells[whichCell].end;


        if(std::find(begin, end, found) != end) cellsWithThisBox.push_back({xCell, yCell});
      }
    }

    //TODO: return a transaction instead
    return std::make_unique<selected>(selected{fBoxes[found], fMaterials[fBoxes[found].material], boxNames[found],
                                               cellsWithThisBox});
  }
}
