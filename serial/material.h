//File: material.h
//Brief: A material describes how light interacts with a shape.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct material_tag
{
  CL(float3) color; //The color of this object
  CL(float3) emission; //The color of light emitted by this object, if any

  //Attributes that will require extra rays
  //TODO: index of refraction
  //TODO: opacity
  //TODO: shininess
} material;

#endif //MATERIAL_H
