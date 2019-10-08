add_library(CmdLine CmdLine.cpp)
target_link_libraries(CmdLine camera yaml-cpp)
install(TARGETS CmdLine DESTINATION lib)
install(FILES CmdLine.h DESTINATION include)

add_executable(oneCell oneCell.cpp)
target_link_libraries(oneCell CmdLine OpenGL OpenCL glfw glad mygl camera engine)
install(TARGETS oneCell DESTINATION bin)
