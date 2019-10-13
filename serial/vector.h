//File: vector.h
//Brief: Since OpenCL types have cl_ prepended when not compiled for the device,
//       this header defines a macro that prepends cl:: to a type only if it is
//       compiled with NOT_ON_DEVICE defined.
//Author: Andrew Olivier aolivier@ur.rochester.edu

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
#else
  #define CL(type) type
  #define SCALAR(type) type
#endif //__cplusplus
