#ifndef MATCHING_H
#define MATCHING_H

#include <stdint.h>

#include "corners.h"

typedef struct {
  int query_idx;
  int train_idx;
  int distance;
} Match;

int brief_hamming_distance(const uint8_t* lhs, const uint8_t* rhs);
Match* brief_match_descriptors(const Keypoint* keys1, const Descriptor* desc1, int count1,
                               const Keypoint* keys2, const Descriptor* desc2, int count2,
                               int* out_count);

#endif
