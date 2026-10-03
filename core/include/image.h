#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>

typedef struct {
  int width;
  int height;
  int channels;
  int stride;
  unsigned char* data;
} PcbImage;

PcbImage* image_load(const char* filename);
void image_free(PcbImage* img);

int image_save_png(const PcbImage* img, const char* filename);

#endif  // PCB_IMAGE_H
