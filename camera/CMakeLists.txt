add_library(camera CameraController.cpp CameraModel.cpp FPSController.cpp)
target_link_libraries(camera OpenCL)
install(TARGETS camera DESTINATION lib)

install(FILES CameraController.h CameraModel.h FPSController.h DESTINATION include/camera)
