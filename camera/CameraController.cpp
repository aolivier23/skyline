//File: CameraController.cpp
//Brief: A CameraController is the user's interface to a CameraModel. 
//Author: Andrew Olivier aolivier@ur.rochester.edu

//Include header
#include "camera/CameraController.h"

namespace eng
{
  CameraController::CameraController(std::unique_ptr<eng::CameraModel>&& model,
                                     const double mouseX, const double mouseY): fModel(nullptr), fPrevMouseX(mouseX), 
                                                                                fPrevMouseY(mouseY)
  {
    SetModel(std::move(model));
  }

  CameraController::~CameraController()
  {
    //A CameraController does not own fModel, so nothing to explicitly manage here
  }

  bool CameraController::OnMouseMotion(const int action, double xpos, double ypos)
  {
    const bool result = HandleMouse(action, xpos, ypos);
    fPrevMouseX = xpos;
    fPrevMouseY = ypos;
    return result;
  }

  bool CameraController::OnKeyPress(int key, int scancode, int action, int mods)
  {
    return HandleKey(key, scancode, action, mods);
  }

  bool CameraController::OnScroll(double xoffset, double yoffset)
  {
    return HandleScroll(xoffset, yoffset);
  }

  void CameraController::SetModel(std::unique_ptr<CameraModel>&& model)
  {
    fModel = std::move(model);
  }

  CameraModel& CameraController::model()
  {
    return *fModel;
  }

  /*bool CameraController::OnScreenSizeChange(double width, double height)
  {
    return fModel->resize(width, height);
  }*/
}  
