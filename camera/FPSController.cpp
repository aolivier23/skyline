//File: FPSContoller.cpp
//Brief: A FPSController lets the user move the camera in a sphere around its' current target and move in a plane orthogonal 
//       to the camera's current direction.  
//Author: Andrew Olivier aolivier@ur.rochester.edu

//Header includes
#include "camera/FPSController.h"

//GLFW includes
#include <GLFW/glfw3.h>

//c++ includes
#include <cmath>

namespace eng
{
  FPSController::FPSController(CameraModel& model, const float moveSpeed, const float zoomSpeed,
                               const double mouseX, const double mouseY): CameraController(model, mouseX, mouseY), 
                                                                          fMoveSpeed(moveSpeed), fZoomSpeed(zoomSpeed)
  {
  }

  FPSController::~FPSController()
  {
  }

  bool FPSController::HandleMouse(const int action, double xpos, double ypos)
  {
    if(action)
    {
      if(ypos) fModel->pitch((ypos - fPrevMouseY)/180.*M_PI/5.);
      if(xpos) fModel->yaw((xpos - fPrevMouseX)/180.*M_PI/5.);
      return true; //The camera model changed
    }
    return false; //No change to model
  }

  bool FPSController::HandleKey(int key, int /*scancode*/, int action, int /*mods*/)
  {
    if(action == GLFW_RELEASE) return false;
   
    if(key == GLFW_KEY_UP)
    {
      fModel->translate({0., 0., fMoveSpeed});
      return true;
    }
    if(key == GLFW_KEY_DOWN)
    {
      fModel->translate({0., 0., -fMoveSpeed});
      return true;
    }
    if(key == GLFW_KEY_LEFT)
    {
      fModel->translate({-fMoveSpeed, 0., 0.});
      return true;
    }
    if(key == GLFW_KEY_RIGHT)
    {
      fModel->translate({fMoveSpeed, 0., 0.});
      return true;
    }

    return false; //If we get here, nothing changed
  }

  bool FPSController::HandleScroll(double /*xoffset*/, double yoffset)
  {
    fModel->zoom(yoffset*fZoomSpeed);
    return true; //The camera model always changes
  }
}
