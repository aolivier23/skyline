//File: CameraModel.h
//Brief: The state of a Camera viewing a ray-traced scene.  Images on a focal plane are viewed from
//       a camera position.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_CAMERAMODEL_H
#define ENG_CAMERAMODEL_H

//local includes
#include "algebra/vector.h"

//serial includes
#define NOT_ON_DEVICE
#include "serial/vector.h"
#include "serial/camera.h"

//c++ includes
#include <random>

namespace eng
{
  class CameraModel
  {
    public:
      CameraModel(const cl::float3& pos, const cl::float3& focal = {0., 0., 1.}, const float size = 1.f);
      virtual ~CameraModel();

      //Manipulate the camera
      void translate(const cl::float3& pos);
      void yaw(const float angle);
      void pitch(const float angle);
      void zoom(const float multiplier);

      //User interface
      void setPosition(const cl::float3 newPosition);
      inline double jitter() const { return fDist.stddev(); }
      void setJitter(const double stddev);

      //Accessors to Camera properties that are useful to send to OpenGL
      inline const camera& state() const { return fCameraState; }
      cl::float3 position() const;
      inline const cl::float3 exactPosition() const { return {fCameraState.position.x, fCameraState.position.y, fCameraState.position.z}; } //Without camera jitter
      inline const cl::float3& focalPlane() const { return fCameraState.focalPos; }
      inline const cl::float3& up() const { return fCameraState.up; }
      inline const cl::float3& right() const { return fCameraState.right; }

    protected:
      //Camera description
      camera fCameraState;

      //State for camera jitter.  Implements anti-aliasing.
      //TODO: Move camera jitter onto the GPU to make pixels independent
      mutable std::minstd_rand0 fLCGen;
      mutable std::normal_distribution<double> fDist; 
      //mutable std::uniform_real_distribution<float> fUniform;

    private:
      void updateDirections();

      //Euler angles
      float fPitch;
      float fYaw;
  };
}

#endif //ENG_CAMERAMODEL_H  
