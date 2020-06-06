//File: grid.cpp
//Brief: Parameters of a grid acceleration structure.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
#include "serial/grid.h"
#endif //NOT_ON_DEVICE

float distToNextCell(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  //Find distance to the closest of the 4 planes that define this cell
  //However, the grid's origin doesn't have to be at (0, 0).
  const CL(float2) one = (CL(float2)){1.f, 1.f}, epsilon = (CL(float2)){0.001f, 0.001f};
  const CL(float2) distancesBack = ((convert_float(currentCell) - epsilon)*params.cellSize + params.origin - thisRay.position.xz) / thisRay.direction.xz;
  const CL(float2) distancesFront = ((convert_float(currentCell) + one + epsilon)*params.cellSize + params.origin - thisRay.position.xz) / thisRay.direction.xz;

  //Pick the largest distnce for each component.  That's the direction thisRay is going.
  const CL(float2) distances = max(distancesBack, distancesFront);

  //Pick the closest intersection with a cell boundary.  That's the distance to the first cell that thisRay enters.
  //TODO: Crazy things happen when the camera is outside the grid.  Boxes get partially and/or suddenly cut off.
  //TODO: Boxes are getting cut off as the camera moves both all at once, like being in the wrong grid cell, and
  //      smoothly with position.  It seems almost like one side of a grid cell, probably the larger (x, z) side, turns on suddenly,
  //      and the other side turns off gradually.  Could this be truncating behavior?
  //      The ternary operator-based version never cut off boxes with camera motion.  Why does that happen to the round()-based version?
  //      The gradual-cutting-off behavior sounds a little like grid cells being cut off of being rounded up to the next integer.
  //      The gradual-cutting-off behavior persists even when using positionToCell(), which rounds to 0, instead of round().
  //      After setting round to 0 mode, it seems like I'm now seeing every building correctly except one at the (-x, -z)
  //      corner of the grid that's being split in half from the middle.  This building seems wide enough to be in multiple grid cells.
  //TODO: I'm getting into an infinite loop at large y with hardEnough.yaml.  Maybe the problem has to do with rays that are
  //      missing all buildings and leaving the grid?  Seems like that happens at ground level too when I see the background.
  //TODO: I learned from https://www.scratchapixel.com/lessons/advanced-rendering/introduction-acceleration-structure/grid that I have to be more careful
  //      with objects that are in multiple cells.  I could intersect a part of an object that's in another cell, and my current
  //      loop would stop there.
  return min(distances.x, distances.y);
}

//Find the next cell that this ray enters assuming that thisRay already intersects currentCell.
//TODO: Deprecate this function and just call distToNextCell() + positionToCell()?
CL(int2) nextCell(const grid params, const ray thisRay, const CL(int2) currentCell)
{
  const float distToIntersect = distToNextCell(params, thisRay, currentCell);

  return positionToCell(params, thisRay.position + thisRay.direction * distToIntersect);
}

//Find the grid cell at a position.  This might return a cell that is outside params.
//If so, try using nextCell with {0, 0} as currentCell.
CL(int2) positionToCell(const grid params, const CL(float3) pos)
{
  //TODO: This line uses swizzling, so it won't compile on the host without major upgrades to my vector library
  return convert_int_sat_rtn((pos.xz - params.origin) / params.cellSize);
}
