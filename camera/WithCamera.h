//WithCamera.h
//Brief: A View with a CameraController available.  The CameraController is connected
//       to user input at construction time, and the user can access its current state
//       via the model() member function.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef ENG_WITHCAMERA_H
#define ENG_WITHCAMERA_H

//engine includes
#include "camera/View.h"

namespace eng
{
  class CameraController;
  class CameraModel;

  class WithCamera: public View
  {
    public:
      WithCamera(GLFWwindow* window, cl::Context& ctx, std::unique_ptr<CameraController>&& camera);
      virtual ~WithCamera();

      const eng::CameraModel& camera() const;

      static void registerWithGLFW(GLFWwindow* window);

     //TODO: Hack to integrate Dear ImGui quickly.  Hide these features from the user again.
     //private:
      //Optional interface to make other scene updates when the camera is changed in any way
      virtual void onCameraChange();

      std::unique_ptr<CameraController> fCamController;
  };
}

#endif //ENG_WITHCAMERA_H
