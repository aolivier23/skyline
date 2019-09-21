//File: pathTrace.cl
//Brief: Kernel to render Axis-Aligned Bounding Boxes (aabbs) by path tracing.  Now, rays
//       are simulated reflecting off of diffuse surfaces.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832f
#endif

//Trace the path of a single ray through nBounces in a scene of boxs
float3 trace(ray thisRay, const unsigned long int nBounces, __global aabb* geometry,
             const unsigned long int nBoxes, __global material* materials, __global aabb* skybox,
             __global size_t* seeds)
{
  //Keep track of 2 colors as thisRay bounces: maskColor and lightColor.  maskColor accumulates the
  //colors of objects thisRay intersects.  lightColor accumulates maskColor * (emitted color of each object).
  float3 maskColor = (float3)(1.f, 1.f, 1.f), lightColor = (float3)(0.f, 0.f, 0.f);

  const size_t index = get_global_id(0) * get_global_size(1) + get_global_id(1);
  size_t mySeed = seeds[index];
  for(size_t bounce = 0; bounce < nBounces; ++bounce)
  {
    //Find the closest aabb that this ray intersects
    float closestDist = aabb_intersect(skybox, thisRay);
    __global const aabb* closestBox = skybox;

    for(size_t whichBox = 0; whichBox < nBoxes; ++whichBox)
    {
      const float dist = aabb_intersect(geometry + whichBox, thisRay);
      if(dist > 0 && dist < closestDist)
      {
        closestDist = dist;
        closestBox = geometry + whichBox;
      }
    }

    //N.B.: Instead of hitting the sky, rays hit the skybox now.
    //If this ray didn't intersect anything, it goes to the background and doesn't come back.  This should only happen for debugging because the ray would be outside the skybox!
    if(closestDist < 0)
    {
      return lightColor + maskColor*(float3)(0., 0., 1.);
    }

    //Reflect thisRay for next simulation step
    const float3 rawNormal = aabb_normal(*closestBox, thisRay.position + thisRay.direction*closestDist);
    const float3 normAtIntersection = (dot(rawNormal, thisRay.direction) < 0)?rawNormal:-rawNormal;

    //Small angle approximation speeds up processing from 47000 us to 43000 us
    const float theta = random(&mySeed), phi = 2.f*M_PI*random(&mySeed);
    //I'm doing a cross product with the y axis unless this vector is along the y axis.  So, I know the result without
    //some multiplication by 0 steps that a cross product would imply.
    const float3 localXAxis = normalize((normAtIntersection.x > 0.1f)?(float3)(normAtIntersection.z, 0.f, -normAtIntersection.x)
                                                                     :(float3)(0.f, -normAtIntersection.z, normAtIntersection.y));
    const float3 localYAxis = cross(normAtIntersection, localXAxis);
   
    thisRay.position = thisRay.position + thisRay.direction*closestDist + normAtIntersection*0.0003f; //sqrt(FLT_EPSILON)
    //I should have to normalize the calculation below, but I can show that it doesn't make any difference.  I'm adding 3
    //normal vectors and weighting them with weights that add in quadrature to 1.  The vectors I'm adding form a basis,
    //so their dot products are 0.  So, when I dot the result with itself, I just get the sum of the squares of the
    //weights which should come out to 1 if I'm actually using trig functions.  It seems to be close enough even with
    //the small angle approximation.
    thisRay.direction = localXAxis*theta*native_cos(phi) + localYAxis*theta*native_sin(phi) + normAtIntersection*sqrt(1.f-theta*theta);

    //Update accumulated color of this ray
    lightColor += maskColor * materials[closestBox->material].emission;
    maskColor *= materials[closestBox->material].color * dot(thisRay.direction, normAtIntersection);
  }
  seeds[index] = mySeed; //Save changes to seed so that I get a different number to start next frame

  return lightColor;
}

//TODO: Camera in gdmlRaytrace used to stretch out scene when turning 90
//      degrees from starting direction.  That could be caused by a bug
//      here.
ray generateRay(const int2 pixel, const float3 cameraPos, const float3 focalPos,
                       unsigned long int width, unsigned long int height)
{
  ray thisRay;
  thisRay.position = cameraPos;

  const float aspectRatio = (float)width / (float)height;
  const float2 ndc = (float2)((float)pixel.x/(float)width, (float)pixel.y/(float)height);
  const float3 pixelPos = (float3)((ndc.x - 0.5f)*aspectRatio, -ndc.y + 0.5f, 0.f) + focalPos;
  thisRay.direction = normalize(pixelPos - thisRay.position);

  return thisRay;
}

__kernel void pathTrace(__read_only image2d_t prev, sampler_t sampler, __write_only image2d_t pixels, __global aabb* geometry,
                        const unsigned long int nBoxes, __global material* materials, __global aabb* skybox,
                        const float3 cameraPos, const float3 focalPos, const long unsigned int nBounces, __global size_t* seeds,
                        long unsigned int iterations)
{
  const int2 pixel = (int2)(get_global_id(0), get_global_id(1));
  
  float4 pixelColor = read_imagef(prev, sampler, pixel)*(1.f-1.f/(float)iterations);

  const long unsigned int nSamplesPerFrame = 1; //TODO: Make this a kernel argument
  
  for(size_t sample = 0; sample < nSamplesPerFrame; ++sample)
  {
    ray thisRay = generateRay(pixel, cameraPos, focalPos, get_global_size(0), get_global_size(1));
    //When I comment out the line just below this one, this kernel runs in about 600us.
    pixelColor += (float4)(trace(thisRay, nBounces, geometry, nBoxes, materials, skybox,
                                 seeds), 1.f) / (float)(iterations*nSamplesPerFrame);
  }

  write_imagef(pixels, pixel, pixelColor);
}
