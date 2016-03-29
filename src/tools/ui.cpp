#include <stdio.h>
#include <GLFW/glfw3.h>
#include "fhd_debug_frame_source.h"
#include "../fhd_math.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../fhd_sqlite_source.h"
#include <stdlib.h>

struct fhd_texture {
  GLuint handle;
  int width;
  int height;
  int pitch;
  int bytes;
  int len;
};

fhd_texture create_texture(int width, int height) {
  fhd_texture t;
  glGenTextures(1, &t.handle);
  t.width = width;
  t.height = height;
  t.pitch = width * 4;
  t.bytes = t.pitch * height;
  t.len = width * height;
  glBindTexture(GL_TEXTURE_2D, t.handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  return t;
}

uint8_t depth_to_byte(uint16_t value, uint16_t min, uint16_t max) {
  const uint32_t v = fhd_clamp(value, min, max);
  const uint32_t lmin = min;
  const uint32_t lmax = max;
  return uint8_t((v - lmin) * 255 / (lmax - lmin));
}

void update_depth_texture(uint8_t* texture_data, const uint16_t* depth,
                          int len) {
  for (int i = 0; i < len; i++) {
    uint16_t reading = depth[i];
    uint8_t normalized =
        depth_to_byte(reading, 500, 4500);

    int idx = 4 * i;

    if (reading >= 500 && reading <= 4500) {
      texture_data[idx] = normalized;
      texture_data[idx + 1] = 255 - normalized;
      texture_data[idx + 2] = 255 - normalized;
    } else {
      texture_data[idx] = 0;
      texture_data[idx + 1] = 0;
      texture_data[idx + 2] = 0;
    }
    texture_data[idx + 3] = 255;
  }
}

void glfwError(int error, const char* description) {
  fprintf(stderr, "GLFW error: %i: %s\n", error, description);
}

int main(int argc, char** argv) {
  glfwSetErrorCallback(glfwError);

  if (!glfwInit()) return 1;

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  GLFWwindow* window =
      glfwCreateWindow(mode->width, mode->height, "HumanDetection", NULL, NULL);
  glfwMakeContextCurrent(window);

  fhd_texture depth_texture = create_texture(512, 424);

  fhd_frame_source* frame_source = new fhd_sqlite_source("train12.db");

  uint8_t* depth_texture_data = (uint8_t*)calloc(depth_texture.bytes, 1);

  ImGui_ImplGlfw_Init(window, true);

  ImGui::GetStyle().WindowRounding = 0.f;

  ImVec4 clear_color = ImColor(218, 223, 225);

  bool show_window = true;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    const uint16_t* depth_data = frame_source->get_frame();
    update_depth_texture(depth_texture_data, depth_data, 512 * 424);

    ImGui_ImplGlfw_NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glBindTexture(GL_TEXTURE_2D, depth_texture.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_texture.width,
                    depth_texture.height, GL_RGBA, GL_UNSIGNED_BYTE,
                    depth_texture_data);

    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("foo", &show_window,
                 ImVec2(float(display_w), float(display_h)), -1.f, flags);
    ImGui::Text("frame %d/%d", frame_source->current_frame(), frame_source->total_frames());
    ImGui::Image((void*)intptr_t(depth_texture.handle), ImVec2(512, 424));
    ImGui::End();

    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    glfwSwapBuffers(window);
  }

  ImGui_ImplGlfw_Shutdown();
  glfwTerminate();

  return 0;
}
