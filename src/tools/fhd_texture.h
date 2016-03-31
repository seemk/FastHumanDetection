#pragma once

#include <GL/glew.h>
#include <stdint.h>

struct fhd_texture {
  GLuint handle;
  int width;
  int height;
  int pitch;
  int bytes;
  int len;
  uint8_t* data;
};

fhd_texture fhd_create_texture(int width, int height);
void fhd_texture_destroy(fhd_texture* texture);
void fhd_texture_update(fhd_texture* texture, const uint8_t* data);
void fhd_texture_upload(fhd_texture* texture);
