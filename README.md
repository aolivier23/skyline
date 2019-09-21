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
- OpenCL 1.2 with `cl_gl_interop` extension
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
