//File: groundPlane.cpp
//Brief: Functions for raytracing an x-z plane at y = 0.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ON_DEVICE
#include "serial/groundPlane.cpp"
#endif //ON_DEVICE

//Distance from a ray's origin to its intersection with y = 0.
CL(float) groundPlane_intersect(const ray thisRay)
{
  return -ray.position.y/ray.direction.y;
}

//Normal at a point on a plane
CL(float3) groundPlane_normal(const CL(float3) /*pos*/)
{
  return (CL(float3)){0., 0., 1.};
}

//Coordinates into a rectangular texture on this plane.
//Normalized so that the original texture fits into a space of size texNorm.
CL(float2) groundPlane_tex_coords(const CL(float2) texNorm, const CL(float3) pos)
{
  return (CL(float2)){pos.x / texNorm.x, pos.z / texNorm.y};
}
