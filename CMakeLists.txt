cmake_minimum_required(VERSION 3.1)
project(FastHumanDetection)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

find_package(Threads)
find_package(OpenMP)

add_subdirectory("third_party/glfw-3.1.2")
add_subdirectory("third_party/fann")

option(KINECT_ENABLED "Use KinectV2 SDK" OFF)
option(FHD_INSTALL "Generate install target" OFF)
option(FHD_BUILD_EXAMPLES "Build examples" OFF)

set (CMAKE_CXX_STANDARD 11)
if (WIN32)
  add_definitions(
    -D_WIN32_WINNT=0x0501
    -D_SCL_SECURE_NO_WARNINGS
    -D_CRT_SECURE_NO_WARNINGS
  )

  find_package(KinectV2 REQUIRED)
else()
  add_definitions(-Wall -Wextra -march=native)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include)

INCLUDE_DIRECTORIES(SYSTEM
  "third_party/glfw-3.1.2/include"
  "third_party/eigen"
  "third_party/fann/src/include"
)

add_subdirectory(src)
