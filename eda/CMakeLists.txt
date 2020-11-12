cmake_minimum_required(VERSION 3.10)

# set the project name
project(spe)

#set(VCPKG_TARGET_TRIPLET x64-linux)
#set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" )
find_package( Boost REQUIRED )
find_package( OpenCV CONFIG REQUIRED )
find_package(Eigen3 CONFIG REQUIRED)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(-bigobj)
endif()
# add the executable
add_executable(eda main.cpp)


target_link_libraries( eda PRIVATE opencv_dnn opencv_core )
