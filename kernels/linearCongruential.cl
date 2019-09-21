//A simple uniform pseudo random number generator for OpenCL kernels.
//Returns numbers on [0, 1].

//Seed suggestion taken from https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
__constant unsigned long int a = 16807, c = 0, m = (1 << 31) + 1;
float random(unsigned long int* prev)
{
  *prev = (a*(*prev)); // + c); // % m; //m is set to the maximum value of a 32-bit integer, so
                                //taking mod(*prev, m) is the same as just letting *prev roll over.
                                //c is 0, so adding it does nothing.
  return (float)(*prev)/(float)m;
}
