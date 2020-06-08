//File: grid.h
//Brief: Parameters of a grid acceleration structure.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef GRID_H
#define GRID_H

typedef struct gridTag
{
  CL(int2) max; //Extent of the grid in x and y
  CL(float2) cellSize; //Size of each cell
  CL(float2) origin; //Global position of the center of this grid
  CL(int2) filler; //Keep alignment the same on CPU and GPU
} grid;

//Find the next cell that this ray enters
CL(int2) nextCell(const grid params, const ray thisRay, const CL(int2) currentCell);

//Find the grid cell at a position.  This might return a cell that is outside params.
//If so, try using nextCell with {-1, -1} as currentCell.
CL(int2) positionToCell(const grid params, const CL(float3) pos);

//Find the distance to a ray's intersection point with the grid
float grid_intersect(const grid rect, const ray thisRay);

#endif //GRID_H
