#include <stdio.h>
#include <GLFW/glfw3.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  return t;
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

  uint8_t* depth_data = (uint8_t*)calloc(depth_texture.bytes, 1);

  ImGui_ImplGlfw_Init(window, true);

  ImGui::GetStyle().WindowRounding = 0.f;

  ImVec4 clear_color = ImColor(218, 223, 225);

  bool show_window = true;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplGlfw_NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    for (int i = 0; i < depth_texture.len; i++) {
      depth_data[i * 4] = display_w % 255;
      depth_data[i * 4 + 1] = 0;
      depth_data[i * 4 + 2] = display_h % 255;
      depth_data[i * 4 + 3] = 255;
    }

    glBindTexture(GL_TEXTURE_2D, depth_texture.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_texture.width, depth_texture.height, GL_RGBA, GL_UNSIGNED_BYTE, depth_data);

    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("foo", &show_window,
                 ImVec2(float(display_w), float(display_h)), -1.f, flags);
    ImGui::Image((void*)depth_texture.handle, ImVec2(512, 424));
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
