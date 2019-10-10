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
