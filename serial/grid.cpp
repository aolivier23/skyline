//File: grid.cpp
//Brief: Parameters of a grid acceleration structure.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/grid.h"
#endif //NOT_ON_DEVICE

//Find the next cell that this ray enters assuming that thisRay already intersects currentCell.
CL(int2) nextCell(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  //Find distance to the closest of the 4 planes that define this cell
  //position = slope*distance + planeOrigin
  //-> distance = (position[i] - planeOrigin[i])/slope[i]
  //However, the grid's origin doesn't have to be at (0, 0).
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  //N.B.: On the next line, I only want a grid in x and z.  So, x stays x, and z gets compared to y.
  //TODO: I'm getting unexpected results from this function.  Seems like buildings are clipped as a function of camera position.
  const CL(float2) distances = (thisRay.position.xz - (sign(thisRay.direction.xz) + convert_float2(currentCell))*params.cellSize - params.origin) / thisRay.direction.xz;
  return currentCell + ((distances.y < distances.x)?(int2){0, 1}:(int2){1, 0});
}

//Find the grid cell at a position.  This might return a cell that is outside params.
//If so, try using nextCell with {0, 0} as currentCell.
CL(int2) positionToCell(const grid params, const CL(float3) pos)
{
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  return convert_int2((pos.xz - params.origin) / params.cellSize);
}
