//File: skyline
//Brief: Kernel to render Axis-Aligned Bounding Boxes (aabbs) by path tracing.  Handles
//       both building geometries and a sky dome with texture sampling.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832f
#endif

//Test a ray for intersecting the aabbs in the scene.  Returns texture coordinates by value
//normal by reference.
//Updates ray's position but not its direction.
float3 intersectScene(ray* thisRay, __global gridCell* cells, const grid gridSize, __global aabb* geometry, __global int* boxIndices,
                      __global material* materials, float3* normal, const float2 groundTexNorm, sphere sky, int2* whichGridCell)
{
  //Intersect the sky
  float closestDist = FLT_MAX;
  { //Parentheses to limit the scope of skyDist
    const float skyDist = sphere_intersect(sky, *thisRay);
    if(skyDist > 0) closestDist = skyDist;
  }
  float3 texCoords = sphere_tex_coords(sky, thisRay->position + thisRay->direction*closestDist);
  *normal = sphere_normal(sky, thisRay->position + thisRay->direction*closestDist);

  //Intersect the ground plane
  { //Parentheses to limit the scope of groundDist
    const float groundDist = groundPlane_intersect(*thisRay);
    if(groundDist > 0 && groundDist < closestDist)
    {
      closestDist = groundDist;
      texCoords = groundPlane_tex_coords(groundTexNorm, thisRay->position + thisRay->direction*closestDist);
      *normal = groundPlane_normal(thisRay->position + thisRay->direction*closestDist);
    }
  }

  //Intersect grid cells instead of buildings
  int index = whichGridCell->x + whichGridCell->y * gridSize.max.x;
  bool hitSomething = false; //TODO: Is there a way to structure this loop without another condition?
  //float nextCellDist = 0;

  while(!hitSomething && whichGridCell->x < gridSize.max.x && whichGridCell->y < gridSize.max.y
        && whichGridCell->x >= 0 && whichGridCell->y >= 0)
  {
    //TODO: Moving this to the top of the loop where I don't need a condition on hitSomething causes a crash
    //      a little sooner.
    //      This uses whichGridCell without checking that it's inside the grid though.  Throwing it away for
    //      that reason.
    //*whichGridCell = positionToCell(gridSize, thisRay->position + thisRay->direction*nextCellDist);
    //nextCellDist = distToNextCell(gridSize, *thisRay, *whichGridCell);

    //TODO: What happens if nextCellDist < 0?  Seems like that wouldn't kill the loop right away.
    //      If nextCellDist < 0 and I don't hit any boxes, I try to calculate the cell I'm in after traveling to
    //      the backwards boundary of this cell.  Then, I get distToNextCell() again.  The hope is that epsilon in
    //      distToNextCell will have bumped me out of the grid by the time positionToCell() happens, but
    //      I could get into a loop if that doesn't happen.  Can I freeze the application with no boxes?
    //
    //      Nope, I can't seem to get stuck with the loop over boxes cut out.  How about if I don't do anything which each box?
    //
    //      Nope, I can't get stuck just calculating whichBox.
    //
    //      I can't even get stuck unless I let hitSomething be set to true.  Of course, setting hitSomething to true
    //      lets rays bounce.  So, rays could be finding their way into bad situations that I'd never run into otherwise.
    //
    //      I can reliably reproduce a crash today.  It happens with both 1x1 and 3x3 grids.  I set the camera y position
    //      to 2.0 and then slowly scroll to the right from the starting position in hardEnough.yaml.  When I start to
    //      see a 3rd corner of the grid while keeping the camera roughly level, the kernel gets stuck.
    //
    //      Moving the camera around at ground level first gave a different result.  The kernel got stuck immediately
    //      when I set the camera position to 2 this time.  It didn't even render a frame of the camera at the new y
    //      position.
    //
    //      I can also get crashes without ever looking at that corner of the geometry.  Both scenes that have crashed
    //      involved the sky to the side of a building.  I'm pretty sure both scenes also had reflections that reached
    //      the bounce limit (4 is the default right now).
    //
    //      So, why would this depend on the camera state?  Maybe the camera's current cell, or just the ray's current
    //      cell, is getting into a bad state?  This feels like it could be a negative distance I'm not handling.
    //TODO: When a ray reaches the edge of the grid, nextCellDist will be < 0.  All geometry comparisons should fail.
    //      This could happen even if whichGridCell is inside the grid.  Then hitSomething will remain false, and
    //      whichGridCell will never get updated!  Infinite loop found!
    //
    //      Not quite.  hitSomething starts as false.  It has to get set to true for whichGridCell to not be updated
    //      which is an end condition for this loop.  When nextCellDist is < 0, I get the position of a ray that went
    //      backwards.  That doesn't seem great either.  I can see ways that it would be perpetually stuck in the same
    //      cell.
    //
    //      I'm not yet convinced that nextCellDist can actually be < 0.  I think a ray's position would have to be
    //      outside the grid to make that happen.  I'll keep thinking about this another day.
    //
    //      Crash happens even with number of bounces set to 2, but not with number of bounces set to 1.  Seems like
    //      the problem is a reflected ray near the y cell=0 (along z axis) edge of the grid at large grid x cell.
    //
    //      One crash may be triggering as a building on the y cell = 0 edge of the grid just comes into view.  It
    //      appears to be the farthest building from the origin in z.  Not quite, but the actual farthest building
    //      is not much farther and small enough that it isn't being reflected off of.

    //TODO: At least check distToNextCell() for correctness against https://www.scratchapixel.com/lessons/advanced-rendering/introduction-acceleration-structure/grid
    //      I think I should also replace it with that "calculate deltaT once" strategy to save some compute power.
    const float nextCellDist = distToNextCell(gridSize, *thisRay, *whichGridCell);
    //if(nextCellDist < 0) hitSomething = true; //TODO: This changes where the crash happens but doesn't fix it

    //Intersect boxes in this grid cell if any
    index = whichGridCell->x + whichGridCell->y * gridSize.max.x;
    for(size_t whichIndex = cells[index].begin; whichIndex < cells[index].end; ++whichIndex)
    {
      const int whichBox = boxIndices[whichIndex];
      const float dist = aabb_intersect(geometry + whichBox, *thisRay);
      if(dist > 0 && dist < closestDist && dist < nextCellDist)
      {
        closestDist = dist;
        *normal = aabb_normal_tex_coords(geometry[whichBox], thisRay->position + thisRay->direction*closestDist,
                                         materials[geometry[whichBox].material], &texCoords);
        hitSomething = true;  //TODO: As soon as I start setting hitSomething to true, I get stuck in what appears to be an infinite loop
      }
    }

    //Calculate the next grid cell to test
    //TODO: Seems like I'm getting stuck in a loop where whichGridCell never changes
    if(!hitSomething) *whichGridCell = positionToCell(gridSize, thisRay->position + thisRay->direction*nextCellDist);
    const int newIndex = whichGridCell->x + whichGridCell->y * gridSize.max.x; 
    if(newIndex == index) hitSomething = true; //TODO: This kludge seems to fix the infinite loop!  That means it is indeed
                                               //      a ray getting stuck in the same cell.  Why?
  }

  //Update thisRay's position to the position where it hit the volume it intersected.
  thisRay->position = thisRay->position + thisRay->direction*closestDist;

  *normal = (dot(*normal, thisRay->direction) < 0)?*normal:-*normal;

  //Make sure I don't intersect the same volume again.
  thisRay->position += *normal*0.0003f; //sqrt(FLT_EPSILON)

  return texCoords;
}

//Trace the path of a single ray through nBounces in a scene of boxes
void scatterAndShade(ray* thisRay, float3* lightColor, float3* maskColor, size_t* mySeed, const float3 normal,
                     const float3 texCoords, /*__global material* struckMaterial,*/ image2d_array_t textures,
                     sampler_t textureSampler, const float gamma)
{
  //Small angle approximation speeds up processing
  const float theta = random(mySeed), phi = 2.f*M_PI*random(mySeed);

  //I'm doing a cross product with the y axis unless this vector is along the y axis.  So, I know the result without
  //some multiplication by 0 steps that a cross product would imply.
  const float3 localXAxis = normalize((fabs(normal.x) > 0.1f)?(float3)(normal.z, 0.f, -normal.x)
                                                             :(float3)(0.f, -normal.z, normal.y));
  const float3 localYAxis = cross(normal, localXAxis);

  const float4 color = pow(read_imagef(textures, textureSampler, (float4){texCoords, 0.}), (float4){gamma, gamma, gamma, 1.f});

  //TODO: Fresnel equation: the fraction of light that is reflected or transmmitted depends on direction.
  //Do specular reflections color.w percent of the time and diffuse otherwise.
  //
  //I could try mathematical shenanigans here, but they have to be at least quadratic because I have
  //3 boundary conditions (0 at random = 0, 1.99 at random = 1, and 1 at random = w).  Getting the floating
  //point precision effects correct with that quadratic makes this problem far too challenging compared to
  //what I gain.  I can't even notice the performance difference yet.
  const bool isSpecular = (random(mySeed) < color.w);

  //I should have to normalize randomDir below, but I can show that it doesn't make any difference.  I'm adding 3
  //normal vectors and weighting them with weights that add in quadrature to 1.  The vectors I'm adding form a basis,
  //so their dot products are 0.  So, when I dot the result with itself, I just get the sum of the squares of the
  //weights which should come out to 1 if I'm actually using trig functions.  It seems to be close enough even with
  //the small angle approximation.
  const float3 randomDir = localXAxis*theta*native_cos(phi) + localYAxis*theta*native_sin(phi) + normal*sqrt(1.f-theta*theta),
               reflectDir = thisRay->direction - 2.f*dot(thisRay->direction, normal)*normal;
  thisRay->direction = (1 - isSpecular)*randomDir + isSpecular * reflectDir; //N.B.: isSpecular is always either 0 or 1, so these
                                                                             //      directions should never actually be mixed.

  //TODO: Emission from a texture for window lights will require a material here
  /**lightColor += *maskColor * struckMaterial->emission;*/
  //TODO: I'm seeing circles of different colors on the ground plane.  This bug is independent of:
  //      isSpecular
  //      texture access for the ground plane
  //      Using fabs(normal.x) for localXAxis
  *maskColor *= color.xyz * dot(thisRay->direction, normal);
}

//Sample the sky for a light color and calculate the light accumulated by a ray that hit it.
//The sky texture can partially occlude the sun according to its alpha channel to simulate the sun
//behind a cloud.
float3 sampleSky(const sphere sky, const sphere sun, const float3 sunEmission, const ray thisRay, const float3 texCoords, const float3 maskColor,
                 __read_only image2d_array_t textures, sampler_t textureSampler, const float gamma)
{
  float4 skyColor = read_imagef(textures, textureSampler, (float4){texCoords, 0.});
  if(sphere_intersect(sun, thisRay) > 0) skyColor.xyz = mix(skyColor.xyz, sunEmission, skyColor.w);
  return maskColor * pow(skyColor.xyz, (float3){gamma, gamma, gamma});
}

__kernel void pathTrace(__read_only image2d_t prev, sampler_t sampler, __write_only image2d_t pixels, __global aabb* geometry,
                        __global int* boxIndices, __global gridCell* gridCells, const grid gridSize, __global material* materials,
                        const sphere sky, const sphere sun, const float3 sunEmission, const float2 groundTexNorm, const camera cam,
                        const int nBounces, __global size_t* seeds, const int iterations, const int nSamplesPerFrame,
                        __read_only image2d_array_t textures, sampler_t textureSampler)
{
  //TODO: Copy geometry into __local memory

  //1 pixel per compute unit
  size_t seed = seeds[get_global_id(0) * get_global_size(1) + get_global_id(1)]; //Keep this in private memory as long as possible
  const int2 pixel = (int2)(get_global_id(0), get_global_id(1));
  
  const float gamma = 2.2; //TODO: Make this an engine setting

  float4 pixelColor = pow(read_imagef(prev, sampler, pixel), (float4){gamma, gamma, gamma, 1.f})*(1.f-1.f/(float)iterations); //undo gamma correction

  //Reuse first intersection before relfection for each sample of this pixel.
  float3 normal, lightColor = {0.f, 0.f, 0.f}, maskColor, texCoords;
  bool hitSky;
  int2 cameraCell = positionToCell(gridSize, cam.position), whichGridCell;

  //For each sample of this pixel
  //TODO: Using higher samplersPerFrame makes the scene darker
  //      after gamma correction and tonemapping updates.  Maybe tonemapping needs to be done each time lightColor
  //      is updated?
  for(size_t sample = 0; sample < nSamplesPerFrame; ++sample)
  {
    //Reset accumulated color
    maskColor = (float3){1.f, 1.f, 1.f};

    //Simulate a camera
    ray localRay = generateRay(cam, pixel, get_global_size(0), get_global_size(1), &seed);

    //Figure out where localRay enters the grid
    whichGridCell = cameraCell;
    if(cameraCell.x < 0 || cameraCell.y < 0 ||
       cameraCell.x >= gridSize.max.x || cameraCell.y >= gridSize.max.y)
    {
      const float distToGrid = grid_intersect(gridSize, localRay);
      if(distToGrid > 0) whichGridCell = positionToCell(gridSize, localRay.position + localRay.direction * (distToGrid + 0.001f));
      //else it doesn't matter what whichGridCell is anyway as long as it's outside the grid's limits
    }

    //Always intersect the scene at least once
    texCoords = intersectScene(&localRay, gridCells, gridSize, geometry, boxIndices, materials, &normal, groundTexNorm, sky, &whichGridCell);
    hitSky = (texCoords.z == SKY_TEXTURE);

    //For each bounce of this ray around the scene.  Stop when I hit the only light source, the sky,
    //and limit the maximum number of bounces.  Rays that bounce too many times without hitting the
    //the sky contribute no color.
    for(size_t bounce = 1; bounce < nBounces && !hitSky; ++bounce)
    {
      //TODO: Sort rays by struck material and process 1 material at a time?
      //      I might get a lot more milage out of sorting rays when I'm reading from
      //      textures.  My performance is already getting killed by the skybox texture.

      //Otherwise, scatter this ray off of whatever it hit and intersect the scene again
      scatterAndShade(&localRay, &lightColor, &maskColor, &seed, normal, texCoords, textures, textureSampler, gamma);
      texCoords = intersectScene(&localRay, gridCells, gridSize, geometry, boxIndices, materials, &normal, groundTexNorm, sky, &whichGridCell);
      hitSky = (texCoords.z == SKY_TEXTURE);
    }

    //Last shade for this sample
    if(hitSky)
    {
      lightColor += sampleSky(sky, sun, sunEmission, localRay, texCoords, maskColor, textures, textureSampler, gamma);
    }
    //TODO: scatterAndShade when there are light sources other than the sun
    //else scatterAndShade(&thisRay, &lightColor, &maskColor, &seed, normal, texCoords, textures, textureSampler);
  }

  //Reinhard tonemapping to convert HDR colors to LDR.  Divide by number of iterations after tonemapping for iterative
  //camera updates when looking at the same scene for a long time.
  const float4 ldrColor = (float4){lightColor, 1.f};
  pixelColor += ldrColor/(ldrColor + (float4){1.f, 1.f, 1.f, 0.f}) / (float)(iterations*nSamplesPerFrame);

  seeds[get_global_id(0) * get_global_size(1) + get_global_id(1)] = seed; //Update seed for next frame

  write_imagef(pixels, pixel, pow(pixelColor, (float4){1.f/gamma, 1.f/gamma, 1.f/gamma, 1.f})); //Gamma correction only
}
