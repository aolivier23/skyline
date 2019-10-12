//File: GUI.cpp
//Brief: Graphical interface functionality for builder and skyline.
//       Builds on top of Dear ImGui.  Kept in a separate file to
//       contain dependencies on Dear ImGui.  Call these functions
//       near the end of the game loop to use their components.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//app includes
#include "app/GUI.h"
#include "app/CmdLine.h"

//camera includes
#include "camera/WithCamera.h"
#include "camera/CameraController.h"

//Dear ImGui includes
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

//GLFW includes
#include "GLFW/glfw3.h"

//c++ includes
#include <algorithm>

namespace app
{
  //Control the camera by emulating GLFW's callbacks with Dear ImGui.
  bool handleCamera(eng::WithCamera& view, const ImGuiIO& io)
  {
    auto& camera = *view.fCamController;
    bool cameraChanged = false;
    if(!io.WantCaptureMouse)
    {
      if(camera.OnMouseMotion(ImGui::IsMouseDragging(), ImGui::GetMousePos().x, ImGui::GetMousePos().y)) cameraChanged = true;

      if(io.MouseWheel != 0 || io.MouseWheelH != 0)
      {
        if(camera.OnScroll(io.MouseWheelH, io.MouseWheel)) cameraChanged = true;
      }
    }
                                             
    if(!io.WantCaptureKeyboard)
    {
      //TODO: If there's ever a CameraController that needs more keys, this
      //      line needs to know about them.
      for(const auto button: {GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT})
      {
        if(ImGui::IsKeyPressed(button))
        {
          if(camera.OnKeyPress(button, -1, GLFW_PRESS, 0)) cameraChanged = true;
        }
        else if(ImGui::IsKeyReleased(button))
        {
          if(camera.OnKeyPress(button, -1, GLFW_RELEASE, 0)) cameraChanged = true;
        }
      }
    }
                                             
    if(cameraChanged) view.onCameraChange();

    return cameraChanged;
  }

  //Draw a menu allowing the user to select and configure a CameraModel.
  void drawCameras(CmdLine& app, eng::WithCamera& view)
  {
    static bool isOpen = false;

    const bool clicked = ImGui::MenuItem("camera");
    if(!isOpen) isOpen = clicked;

    if(isOpen)
    {
      ImGui::Begin("Cameras", &isOpen);
      cl::float3 newPos = view.fCamController->model().exactPosition();
      if(ImGui::InputFloat3("Position", newPos.data.s, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        view.fCamController->model().setPosition(newPos);
        view.onCameraChange();
      }

      if(ImGui::CollapsingHeader("Active Cameras"))
      {
        for(auto& camera: app.cameras)
        {
          if(ImGui::Selectable(camera.first.c_str()))
          {
            view.fCamController->SetModel(camera.second);
            view.onCameraChange();
          }
        }
      }

      if(ImGui::Button("Save Camera As")) ImGui::OpenPopup("Save Camera");

      if(ImGui::BeginPopupModal("Save Camera"))
      {
        static std::string newName;
        if(ImGui::InputText("Camera Name", &newName, ImGuiInputTextFlags_EnterReturnsTrue))
        {
          app.cameras.emplace_back(newName, view.fCamController->model());
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
      ImGui::End();
    }
  }

  void drawMetrics(const ImGuiIO& io)
  {
    static bool isOpen = false;

    const bool clicked = ImGui::MenuItem("metrics");
    if(!isOpen) isOpen = clicked;

    if(isOpen)
    {
      ImGui::Begin("Metrics", &isOpen);

      //Average Overall framerate
      static long int nFrames = 0.;
      static float runningAvg = 0.;
      static std::array<float, 30> timeBuffer{0};

      ++nFrames;

      //Average over 10 frames
      constexpr int samplesPerUpdate = 10;
      runningAvg += io.DeltaTime * 1000.; //in ms
      if(nFrames % samplesPerUpdate == 0)
      {
        std::rotate(timeBuffer.begin(), timeBuffer.begin() + 1, timeBuffer.end());
        timeBuffer[0] = runningAvg / (float)samplesPerUpdate;
        runningAvg = 0.;
      }

      ImGui::PlotLines("", timeBuffer.data(), timeBuffer.size(), 0,
                       ("Average Frame Time: " + std::to_string(timeBuffer[0]) + " ms (~" + std::to_string(io.Framerate) + " FPS)").c_str(),
                       0., 2.*timeBuffer[0], ImVec2(0, 80));
      
      ImGui::End();
    }
  }

  void drawHelp()
  {
    static bool isOpen = false;

    const bool clicked = ImGui::MenuItem("help");
    if(!isOpen) isOpen = clicked;

    if(isOpen)
    {
      ImGui::Begin("Help", &isOpen);
      if(ImGui::ListBoxHeader("Camera"))
      {
        ImGui::BulletText("Use the arrow keys to move around the floor plane.");
        ImGui::BulletText("Click and drag the left mouse button to aim the camera.");
        ImGui::BulletText("Scroll to zoom or out.");
        ImGui::BulletText("The camera is supposed to \"jitter\" slightly.\nThis implements anti-aliasing.");
        ImGui::BulletText("You can save multiple camera configurations and\nmanually enter camera positions in the \"camera\" menu.");
        ImGui::ListBoxFooter();
      }

      if(ImGui::ListBoxHeader("GUI"))
      {
        ImGui::ShowUserGuide();
        ImGui::ListBoxFooter();
      }
      ImGui::End();
    }
  }

  void drawFile(app::CmdLine& app)
  {
    if(ImGui::BeginMenu("file"))
    {
      if(ImGui::MenuItem("open")) {} //TODO: File browser for YAML files
      if(ImGui::MenuItem("save as")) {} //TODO: Save As menu -> serialization logic
      ImGui::EndMenu();
    }
  }
}
