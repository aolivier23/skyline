//File: generateRay.h
//Brief: Generate a ray from cameraPos to a pixel in the plane at focalPos defined by up and right.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef GENERATERAY_H
#define GENERATERAY_H

ray generateRay(const CL(int2) pixel, const CL(float3) cameraPos, const CL(float3) focalPos,
                const CL(float3) up, const CL(float3) right, unsigned long int width, unsigned long int height)
{
  ray thisRay;
  thisRay.position = cameraPos;

  const float aspectRatio = (float)width / (float)height;
  const CL(float2) ndc = (CL(float2)){(float)pixel.x/(float)width, (float)pixel.y/(float)height};
  const CL(float3) pixelPos = right*(ndc.x - 0.5f)*aspectRatio + up*(ndc.y - 0.5f) + focalPos;
  thisRay.direction = normalize(pixelPos - thisRay.position);

  return thisRay;
}

#endif //GENERATERAY_H
