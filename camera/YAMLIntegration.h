//File: YAMLIntegration.h
//Brief: Allow reading OpenCL vector types from vector.h directly via yaml-cpp.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//camera includes
#include "vector.h"

//yaml-cpp includes
#include "yaml-cpp/yaml.h"

//Based on https://github.com/jbeder/yaml-cpp/wiki/Tutorial
namespace YAML
{
  template <class TYPE, unsigned int SIZE, class BASE_TYPE>
  struct convert<cl::vector<TYPE, SIZE, BASE_TYPE>>
  {
    static Node encode(const cl::vector<TYPE, SIZE, BASE_TYPE>& rhs)
    {
      Node result;

      //I could unroll this for loop, but something is very
      //wrong if serialization is dominating the performance
      //of skyline.
      for(size_t which = 0; which < SIZE; ++which)
      {
        result.push_back(rhs.data[which]);
      }

      return result;
    }

    static bool decode(const Node& node, cl::vector<TYPE, SIZE, BASE_TYPE>& rhs)
    {
      if(!node.IsSequence() || node.size() != SIZE) return false;

      //I could unroll this for loop, but something is very
      //wrong if serialization is dominating the performance
      //of skyline.
      for(size_t which = 0; which < SIZE; ++which)
      {
        rhs.data.s[which] = node[which].as<TYPE>();
      }

      return true;
    }
  };
}
