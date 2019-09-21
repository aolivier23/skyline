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

  CL(int) material;
  //Dummy ints to ensure same alignment on host and device
  CL(int) dummy1;
  CL(int) dummy2;
  CL(int) dummy3;
} aabb;

#ifndef NOT_ON_DEVICE
//OpenCL functions for intersection tests of aabbes
//Return the distance from thisRay's origin to its intersection with shape
float aabb_intersect(__global const aabb* shape, const ray thisRay)
{
  const float3 diff = thisRay.position - shape->center;
  const float3 dirInv = (float3)(1./thisRay.direction.x, 1./thisRay.direction.y, 1./thisRay.direction.z);
  
  //I learned this algorithm from a combination of https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-aabb-intersection and http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
  //TODO: Could I frame this as a vector operation to allow OpenCL to do some SIMD magic?
  //X
  int sign = (dirInv.x > 0)?1:-1;
  float tmin = (-sign*shape->width.x/2. - diff.x)*dirInv.x;
  float tmax = (sign*shape->width.x/2. - diff.x)*dirInv.x;

  //Y
  sign = (dirInv.y > 0)?1:-1;
  float tother0 = (-sign*shape->width.y/2. - diff.y)*dirInv.y;
  float tother1 = (sign*shape->width.y/2. - diff.y)*dirInv.y;

  if(tmin > tother1 || tother0 > tmax) return -1;
  tmin = max(tmin, tother0);
  tmax = min(tmax, tother1);

  //Z
  sign = (dirInv.z > 0)?1:-1;
  tother0 = (-sign*shape->width.z/2. - diff.z)*dirInv.z;
  tother1 = (sign*shape->width.z/2. - diff.z)*dirInv.z;

  if(tmin > tother1 || tother0 > tmax) return -1;
  tmin = max(tmin, tother0);
  tmax = min(tmax, tother1);

  return (tmin > 0)?tmin:tmax;
}

//Return the normal vector at a point on shape 
float3 aabb_normal(const aabb shape, const float3 pos)
{
  //TODO: Actually calculate safety margin
  const float3 diff = pos - shape.center;
  if(fabs(diff.x - shape.width.x/2.) < FLT_EPSILON*3.) return (float3)(1., 0., 0.);
  if(fabs(diff.x + shape.width.x/2.) < FLT_EPSILON*3.) return -(float3)(1., 0., 0.);

  if(fabs(diff.y - shape.width.y/2.) < FLT_EPSILON*3.) return (float3)(0., 1., 0.);
  if(fabs(diff.y + shape.width.y/2.) < FLT_EPSILON*3.) return -(float3)(0., 1., 0.);

  if(fabs(diff.z - shape.width.z/2.) < FLT_EPSILON*3.) return (float3)(0., 0., 1.);

  return -(float3)(0., 0., 1.);
}
#endif //ifndef NOT_ON_DEVICE
#endif //AABB_H
