//File: sphere.h
//Brief: A perfect sphere with functions to intersect it and calculate normals.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef SPHERE_H
#define SPHERE_H

#define SKY_TEXTURE 0

typedef struct sphere_tag
{
  CL(float3) center;
  float radius;
  float dummy[3]; //Ensure alignment matches between host and device
} sphere;

//Return distance from thisRay's origin to its intersection with shape
float sphere_intersect(const sphere shape, const ray thisRay);

//Return normal at pos on a sphere
CL(float3) sphere_normal(const sphere shape, const CL(float3) pos);

//Return texture coordinates for mapping a rectangle onto a sphere at pos
CL(float3) sphere_tex_coords(const sphere shape, const CL(float3) pos);

#endif //SPHERE_H
