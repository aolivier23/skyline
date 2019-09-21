//File: FPSController.h
//Brief: A FPSController is a CameraController that lets the user move a camera
//       like in a first-person shooter video game.  The left and right arrows
//       move the camera to either side, and the front and back arrows move the
//       camera forward and backward in the direction the camera is facing (from
//       the camera position to the center of the focal plane).  The mouse lets
//       the camera be aimed.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_FPSCONTROLLER_H
#define ENG_FPSCONTROLLER_H

//Base class includes
#include "camera/CameraController.h" 

namespace eng
{
  class FPSController: public CameraController
  {
    public:
      FPSController(std::unique_ptr<CameraModel>&& model, const float moveSpeed, const float zoomSpeed,
                    const double mouseX = 0., const double mouseY = 0.);
      virtual ~FPSController();

      //Public interface is provided by CameraController

    protected:
      //Additional state needed to interpret user input
      float fMoveSpeed; //Camera movement speed when reacting to user
      float fZoomSpeed; //Camera zoom speed when reacting to user

      //Interfaces for reacting to user input.  They return true when the CameraModel is changed.
      virtual bool HandleMouse(const int action, double xpos, double ypos) override;
      virtual bool HandleKey(int key, int scancode, int action, int mods) override;
      virtual bool HandleScroll(double xoffset, double yoffset) override;
  };
}

#endif //ENG_FPSCONTROLLER_H
