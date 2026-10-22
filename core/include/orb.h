#ifndef ORB_H
#define ORB_H

#include <image.h>

typedef struct {
  int x;
  int y;
  double score;
} Keypoint;

typedef enum { PIXEL_SIMILAR = 0, PIXEL_LIGHTER = 1, PIXEL_DARKER = 2 } PixelState;

Keypoint* fast_detect(const PcbImage* img, int threshold, int* count_out);
Keypoint* nms_suppress(const Keypoint* keypoints, int count, int radius, int* out_count);

#endif  // ORB_H
