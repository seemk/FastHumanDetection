#pragma once

#include <GLFW/glfw3.h>

struct fhd_texture {
  GLuint handle;
  int width;
  int height;
  int pitch;
  int bytes;
  int len;
};

fhd_texture fhd_create_texture(int width, int height);
void fhd_texture_destroy(fhd_texture texture);
void fhd_texture_update(fhd_texture* texture, const uint8_t* data);

