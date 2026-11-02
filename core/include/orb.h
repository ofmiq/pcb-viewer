#ifndef ORB_H
#define ORB_H

#include <image.h>

#define FAST_ARC_LENGTH 12
#define BRIEF_DESCRIPTOR_BYTES 32
#define BRIEF_HALF_PATCH 15

typedef struct {
  uint8_t bytes[BRIEF_DESCRIPTOR_BYTES];
} Descriptor;

typedef struct {
  int x;
  int y;
  double score;
} Keypoint;

typedef struct {
  int query_idx;
  int train_idx;
  int distance;
} Match;

typedef enum { PIXEL_SIMILAR = 0, PIXEL_LIGHTER = 1, PIXEL_DARKER = 2 } PixelState;

Keypoint* fast_detect(const PcbImage* img, int threshold, int* count_out);
Keypoint* nms_suppress(const Keypoint* keypoints, int count, int radius, int* out_count);
Keypoint* top_k_select(const Keypoint* keypoints, int count, int k, int* out_count);

Descriptor* brief_compute_descriptors(const PcbImage* img, Keypoint* keypoints, int count);
int brief_hamming_distance(const uint8_t* lhs, const uint8_t* rhs);
Match* brief_match_descriptors(const Keypoint* keys1, const Descriptor* desc1, int count1,
                               const Keypoint* keys2, const Descriptor* desc2, int count2,
                               int* out_count);
#endif  // ORB_H
