cmake_minimum_required(VERSION 3.1)
project(FastHumanDetection)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

find_package(Threads)
find_package(OpenMP)

add_subdirectory("third_party/glfw-3.1.2")
add_subdirectory("third_party/fann")
add_subdirectory("third_party/glew-1.13.0/build/cmake")

option(KINECT_ENABLED "Use KinectV2 SDK" OFF)
option(FHD_INSTALL "Generate install target" OFF)

set (CMAKE_CXX_STANDARD 11)
if (WIN32)
  add_definitions(
    -D_WIN32_WINNT=0x0501
    -D_SCL_SECURE_NO_WARNINGS
    -D_CRT_SECURE_NO_WARNINGS
  )

  if (KINECT_ENABLED)
    find_package(KinectV2 REQUIRED)
    add_definitions(
      -DFHD_KINECT_ENABLED=1
    )
  endif()
else()
  add_definitions(-Wall -Wextra -march=native)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include)

INCLUDE_DIRECTORIES(SYSTEM
  "third_party/glfw-3.1.2/include"
  "third_party/eigen"
  "third_party/fann/src/include"
  "third_party/glew-1.13.0/include"
)

add_subdirectory(src)
