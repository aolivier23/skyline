//File: WithRandomSeeds.cpp
//Brief: A WithCamera engine that keeps track of a buffer of seeds for a PRNG.
//       Each thread needs its own seed, so this buffer needs to be resized with
//       the window.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//engine includes
#include "engine/WithRandomSeeds.h"

namespace eng
{
  WithRandomSeeds::WithRandomSeeds(GLFWwindow* window, cl::Context& ctx,
                                   std::unique_ptr<eng::CameraController>&& camera): WithCamera(window, ctx, std::move(camera)), fLatency(0), fNIterations(0), fNBounces(4), fNSamples(1)
  {
    std::vector<size_t> hostSeeds(fWidth*fHeight);
    for(size_t id = 1; id <= fWidth*fHeight; ++id) hostSeeds[id] = id;
    fSeeds = cl::Buffer(ctx, hostSeeds.begin(), hostSeeds.end(), false);
  }

  void WithRandomSeeds::userResize(const int width, const int height)
  {
    std::vector<size_t> hostSeeds(width*height);
    for(size_t id = 1; id <= width*height; ++id) hostSeeds[id] = id;
    fSeeds = cl::Buffer(fContext, hostSeeds.begin(), hostSeeds.end(), false);
    
    fNIterations = 0;
  };
                                                                                                                    
  void WithRandomSeeds::onCameraChange()
  {
    fNIterations = fLatency; //Weight the Scene starts with when the camera moves.  Basically, making this number
                             //larger causes the scene to blur more rather than be disrupted by bad sampling.
  }
};
