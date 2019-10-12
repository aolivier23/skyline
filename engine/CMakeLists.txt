add_library(engine View.cpp WithCamera.cpp WithRandomSeeds.cpp)
target_link_libraries(engine mygl OpenCL camera)
install(TARGETS engine DESTINATION lib)

install(FILES View.h WithCamera.h WithRandomSeeds.h DESTINATION include/engine)
