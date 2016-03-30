#pragma once

#include <vector>
#include <string>

struct fhd_file {
  std::string name;
  std::string path;
  bool is_dir;
};

struct fhd_filebrowser {
  fhd_filebrowser(const char* root);

  void set_root(const char* root);
  const fhd_file* get_file(int index);

  std::vector<fhd_file> files;
};
