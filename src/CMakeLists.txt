add_library(imgui
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw.cpp
)

add_library(sqlite
  sqlite3/sqlite3.c
)

add_executable(
  fhd_ui
  tools/ui.cpp
)

target_link_libraries(
  fhd_ui
  sqlite
  imgui
  glfw
  ${GLFW_LIBRARIES}
)
