//File: ray.h
//Brief: Data needed to test for intersections of a ray with a scene.
//       Common memory format between c++ and OpenCL lets me transfer
//       rays from host to device.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef RAY_H
#define RAY_H

typedef struct ray_tag
{
  CL(float3) position;
  CL(float3) direction;
} ray;

#endif //RAY_H
