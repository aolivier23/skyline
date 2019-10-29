//File: aabb.h
//Brief: Parameters of an axis-aligned box-shaped building for path tracing on
//       the GPU.  This is both valid OpenCL C and c++ code so that
//       it can be used on both the host and the device.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/aabb.h"
#endif //NOT_ON_DEVICE

//OpenCL functions for intersection tests of aabbs
//Return the distance from thisRay's origin to its intersection with shape
float aabb_intersect(__global const aabb* shape, const ray thisRay)
{
  const CL(float3) diff = thisRay.position - shape->center;
  const CL(float3) dirInv = (CL(float3)){1.f/thisRay.direction.x, 1.f/thisRay.direction.y, 1.f/thisRay.direction.z};
  
  //I learned this algorithm from a combination of https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-aabb-intersection and http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
  //TODO: Could I frame this as a vector operation to allow OpenCL to do some SIMD magic?
  //X
  int sign = (dirInv.x > 0)?1:-1;
  float tmin = (-sign*shape->width.x/2.f - diff.x)*dirInv.x;
  float tmax = (sign*shape->width.x/2.f - diff.x)*dirInv.x;

  //Y
  sign = (dirInv.y > 0)?1:-1;
  float tother0 = (-sign*shape->width.y/2.f - diff.y)*dirInv.y;
  float tother1 = (sign*shape->width.y/2.f - diff.y)*dirInv.y;

  if(tmin > tother1 || tother0 > tmax) return -1;
  tmin = max(tmin, tother0);
  tmax = min(tmax, tother1);

  //Z
  sign = (dirInv.z > 0)?1:-1;
  tother0 = (-sign*shape->width.z/2.f - diff.z)*dirInv.z;
  tother1 = (sign*shape->width.z/2.f - diff.z)*dirInv.z;

  if(tmin > tother1 || tother0 > tmax) return -1;
  tmin = max(tmin, tother0);
  tmax = min(tmax, tother1);

  return (tmin > 0)?tmin:tmax;
}

//Return the normal vector at a point on shape 
CL(float3) aabb_normal(const aabb shape, const CL(float3) pos)
{
  //TODO: Actually calculate safety margin
  const CL(float3) diff = pos - shape.center;
  if(fabs(diff.x - shape.width.x/2.f) < FLT_EPSILON*3.f) return (CL(float3)){1.f, 0.f, 0.f};
  if(fabs(diff.x + shape.width.x/2.f) < FLT_EPSILON*3.f) return (CL(float3)){-1.f, 0.f, 0.f};

  if(fabs(diff.y - shape.width.y/2.f) < FLT_EPSILON*3.f) return (CL(float3)){0.f, 1.f, 0.f};
  if(fabs(diff.y + shape.width.y/2.f) < FLT_EPSILON*3.f) return (CL(float3)){0.f, -1.f, 0.f};

  if(fabs(diff.z - shape.width.z/2.f) < FLT_EPSILON*3.f) return (CL(float3)){0.f, 0.f, 1.f};

  return (CL(float3)){0.f, 0.f, -1.f};
}

//Return the normal vector by value and texture coordinates by reference
CL(float3) aabb_normal_tex_coords(const aabb shape, const CL(float3) pos, const material mat, CL(float3)* texCoords)
{
  //TODO: Actually calculate safety margin
  const CL(float3) diff = pos - shape.center;
  //x
  if(fabs(diff.x - shape.width.x/2.f) < FLT_EPSILON*3.f)
  {
    *texCoords = (CL(float3)){diff.z/mat.norm.z + 0.5f, diff.y/mat.norm.y + 0.5f, (float)((unsigned char*)&mat.textures)[0]};
    return (CL(float3)){1.f, 0.f, 0.f};
  }

  if(fabs(diff.x + shape.width.x/2.f) < FLT_EPSILON*3.f)
  {
    *texCoords = (CL(float3)){diff.z/mat.norm.z + 0.5f, diff.y/mat.norm.y + 0.5f, (float)((unsigned char*)&mat.textures)[1]};
    return (CL(float3)){-1.f, 0.f, 0.f};
  }

  //y
  if(fabs(diff.y - shape.width.y/2.f) < FLT_EPSILON*3.f)
  {
    *texCoords = (CL(float3)){diff.x/mat.norm.x + 0.5f, diff.z/mat.norm.z + 0.5f, (float)((unsigned char*)&mat.textures)[2]};
    return (CL(float3)){0.f, 1.f, 0.f};
  }

  if(fabs(diff.y + shape.width.y/2.f) < FLT_EPSILON*3.f)
  {
    *texCoords = (CL(float3)){diff.x/mat.norm.x + 0.5f, diff.z/mat.norm.z + 0.5f, (float)((unsigned char*)&mat.textures)[3]};
    return (CL(float3)){0.f, -1.f, 0.f};
  }

  //z
  if(fabs(diff.z - shape.width.z/2.f) < FLT_EPSILON*3.f)
  {
    *texCoords = (CL(float3)){diff.x/mat.norm.x + 0.5f, diff.y/mat.norm.y + 0.5f, (float)((unsigned char*)&mat.textures)[4]};
    return (CL(float3)){0.f, 0.f, 1.f};
  }

  *texCoords = (CL(float3)){diff.x/mat.norm.x + 0.5f, diff.y/mat.norm.y + 0.5f, (float)((unsigned char*)&mat.textures)[5]};
  return (CL(float3)){0.f, 0.f, -1.f};
}
