//File: pathTrace.cl
//Brief: Kernel to render Axis-Aligned Bounding Boxes (aabbs) by path tracing.  Now, rays
//       are simulated reflecting off of diffuse surfaces.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832f
#endif

//Test a ray for intersecting the aabbs in the scene.  Returns texture coordinates by value
//normal by reference.
//Updates ray's position but not its direction.
float3 intersectScene(ray* thisRay, __global aabb* geometry, __global material* materials, const unsigned long int nBoxes, float3* normal,
                      const float2 groundTexNorm, sphere sky)
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

  //Intersect buildings
  for(size_t whichBox = 0; whichBox < nBoxes; ++whichBox)
  {
    const float dist = aabb_intersect(geometry + whichBox, *thisRay);
    if(dist > 0 && dist < closestDist)
    {
      closestDist = dist;
      *normal = aabb_normal_tex_coords(geometry[whichBox], thisRay->position + thisRay->direction*closestDist,
                                       materials[geometry[whichBox].material], &texCoords);
    }
  }

  //Update thisRay's position to the position where it hit the volume it intersected.
  thisRay->position = thisRay->position + thisRay->direction*closestDist;

  *normal = (dot(*normal, thisRay->direction) < 0)?*normal:-*normal;

  //Make sure I don't intersect the same volume again.
  thisRay->position += *normal*0.0003f; //sqrt(FLT_EPSILON)

  return texCoords;
}

//Trace the path of a single ray through nBounces in a scene of boxs
void scatterAndShade(ray* thisRay, float3* lightColor, float3* maskColor, size_t* mySeed, const float3 normal,
                     const float3 texCoords, /*__global material* struckMaterial,*/ image2d_array_t textures,
                     sampler_t textureSampler, const float gamma)
{
  //Small angle approximation speeds up processing from 47000 us to 43000 us
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
  const bool isSpecular = random(mySeed) < color.w;

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
                        const unsigned long int nBoxes, __global material* materials, const sphere sky, const sphere sun,
                        const float3 sunEmission, const float2 groundTexNorm, const camera cam,
                        const int nBounces, __global size_t* seeds, const int iterations,
                        const int nSamplesPerFrame, __read_only image2d_array_t textures, sampler_t textureSampler)
{
  //TODO: Copy geometry into __local memory

  //1 pixel per compute unit
  size_t seed = seeds[get_global_id(0) * get_global_size(1) + get_global_id(1)]; //Keep this in private memory as long as possible
  const int2 pixel = (int2)(get_global_id(0), get_global_id(1));
  
  const float gamma = 2.2; //TODO: Make this an engine setting

  float4 pixelColor = pow(read_imagef(prev, sampler, pixel), (float4){gamma, gamma, gamma, gamma})*(1.f-1.f/(float)iterations); //undo gamma correction
  //float4 pixelColor = read_imagef(prev, sampler, pixel)*(1.f-1.f/(float)iterations); //Without gamma correction

  //Simulate a camera
  ray thisRay = generateRay(cam, pixel, get_global_size(0), get_global_size(1));

  //Reuse first intersection before relfection for each sample of this pixel.
  float3 normal, lightColor = {0.f, 0.f, 0.f}, maskColor = {1.f, 1.f, 1.f};

  float3 texCoords = intersectScene(&thisRay, geometry, materials, nBoxes, &normal, groundTexNorm, sky);
  bool hitSky = (texCoords.z == SKY_TEXTURE);

  //For each sample of this pixel
  for(size_t sample = 0; sample < nSamplesPerFrame; ++sample)
  {
    //I don't need to do the first ray intersection again for each sample.
    //TODO: Redo first ray intersection for each sample anyway to get more
    //      natural camera jitter and a real lens.
    //ray localRay = generateRay(cam, pixel, get_global_size(0), get_global_size(1), &seed);
    ray localRay = thisRay;

    //For each bounce of this ray around the scene.  Stop when I hit the only light source, the sky,
    //and limit the maximum number of bounces.  Rays that bounce too many times without hitting the
    //the sky contribute no color.
    for(size_t bounce = 1; bounce < nBounces - 1 && !hitSky; ++bounce)
    {
      //TODO: Sort rays by struck material and process 1 material at a time?
      //      I might get a lot more milage out of sorting rays when I'm reading from
      //      textures.  My performance is already getting killed by the skybox texture.

      //Otherwise, scatter this ray off of whatever it hit and intersect the scene again
      scatterAndShade(&localRay, &lightColor, &maskColor, &seed, normal, texCoords, textures, textureSampler, gamma);
      texCoords = intersectScene(&localRay, geometry, materials, nBoxes, &normal, groundTexNorm, sky);
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
