cmake_minimum_required(VERSION 3.10.2)
project(skyline)

#Set up different build modes with custom compiler flags
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS ON )

set( GCC_Flags_For_CXX "-Wall -fPIC -pthread" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_Flags_For_CXX}" )
set( CMAKE_CXX_FLAGS_DEBUG "-ggdb" )

#Required packages
#link_directories( /usr/local/lib )
find_package( OpenGL REQUIRED )
include_directories( ${OPENGL_INCLUDE_DIR} )

find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIR} )

find_package( glfw3 3.2 REQUIRED )
include_directories( ${GLFW3_INCLUDE_DIR} )

#YAML parser library for configuration
find_package( yaml-cpp REQUIRED )

#Support for glad
add_library(glad SHARED glad/src/glad.c)
target_include_directories("glad" PRIVATE "glad/include")
target_link_libraries("glad" ${CMAKE_DL_LIBS})
install(TARGETS glad DESTINATION lib)
file(GLOB GLAD_HEADERS ${PROJECT_SOURCE_DIR}/glad/include/*/*.h)
install(FILES ${GLAD_HEADERS} DESTINATION include)

#All components in this project can refer to other components by
#relative path from the root source code directory.
include_directories( ${CMAKE_SOURCE_DIR} )

#Let source code know where this project is installed.  Needed for finding shader sources.  
add_definitions(-DINSTALL_DIR="${CMAKE_INSTALL_PREFIX}")

#Include build system for the rest of this project.  Add new top-level directories here.
add_subdirectory(gl)
add_subdirectory(algebra)
add_subdirectory(camera)
add_subdirectory(engine)
add_subdirectory(serial)
add_subdirectory(imgui)
add_subdirectory(app)
add_subdirectory(kernels)
add_subdirectory(examples)

#install setup script
configure_file(setup.sh.in setup.sh)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.sh DESTINATION bin)

#Make the results of this build into a distributable package.  Designed to be distributed as a .tar.gz
#Learned to do this from http://agateau.com/2009/cmake-and-make-dist/
set( CPACK_PACKAGE_VERSION_MAJOR "0" )
set( CPACK_PACKAGE_VERSION_MINOR "0" )
set( CPACK_PACKAGE_VERSION_PATCH "0" )
set( CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
set( CPACK_SOURCE_GENERATOR "TGZ" )
#set( CPACK_SOURCE_IGNORE_FILES 

include( CPack )
add_custom_target( dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source )
