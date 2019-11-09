add_library(Geometry Geometry.cpp)
target_link_libraries(Geometry camera yaml-cpp)
install(TARGETS Geometry DESTINATION lib)
install(FILES Geometry.h DESTINATION include)

add_library(gui GUI.cpp)
target_link_libraries(gui engine camera imgui stdc++fs Geometry)
install(TARGETS gui DESTINATION lib)
install(FILES GUI.h DESTINATION include)

add_library(mycl LoadIntoCL.cpp)
target_link_libraries(mycl OpenCL)
install(TARGETS mycl DESTINATION lib)
install(FILES LoadIntoCL.h DESTINATION include)

#add_executable(oneCell oneCell.cpp)
#target_link_libraries(oneCell Geometry OpenGL OpenCL glfw glad mygl camera engine mycl)
#install(TARGETS oneCell DESTINATION bin)

add_executable(builder builder.cpp)
target_link_libraries(builder Geometry OpenGL OpenCL glfw glad mygl camera engine imgui gui mycl)
install(TARGETS builder DESTINATION bin)
