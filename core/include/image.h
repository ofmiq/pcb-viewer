#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

typedef struct {
  int width;
  int height;
  int channels;
  int stride;  // assumed: stride = width * channels
  uint8_t* data;
} PcbImage;

PcbImage* image_load(const char* filename);
void image_free(PcbImage* img);

int image_save_png(const PcbImage* img, const char* filename);

PcbImage* preprocess_grayscale(const PcbImage* src);
PcbImage* preprocess_downscale(const PcbImage* src, const int targe_width);
PcbImage* preprocess_gaussian_blur(const PcbImage* src);

#endif  // PCB_IMAGE_H
