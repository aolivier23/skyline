//File: Texture.cpp
//Brief: c++ bindings for an OpenGL texture object.  Referenced texture
//       object is created and destroyed with the c++ object.
//Author: Andrew Olivier aolivier@ur.rochester

#ifndef GL_TEXTURE_CPP
#define GL_TEXTURE_CPP

//OpenGL C API includes
#include "glad/include/glad/glad.h"

//gl includes
#include "gl/DebuggingMacros.h"

namespace gl
{
  //TARGET: The OpenGL texture target to which this object can be bound.  Example: GL_TEXTURE_2D
  //FORMAT: The name of the internal and external format in which this texture will be stored.
  //        Example: GL_RGBA
  //COMPONENT: Type of each component of the vector this texture stores.  Example: GL_UNSIGNED_BYTE
  //TODO: Separate internal and external format
  //TODO: Enforce compatility between (INTERNAL_)FORMAT and COMPONENT
  template <unsigned int TARGET, unsigned int FORMAT, unsigned int COMPONENT> 
  struct Texture
  {
    Texture(const int width, const int height, void* data = nullptr)
    {
      glGenTextures(1, &name);
      glBindTexture(TARGET, name);
  
      CHECK_GL_ERROR(glTexImage2D, TARGET, 0, FORMAT, width, height, 0, FORMAT, COMPONENT, data);
  
      glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  
    ~Texture()
    {
      glDeleteTextures(1, &name);
    }
  
    unsigned int name; //OpenGL identifier for this texture object
  };
}

#endif //GL_TEXTURE_CPP
