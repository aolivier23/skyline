//File: sphere.cpp
//Brief: A perfect sphere for raytracing rendering along with raytracing
//       queries that act on it.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/sphere.h"
#endif //NOT_ON_DEVICE

//Returns distance from thisRay's origin to shape's surface.
//Adapted from TODO
float sphere_intersect(const sphere shape, const ray thisRay)
{
  const CL(float3) diff = thisRay.position - shape.center;
  const float diffDotDir = dot(diff, thisRay.direction);
  const float disc = diffDotDir*diffDotDir - dot(diff, diff) + shape.radius*shape.radius;
  if(disc < 0) return -1;

  const float sqrtDisc = sqrt(disc);
  float dist = -diffDotDir - sqrtDisc;
  if(dist < 0) dist = -diffDotDir + sqrtDisc;
  return dist;

}

//Return normal vector at point on a sphere
CL(float3) sphere_normal(const sphere shape, const CL(float3) pos)
{
  return (pos - shape.center) / shape.radius;
}

//Return texture coordinates for mapping a rectangle onto a sphere at pos.
CL(float3) sphere_tex_coords(const sphere shape, const CL(float3) pos)
{
  return (CL(float3)){0.5f + atan2(pos.z, pos.x)/2.f/M_PI, 0.5f - asin(pos.y)/M_PI, SKY_TEXTURE};
}
