#include "fhd_texture.h"
#include <stdlib.h>

fhd_texture fhd_create_texture(int width, int height) {
  fhd_texture t;
  t.width = width;
  t.height = height;
  t.pitch = width * 4;
  t.bytes = t.pitch * height;
  t.len = width * height;
  t.data = (uint8_t*)calloc(t.bytes, 1);
  glGenTextures(1, &t.handle);
  glBindTexture(GL_TEXTURE_2D, t.handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
  return t;
}

void fhd_texture_destroy(fhd_texture* texture) {
  glDeleteTextures(1, &texture->handle);
  free(texture->data);
}

void fhd_texture_update(fhd_texture* texture, const uint8_t* data) {
  glBindTexture(GL_TEXTURE_2D, texture->handle);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->width, texture->height,
                  GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void fhd_texture_upload(fhd_texture* texture) {
  glBindTexture(GL_TEXTURE_2D, texture->handle);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->width, texture->height,
                  GL_RGBA, GL_UNSIGNED_BYTE, texture->data);
}
