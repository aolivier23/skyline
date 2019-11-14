//File: camera.h
//Brief: Camera data used by skyline's rendering kernels.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/camera.h"
#endif

//TODO: lens simulation
ray generateRay(const camera cam, const CL(int2) pixel, unsigned long int width, unsigned long int height)
{
  ray thisRay;
  thisRay.position = cam.position;

  const float aspectRatio = (float)width / (float)height;
  const CL(float2) ndc = (CL(float2)){(float)pixel.x/(float)width, (float)pixel.y/(float)height};
  const CL(float3) pixelPos = cam.right*(ndc.x - 0.5f)*aspectRatio + cam.up*(ndc.y - 0.5f) + cam.focalPos;
  thisRay.direction = normalize(pixelPos /** cam.size*/ - thisRay.position);

  return thisRay;
}
