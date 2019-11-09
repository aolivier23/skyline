//File: vector.h
//Brief: Since OpenCL types have cl_ prepended when not compiled for the device,
//       this header defines a macro that prepends cl:: to a type only if it is
//       compiled with NOT_ON_DEVICE defined.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef VECTOR_H
#define VECTOR_H

#ifdef NOT_ON_DEVICE
  #include "algebra/vector.h"
  #include <cmath>
  #define CL(type) cl:: type
  #define SCALAR(type) cl_##type
  #define __global
  #define __local
  #define __private
  using std::min;
  using std::max;
  #define FLT_EPSILON CL_FLT_EPSILON
  template <class VECTOR>
  VECTOR normalize(const VECTOR& vec)
  {
    return vec.norm();
  }
  template <class VECTOR>
  float dot(const VECTOR& lhs, const VECTOR& rhs)
  {
    return lhs.dot(rhs);
  }
#else
  #define CL(type) type
  #define SCALAR(type) type
#endif //NOT_ON_DEVICE

#endif //VECTOR_H
