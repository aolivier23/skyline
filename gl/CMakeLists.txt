install(FILES DebuggingMacros.h DESTINATION include/gl)

add_library(mygl Exception.cpp Texture.cpp TextureArray.cpp Framebuffer.cpp)
target_link_libraries(mygl OpenGL glad)
install(TARGETS mygl DESTINATION lib)
install(FILES Exception.h Framebuffer.h DESTINATION include/gl)
