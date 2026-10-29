#ifndef ORB_H
#define ORB_H

#include <image.h>

#define BRIEF_DESCRIPTOR_BYTES 32
#define BRIEF_HALF_PATCH 15

typedef struct {
  uint8_t bytes[];
} Descriptor;

typedef struct {
  int x;
  int y;
  double score;
  Descriptor descriptor;
} Keypoint;

typedef enum { PIXEL_SIMILAR = 0, PIXEL_LIGHTER = 1, PIXEL_DARKER = 2 } PixelState;

Keypoint* fast_detect(const PcbImage* img, int threshold, int* count_out);
Keypoint* nms_suppress(const Keypoint* keypoints, int count, int radius, int* out_count);
Keypoint* top_k_select(const Keypoint* keypoints, int count, int k, int* out_count);

void brief_compute_descriptors(const PcbImage* img, Keypoint* keypoints, int count);

#endif  // ORB_H
