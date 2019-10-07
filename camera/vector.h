//File: vector.h
//Brief: Defines c++ bindings to OpenCL vector types for
//       convenience of doing calculations in the host thread.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//OpenCL includes
#include <CL/cl_platform.h>

//c++ includes
#include <initializer_list>
#include <cmath> //std::sqrt
#include <ostream>
#include <vector>
#include <cassert>

namespace
{
  //Compile-time loop unrolling
  //ARRAY is any class that implements the array element access operator, []
  template <class ARRAY, int SIZE, int POS>
  struct For
  {
    static_assert(POS > 0); //There's a specialization for POS = 0 below to stop recursion
    static_assert(POS < SIZE);

    //Begin unrolled loop
    template <class FUNC>
    static ARRAY each(const ARRAY& lhs, const ARRAY& rhs, FUNC operation)
    {
      ARRAY result{0.};
      result.s[POS] = operation(lhs.s[POS], rhs.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(result, lhs, rhs, operation);
      return result;
    }

    //Continue unrolled loop
    template <class FUNC>
    static void each(ARRAY& result, const ARRAY& lhs, const ARRAY& rhs,
                     FUNC operation)
    {
      result.s[POS] = operation(lhs.s[POS], rhs.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(result, lhs, rhs, operation);
    }

    //Perform operation on lhs in place
    template <class FUNC>
    static void each(ARRAY& lhs, const ARRAY& rhs, FUNC operation)
    {
      operation(lhs.s[POS], rhs.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(lhs, rhs, operation);
    }

    //Scalar operation.  Capture other data in operation.
    //Begin unrolled loop
    template <class FUNC>
    static ARRAY each(const ARRAY& array, FUNC operation)
    {
      ARRAY result{0.};
      result.s[POS] = operation(array.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(array, result, operation);
      return result;
    }

    //End unrolled loop
    template <class FUNC>
    static void each(const ARRAY& array, ARRAY& result, FUNC operation)
    {
      result.s[POS] = operation(array.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(array, result, operation);
    }

    //Perform scalar operation in place
    template <class FUNC>
    static void each(ARRAY& array, FUNC operation)
    {
      operation(array.s[POS]);
      For<ARRAY, SIZE, POS-1>::each(array, operation);
    }
  };

  //End recursion on first position
  template <class ARRAY, int SIZE>
  struct For<ARRAY, SIZE, 0>
  {
    static_assert(SIZE > 0);

    //Begin unrolled loop
    template <class FUNC>
    static ARRAY each(const ARRAY& lhs, const ARRAY& rhs, FUNC operation)
    {
      ARRAY result{0.};
      result.s[0] = operation(lhs.s[0], rhs.s[0]);
      return result;
    }
                                                                                                                                   
    //Continue unrolled loop
    template <class FUNC>
    static void each(ARRAY& result, const ARRAY& lhs, const ARRAY& rhs,
                     FUNC operation)
    {
      result.s[0] = operation(lhs.s[0], rhs.s[0]);
    }
                                                                                                                                   
    //Perform operation on lhs in place
    template <class FUNC>
    static void each(ARRAY& lhs, const ARRAY& rhs, FUNC operation)
    {
      operation(lhs.s[0], rhs.s[0]);
    }

    //Scalar operation.  Capture other data in operation.
    //Begin unrolled loop
    template <class FUNC>
    static ARRAY each(const ARRAY& array, FUNC operation)
    {
      ARRAY result{0.};
      result.s[0] = operation(array.s[0]);
      return result;
    }

    //End unrolled loop
    template <class FUNC>
    static void each(const ARRAY& array, ARRAY& result, FUNC operation)
    {
      result.s[0] = operation(array.s[0]);
    }

    //Perform scalar operation in place
    template <class FUNC>
    static void each(ARRAY& array, FUNC operation)
    {
      operation(array.s[0]);
    }
  };
}

namespace cl
{
  //Core implementation of component-wise operations.  Defines vector
  //addition, subtraction, dot product, Hadderhad
  //product, and magnitude.  I will provide typedefs for specific
  //OpenCL types.  Implemented as a mixin so that BASE provides
  //the data.
  //
  //BASE must provide a data member that has an array member of TYPE with
  //SIZE entries called s.
  template <class TYPE, unsigned int SIZE, class BASE_TYPE>
  class vector
  {
    public:
      //Default constructor is the 0 vector
      vector()
      {
        //TODO: unroll at compile time?
        for(unsigned long int pos = 0ul; pos < SIZE; ++pos) data.s[pos] = 0;
      }

      //constructor from initializer list
      vector(const std::initializer_list<TYPE> values)
      {
        //TODO: unroll at compile time
        for(unsigned long int pos = 0ul; pos < SIZE; ++pos) data.s[pos] = *(values.begin() + pos);
      }

      //Constructor from BASE_TYPE
      vector(const BASE_TYPE otherBase)
      {
        data = otherBase;
      }

      //constructor from another vector to avoid infinite recursion on the next constructor
      vector(const vector<TYPE, SIZE, BASE_TYPE>& other): data(other.data)
      {
      }

      //constructor from components
      template <class ...ARGS>
      vector(ARGS... args): vector({args...})
      {
      }

      BASE_TYPE data; //OpenCL memory-aligned type with underlying data

      //Multiplication by a scalar
      vector<TYPE, SIZE, BASE_TYPE>operator *(const TYPE scalar) const
      {
        return ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, [scalar](const auto& value) { return value * scalar; });
      }

      vector<TYPE, SIZE, BASE_TYPE>operator /(const TYPE scalar) const
      {
        return ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, [scalar](const auto& value) { return value / scalar; });
      }

      //Vector addition
      vector<TYPE, SIZE, BASE_TYPE>operator +(const vector<TYPE, SIZE, BASE_TYPE>& other) const
      {
        return ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](const auto& lhs, const auto& rhs) { return lhs + rhs; });
      }

      vector<TYPE, SIZE, BASE_TYPE>operator -(const vector<TYPE, SIZE, BASE_TYPE>& other) const
      {
        return ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](const auto& lhs, const auto& rhs) { return lhs - rhs; });
      }

      //Hadderhad product: multiply each component of each vectortogether
      vector<TYPE, SIZE, BASE_TYPE>operator *(const vector<TYPE, SIZE, BASE_TYPE>& other) const
      {
        return ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](const auto& lhs, const auto& rhs) { return lhs * rhs; });
      }

      //Perform operations that modify this vector in place
      vector<TYPE, SIZE, BASE_TYPE>& operator *=(const TYPE scalar)
      {
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, [scalar](auto& value) { value *= scalar; });
        return *this;
      }

      vector<TYPE, SIZE, BASE_TYPE>& operator /=(const TYPE scalar)
      {
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, [scalar](auto& value) { value /= scalar; });
        return *this;
      }

      vector<TYPE, SIZE, BASE_TYPE>& operator +=(const vector<TYPE, SIZE, BASE_TYPE>& other)
      {
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](auto& lhs, const auto& rhs) { return lhs += rhs; });
        return *this;
      }

      vector<TYPE, SIZE, BASE_TYPE>& operator -=(const vector<TYPE, SIZE, BASE_TYPE>& other)
      {
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](auto& lhs, const auto& rhs) { return lhs -= rhs; });
        return *this;
      }

      //Hadderhad product.  See const version above for explanation.
      vector<TYPE, SIZE, BASE_TYPE>& operator *=(const vector<TYPE, SIZE, BASE_TYPE>& other)
      {
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [](const auto& lhs, const auto& rhs) { lhs *= rhs; });
        return *this;
      }

      //Dot product
      TYPE dot(const vector<TYPE, SIZE, BASE_TYPE>& other) const
      {
        TYPE result = 0;
        ::For<BASE_TYPE, SIZE, SIZE-1>::each(data, other.data, [&result](const auto& lhs, const auto& rhs) { return (result += lhs * rhs); });
        return result;
      }

      //Cross product
      vector<TYPE, SIZE, BASE_TYPE> cross(const vector<TYPE, SIZE, BASE_TYPE>& rhs) const
      {
        static_assert(SIZE == 3, "The cross product is only defined for 3-vectors.");
        return vector<TYPE, SIZE, BASE_TYPE>{data.y*rhs.data.z - data.z*rhs.data.y,
                                   data.z*rhs.data.x - data.x*rhs.data.z,
                                   data.x*rhs.data.y - data.y*rhs.data.x};
      }

      //Magnitude of a vector
      TYPE mag2() const
      {
        return dot(*this);
      }

      TYPE mag() const
      {
        return std::sqrt(mag2());
      }

      //Normalized version of a vector
      vector<TYPE, SIZE, BASE_TYPE> norm() const
      {
        assert(mag() != 0 && "Trying to normalize the 0 vector!");
        return *this * 1./mag();
      }
  };

  template <class TYPE, unsigned int SIZE, class BASE_TYPE>
  std::ostream& operator <<(std::ostream& os, const vector<TYPE, SIZE, BASE_TYPE>& vec)
  {
    os << "(";
    std::vector<TYPE> components;
    ::For<BASE_TYPE, SIZE, SIZE-1>::each(vec.data, [&components](const auto& component) 
                                                   {
                                                     components.push_back(component);
                                                     return component;
                                                   });
    for(auto pos = components.rbegin(); pos != components.rend()-1; ++pos) os << *pos << ", ";
    os << components.front();
    os << ")";
    return os;
  }

  #define CL_WRAPPER(TYPE, SIZE)\
    using TYPE##SIZE = vector<TYPE, SIZE, cl_##TYPE##SIZE>;

  //Define OpenCL vector wrappers
  CL_WRAPPER(float, 4)
  CL_WRAPPER(float, 3)
  CL_WRAPPER(float, 2)

  CL_WRAPPER(int, 4)
  CL_WRAPPER(int, 3)
  CL_WRAPPER(int, 2)

  CL_WRAPPER(double, 4)
  CL_WRAPPER(double, 3)
  CL_WRAPPER(double, 2)
}