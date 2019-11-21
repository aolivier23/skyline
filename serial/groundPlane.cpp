//File: groundPlane.cpp
//Brief: Functions for raytracing an x-z plane at y = 0.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/groundPlane.h"
#endif //ON_DEVICE

//Distance from a ray's origin to its intersection with y = 0.
float groundPlane_intersect(const ray thisRay)
{
  if(fabs(thisRay.direction.y) < 1.e-5) return -1;  //No intersection with the ground unless a ray is pointing down
  return -thisRay.position.y/thisRay.direction.y;
}

//Normal at a point on a plane
CL(float3) groundPlane_normal(const CL(float3) pos)
{
  return (CL(float3)){0., 1., 0.};
}

//Coordinates into a rectangular texture on this plane.
//Normalized so that the original texture fits into a space of size texNorm.
CL(float3) groundPlane_tex_coords(const CL(float2) texNorm, const CL(float3) pos)
{
  return (CL(float3)){pos.x / texNorm.x, pos.z / texNorm.y, GROUND_TEXTURE};
}
