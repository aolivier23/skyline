//File: camera.h
//Brief: Camera data used by skyline's rendering kernels.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef CAMERA_H
#define CAMERA_H

#ifdef NOT_ON_DEVICE
#include "serial/ray.h"
#endif //NOT_ON_DEVICE

typedef struct camera_tag
{
  CL(float3) position;
  CL(float3) focalPos;
  CL(float3) right;
  CL(float3) up;
  float size; //How tall is this camera in the same units as the geometry
  float dummy[3]; //Ensure alignment
} camera;

ray generateRay(const camera cam, const CL(int2) pixel, unsigned long int width, unsigned long int height, size_t* seed);

#endif //CAMERA_H
