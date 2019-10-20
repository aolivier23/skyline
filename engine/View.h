//View.h
//Brief: A View is a user's perspective on a ray-traced scene.  Internally,
//       View works with GLFW to adapt to the window geometry and react to user
//       input.  To render to an OpenGL window, write to clImage, then call View::render().
//       If you want to add to the previous frame, read glImage in the kernel that writes
//       to clImage. 
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_VIEW_H
#define ENG_VIEW_H

//OpenCL includes
#include <CL/cl_gl.h>

//c++ includes
#include <memory> //std::unique_ptr
#include <vector>

namespace cl
{
  class Context;
  class ImageGL;
  class Image2D;
  class Memory;
  class CommandQueue;
}

namespace gl
{
  class Framebuffer;
}

class GLFWwindow;

namespace eng
{
  //TODO: Rename me to something like Screen or Presenter.
  //      Maybe even make this a class template whose parameter is
  //      objects to reallocate on screen resize.
  class View
  {
    protected:
      cl::Context& fContext;

    public:
      View(GLFWwindow* window, cl::Context& ctx);
      virtual ~View();
  
      void render(cl::CommandQueue& queue);
      void resize(const int width, const int height);

      unsigned int fWidth;
      unsigned int fHeight;
      std::unique_ptr<cl::Image2D> clImage;
      std::unique_ptr<cl::ImageGL> glImage;

    protected:
      //Optional user extension interface.  Derive from View if you need anything
      //else to be done on a window resize.
      virtual void userResize(const int /*width*/, const int /*height*/) {}
      virtual void onRender() {}

    private:
      std::unique_ptr<gl::Framebuffer> fbo;
      std::vector<cl::Memory> clMemory;
  };
}

#endif //ENG_VIEW_H
