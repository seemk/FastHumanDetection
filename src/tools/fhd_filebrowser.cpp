#include "fhd_filebrowser.h"
#include "../tinydir/tinydir.h"
#include <stdio.h>
#include <stdlib.h>

fhd_filebrowser::fhd_filebrowser(const char* root) {
  set_root(root);
}

void fhd_filebrowser::set_root(const char* root) {
  tinydir_dir dir;
  if (tinydir_open_sorted(&dir, root) != -1) {
    selected_path = -1;

    files.clear();
    paths.clear();

    for (size_t i = 0; i < dir.n_files; i++) {
      tinydir_file f;
      tinydir_readfile_n(&dir, &f, i);
      if (strcmp(f.name, ".") != 0) {
        files.push_back(f.name);
        paths.push_back(f.path);
      }
      tinydir_next(&dir);
    }

    tinydir_close(&dir);
  }
}

const char* fhd_filebrowser::get_path(int index) {
  if (index >= 0 && index < int(paths.size())) {
    return paths[index].c_str();
  }

  return NULL;
}
