//File: gridCell.h
//Brief: A group of volumes in a cell of a grid acceleration structure.
//       To save memory and organize host-side programs more effectively,
//       boxes are referred to by another "layer" of indices.  These
//       intermediate indices must be contiguous in memory so that gridCell
//       works.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef GRIDCELL_H
#define GRIDCELL_H

typedef struct gridCellTag
{
  int begin; //Index of first volume index in this cell in list of indices
  int end; //Index of last volume index in this cell in list of volume indices

  int filler[2]; //Fill out this structure so that alignment always matches
} gridCell;

#endif //GRIDCELL_H
