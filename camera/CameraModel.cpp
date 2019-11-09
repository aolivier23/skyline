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
  CameraModel::CameraModel(const cl::float3& pos, const cl::float3& focal,
                           const float size): fCameraState{{pos.x, pos.y, pos.z, 1.f}, focal, size},
                                              fLCGen(std::chrono::system_clock::now().time_since_epoch().count()),
                                              fDist(0., 0.0),
                                              fPitch(asin((fCameraState.focalPos - fCameraState.position).y)),
                                              fYaw(atan2((fCameraState.focalPos - fCameraState.position).z, (fCameraState.focalPos - fCameraState.position).x))
                     
  {
    updateDirections();
  }

  CameraModel::~CameraModel()
  {
  }

  //Allow the user to configure the camera's position
  void CameraModel::setPosition(const cl::float3 newPosition)
  {
    fCameraState.focalPos += newPosition - fCameraState.position;
    fCameraState.position = newPosition;
  }

  void CameraModel::translate(const cl::float3& pos)
  {
    fCameraState.position += pos;
    //TODO: This seems like the kind of decision CameraController should make
    fCameraState.focalPos += pos; //Look in the same direction so that translating has the effect of strafing
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
    fCameraState.focalPos += (fCameraState.focalPos - fCameraState.position).norm()*multiplier;
  }

  //Anti-aliasing via camera jitter.
  //TODO: Move this to the GPU
  cl::float3 CameraModel::position() const
  {
    return fCameraState.position + cl::float3(fDist(fLCGen), fDist(fLCGen), fDist(fLCGen));
  }

  //Simple Euler-angle based camera from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h
  void CameraModel::updateDirections()
  {
    cl::float3 direction{cos(fYaw)*cos(fPitch), sin(fPitch), sin(fYaw)*cos(fPitch)};
    fCameraState.focalPos = direction*(fCameraState.focalPos - fCameraState.position).mag() + fCameraState.position;
    fCameraState.right = cl::float3{0., 1., 0.}.cross(direction).norm();
    fCameraState.up = direction.cross(fCameraState.right).norm();
  }

  void CameraModel::setJitter(const double stddev)
  {
    fDist = std::normal_distribution(0., stddev);
  }
}
