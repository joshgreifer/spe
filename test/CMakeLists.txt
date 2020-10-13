cmake_minimum_required(VERSION 3.10)

# set the project name
project(spe)

#set(VCPKG_TARGET_TRIPLET x64-linux)
#set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" )
#set(PYTHONHOME "C:/ProgramData/MiniConda3")
#set(PYTHONHPATH "C:/ProgramData/Miniconda3;c:/ProgramData/Miniconda3/DLLs")
#set(PYTHON_EXECUTABLE:FILEPATH="C:/ProgramData/MiniConda3/python.exe")
#set(PYTHON_LIBRARY "C:/ProgramData/MiniConda3/include")
#set(PYTHON_INCLUDE_DIR "C:/ProgramData/MiniConda3/libs/python37.lib")
find_package( Boost REQUIRED )
find_package( OpenCV CONFIG REQUIRED )
find_package(Eigen3 CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
message(STATUS "Found pybind11 v${pybind11_VERSION}: ${pybind11_INCLUDE_DIRS} ${pybind11_DEFINITIONS} ${pybind11_LIBRARIES}")
message(STATUS "Found Python ${PYTHON_LIBRARY_SUFFIX} in: ${PYTHON_PREFIX}: ${PYTHON_INCLUDE_DIRS} ${PYTHON_LIBRARIES} ${PYTHON_SITE_PACKAGES}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/bigobj)
endif()
# add the executable
add_executable(spe_test eng6_tests.cpp)

file(COPY artefacts/dnn_ut.onnx DESTINATION .)

target_link_libraries( spe_test PRIVATE opencv_dnn opencv_core pybind11::embed pybind11::module pybind11::pybind11)
