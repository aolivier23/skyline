//File: GUI.h
//Brief: Graphical interface functionality for builder and skyline.
//       Builds on top of Dear ImGui.  Kept in a separate file to
//       contain dependencies on Dear ImGui.  Call these functions
//       near the end of the game loop to use their components.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef APP_GUI_H
#define APP_GUI_H

class ImGuiIO;

namespace eng
{
  class WithCamera;
  class WithRandomSeeds;
}

namespace app
{
  class CmdLine;

  //Control the camera by emulating GLFW's callbacks with Dear ImGui.
  bool handleCamera(eng::WithCamera& view, const ImGuiIO& io);

  //Draw a menu allowing the user to select and configure a CameraModel.
  void drawCameras(CmdLine& app, eng::WithCamera& view);

  //Draw a window displaying application metrics like framerate and time to
  //complete the path tracing kernel(s).
  void drawMetrics(const ImGuiIO& io);

  //Show help information about controlling the camera and navigating the GUI.
  void drawHelp();

  //Show a menu for loading a new geometry or saving the application state to
  //a YAML file.  Returns true if a new file has been loaded into app.
  bool drawFile(app::CmdLine& app);

  //Show a window for controlling the skyline engine.  Exposes features like
  //the number of bounces per frame and number of samples per frame.
  void drawEngine(eng::WithRandomSeeds& engine);
}

#endif //APP_GUI_H
