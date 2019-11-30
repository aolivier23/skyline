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
  //N.B.: On the next line, I only want a grid in x and z.  So, x becomes u, and z becomes v.
  const CL(float2) distances = (thisRay.position.xz - (sign(thisRay.direction.xz) + convert_float2(currentCell))*params.cellSize - params.origin) / thisRay.direction.xz;
  const float distToIntersect = min(distances.x, distances.y);

  //Let the automatic conversion from int to float in each component truncate to grid cell index
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  //TODO: Do I really need to add currentCell here?  I don't think so atm.
  return convert_int2(((thisRay.position + thisRay.direction*distToIntersect).xz - params.origin)/params.cellSize);
}

//Find the grid cell at a position.  This might return a cell that is outside params.
//If so, try using nextCell with {0, 0} as currentCell.
CL(int2) positionToCell(const grid params, const CL(float3) pos)
{
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  return convert_int2((pos.xz - params.origin) / params.cellSize);
}
