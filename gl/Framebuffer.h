//File: Framebuffer.h
//Brief: Wrapper over an OpenGL framebuffer object in c++.  Automatically creates
//       a new texture when resized.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef GL_FRAMEBUFFER_CPP
#define GL_FRAMEBUFFER_CPP

//gl includes
#include "gl/Texture.cpp"

//c++ includes
#include <memory> //For std::unique_ptr

namespace gl
{
  struct Framebuffer
  {
    using texture_t = gl::Texture<GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE>;

    Framebuffer(const int width, const int height);

    ~Framebuffer();

    void resize(const int width, const int height);

    std::unique_ptr<texture_t> clTexture; //Texture that OpenCL will "render" to
    unsigned int name; //Framebuffer object that clTexture is attached to
  };
}

#endif //GL_FRAMEBUFFER_CPP
