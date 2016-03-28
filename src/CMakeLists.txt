add_library(imgui
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw.cpp
)

add_library(sqlite
  sqlite3/sqlite3.c
)

if (OPENMP_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  add_definitions(-DFHD_OMP)
endif()

add_library(fhd
  fhd.cpp
  fhd_block_allocator.cpp
  fhd_candidate.cpp
  fhd_candidate_db.cpp
  fhd_classifier.cpp
  fhd_hash.cpp
  fhd_image.cpp
  fhd_kinect.cpp
  fhd_math.cpp
  fhd_perf.cpp
  fhd_sampler.cpp
  fhd_segmentation.cpp
  fhd_sqlite_source.cpp

  pcg/pcg_basic.c
)

add_executable(
  fhd_ui
  tools/fhd_debug_frame_source.cpp
  tools/ui.cpp
)

target_link_libraries(
  fhd_ui
  sqlite
  imgui
  glfw
  ${GLFW_LIBRARIES}
)
