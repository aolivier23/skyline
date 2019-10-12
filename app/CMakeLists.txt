add_library(CmdLine CmdLine.cpp)
target_link_libraries(CmdLine camera yaml-cpp)
install(TARGETS CmdLine DESTINATION lib)
install(FILES CmdLine.h DESTINATION include)

add_library(gui GUI.cpp)
target_link_libraries(gui camera imgui stdc++fs CmdLine)
install(TARGETS gui DESTINATION lib)
install(FILES GUI.h DESTINATION include)

add_executable(oneCell oneCell.cpp)
target_link_libraries(oneCell CmdLine OpenGL OpenCL glfw glad mygl camera engine)
install(TARGETS oneCell DESTINATION bin)

add_executable(builder builder.cpp)
target_link_libraries(builder CmdLine OpenGL OpenCL glfw glad mygl camera engine imgui gui)
install(TARGETS builder DESTINATION bin)
