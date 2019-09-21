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
                                                                            fUniform(-0.01, 0.01)
                     
  {
  }

  CameraModel::~CameraModel()
  {
  }

  void CameraModel::translate(const cl::float3& pos)
  {
    fPosition += pos;
    //TODO: This seems like the kind of decision CameraController should make
    fFocalPlane += pos; //Look in the same direction so that translating has the effect of strafing
  }

  void CameraModel::yaw(const float angle)
  {
    const auto disp = fFocalPlane - fPosition;
    const float r = sqrt(disp.data.s[0]*disp.data.s[0] + disp.data.s[2]*disp.data.s[2]);
    cl::float3 vUnit{disp.data.s[0], 0., disp.data.s[2]};
    vUnit = vUnit.norm();
    cl::float3 orthogonal{0., 1., 0.};
    cl::float3 uUnit = vUnit.cross(orthogonal).norm();
    fFocalPlane = uUnit*r*sin(angle) + vUnit*r*cos(angle) + fPosition + orthogonal*(fFocalPlane.data.s[1] - fPosition.data.s[1]);
  }

  void CameraModel::pitch(const float angle)
  {
    const auto disp = fFocalPlane - fPosition;
    const float r = sqrt(disp.data.s[1]*disp.data.s[1] + disp.data.s[2]*disp.data.s[2]);
    cl::float3 vUnit{0., disp.data.s[1], disp.data.s[2]};
    vUnit = vUnit.norm();
    cl::float3 orthogonal{1., 0., 0.};
    cl::float3 uUnit = vUnit.cross(orthogonal).norm();
    fFocalPlane = uUnit*r*sin(angle) + vUnit*r*cos(angle) + fPosition + orthogonal*(fFocalPlane.data.s[0] - fPosition.data.s[0]);
  }

  void CameraModel::zoom(const float multiplier)
  {
    fFocalPlane += (fFocalPlane - fPosition).norm()*multiplier;
  }

  cl_float3 CameraModel::position() const
  {
    return (fPosition + cl::float3(fUniform(fLCGen), fUniform(fLCGen), fUniform(fLCGen))).data; //TODO: camera jitter here using fUniform and fLCGen
  }
} 
