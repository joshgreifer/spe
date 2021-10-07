cmake_minimum_required(VERSION 3.10)

# set the project name
project(sdft)

#set(VCPKG_TARGET_TRIPLET x64-linux)
#set(CMAKE_TOOLCHAIN_FILE "/Users/josh/vcpkg/scripts/buildsystems/vcpkg.cmake" )
find_package(FFTW3 CONFIG REQUIRED)

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

add_executable(sdft sdft.cpp)


set_property(TARGET spe_test6 PROPERTY CXX_STANDARD 17)
set_property(TARGET spe_test6 PROPERTY CXX_STANDARD_REQUIRED ON)


file(GLOB ARTEFACTS artefacts/*)
file(COPY ${ARTEFACTS} DESTINATION .)

target_link_libraries(sdft PRIVATE FFTW3::fftw3)