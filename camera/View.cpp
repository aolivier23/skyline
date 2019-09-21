//Brief: A View is a user's perspective on a ray-traced scene.  Internally,
//       View works with GLFW to adapt to the window geometry and react to user
//       input.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//engine includes
#include "camera/View.h"

//OpenCL includes
#define __CL_ENABLE_EXCEPTIONS //OpenCL c++ API now throws exceptions
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl.hpp>

//gl includes
#include "gl/Framebuffer.h"

//GLFW includes
#include <GLFW/glfw3.h>

//c++ includes
#include <fstream>
#include <iostream>

namespace eng
{
  View::View(GLFWwindow* window, cl::Context& ctx): fContext(ctx)
  {
    //Set up viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    //Set up reactions to user interaction
    //TODO: On camera interaction, throw away depth buffer with glClear()?  Maybe I can reuse
    //      part of the scene when the camera moves or zooms?
    glfwSetFramebufferSizeCallback(window, [](auto window, const int width, const int height)
                                           {
                                             auto view = (View*)(glfwGetWindowUserPointer(window));
                                             view->resize(width, height);
                                           });

    glfwSetKeyCallback(window, [](auto window, int key, int scancode, int action, int mode)
                               {
                                 if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
                               });

    //Initialize everything
    try
    {
      fbo.reset(new gl::Framebuffer(width, height));
      fWidth = width;
      fHeight = height;
      clImage.reset(new cl::Image2D(ctx, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), width, height));
      glImage.reset(new cl::ImageGL(ctx, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, fbo->clTexture->name));
    }
    catch(const cl::Error& e)
    {
      std::cerr << "Failed to create OpenCL image objects with error " << e.err() << ": " << e.what() << "\n";
      throw e;
    }
    clMemory.push_back(*glImage);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->name);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glfwSetWindowUserPointer(window, this); //Do this last in case an exception is thrown?
  }

  View::~View()
  {
  }

  void View::render(cl::CommandQueue& queue)
  {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    cl::size_t<3> imageSize;
    imageSize[0] = 0;
    imageSize[1] = 0;
    imageSize[2] = 0;

    cl::size_t<3> copySize;
    copySize[0] = fWidth;
    copySize[1] = fHeight;
    copySize[2] = 1;

    queue.enqueueAcquireGLObjects(&clMemory);
    queue.enqueueCopyImage(*clImage, *glImage, imageSize, imageSize, copySize);
    queue.finish();
    queue.enqueueReleaseGLObjects(&clMemory);

    CHECK_GL_ERROR(glBlitFramebuffer, 0, 0, fWidth, fHeight, 0, 0, fWidth, fHeight,
                   GL_COLOR_BUFFER_BIT, GL_NEAREST);

    onRender();
  }

  void View::resize(const int width, const int height)
  {
    fWidth = width;
    fHeight = height;
    glViewport(0, 0, width, height);

    fbo->resize(width, height); //This changes fbo.clTexture->name, so I have to create a new cl::ImageGL
    clImage.reset(new cl::Image2D(fContext, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), width, height));
    glImage.reset(new cl::ImageGL(fContext, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, fbo->clTexture->name));

    clMemory.clear();
    clMemory.push_back(*glImage);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->name);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    userResize(width, height);
  }
}
