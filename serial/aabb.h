//File: aabb.h
//Brief: Parameters of an axis-aligned box-shaped building for path tracing on
//       the GPU.  This is both valid OpenCL C and c++ code so that
//       it can be used on both the host and the device.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef AABB_H
#define AABB_H

typedef struct aabb_tag
{
  CL(float3) width; //x, y, and z total widths
  CL(float3) center; //Center in global coordinates
  CL(float3) texNorm; //Divide position on a face by these values to get
                      //texture coordinates.  Setting this to match width
                      //stretches a texture over a box's face.  Setting it
                      //to {1.f, 1.f, 1.f} maps a texture across a
                      //building's face at the texture's original size,
                      //repeating if necessary.

  SCALAR(int) material;
  //Dummy ints to ensure same alignment on host and device
  SCALAR(int) dummy1;
  SCALAR(int) dummy2;
  SCALAR(int) dummy3;
} aabb;

//OpenCL functions for intersection tests of aabbs
//Return the distance from thisRay's origin to its intersection with shape
float aabb_intersect(__local const aabb* shape, const ray thisRay);

//Return the normal vector at a point on shape 
CL(float3) aabb_normal(const aabb shape, const CL(float3) pos);

//Return normal by value and texture coordinates by reference
CL(float3) aabb_normal_tex_coords(const aabb shape, const CL(float3) pos, const material mat, CL(float3)* texCoords);
#endif //AABB_H
