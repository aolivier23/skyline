#skyline: Render Cities with Raytracing powered by OpenCL

##Brief:

Skyline is a real-time raytracing renderer for cities.  It will
be suitable for rendering both from ground level and from roof-top
level.

##Planned Features:

1. Define buildings as collections of [axis-aligned boxes](#AABB-algorithm).
2. Building geometries and [bounding volume hierarchies](#BVH-algorithm) (BVHs)
   serializable in YAML.
3. Buildings organized into a [2D (sparse?) grid](#sparse-BVH-algorithm) to reduce BVH
   traversal to a 2D problem.
4. Texture mapping for both [color and reflectivity](#color-and-reflectivity-algorithm).
5. [Dynamic skybox](#skybox-algorithm) for multiple light directions and cloud
   conditions.
6. "[Conformal mapping](#conformal-mapping-algorithm)" of building surfaces to procedurally
   generate architectural features like window sills.
7. Dynamic [BVH optimization](#BVH-optimization-algorithm)?  If cast as assigning to grid cells
   and can run BVH async on a second GPU, sounds a little like
   MINERvA ML vertexing inferences?

##Applications:

1. [oneCell](#oneCell-app): Proof of concept that renders a few axis-aligned box-like
            buildings with no BVH.  Only expected to support a few 10s
            of buildings on NVIDIA Quadro P3200.

2. [skyline](#sklyine-app): User navigates a city geometry from a given start point with
            arrow keys and the mouse.  Consumes YAML geometry files.

3. [builder](#builder-app): User constructs a building geometry from a grid in a WYSIWYG
            editor powered by the renderer from skyline.  Can load existing
            geometries as a starting point.

##Requirements:
- GLFW3
- OpenCL 1.2 with `cl_krh_gl_sharing` extension
- OpenGL 4.0
- c++17 compiler
- CMake 3.1 for build

##Contains:
- GLAD
- Dear IMGUI

#Working Notes:

##oneCell app

- Proof of concept
- One kernel per frame like pathTraceSpheres
- AABB
- Probably a room with a light.  Does my AABB algorithm work from inside a box?

##skyline app

- Special case of [builder app](#builder-app)?  Maybe disable some GUI features with a
  compile-time flag?  Otherwise, renderer becomes an object.
- TODO: YAML prototype.  Probably makes sense for [oneCell](#oneCell-app) too.

##builder app

- Only have to drag buildings along ground in 2D.
- Separate view for constructing a composite building.
- Material selection
- Support for mipmapping-like manipulations for textures?  Think of windows.

###GUI Notes

- Main GUI components: Camera, object editor/selection, Save As, and renderer control.
- Everything except Camera GUI panel needs to be contained in builder?  Maybe object editor is only builder-specific GUI element?
- Camera GUI shared with skyline
- Multiple cameras -> select one in list tree (constant depth of 2)
- Click on a camera to choose
- Expand a camera to see its parameters
- Camera parameters include position and focal position
- Also expose details of CameraController?  Seems like virtual functions enter here.  It doesn't need to be a plugin, so handle specific cases.
- Concept of object selection: Run ray tracing stage on host.  Can probably reuse aabb\_intersect.  Associate name/metadata with object in container.
  No BVH needed because shouldn't be performance bottleneck.  CTRL + click for multi-selection.
- TODO: How to highlight?  Specific color for highlighted objects?
- Can drag object along ground -> alternative mouse mode to Camera -> mouse mode selector?  Maybe ALT + click instead?  Or camera could follow mouse
  directly?  Sounds like integration with different CameraControllers.  I can always match CameraController to mouse mode in builder itself.
- Material and object editor.  For now, just need to adjust width (vector).  Eventually want file chooser for textures.  Just color and emission for now.
- Eventually stack objects into a "local model"?  Only in object editor.  How would hierarchies interact with BVH?  With YAML?
- Serialization logic for Save As specific to builder.  Save As button pops up a dialog window.
- Renderer control: "scene persistence", samples per frame, and framerate
- TODO: Update whole scene at once every time something changes?  How to handle transition to editing a single object?
- TODO: Draw grid?  Sounds like rasterization on top of raytracing.  Use/generate a grid texture instead?  Grid control seems very messy.
- Stretch: Some plots like framerate could be VERY powerful for BVH optimization.  Could even load new kernels dynamically!
- Separate geometry format from metadata with mapping from aabb pointers to metadata.
- TODO: Make sure raytracing engine is modular.  It should be easy to transplant into both sklyine and external applications as a library that
        knows nothing about Dear IMGUI/yaml-cpp.
- TODO: Special handling for the skybox?  I will eventually want a different texture on the ground.

##AABB algorithm

- Many city blocks are axis-aligned
- Better performance: fewer dot products
- TODO: Were normals on corners failing in gdmlRaytrace?

##BVH algorithm

- Assume blocks axis-aligned -> use a grid
- First level is a 2D grid.  Might want 3D for some buildings -> NOT a general BVH
- Use modulus instead of box intersections
- 2D intersections hopefully cheaper
- Traverse neighbors in correct direction

##sparse BVH algorithm

- Skip over streets and parks
- "Teapot in a stadium" from ScratchAPixel shouldn't be a problem for this problem
  domain.

##color and reflectivity algorithm

- Texture mapping -> RGBA?
- Seems like mipmapping useful here
- Objects inside buildings?

##skybox-algorithm

- Initially just a blue sky
- Later want directional light source
- Mountains in background?
- Clouds -> texture?  Moving clouds.

##conformal-mapping-algorithm

- Procedurally generate window sills
- Think of pumpkin example
- Seems like texture mapped to surface that adjusts intersection normal and position 

##BVH-optimization-algorithm

- Take advantage of second GPU and/or CPUs
- Reassign or merge grid cells -> output format like MINERvA ML vertexing?
- Run BVH async on second device or collect information from rendering?
- Doesn't need to update every frame.  Buffer if necessary.
- Data per cell: number entering rays, number intersections, camera position, ray start
  positions?
- Optimize per frame or per bounce?  Seems like former will be a handul, but automating
  latter could be really cool.

##Selection Algorithm

- Some key press or GUI element triggers "selection mode".  I'm thinking about CTRL + click right now.
  I eventually want the concept of selecting multiple boxes to move at once.
- Path trace all boxes except skybox on the CPU.  If limits performance one day, I could
  try passing back an index from a secondary GPU.
- Compare distance to closest box to distance to skybox.
- If skybox is closer, create a new box with some default parameters and select it.
- If an existing box is closer, it's selected.
- Focus on selected box.  There is a GUI window with box and material properties for the selected box.
  Literally focus the camera on center of the selection (new CameraModel functionality).  One day, it
  might be nice to have a totally different CameraController for a "texture editing mode" to work on
  a box or a model itself rather than its global position.
- Dragging the focused box moves it in the plane of the ground.  Seems like fPosition and fUp, fRight,
  and fFocalPlane should be enough to translate mouse pixel to global position.  One day, it might be
  interesting if the camera follows the selected box.
- One day, it might be interesting to have a different mouse mode when working on a selected box.
  Dragging the mouse over that box would change its dimension in the direction in which the mouse
  is dragged.  I would figure out which dimension to change by getting the normal at the clicked point
  (part of the ray tracing kernel).  I have to be able to see the side of the box I want to expand,
  so it would be important to be able to switch quickly from dragging mode to controlling the camera.
  Instead, maybe I could just provide a camera on each side of the box when it's selected.
- I've described 3 mouse operation modes: "normal" camera mode, "global" edit mode, and "local" edit mode.
  Moving from one mode to another at a minimum means redirecting mouse input.  I probably also want to
  change out the CameraController in at least "local" edit mode.  Each mode could have a virtual function
  that handles camera movement.  Sounds a little like the "state machine" design I ended up with when
  parallelizing edepViewer.  I guess there's also per-state data: selected object in each edit mode.  Each
  mouse mode might also want to do something before rendering.  The edit modes in particular want to uploadToGPU()
  any time something has changed.
- Any time there is a click when CTRL is down and IMGUI does not want the mouse, I am going to enter selection
  mode.  I guess the raytrace is actually part of the state transition to global edit mode.  If the selection ray
  trace starts taking longer than a frame, I guess I could have a "waiting" mode like in edepViewer.  How do I get to
  local edit mode?  I'm thinking about a button for local edit mode in a GUI that global edit mode draws.  Closing
  the edit mode window goes back to normal camera mode.
- I'd like to be able to highlight the selected object, but I have no idea how to do that yet.  Maybe I could apply
  some (not OpenCL) kernel to the framebuffer for edge detection?  I guess I could change the selected object's
  emission spectrum.
- I always want to draw the main menu bar.
- I should clean up any special edit mode cameras when I go back to normal camera mode.
- Each of these three modes is a MouseController.  A MouseController has a handleInput() function and a drawGUI()
  function.  In edepViewer, I was marginally successful returning a `std::unique_ptr<State>` from every function
  that could result in a state change.
