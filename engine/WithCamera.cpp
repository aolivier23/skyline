//Brief: A WithCamera is a user's perspective on a ray-traced scene.  Internally,
//       WithCamera works with GLFW to adapt to the window geometry and react to user
//       input.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//engine includes
#include "engine/WithCamera.h"
#include "camera/CameraModel.h"
#include "camera/CameraController.h"

//GLFW includes
#include <GLFW/glfw3.h>

namespace eng
{
  WithCamera::WithCamera(GLFWwindow* window, cl::Context& ctx,
                         std::unique_ptr<CameraController>&& camera): View(window, ctx), fCamController(std::move(camera))
  {
    //Set up reactions to user interaction
    //TODO: On camera interaction, throw away depth buffer with glClear()?  Maybe I can reuse
    //      part of the scene when the camera moves or zooms?
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1); //Keep mouse down until it is polled
  }

  WithCamera::~WithCamera()
  {
  }

  //TODO: Ideally, WithCamera doesn't depend on GLFW at all.
  void WithCamera::registerWithGLFW(GLFWwindow* window)
  {
    glfwSetKeyCallback(window, [](auto window, int key, int scancode, int action, int mode)
                               {
                                 if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
                                 else
                                 {
                                   auto view = (WithCamera*)(glfwGetWindowUserPointer(window));
                                   if(view->fCamController->OnKeyPress(key, scancode, action, mode)) view->onCameraChange();
                                 }
                               });
                                                                                                                                         
    glfwSetCursorPosCallback(window, [](auto window, const double x, const double y)
                                     {
                                       auto view = (WithCamera*)(glfwGetWindowUserPointer(window));
                                       if(view->fCamController->OnMouseMotion(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT), x, y))
                                       {
                                         view->onCameraChange();
                                       }
                                     });
                                                                                                                                         
    glfwSetScrollCallback(window, [](auto window, const double xOff, const double yOff)
                                  {
                                    auto view = (WithCamera*)(glfwGetWindowUserPointer(window)); 
                                    if(view->fCamController->OnScroll(xOff, yOff)) view->onCameraChange();
                                  });
  }

  const eng::CameraModel& WithCamera::camera() const
  {
    return fCamController->model();
  }

  void WithCamera::onCameraChange()
  {
    //glClearColor(0, 0, 0, 1);
    //glClear(GL_COLOR_BUFFER_BIT);
  }
}
