add_library(imgui
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

add_library(sqlite
  sqlite3/sqlite3.c
)

if (OPENMP_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  add_definitions(-DFHD_OMP)
endif()

if (WIN32)
  include_directories(${KINECTV2_INCLUDE_DIR})
endif()

add_library(fhd
  fhd.cpp
  fhd_block_allocator.cpp
  fhd_candidate.cpp
  fhd_classifier.cpp
  fhd_hash.cpp
  fhd_image.cpp
  fhd_kinect.cpp
  fhd_math.cpp
  fhd_perf.cpp
  fhd_sampler.cpp
  fhd_segmentation.cpp

  pcg/pcg_basic.c
)

add_library(fhd_util
  fhd_candidate_db.cpp
  fhd_sqlite_source.cpp
)

set (UI_SOURCES
  tools/fhd_debug_frame_source.cpp
  tools/fhd_filebrowser.cpp
  tools/fhd_texture.cpp
  tools/ui.cpp
)

if (WIN32)
  list(APPEND UI_SOURCES tools/fhd_kinect_source.cpp)
endif()

add_executable(
  fhd_ui
  ${UI_SOURCES}
)

add_executable(
  fhd_train
  tools/fhd_train.cpp
)

add_executable(
  fhd_test
  tools/fhd_test.cpp
)

target_link_libraries(
  fhd_ui
  ${KINECTV2_LIBRARY}
  fhd_util
  fhd
  sqlite
  imgui
  glfw
  glew
  floatfann
  ${GLFW_LIBRARIES}
)

target_link_libraries(
  fhd_train
  fhd_util
  fhd
  floatfann
  sqlite
  ${CMAKE_DL_LIBS}
)

target_link_libraries(
  fhd_test
  fhd_util
  fhd
  floatfann
  sqlite
  ${CMAKE_DL_LIBS}
)

if (FHD_BUILD_EXAMPLES)
    add_executable(example_detect
      examples/example_detect.cpp
      tools/fhd_debug_frame_source.cpp
    )

    target_link_libraries(example_detect
      fhd_util
      fhd
      floatfann
      sqlite
    )
endif()

set (FHD_HEADERS
  fhd.h
  fhd_candidate.h
  fhd_config.h
  fhd_image.h
  fhd_math.h
  fhd_perf.h
  fhd_sampler.h
)

install(FILES ${FHD_HEADERS} DESTINATION include)
install(TARGETS fhd EXPORT fhd DESTINATION lib)
install(TARGETS fhd_ui fhd_train fhd_test RUNTIME DESTINATION bin)
