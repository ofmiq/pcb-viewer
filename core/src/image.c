#include "image.h"

#include <stdlib.h>

#include "stb_image.h"
#include "stb_image_write.h"

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

PcbImage* preprocess_grayscale(const PcbImage* src) {
  if (!src || !src->data || src->channels < 3) {
    return NULL;
  }

  PcbImage* img = (PcbImage*)malloc(sizeof(PcbImage));
  if (!img) {
    return NULL;
  }

  img->width = src->width;
  img->height = src->height;
  img->channels = 1;
  img->stride = img->width;

  img->data = (uint8_t*)malloc(img->width * img->height);
  if (!img->data) {
    free(img);
    return NULL;
  }

  for (int i = 0; i < img->width * img->height; ++i) {
    const uint8_t* p = src->data + i * src->channels;

    uint8_t r = p[0];
    uint8_t g = p[1];
    uint8_t b = p[2];

    double y = 0.299 * r + 0.587 * g + 0.114 * b;

    int value = (int)(y + 0.5f);
    if (value < 0) {
      value = 0;
    }
    if (value > 255) {
      value = 255;
    }

    img->data[i] = (uint8_t)value;
  }

  return img;
}
