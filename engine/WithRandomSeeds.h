//File: WithRandomSeeds.h
//Brief: A WithCamera engine that keeps track of a buffer of seeds for a PRNG.
//       Each thread needs its own seed, so this buffer needs to be resized with
//       the window.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_WITHRANDOMSEEDS_H
#define ENG_WITHRANDOMSEEDS_H

//engine includes
#include "engine/WithCamera.h"

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.hpp>

namespace eng
{
  class WithRandomSeeds: public eng::WithCamera
  {
    public:
      WithRandomSeeds(GLFWwindow* window, cl::Context& ctx,
                      std::unique_ptr<eng::CameraController>&& camera);
  
      //When the camera changes, update nIterations.
      virtual void onCameraChange() override;
  
      //Access data to send to the GPU
      inline int& nIterations() { return fNIterations; }
      inline const cl::Buffer& seeds() const { return fSeeds; }
      inline int& nSamples() { return fNSamples; }
  
      //Reconfigure the engine.
      //Provided through accessor functions so that I can
      //refactor this interface in the future if something needs
      //to be updated when these parameters change.
      inline int& latency() { return fLatency; }
      inline int& nBounces() { return fNBounces; }
  
    protected:
      virtual void userResize(const int width, const int height) override;
  
    private:
      //Engine configuration
      int fLatency; //Reset fNIterations to this number onCameraChange().  Helps the scene transition more smoothly
                    //when frame times are long.
  
      //Data to be sent to the GPU
      cl::Buffer fSeeds;
      int fNIterations; //Number of times this frame has been path traced since a camera change
      int fNBounces; //Number of ray reflections allowed per frame
      int fNSamples; //Number of times to path trace the scene before presenting a frame
  };
}

#endif //ENG_WITHRANDOMSEEDS_H
