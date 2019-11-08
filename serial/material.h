//File: material.h
//Brief: A material describes how light interacts with a shape.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct material_tag
{
  //TODO: Transition to textures for emission too.
  CL(float3) emission; //The color of light emitted by this object, if any

  //Indexes into the array of textures.  Order is front, back, left, right, top, bottom (with 2
  //dummy values for alignment) when the z axis is pointing from back to front and the y axis is
  //up.
  CL(uchar8) textures;
  CL(uchar8) dummy; //For padding
} material;
#endif //MATERIAL_H
