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
#include "camera/CameraController.h"

//engine includes
#include "engine/WithCamera.h"
#include "engine/WithRandomSeeds.h"

//Dear ImGui includes
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

//GLFW includes
#include "GLFW/glfw3.h"

//c++ includes
#include <algorithm>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace
{
  //Recursively draw a tree of directories.  Each entry in the tree can be selected.
  //Selecting a directory sets pwd.  Selecting a file sets fileName.
  //Returns true only if fileName was changed.
  bool directoryTree(const fs::path& path, fs::path& pwd, std::string& fileName)
  {
    bool foundFile = false;

    ImGuiTreeNodeFlags flags = fs::is_directory(path)?0:ImGuiTreeNodeFlags_Leaf;
    const bool open = ImGui::TreeNodeEx(("##" + path.filename().string()).c_str(), flags);
    ImGui::SameLine(); if(ImGui::Button(path.filename().c_str()))
    {
      if(fs::is_directory(path)) pwd = path;
      else
      {
        fileName = path;
        foundFile = true;
      }
    }
    if(open)
    {
      if(fs::is_directory(path))
      {
        for(const auto& child: fs::directory_iterator(path))
        {
          if(directoryTree(child, pwd, fileName)) foundFile = true;
        }
      }
      ImGui::TreePop();
    }
    return foundFile;
  }

  //Select the volume under a pixel by path tracing on the host.
  //TODO: Should this be a member function of CmdLine to access the aabbs
  //      on the host?  I could imagine using it with CameraController
  //      directly in oneCell and skyline.  I have to remember to upload
  //      each change that I make to the GPU, and CmdLine might be a good
  //      place to centralize that behavior.  Some kind of smart pointer
  //      that calls uploadToGPU() after each non-const access or on
  //      destruction might be ideal.  If I'm going to expose aabbs, I
  //      have to expose materials too.
  /*aabb& select(const app::CmdLine& geom)
  {
    
  }*/
}

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
      ImGui::Begin("Camera", &isOpen);
      cl::float3 newPos = view.fCamController->model().exactPosition();
      if(ImGui::InputFloat3("Position", newPos.data.s, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        view.fCamController->model().setPosition(newPos);
        view.onCameraChange();
      }

      if(ImGui::CollapsingHeader("Perspectives"))
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

      //TODO: Provide a list of CameraControllers?

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

      if(ImGui::ListBoxHeader("Editor"))
      {
        ImGui::BulletText("Select a box for editing by double-clicking on it.  The skybox cannot be selected this way.");
        ImGui::BulletText("Create a new box by clicking on empty space (i.e. the skybox).");
        ImGui::BulletText("A box is de-selected when a new box is selected or the editor GUI is closed.");
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

  //Draw a file selection window if the "file" menu item is clicked.
  //Allows the user to load a new CmdLine from a file.  The file is selected
  //from a list tree rooted in a working directory internal to this function.
  //Returns true if a new file was loaded.
  bool drawFile(app::CmdLine& app)
  {
    static bool showOpen = false, showSave = false;

    if(ImGui::BeginMenu("file"))
    {
      ImGui::MenuItem("open", "CTRL+o", &showOpen);
      ImGui::MenuItem("save as", "CTRL+s", &showSave);

      ImGui::EndMenu();
    }

    if(showOpen)
    {
      ImGui::Begin("Open", &showOpen);

      //Show the components of the current working directory like the top of the Ubuntu 18 file browser.
      static fs::path pwd = fs::current_path();
      auto foundDir = pwd.end();

      for(auto dirPtr = pwd.begin(); dirPtr != pwd.end(); ++dirPtr)
      {
        ImGui::SameLine();
        if(ImGui::Button(dirPtr->filename().c_str())) foundDir = dirPtr;
        ImGui::SameLine();
        ImGui::Text("/");
      }

      //If the user selected a new directory, make it the working directory for this GUI.
      //The OS's pwd remains the same.
      if(foundDir != pwd.end())
      {
        fs::path newPwd;
        for(auto dir = pwd.begin(); dir != std::next(foundDir); ++dir) newPwd /= *dir;
        pwd = newPwd;
      }

      //Show a directory tree rooted in pwd
      std::string fileName;
      if(::directoryTree(pwd, pwd, fileName))
      {
        app::CmdLine newFile;
        try
        {
          newFile.load(fileName);
          app = newFile;
          showOpen = false;
          ImGui::End();
          return true;
        }
        catch(const CmdLine::exception& e)
        {
          //TODO: Warn the user somehow that we couldn't open fileName
        }
      }

      ImGui::End();
    }

    if(showSave)
    {
      ImGui::Begin("Save As", &showSave);

      //Show the components of the current working directory like the top of the Ubuntu 18 file browser.
      static fs::path pwd = fs::current_path();
      auto foundDir = pwd.end();

      for(auto dirPtr = pwd.begin(); dirPtr != pwd.end(); ++dirPtr)
      {
        ImGui::SameLine();
        if(ImGui::Button(dirPtr->filename().c_str())) foundDir = dirPtr;
        ImGui::SameLine();
        ImGui::Text("/");
      }

      //If the user selected a new directory, make it the working directory for this GUI.
      //The OS's pwd remains the same.
      if(foundDir != pwd.end())
      {
        fs::path newPwd;
        for(auto dir = pwd.begin(); dir != std::next(foundDir); ++dir) newPwd /= *dir;
        pwd = newPwd;
      }

      //Allow the user to enter a file name in the current directory.
      static std::string fileName;
      if(ImGui::InputText("Save As", &fileName, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        try
        {
          app.write(pwd / fileName);
        }
        catch(const YAML::Exception& e)
        {
          //TODO: Warn the user that serialization failed?
        }

        showSave = false;
      }

      //Show a directory tree rooted in pwd.
      //Only used for selecting a new pwd.
      std::string unusedFileName;
      ::directoryTree(pwd, pwd, unusedFileName);

      ImGui::End();
    }

    //TODO: Load a model by adding to the current CmdLine?  I'd have to be clever and avoid
    //      creating a new skybox.

    return false;
  }

  bool drawEngine(eng::WithRandomSeeds& engine)
  {
    static bool isOpen = false;
    const bool clicked = ImGui::MenuItem("engine");
    if(!isOpen) isOpen = clicked;

    bool changed = false;

    if(isOpen)
    {
      ImGui::Begin("engine", &isOpen);
      if(ImGui::InputInt("Bounces per Frame", &engine.nBounces(), ImGuiInputTextFlags_EnterReturnsTrue)) changed = true;
      if(ImGui::InputInt("Latency", &engine.latency(), ImGuiInputTextFlags_EnterReturnsTrue)) changed = true;
      if(ImGui::InputInt("Samples per Frame", &engine.nSamples(), ImGuiInputTextFlags_EnterReturnsTrue)) changed = true;
      ImGui::End();
    }

    return changed;
  }

  bool editBox(std::unique_ptr<CmdLine::selected>& selection)
  {
    //If I call editBox at all, that means that selection is not nullptr and so the GUI should open.
    bool isOpen = true, changed = false;

    //Draw a box editor window.
    ImGui::Begin("Box Editor", &isOpen);
    ImGui::InputText("name", &selection->name, ImGuiInputTextFlags_EnterReturnsTrue);
    if(ImGui::InputFloat3("center", selection->box.center.data.s, ImGuiInputTextFlags_EnterReturnsTrue)) changed = true;
    if(ImGui::InputFloat3("size", selection->box.width.data.s, ImGuiInputTextFlags_EnterReturnsTrue)) changed = true;

    //TODO: Material chooser and editor
    ImGui::End();

    if(!isOpen) selection.reset();

    return changed;
  }
}
