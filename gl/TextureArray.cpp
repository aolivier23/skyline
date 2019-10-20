//File: TextureArray.cpp
//Brief: c++ bindings for an OpenGL 2D texture array object.  Referenced texture
//       object is created and destroyed with the c++ object.
//Author: Andrew Olivier aolivier@ur.rochester

#ifndef GL_TEXTUREARRAY_CPP
#define GL_TEXTUREARRAY_CPP

//c++ includes
#include <cassert>

//OpenGL C API includes
#include "glad/include/glad/glad.h"

//gl includes
#include "gl/DebuggingMacros.h"

namespace gl
{
  //INTERNAL_FORMAT: The name of the internal format in which this texture will be stored.
  //        Example: GL_RGBA32UI from https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage3D.xhtml
  //COMPONENT: Type of each component of the vector this texture stores.  Example: GL_UNSIGNED_BYTE
  //TODO: Enforce compatility between (INTERNAL_)INTERNAL_FORMAT and COMPONENT
  template <unsigned int INTERNAL_FORMAT, unsigned int COMPONENT> 
  class TextureArray
  {
    private:
      static constexpr unsigned int TARGET = GL_TEXTURE_2D_ARRAY;

    public:
      TextureArray(const unsigned int width, const unsigned int height,
                   const unsigned int size): fWidth(width), fHeight(height), fSize(size)
      {
        glGenTextures(1, &name);
        glBindTexture(TARGET, name);
  
        //I learned to set up 2D texture arrays from https://www.khronos.org/opengl/wiki/Array_Texture
        //CHECK_GL_ERROR(glTexImage3D, TARGET, 0, INTERNAL_FORMAT, fWidth, fHeight, fSize, 0, INTERNAL_FORMAT, COMPONENT, data);
        CHECK_GL_ERROR(glTexStorage3D, TARGET, 1, INTERNAL_FORMAT, fWidth, fHeight, fSize);
    
        glTexParameteri(TARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(TARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
        glBindTexture(TARGET, 0);
      }
  
      //Replace the image at pos in this texture array.  format is an OpenGL
      //enum for an "external" texutre format, like GL_RGBA, that must be compatible
      //with INTERNAL_FORMAT.
      //The user is responsible for checking that width and height of the data match
      //how this texture array was set up.
      void insert(const unsigned int pos, const unsigned int format, void* data,
                  const unsigned int xOffset = 0, const unsigned int yOffset = 0)
      {
        assert(pos < fSize && "Texture arrays have a fixed size!");
        assert(xOffset < fWidth && "X offset into a texture must be < its width!");
        assert(yOffset < fHeight && "Y offset into a texture must be < its height!");
  
        glBindTexture(TARGET, name);
        CHECK_GL_ERROR(glTexSubImage3D, TARGET, 0, xOffset, yOffset, pos, fWidth, fHeight, 1, format, COMPONENT, data);
  
        glBindTexture(TARGET, 0);
      }
  
      ~TextureArray()
      {
        glDeleteTextures(1, &name);
      }
 
      //Accessor and helper functions 
      inline unsigned int size() const { return fSize; }
      inline bool checkDimensions(const unsigned int width, const unsigned int height) const { return width == fWidth && height == fHeight; }
    
      unsigned int name; //OpenGL identifier for this texture object

    private:
      unsigned int fWidth;
      unsigned int fHeight;
      unsigned int fSize;
  };
}

#endif //GL_TEXTUREARRAY_CPP
