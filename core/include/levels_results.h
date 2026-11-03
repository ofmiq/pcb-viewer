#ifndef LEVELS_RESULTS_H
#define LEVELS_RESULTS_H

#include "feature_pyramid.h"
#include "matching.h"

typedef struct {
  Match* matches;
  int count;
} LevelResult;

LevelResult* compute_levels_results(const Keypoint* keypoints, const Descriptor* descriptors,
                                    int size, const FeaturePyramid* feature_pyramid);

void free_levels_results(LevelResult* leveles_result, int count);

#endif  // LEVELS_RESULTS_H
