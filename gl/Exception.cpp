//File: Exception.cpp
//Brief: Implements a general-purpose exception class.  Somewhat inspired by cet::exception.
//Author: Andrew Olivier aolivier@ur.rochester.edu
//Date: 4/14/2017

//Local includes
#include "Exception.h" //The base class

namespace gl
{
  Exception::Exception(const std::string& cat) noexcept
  {
    fWhat = cat+":\n";
  }
  
  Exception::Exception(const Exception& e) noexcept
  {
    fWhat = e.fWhat;
  }
  
  Exception& Exception::operator=(const Exception& e) noexcept
  {
    fWhat = e.fWhat;
    return *this;
  }
  
  const char* Exception::what() const noexcept
  {
    return fWhat.c_str();
  }
}
