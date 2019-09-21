add_library(engine View.cpp)
target_link_libraries(engine mygl OpenCL)
install(TARGETS engine DESTINATION lib)

add_library(camera CameraController.cpp CameraModel.cpp FPSController.cpp WithCamera.cpp)
target_link_libraries(camera OpenCL)
install(TARGETS camera DESTINATION lib)

install(FILES View.h vector.h CameraController.h CameraModel.h FPSController.h WithCamera.h DESTINATION include/engine)
