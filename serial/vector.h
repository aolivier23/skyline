//File: vector.h
//Brief: Since OpenCL types have cl_ prepended when not compiled for the device,
//       this header defines a macro that prepends cl_ to a type only if it is
//       compiled with NOT_ON_DEVICE defined.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifdef NOT_ON_DEVICE
  #define CL(type) cl_##type
#else
  #define CL(type) type
#endif //__cplusplus
