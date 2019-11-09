//File: groundPlane.h
//Brief: Functions for raytracing an x-z plane at y = 0.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef GROUNDPLANE_H
#define GROUNDPLANE_H

#define GROUND_TEXTURE 1

//Distance from a ray's origin to its intersection with y = 0.
float plane_intersect(const ray thisRay);

//Normal at a point on a plane
CL(float3) plane_normal(const CL(float3) pos);

//Coordinates into a rectangular texture on this plane
CL(float3) plane_tex_coords(const CL(float2) texNorm, const CL(float3) pos);

#endif //GROUNDPLANE_H
