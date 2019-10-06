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
  CameraModel::CameraModel(const cl::float3& pos, const cl::float3& focal, const cl::float3& up): fPosition(pos),
                                                                                                  fFocalPlane(focal),
                                                                                                  fUp(up),
                                                                                                  fRight(up.cross(fFocalPlane - fPosition).norm()),
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

  //The rodriges formula for rotating one vector about another: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
  cl::float3 CameraModel::rodrigesFormula(const cl::float3 vector, const cl::float3 about, const float angle) const
  {
    assert(fabs(about.mag() - 1.) < 0.001 && "Trying to rotate a CameraModel about a vector that isn't a unit vector!");
    return vector*cos(angle) +  about.cross(vector)*sin(angle) + about*(about.dot(vector))*(1 - cos(angle));
  }

  void CameraModel::yaw(const float angle)
  {
    fFocalPlane = rodrigesFormula(fFocalPlane - fPosition, fUp, angle) + fPosition;
    fRight = fUp.cross(fFocalPlane - fPosition).norm();
  }

  void CameraModel::pitch(const float angle)
  {
    fFocalPlane = rodrigesFormula(fFocalPlane - fPosition, fRight, angle) + fPosition;
    fUp = (fFocalPlane - fPosition).cross(fRight).norm();
  }

  void CameraModel::zoom(const float multiplier)
  {
    fFocalPlane += (fFocalPlane - fPosition).norm()*multiplier;
  }

  cl_float3 CameraModel::position() const
  {
    //return (fPosition + cl::float3(fUniform(fLCGen), fUniform(fLCGen), fUniform(fLCGen))).data; //TODO: camera jitter here using fUniform and fLCGen
    return fPosition.data;
  }
}
