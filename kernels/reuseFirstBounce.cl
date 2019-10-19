//File: pathTrace.cl
//Brief: Kernel to render Axis-Aligned Bounding Boxes (aabbs) by path tracing.  Now, rays
//       are simulated reflecting off of diffuse surfaces.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832f
#endif

//Test a ray for intersecting the aabbs in the scene.  Returns index of struck material or -1 on
//hitting the skybox by value and normal by reference.
//Updates ray's position but not its direction.
long int intersectScene(ray* thisRay, __global aabb* geometry, const unsigned long int nBoxes, float3* normal, __global aabb* skybox)
{
  //Find the closest aabb that this ray intersects
  float closestDist = aabb_intersect(skybox, *thisRay);
  __global const aabb* closestBox = skybox;

  for(size_t whichBox = 0; whichBox < nBoxes; ++whichBox)
  {
    const float dist = aabb_intersect(geometry + whichBox, *thisRay);
    if(dist > 0 && dist < closestDist)
    {
      closestDist = dist;
      closestBox = geometry + whichBox;
    }
  }

  //Update thisRay's position to the position where it hit the volume it intersected.
  thisRay->position = thisRay->position + thisRay->direction*closestDist;

  const float3 rawNormal = aabb_normal(*closestBox, thisRay->position);
  *normal = (dot(rawNormal, thisRay->direction) < 0)?rawNormal:-rawNormal;

  //Make sure I don't intersect the same volume again.
  thisRay->position += *normal*0.0003f; //sqrt(FLT_EPSILON)

  return closestBox->material;
}

//Trace the path of a single ray through nBounces in a scene of boxs
void scatterAndShade(ray* thisRay, float3* lightColor, float3* maskColor, size_t* mySeed, const float3 normal, __global material* struckMaterial)
{
  //Small angle approximation speeds up processing from 47000 us to 43000 us
  const float theta = random(mySeed), phi = 2.f*M_PI*random(mySeed);
  //I'm doing a cross product with the y axis unless this vector is along the y axis.  So, I know the result without
  //some multiplication by 0 steps that a cross product would imply.
  const float3 localXAxis = normalize((fabs(normal.x) > 0.1f)?(float3)(normal.z, 0.f, -normal.x)
                                                             :(float3)(0.f, -normal.z, normal.y));
  const float3 localYAxis = cross(normal, localXAxis);
 
  //I should have to normalize the calculation below, but I can show that it doesn't make any difference.  I'm adding 3
  //normal vectors and weighting them with weights that add in quadrature to 1.  The vectors I'm adding form a basis,
  //so their dot products are 0.  So, when I dot the result with itself, I just get the sum of the squares of the
  //weights which should come out to 1 if I'm actually using trig functions.  It seems to be close enough even with
  //the small angle approximation.
  thisRay->direction = localXAxis*theta*native_cos(phi) + localYAxis*theta*native_sin(phi) + normal*sqrt(1.f-theta*theta);

  //Update accumulated color of this ray
  *lightColor += *maskColor * struckMaterial->emission;
  *maskColor *= struckMaterial->color * dot(thisRay->direction, normal);
}

float3 hitSky(const ray thisRay, const float3 maskColor, __global material* skyboxMaterial)
{
  //TODO: Look up struck emission color from textures.  I'll need one for each
  //      of the ceiling and the 4 walls.
  return maskColor*(float3){0.529, 0.808, 0.922}; //Sky blue from https://www.colorhexa.com/87ceeb
}

__kernel void pathTrace(__read_only image2d_t prev, sampler_t sampler, __write_only image2d_t pixels, __global aabb* geometry,
                        const unsigned long int nBoxes, __global material* materials, __global aabb* skybox,
                        const float3 cameraPos, const float3 focalPos, const float3 up, const float3 right,
                        const int nBounces, __global size_t* seeds, const int iterations, const int nSamplesPerFrame)
{
  //TODO: Copy geometry into __local memory

  size_t seed = seeds[get_global_id(0) * get_global_size(1) + get_global_id(1)]; //Keep this in private memory as long as possible
  const int2 pixel = (int2)(get_global_id(0), get_global_id(1));
  
  float4 pixelColor = read_imagef(prev, sampler, pixel)*(1.f-1.f/(float)iterations);

  ray thisRay = generateRay(pixel, cameraPos, focalPos, up, right, get_global_size(0), get_global_size(1));

  //Reuse first intersection before relfection for each sample of this pixel.
  float3 normal, lightColor = {0.f, 0.f, 0.f}, maskColor = {1.f, 1.f, 1.f};
  long int material = intersectScene(&thisRay, geometry, nBoxes, &normal, skybox);
  bool hitSkybox = (material == skybox->material) && (thisRay.position.y > skybox->center.y - 0.49*skybox->width.y);

  //For each sample of this pixel
  for(size_t sample = 0; sample < nSamplesPerFrame; ++sample)
  {
    //For each bounce of this ray around the scene
    for(size_t bounce = 1; bounce < nBounces - 1 && !hitSkybox; ++bounce)
    {
      //TODO: Sort rays by struck material and process 1 material at a time?
      //      I might get a lot more milage out of sorting rays when I'm reading from
      //      textures.

      //Otherwise, scatter this ray off of whatever it hit and intersect the scene again
      scatterAndShade(&thisRay, &lightColor, &maskColor, &seed, normal, materials + material);
      material = intersectScene(&thisRay, geometry, nBoxes, &normal, skybox);
      hitSkybox = (material == skybox->material) && (thisRay.position.y > skybox->center.y - 0.49*skybox->width.y);
    }

    //Final shade for this sample
    if(hitSkybox)
    {
      //TODO: Emission is hard-coded for now so that skybox material is the floor material.
      //      Next, I need to look up sky color from hitSky.
      lightColor += hitSky(thisRay, maskColor, materials + skybox->material);
    }
    else scatterAndShade(&thisRay, &lightColor, &maskColor, &seed, normal, materials + material);

    //pixelColor += (float4)(lightColor, 1.f) / (float)(iterations*nSamplesPerFrame);
  }
  pixelColor += (float4)(lightColor, 1.f) / (float)(iterations*nSamplesPerFrame);
  seeds[get_global_id(0) * get_global_size(1) + get_global_id(1)] = seed; //Update seed for next frame

  write_imagef(pixels, pixel, pixelColor);
}
