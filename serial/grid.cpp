//File: grid.cpp
//Brief: Parameters of a grid acceleration structure.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/grid.h"
#endif //NOT_ON_DEVICE

//TODO: The following code only works in OpenCL C
#ifndef NOT_ON_DEVICE
float distToNextCell(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  //Find distance to the closest of the 4 planes that define this cell
  //However, the grid's origin doesn't have to be at (0, 0).
  const CL(float2) dirInv = (CL(float2)){1.f/thisRay.direction.x, 1.f/thisRay.direction.z};
  const CL(float2) one = (CL(float2)){1.f, 1.f}, epsilon = (CL(float2)){0.001f, 0.001f}; //TODO: Maybe epsilon is the problem?  Scratchapixel doesn't have that.
  const CL(float2) distancesBack = ((convert_float(currentCell) - epsilon)*params.cellSize + params.origin - thisRay.position.xz) * dirInv;
  const CL(float2) distancesFront = ((convert_float(currentCell) + one + epsilon)*params.cellSize + params.origin - thisRay.position.xz) * dirInv;

  //Pick the largest distance for each component.  That's the direction thisRay is going.
  const CL(float2) distances = max(distancesBack, distancesFront);

  //Pick the closest intersection with a cell boundary.  That's the distance to the first cell that thisRay enters.
  //TODO: I'm getting into an infinite loop at large y with hardEnough.yaml.  Maybe the problem has to do with rays that are
  //      missing all buildings and leaving the grid?  Seems like that happens at ground level too when I see the background.
  return min(distances.x, distances.y);
}

//Find the next cell that this ray enters assuming that thisRay already intersects currentCell.
//TODO: Deprecate this function and just call distToNextCell() + positionToCell()?
CL(int2) nextCell(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  const float distToIntersect = distToNextCell(params, thisRay, currentCell);

  return positionToCell(params, thisRay.position + thisRay.direction * distToIntersect);
}
#endif //NOT_ON_DEVICE

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
