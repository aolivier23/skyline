//File: CameraModel.cpp
//Brief: A CameraModel can be translated, rotated, or zoomed.  It represents a camera that can be moved around an OpenGL scene. 
//Author: Andrew Olivier aolivier@ur.rochester.edu

//Header include
#include "camera/CameraModel.h"

//c++ includes
#include <cmath>
#include <chrono>

namespace eng
{
  CameraModel::CameraModel(const cl::float3& pos, const cl::float3& focal): fPosition(pos),
                                                                            fFocalPlane(focal),
                                                                            fLCGen(std::chrono::system_clock::now().time_since_epoch().count()),
                                                                            fUniform(-0.01, 0.01),
                                                                            fPitch(asin((fFocalPlane - fPosition).data.y)),
                                                                            fYaw(atan2((fFocalPlane - fPosition).data.z, (fFocalPlane - fPosition).data.x))
                     
  {
    updateDirections();
  }

  CameraModel::~CameraModel()
  {
  }

  //Allow the user to configure the camera's position
  void CameraModel::setPosition(const cl::float3 newPosition)
  {
    fFocalPlane += newPosition - fPosition;
    fPosition = newPosition;
  }

  void CameraModel::translate(const cl::float3& pos)
  {
    fPosition += pos;
    //TODO: This seems like the kind of decision CameraController should make
    fFocalPlane += pos; //Look in the same direction so that translating has the effect of strafing
  }

  void CameraModel::yaw(const float angle)
  {
    fYaw -= angle;
    updateDirections();
  }

  void CameraModel::pitch(const float angle)
  {
    if(fabs(fabs(fPitch - angle) - M_PI/2.) > M_PI/180.)
    {
      fPitch -= angle;
      updateDirections();
    }
  }

  void CameraModel::zoom(const float multiplier)
  {
    fFocalPlane += (fFocalPlane - fPosition).norm()*multiplier;
  }

  //Anti-aliasing via camera jitter.
  cl_float3 CameraModel::position() const
  {
    return (fPosition + cl::float3(fUniform(fLCGen), fUniform(fLCGen), fUniform(fLCGen))).data;
  }

  //Simple Euler-angle based camera from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h
  void CameraModel::updateDirections()
  {
    cl::float3 direction{cos(fYaw)*cos(fPitch), sin(fPitch), sin(fYaw)*cos(fPitch)};
    fFocalPlane = direction*(fFocalPlane - fPosition).mag() + fPosition;
    fRight = cl::float3{0., 1., 0.}.cross(direction).norm();
    fUp = direction.cross(fRight).norm();
  }
}
