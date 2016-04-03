#include "fhd_filebrowser.h"
#include "../tinydir/tinydir.h"

fhd_filebrowser::fhd_filebrowser(const char* root) {
  set_root(root);
}

void fhd_filebrowser::set_root(const char* root) {
  tinydir_dir dir;
  if (tinydir_open_sorted(&dir, root) != -1) {
    files.clear();

    for (size_t i = 0; i < dir.n_files; i++) {
      tinydir_file f;
      tinydir_readfile_n(&dir, &f, i);
      if (strcmp(f.name, ".") != 0) {
        files.push_back({f.name, f.path, bool(f.is_dir)});
      }
      tinydir_next(&dir);
    }

    tinydir_close(&dir);
  }
}

const fhd_file* fhd_filebrowser::get_file(int index) {
  if (index >= 0 && index < int(files.size())) {
    return &files[index];
  }

  return NULL;
}
