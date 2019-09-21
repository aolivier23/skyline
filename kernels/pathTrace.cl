//File: pathTrace.cl
//Brief: Kernel to render simple_spheres by path tracing.  Now, rays
//       are simulated reflecting off of diffuse surfaces.
//Author: Andrew Olivier aolivier@ur.rochester.edu

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable
#pragma OPENCL EXTENSION cl_khr_fp_64: disable

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832f
#endif

struct ray
{
  float3 position;
  float3 direction;
};

//Sphere intersection
float intersect(const struct simple_sphere sphere, const struct ray thisRay)
{
  //TODO: I can run in 11ms instead of 23ms when I return 0.2 from this function immediately.
  //      I can run in 14ms instead of 23ms when I don't calculate the discriminant.
  const float3 diff = thisRay.position - sphere.center;
  const float diffDotDir = dot(diff, thisRay.direction);
  const float disc = diffDotDir*diffDotDir - dot(diff, diff) + sphere.radius*sphere.radius;
  if(disc < 0) return -1;

  const float sqrtDisc = sqrt(disc);
  float dist = -diffDotDir - sqrtDisc;
  if(dist < 0) dist = -diffDotDir + sqrtDisc;
  return dist;
}

//Sphere normal vector
float3 normal(const struct simple_sphere sphere, const float3 pos)
{
  //pos must be on the surface of sphere
  return (pos - sphere.center) / sphere.radius;
}

//Trace the path of a single ray through nBounces in a scene of spheres
float3 trace(struct ray thisRay, const unsigned long int nBounces, __global struct simple_sphere* geometry,
             const unsigned long int nSpheres, const float3 background, __global size_t* seeds)
{
  //Keep track of 2 colors as thisRay bounces: maskColor and lightColor.  maskColor accumulates the
  //colors of objects thisRay intersects.  lightColor accumulates maskColor * (emitted color of each object).
  float3 maskColor = (float3)(1.f, 1.f, 1.f), lightColor = (float3)(0.f, 0.f, 0.f);

  const size_t index = get_global_id(0) * get_global_size(1) + get_global_id(1);
  size_t mySeed = seeds[index];
  for(size_t bounce = 0; bounce < nBounces; ++bounce)
  {
    //Find the closest Sphere that this ray intersects
    //TODO: I can run in about 8ms instead of 23ms when I comment out this loop and set closestDist to 10 and closestSphere to 0.
    float closestDist = FLT_MAX;
    size_t closestSphere = nSpheres;
    for(size_t whichSphere = 0; whichSphere < nSpheres; ++whichSphere)
    {
      const float dist = intersect(geometry[whichSphere], thisRay);
      //TODO: I can run in 14ms when I comment out this if statement and set closestDist and closestSphere unconditionally!
      if(dist > 0 && dist < closestDist)
      {
        closestDist = dist;
        closestSphere = whichSphere;
      }
    }

    //If this ray didn't intersect anything, it goes to the background and doesn't come back
    if(closestSphere == nSpheres) return lightColor + maskColor*background;

    //Reflect thisRay for next simulation step
    const float3 rawNormal = normal(geometry[closestSphere], thisRay.position + thisRay.direction*closestDist);
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
    lightColor += maskColor * geometry[closestSphere].emission;
    maskColor *= geometry[closestSphere].color * dot(thisRay.direction, normAtIntersection);
  }
  seeds[index] = mySeed; //Save changes to seed so that I get a different number to start next frame

  return lightColor;
}

struct ray generateRay(const int2 pixel, const float3 cameraPos, const float3 focalPos,
                       unsigned long int width, unsigned long int height)
{
  struct ray thisRay;
  thisRay.position = cameraPos;

  const float aspectRatio = (float)width / (float)height;
  const float2 ndc = (float2)((float)pixel.x/(float)width, (float)pixel.y/(float)height);
  const float3 pixelPos = (float3)((ndc.x - 0.5f)*aspectRatio, -ndc.y + 0.5f, 0.f) + focalPos;
  thisRay.direction = normalize(pixelPos - thisRay.position);

  return thisRay;
}

__kernel void pathTrace(__read_only image2d_t prev, sampler_t sampler, __write_only image2d_t pixels, __global struct simple_sphere* geometry,
                        const unsigned long int nSpheres, const float3 background, const float3 cameraPos, const float3 focalPos,
                        const long unsigned int nBounces, __global size_t* seeds, long unsigned int iterations)
{
  const int2 pixel = (int2)(get_global_id(0), get_global_id(1));
  
  float4 pixelColor = read_imagef(prev, sampler, pixel)*(1.f-1.f/(float)iterations);

  const long unsigned int nSamplesPerFrame = 1; //TODO: Make this a kernel argument
  
  for(size_t sample = 0; sample < nSamplesPerFrame; ++sample)
  {
    struct ray thisRay = generateRay(pixel, cameraPos, focalPos, get_global_size(0), get_global_size(1));
    //When I comment out the line just below this one, this kernel runs in about 600us.
    pixelColor += (float4)(trace(thisRay, nBounces, geometry, nSpheres, background,
                                 seeds), 1.f) / (float)(iterations*nSamplesPerFrame);
  }

  write_imagef(pixels, pixel, pixelColor);
}
