#include "image_utils.h"

#include <stdint.h>
#include <stdlib.h>

#include "image.h"

PcbImage* grayscale_to_rgb(const PcbImage* src) {
  if (!src) {
    return NULL;
  }

  PcbImage* img = (PcbImage*)malloc(sizeof(PcbImage));
  if (!img) {
    return NULL;
  }

  int width = src->width;
  int height = src->height;
  int total_pixels = width * height;

  img->width = width;
  img->height = height;
  img->channels = 3;
  img->stride = width * 3;

  img->data = (uint8_t*)malloc(total_pixels * 3);
  if (!img->data) {
    free(img);
    return NULL;
  }

  for (int i = 0; i < total_pixels; ++i) {
    const uint8_t gray = src->data[i];
    img->data[i * 3] = gray;
    img->data[i * 3 + 1] = gray;
    img->data[i * 3 + 2] = gray;
  }

  return img;
}

void draw_keypoints(PcbImage* src, const Keypoint* keypoints, int count) {
  if (!src || !keypoints || src->channels != 3) {
    return;
  }

  int width = src->width;
  int height = src->height;

  for (int k = 0; k < count; k++) {
    int cx = keypoints[k].x;
    int cy = keypoints[k].y;

    for (int dy = -2; dy <= 2; dy++) {
      for (int dx = -2; dx <= 2; dx++) {
        if (dx == 0 || dy == 0) {
          int nx = cx + dx;
          int ny = cy + dy;

          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            int idx = (ny * src->width + nx) * 3;
            src->data[idx + 0] = 255;
            src->data[idx + 1] = 0;
            src->data[idx + 2] = 0;
          }
        }
      }
    }
  }
}
