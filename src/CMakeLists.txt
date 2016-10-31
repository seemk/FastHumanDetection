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

set(UTIL_SOURCES
  fhd_candidate_db.cpp
  fhd_sqlite_source.cpp
  tools/fhd_debug_frame_source.cpp
)

if (WIN32)
  list(APPEND UTIL_SOURCES tools/fhd_kinect_source.cpp)
endif()

add_library(fhd_util
  ${UTIL_SOURCES}
)

add_executable(
  fhd_ui
  tools/fhd_filebrowser.cpp
  tools/fhd_training.cpp
  tools/fhd_texture.cpp
  tools/ui.cpp
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
  floatfann
  ${GLFW_LIBRARIES}
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
    )

    target_link_libraries(example_detect
      fhd_util
      fhd
      floatfann
      sqlite
      ${KINECTV2_LIBRARY}
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
install(TARGETS fhd_ui fhd_test RUNTIME DESTINATION bin)

if (WIN32)
  add_custom_command(
    TARGET fhd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:floatfann>/floatfann.dll ${ARTIFACT_DIR}
  )
endif()