cmake_minimum_required(VERSION 3.10)

# set the project name
#project(spe)

#set(VCPKG_TARGET_TRIPLET x64-linux)
#set(CMAKE_TOOLCHAIN_FILE "/Users/josh/vcpkg/scripts/buildsystems/vcpkg.cmake" )
#set(PYTHONHOME "C:/ProgramData/MiniConda3")
#set(PYTHONHPATH "C:/ProgramData/Miniconda3;c:/ProgramData/Miniconda3/DLLs")
#set(PYTHON_EXECUTABLE:FILEPATH="C:/ProgramData/MiniConda3/python.exe")
#set(PYTHON_LIBRARY "C:/ProgramData/MiniConda3/include")
#set(PYTHON_INCLUDE_DIR "C:/ProgramData/MiniConda3/libs/python37.lib")
#find_package( Boost REQUIRED )
#find_package( OpenCV CONFIG REQUIRED )
#find_package(Eigen3 CONFIG REQUIRED)
#find_package(pybind11 CONFIG REQUIRED)

#message(STATUS "Found pybind11 v${pybind11_VERSION}: ${pybind11_INCLUDE_DIRS} ${pybind11_DEFINITIONS} ${pybind11_LIBRARIES}")
#message(STATUS "Found Python ${PYTHON_LIBRARY_SUFFIX} in: ${PYTHON_PREFIX}: ${PYTHON_INCLUDE_DIRS} ${PYTHON_LIBRARIES} ${PYTHON_SITE_PACKAGES}")

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
	add_compile_options(-std:c++latest -await -bigobj)
	find_package(cppcoro CONFIG REQUIRED)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-fvisibility=hidden -std=c++20 -Wno-parentheses -Wno-undefined-var-template)
else()
	add_compile_options(-std=c++17)
endif()

# add the executable

add_executable(core8_test core8_test.cpp)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set_property(TARGET core8_test PROPERTY CXX_STANDARD 20)
	set_property(TARGET core8_test PROPERTY CXX_STANDARD_REQUIRED ON)
#	set_property(TARGET core8_test PROPERTY CXX_EXTENSIONS ON)
endif()
file(GLOB ARTEFACTS artefacts/*)
file(COPY ${ARTEFACTS} DESTINATION .)

target_link_libraries( core8_test PRIVATE cppcoro)

