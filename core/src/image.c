#include "../include/image.h"

#include <stdlib.h>

#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"

PcbImage* image_load(const char* filename) {
  PcbImage* img = (PcbImage*)malloc(sizeof(PcbImage));
  if (!img) {
    return NULL;
  }

  img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
  if (!img->data) {
    free(img);
    return NULL;
  }

  img->stride = img->width * img->channels;

  return img;
}

void image_free(PcbImage* img) {
  if (!img) {
    return;
  }

  if (img->data) {
    stbi_image_free(img->data);
  }
  free(img);
}

int image_save_png(const PcbImage* img, const char* filename) {
  if (!img || !img->data) {
    return 0;
  }
  stbi_write_png_compression_level = 0;
  stbi_write_force_png_filter = 0;
  return stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->stride);
}
