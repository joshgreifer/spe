cmake_minimum_required(VERSION 3.17)
project(spe)

set(CMAKE_CXX_STANDARD 17)
# add_compile_options(-bigobj)
add_subdirectory(demo)
add_subdirectory(test)

add_subdirectory(pix2pix)
add_subdirectory(core8)
add_subdirectory(eda)
