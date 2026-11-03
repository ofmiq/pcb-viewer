#include "levels_results.h"

#include <stdlib.h>

LevelResult* compute_levels_results(const Keypoint* keypoints, const Descriptor* descriptors,
                                    int size, const FeaturePyramid* feature_pyramid) {
  if (!keypoints || !descriptors || size <= 0 || !feature_pyramid) {
    return NULL;
  }

  LevelResult* levels_results = (LevelResult*)calloc(feature_pyramid->count, sizeof(LevelResult));
  if (!levels_results) {
    return NULL;
  }

  for (int i = 0; i < feature_pyramid->count; ++i) {
    int n = 0;
    levels_results[i].matches = brief_match_descriptors(
        keypoints, descriptors, size, feature_pyramid->levels[i].keypoints,
        feature_pyramid->levels[i].descriptors, feature_pyramid->levels[i].count, &n);
    levels_results[i].count = n;
  }
  return levels_results;
}

void free_levels_results(LevelResult* levels_results, int count) {
  if (!levels_results) {
    return;
  }
  for (int i = 0; i < count; ++i) {
    free(levels_results[i].matches);
  }
  free(levels_results);
}
