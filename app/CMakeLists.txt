add_executable(oneCell oneCell.cpp)
target_link_libraries(oneCell OpenGL OpenCL glfw glad mygl camera engine)
install(TARGETS oneCell DESTINATION bin)
