//File: DebuggingMacros.h
//Brief: Macros that redirect an OpenGL function call through an assertion that it
//       doesn't cause an error and print its name if it does.  Compiles to a single
//       macro that just calls a function unless this is a debug build to minimize code
//       bloat in releases.
//Example: CHECK_GL_ERROR(glUseProgram(programID));
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef DEBUGGINGMACROS_H
#define DEBUGGINGMACROS_H

#include "Exception.h" //OpenGL exception class

#ifndef NDEBUG
  //Macro to print name of a function with the values of its arguments
  //if there's an OpenGL error.
  #define CHECK_FOR_FUNCTION_ERROR(NAME, ...) \
    const auto err = glGetError(); \
    if(err != GL_NO_ERROR) \
    { \
      gl::Exception e("OpenGL Error"); \
      ::printArgs(e << "Got OpenGL error code " << err << " calling function " \
                    << NAME << "(", __VA_ARGS__) << ")\n"; \
      throw e; \
    } \
  
  namespace
  {
    //End compile-time recursion by printing a single argument
    template <class STREAM, class ARG>
    auto printArgs(STREAM& stream, ARG arg) -> decltype(stream << arg)
    {
      return stream << arg;
    }
  
    //Print all of the arguments given at compile time
    template <class STREAM, class ARG, class ...ARGS>
    auto printArgs(STREAM& stream, ARG arg, ARGS... args) -> decltype(stream << arg)
    {
      return ::printArgs(stream << arg << ", ", args...);
    }
  
    //Evaluate a function and print it's name and arguments if there's an OpenGL error
    template <class RETURN>
    struct ev
    {
      template <class FUNC, class ...ARGS>
      ev(FUNC function, const char* name, ARGS... args): value(function(args...))
      {
        CHECK_FOR_FUNCTION_ERROR(name, args...)
      }
  
      operator RETURN() { return value; }
  
      RETURN value;
    };
  
    //Specialization for debugging void-returning functions
    template <>
    struct ev<void>
    {
      template <class FUNC, class ...ARGS>
      ev(FUNC function, const char* name, ARGS... args)
      {
        function(args...);
        CHECK_FOR_FUNCTION_ERROR(name, args...)
      }
    };
  }

  //Evaluate an OpenGL function.  If and only if this program
  //was built in debug mode, print out an error and throw an 
  //exception if there is an OpenGL error.  Calling glGetError() here
  //resets the global error state to GL_NO_ERROR.  
  #define CHECK_GL_ERROR(FUNCTION, ...) \
  ::ev<decltype(FUNCTION(__VA_ARGS__))>(FUNCTION, #FUNCTION, __VA_ARGS__)
#else
  #define CHECK_GL_ERROR(FUNCTION, ...) \
    FUNCTION(__VA_ARGS__)
#endif //NDEBUG

#endif //DEBUGGINGMACROS_H
