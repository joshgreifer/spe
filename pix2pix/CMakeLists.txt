project(spe)


find_package( Boost REQUIRED )
find_package( OpenCV CONFIG REQUIRED )
find_package(Eigen3 CONFIG REQUIRED)


if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    MESSAGE("Using Clang compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    MESSAGE("Using Gnu compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    MESSAGE("Using Intel compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    MESSAGE("Using Microsoft Visual Studio Compiler")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(-std:c++17 -bigobj)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fvisibility=hidden -std=c++17 -Wno-parentheses -Wno-undefined-var-template)
else()
    add_compile_options(-std=c++17)
endif()

# add the executable

add_executable(pix2pix_test pix2pix_test.cpp)

file(COPY artefacts/netG.onnx DESTINATION .)
file(COPY artefacts/dnn_ut.onnx DESTINATION .)
target_link_libraries( pix2pix_test PRIVATE opencv_dnn opencv_core)

