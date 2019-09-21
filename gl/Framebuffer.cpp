//File: Framebuffer.cpp
//Brief: Wrapper over an OpenGL framebuffer object in c++.  Automatically creates
//       a new texture when resized.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//OpenGL C API includes
#include "glad/include/glad/glad.h"

//gl includes
#include "gl/Texture.cpp"
#include "gl/Framebuffer.h"
#include "gl/Exception.h"

namespace gl
{
  Framebuffer::Framebuffer(const int width, const int height)
  {
    glGenFramebuffers(1, &name);
    resize(width, height);
  }

  Framebuffer::~Framebuffer()
  {
    glDeleteFramebuffers(1, &name);
  }

  void Framebuffer::resize(const int width, const int height)
  {
    //glViewport(0, 0, width, height);
    clTexture.reset(new texture_t(width, height));

    glBindFramebuffer(GL_FRAMEBUFFER, name);
    CHECK_GL_ERROR(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, clTexture->name, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      throw gl::Exception("Bad Framebuffer") << "Failed to create a complete framebuffer.  OpenGL context seems to be working, but I can't"
                                             << " render anything.  So, returning with an error.\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}
