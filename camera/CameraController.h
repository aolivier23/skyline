//File: CameraController.h
//Brief: A CameraController interprets user input into interactions with a CameraModel. 
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_CAMERACONTROLLER_H
#define ENG_CAMERACONTROLLER_H

//Model includes
#include "camera/CameraModel.h"

//c++ includes
#include <memory>

namespace eng
{
  class CameraController
  {
    public:
      CameraController(CameraModel& model, const double mouseX = 0., const double mouseY = 0.);
      virtual ~CameraController();

      //Hooks for GLFW callbacks.  They return true if anything changed.
      bool OnMouseMotion(const int action, double xpos, double ypos);
      bool OnKeyPress(int key, int scancode, int action, int mods);
      bool OnScroll(double xoffset, double yoffset);
      //virtual bool OnScreenSizeChange(double width, double height);

      //Control behavior of CameraController
      void SetModel(CameraModel& model); //Forget old model and start managing a new one
      CameraModel& model();

    protected:
      CameraModel* fModel; //Observer pointer to model of where the camera is

      //Cache previous position of mouse
      double fPrevMouseX;
      double fPrevMouseY;  

      //Interfaces that user must implement.  They should return true when the CameraModel changes.
      virtual bool HandleMouse(const int action, double xpos, double ypos) = 0;
      virtual bool HandleKey(int key, int scancode, int action, int mods) = 0;
      virtual bool HandleScroll(double xoffset, double yoffset) = 0;
  };
}

#endif //ENG_CAMERACONTROLLER_H 
