add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

add_library(imgui imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_widgets.cpp examples/imgui_impl_glfw.cpp examples/imgui_impl_opengl3.cpp)

target_include_directories("imgui" PRIVATE ${CMAKE_SOURCE_DIR}/imgui)
target_include_directories("imgui" PRIVATE ${CMAKE_SOURCE_DIR}/glad/include)

target_link_libraries(imgui glad glfw)
install(TARGETS imgui DESTINATION lib)

install(FILES imgui.h examples/imgui_impl_glfw.h examples/imgui_impl_opengl3.h DESTINATION include)
