#include <stdio.h>
#include <GLFW/glfw3.h>
#include "fhd_debug_frame_source.h"
#include "../fhd_math.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../fhd_sqlite_source.h"
#include "../fhd_classifier.h"
#include "../fhd_image.h"
#include "../fhd.h"
#include "fhd_texture.h"
#include "fhd_filebrowser.h"
#include <stdlib.h>
#include <memory>

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
    uint8_t normalized = depth_to_byte(reading, 500, 4500);

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

struct fhd_debug_ui {
  fhd_debug_ui(fhd_context* fhd);
  ~fhd_debug_ui();

  fhd_context* fhd = NULL;
  uint8_t* texture_data;
  bool show_candidates = false;
  std::vector<fhd_texture> textures;

  fhd_image* candidate_images;
};

fhd_debug_ui::fhd_debug_ui(fhd_context* fhd)
    : fhd(fhd),
      texture_data((uint8_t*)calloc(
          FHD_HOG_WIDTH * FHD_HOG_HEIGHT * 4 * fhd->candidates_capacity, 1)) {
  candidate_images =
      (fhd_image*)calloc(fhd->candidates_capacity, sizeof(fhd_image));
  for (int i = 0; i < fhd->candidates_capacity; i++) {
    fhd_texture t = fhd_create_texture(FHD_HOG_WIDTH, FHD_HOG_HEIGHT);
    textures.push_back(t);
    fhd_image_init(&candidate_images[i], FHD_HOG_WIDTH, FHD_HOG_HEIGHT);
  }
}

fhd_debug_ui::~fhd_debug_ui() {
  for (fhd_texture t : textures) {
    fhd_texture_destroy(t);
  }
}

void fhd_debug_update_candidates(fhd_debug_ui* ui,
                                 const fhd_candidate* candidates, int len) {
  for (int i = 0; i < len; i++) {
    const fhd_candidate* candidate = &candidates[i];
    fhd_image* candidate_img = &ui->candidate_images[i];

    fhd_image_region src_reg;
    src_reg.x = 1;
    src_reg.y = 1;
    src_reg.width = candidate->depth.width - 1;
    src_reg.height = candidate->depth.height - 1;

    fhd_image_region dst_reg;
    dst_reg.x = 0;
    dst_reg.y = 0;
    dst_reg.width = candidate_img->width;
    dst_reg.height = candidate_img->height;

    fhd_copy_sub_image(&candidate->depth, &src_reg, candidate_img, &dst_reg);
    update_depth_texture(ui->texture_data, candidate_img->data,
                         candidate_img->len);
    fhd_texture_update(&ui->textures[i], ui->texture_data);
  }
}

void render_directory(fhd_filebrowser* browser) {
  ImGui::BeginChild("files", ImVec2(400, 200), true);

  int current = browser->selected_path;
  int new_selection = -1;
  for (int i = 0; i < int(browser->files.size()); i++) {
    const char* file = browser->files[i].c_str();
    if (ImGui::Selectable(file, i == current)) {
      new_selection = i;
    }
  }

  ImGui::EndChild();

  if (const char* selected_path = browser->get_path(new_selection)) {
    browser->selected_path = new_selection;
    browser->set_root(selected_path);
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

  fhd_texture depth_texture = fhd_create_texture(512, 424);

  fhd_filebrowser file_browser(".");

  fhd_context detector;
  fhd_context_init(&detector, 512, 424, 8, 8);

  auto frame_source =
      std::unique_ptr<fhd_frame_source>(new fhd_debug_frame_source());
  fhd_classifier classifier;
  fhd_classifier_init(&classifier, "classifier.nn");
  detector.classifier = &classifier;

  uint8_t* depth_texture_data = (uint8_t*)calloc(depth_texture.bytes, 1);

  ImGui_ImplGlfw_Init(window, true);

  ImGui::GetStyle().WindowRounding = 0.f;

  ImVec4 clear_color = ImColor(218, 223, 225);

  bool show_window = true;

  fhd_debug_ui debug_ui(&detector);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    const uint16_t* depth_data = frame_source->get_frame();

    if (depth_data) {
      update_depth_texture(depth_texture_data, depth_data, 512 * 424);
      fhd_texture_update(&depth_texture, depth_texture_data);

      fhd_run_pass(&detector, depth_data);
    }

    ImGui_ImplGlfw_NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("foo", &show_window,
                 ImVec2(float(display_w), float(display_h)), -1.f, flags);

    ImGui::BeginGroup();
    ImGui::Text("frame %d/%d", frame_source->current_frame(),
                frame_source->total_frames());

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::Image((void*)intptr_t(depth_texture.handle), ImVec2(512, 424));
    for (int i = 0; i < detector.candidates_len; i++) {
      const fhd_candidate* candidate = &detector.candidates[i];
      if (candidate->weight >= 1.f) {
        const fhd_image_region region = candidate->depth_position;

        const float x = p.x + float(region.x);
        const float y = p.y + float(region.y);
        const float w = float(region.width);
        const float h = float(region.height);
        draw_list->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
                           IM_COL32(255, 255, 255, 255));
      }
    }

    render_directory(&file_browser);

    ImGui::BeginGroup();
    if (ImGui::Button("open")) {
      int path_index = file_browser.selected_path;
      const char* selected_path = file_browser.get_path(path_index);
      if (selected_path) {
        frame_source.reset(new fhd_sqlite_source(selected_path));
      }
    }

    ImGui::EndGroup();
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();

    /*
    for (int i = 0; i < detector.candidates_len; i++) {
      fhd_texture* t = &debug_ui.textures[i];
      printf("%d\n", t->handle);
      ImGui::Image((void*)intptr_t(t->handle), ImVec2(t->width, t->height));

      if (i % 5 < 4) ImGui::SameLine();
    }*/

    ImGui::EndGroup();

    ImGui::End();

    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    glfwSwapBuffers(window);
  }

  fhd_classifier_destroy(&classifier);
  fhd_texture_destroy(depth_texture);
  ImGui_ImplGlfw_Shutdown();
  glfwTerminate();

  return 0;
}
