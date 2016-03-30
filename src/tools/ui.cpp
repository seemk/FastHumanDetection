#include <stdio.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw_gl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "fhd_debug_frame_source.h"
#include "../fhd_math.h"
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

  fhd_context* fhd = NULL;
  fhd_filebrowser file_browser;
  bool show_candidates = false;
  bool show_file_selection = false;
  int filebrowser_selected_index = -1;
  std::vector<fhd_texture> textures;
  std::string database_name = "none";
  std::string classifier_name = "none";

  fhd_image* candidate_images;
};

fhd_debug_ui::fhd_debug_ui(fhd_context* fhd) : fhd(fhd), file_browser(".") {
  candidate_images =
      (fhd_image*)calloc(fhd->candidates_capacity, sizeof(fhd_image));
  for (int i = 0; i < fhd->candidates_capacity; i++) {
    fhd_texture t = fhd_create_texture(FHD_HOG_WIDTH, FHD_HOG_HEIGHT);
    textures.push_back(t);
    fhd_image_init(&candidate_images[i], FHD_HOG_WIDTH, FHD_HOG_HEIGHT);
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
    update_depth_texture(ui->textures[i].data, candidate_img->data,
                         candidate_img->len);
    fhd_texture_update(&ui->textures[i], ui->textures[i].data);
  }
}

void render_directory(fhd_debug_ui* ui) {
  ImGui::BeginChild("files", ImVec2(400, 200), true);

  int current = ui->filebrowser_selected_index;
  int new_selection = -1;
  for (int i = 0; i < int(ui->file_browser.files.size()); i++) {
    const char* file = ui->file_browser.files[i].name.c_str();
    if (ImGui::Selectable(file, i == current)) {
      new_selection = i;
    }
  }

  ImGui::EndChild();

  if (const fhd_file* selected_file = ui->file_browser.get_file(new_selection)) {
    if (selected_file->is_dir) {
      ui->filebrowser_selected_index = -1;
    } else {
      ui->filebrowser_selected_index = new_selection;
    }

    ui->file_browser.set_root(selected_file->path.c_str());
  }
}

void glfwError(int error, const char* description) {
  fprintf(stderr, "GLFW error: %i: %s\n", error, description);
}

int main(int argc, char** argv) {
  glfwSetErrorCallback(glfwError);

  if (!glfwInit()) return 1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  GLFWwindow* window =
      glfwCreateWindow(mode->width, mode->height, "HumanDetection", NULL, NULL);
  glfwMakeContextCurrent(window);
  glewExperimental = GL_TRUE;
  glewInit();

  auto frame_source =
      std::unique_ptr<fhd_frame_source>(new fhd_debug_frame_source());

  fhd_context detector;
  fhd_context_init(&detector, 512, 424, 8, 8);

  ImGui_ImplGlfwGL3_Init(window, true);
  ImGui::GetStyle().WindowRounding = 0.f;
  bool show_window = true;

  fhd_debug_ui debug_ui(&detector);
  fhd_texture depth_texture = fhd_create_texture(512, 424);

  ImVec4 clear_color = ImColor(218, 223, 225);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    const uint16_t* depth_data = frame_source->get_frame();

    if (depth_data) {
      update_depth_texture(depth_texture.data, depth_data, 512 * 424);
      fhd_texture_update(&depth_texture, depth_texture.data);

      fhd_run_pass(&detector, depth_data);
      fhd_debug_update_candidates(&debug_ui, detector.candidates,
                                  detector.candidates_len);
    }

    ImGui_ImplGlfwGL3_NewFrame();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("foo", &show_window,
                 ImVec2(float(display_w), float(display_h)), -1.f, flags);

    ImGui::BeginGroup();

    if (ImGui::Button("open database")) {
      ImGui::OpenPopup("select database");
    }

    if (ImGui::Button("open classifier")) {
      ImGui::OpenPopup("select classifier");
    }

    if (ImGui::BeginPopupModal("select database", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      render_directory(&debug_ui);
      if (ImGui::Button("open")) {
        int path_index = debug_ui.filebrowser_selected_index;
        const fhd_file* selected_file = debug_ui.file_browser.get_file(path_index);
        if (selected_file) {
          frame_source.reset(
              new fhd_sqlite_source(selected_file->path.c_str()));
          debug_ui.database_name = selected_file->name;
          debug_ui.filebrowser_selected_index = -1;
        }

        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("select classifier", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      render_directory(&debug_ui);

      if (ImGui::Button("open")) {
        int path_index = debug_ui.filebrowser_selected_index;
        const fhd_file* selected_file = debug_ui.file_browser.get_file(path_index);
        if (selected_file) {
          fhd_classifier_destroy(detector.classifier);

          detector.classifier =
              fhd_classifier_create(selected_file->path.c_str());
          if (detector.classifier) {
            debug_ui.classifier_name = selected_file->name;
          } else {
            debug_ui.classifier_name = "none";
          }
        }

        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::Text("frame source: %s; frame %d/%d", debug_ui.database_name.c_str(),
                frame_source->current_frame(), frame_source->total_frames());
    ImGui::Text("classifier: %s", debug_ui.classifier_name.c_str());

    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();

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

    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();

    for (int i = 0; i < detector.candidates_len; i++) {
      fhd_texture* t = &debug_ui.textures[i];
      ImGui::Image((void*)intptr_t(t->handle),
                   ImVec2(t->width * 2, t->height * 2));
      if (i % 4 < 3) ImGui::SameLine();
    }

    ImGui::EndGroup();
    ImGui::End();

    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    glfwSwapBuffers(window);
  }

  fhd_texture_destroy(&depth_texture);
  fhd_classifier_destroy(detector.classifier);
  ImGui_ImplGlfwGL3_Shutdown();
  glfwTerminate();

  return 0;
}
