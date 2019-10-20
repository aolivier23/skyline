//File: aabb.h
//Brief: Parameters of an axis-aligned box-shaped building for path tracing on
//       the GPU.  This is both valid OpenCL C and c++ code so that
//       it can be used on both the host and the device.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef AABB_H
#define AABB_H

typedef struct aabb_tag
{
  CL(float3) width;
  CL(float3) center;

  SCALAR(int) material;
  //Dummy ints to ensure same alignment on host and device
  SCALAR(int) dummy1;
  SCALAR(int) dummy2;
  SCALAR(int) dummy3;
} aabb;

//OpenCL functions for intersection tests of aabbs
//Return the distance from thisRay's origin to its intersection with shape
float aabb_intersect(__global const aabb* shape, const ray thisRay);

//Return the normal vector at a point on shape 
CL(float3) aabb_normal(const aabb shape, const CL(float3) pos);

//Return normal by value and texture coordinates by reference
CL(float3) aabb_normal_tex_coords(const aabb shape, const CL(float3) pos, CL(float3)* texCoords);
#endif //AABB_H
