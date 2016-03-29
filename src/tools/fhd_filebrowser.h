#pragma once

#include <vector>
#include <string>

struct fhd_file {
  std::string name;
  std::string path;
};

struct fhd_filebrowser {
  fhd_filebrowser(const char* root);

  void set_root(const char* root);
  const char* get_path(int index);

  int selected_path = -1;
  std::vector<std::string> files;
  std::vector<std::string> paths;
};
