#include "matching.h"

#include <limits.h>
#include <stdlib.h>

int brief_hamming_distance(const uint8_t* lhs, const uint8_t* rhs) {
  int distance = 0;

  const uint64_t* descriptor_a = (uint64_t*)lhs;
  const uint64_t* descriptor_b = (uint64_t*)rhs;

  for (int i = 0; i < 4; ++i) {
    distance += __builtin_popcountll(descriptor_a[i] ^ descriptor_b[i]);
  }

  return distance;
}

Match* brief_match_descriptors(const Keypoint* keys1, const Descriptor* desc1, int count1,
                               const Keypoint* keys2, const Descriptor* desc2, int count2,
                               int* out_count) {
  if (!keys1 || count1 <= 0 || !keys2 || count2 <= 0 || !out_count) {
    if (out_count) *out_count = 0;
    return NULL;
  }

  Match* matches = (Match*)malloc(sizeof(Match) * count1);
  if (!matches) {
    *out_count = 0;
    return NULL;
  }

  int match_count = 0;
  int threshold = 50;

  for (int i = 0; i < count1; ++i) {
    int best_dist = INT_MAX;
    int second_best_dist = INT_MAX;
    int best_idx = -1;

    for (int j = 0; j < count2; ++j) {
      int dist = brief_hamming_distance(desc1[i].bytes, desc2[j].bytes);

      if (dist < best_dist) {
        second_best_dist = best_dist;
        best_dist = dist;
        best_idx = j;
      } else if (dist < second_best_dist) {
        second_best_dist = dist;
      }
    }

    if (best_dist <= threshold && best_idx != -1) {
      if ((long long)best_dist * 4 < (long long)second_best_dist * 3) {
        matches[match_count].query_idx = i;
        matches[match_count].train_idx = best_idx;
        matches[match_count].distance = best_dist;
        ++match_count;
      }
    }
  }

  *out_count = match_count;
  return matches;
}
