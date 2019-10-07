//File: CameraModel.h
//Brief: The state of a Camera viewing a ray-traced scene.  Images on a focal plane are viewed from
//       a camera position.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_CAMERAMODEL_H
#define ENG_CAMERAMODEL_H

//local includes
#include "camera/vector.h"

//c++ includes
#include <random>

namespace eng
{
  class CameraModel
  {
    public:
      CameraModel(const cl::float3& pos, const cl::float3& focal = {0., 0., 1.});
      virtual ~CameraModel();

      //Manipulate the camera
      void translate(const cl::float3& pos);
      void yaw(const float angle);
      void pitch(const float angle);
      void zoom(const float multiplier);

      //Accessors to Camera properties that are useful to send to OpenGL
      cl_float3 position() const;
      inline const cl_float3& focalPlane() const { return fFocalPlane.data; }
      inline const cl_float3& up() const { return fUp.data; }
      inline const cl_float3& right() const { return fRight.data; }

    protected:
      //Camera description
      cl::float3 fPosition; //Displacement of the camera from the origin
      cl::float3 fFocalPlane; //Vector from fPosition to the center of the focal plane
      cl::float3 fUp; //"Up" direction for the camera.  Orthogonal to both fFocalPlane and fRight.
      cl::float3 fRight; //"Right" direction for the camera.  Orthogonal to both fFocalPlane and fUp.

      mutable std::minstd_rand0 fLCGen;
      mutable std::uniform_real_distribution<float> fUniform;

    private:
      void updateDirections();

      //Euler angles
      float fPitch;
      float fYaw;
  };
}

#endif //ENG_CAMERAMODEL_H  
