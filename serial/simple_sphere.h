//File: simple_sphere.h
//Brief: A simple_sphere has the not-so-generalized parameters needed to define a sphere
//       as simply as possible for my first steps into raytracing.  I should define a sphere
//       struct later that will have more generalized properties.
//Author: Andrew Olivier aolivier@ur.rochester.edu

struct simple_sphere
{
  //Pad struct manually so that it has same alignment on CPU and GPU
  //TODO: Pack radius and center into a float4?
  CL(float) radius;
  CL(float) dummy1;
  CL(float) dummy2;
  CL(float) dummy3;

  CL(float3) center;
  CL(float3) color;
  CL(float3) emission;
};
