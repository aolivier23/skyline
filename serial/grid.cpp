//File: grid.cpp
//Brief: Parameters of a grid acceleration structure.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/grid.h"
#endif //NOT_ON_DEVICE

//Find the distance between intersections along each axis for a given ray.
CL(float2) distBetweenCells(const grid params, const ray thisRay)
{
  return params.cellSize/fabs(thisRay.direction.xz);
}

//Find distance to the edge of this cell
CL(float2) distToCellEdge(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  return ((convert_float(currentCell) + step(0.f, thisRay.direction.xz))*params.cellSize + params.origin - thisRay.position.xz)/thisRay.direction.xz;
}

//Find the grid cell at a position.  This might return a cell that is outside params.
//If so, try using nextCell with {0, 0} as currentCell.
CL(int2) positionToCell(const grid params, const CL(float3) pos)
{
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  return convert_int_sat_rtn((pos.xz - params.origin) / params.cellSize);
}

//Find the distance to the camera's first intersection with a grid's boundary.
float grid_intersect(const grid rect, const ray thisRay)
{
  const CL(float2) diff = thisRay.position.xz - (rect.origin + convert_float(rect.max) * rect.cellSize / 2.f);
  const CL(float2) dirInv = (CL(float2)){1.f/thisRay.direction.x, 1.f/thisRay.direction.z};

  //X
  int sign = (dirInv.x > 0)?1:-1;
  //TODO: shape->width needs to be replaced
  float tmin = (-sign * rect.max.x * rect.cellSize.x/2.f - diff.x)*dirInv.x;
  float tmax = (sign * rect.max.x * rect.cellSize.x/2.f - diff.x)*dirInv.x;

  //Y
  sign = (dirInv.y > 0)?1:-1;
  float tother0 = (-sign * rect.max.y * rect.cellSize.y/2.f - diff.y)*dirInv.y;
  float tother1 = (sign * rect.max.y * rect.cellSize.y/2.f - diff.y)*dirInv.y;

  if(tmin > tother1 || tother0 > tmax) return -1;
  tmin = max(tmin, tother0);
  tmax = min(tmax, tother1);

  return (tmin > 0)?tmin:tmax;
}
