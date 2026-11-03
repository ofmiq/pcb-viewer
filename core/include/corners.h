#ifndef FEATURES_H
#define FEATURES_H

#include "config.h"
#include "image.h"

typedef struct {
  int x;
  int y;
  double score;
} Keypoint;

typedef struct {
  uint8_t bytes[BRIEF_DESCRIPTOR_BYTES];
} Descriptor;

typedef enum { PIXEL_SIMILAR = 0, PIXEL_LIGHTER = 1, PIXEL_DARKER = 2 } PixelState;

Keypoint* fast_detect(const PcbImage* img, int threshold, int* count_out);
Keypoint* nms_suppress(const Keypoint* keypoints, int count, int radius, int* out_count);
Keypoint* top_k_select(const Keypoint* keypoints, int count, int k, int* out_count);
Descriptor* brief_compute_descriptors(const PcbImage* img, Keypoint* keypoints, int count);

#endif  // FEATURES_H
