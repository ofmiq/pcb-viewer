#include "orb.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "image.h"
#include "preprocessing.c"

static FeatureSet detect_level(const PcbImage* img) {
  FeatureSet set = {NULL, NULL, 0};
  if (!img) {
    return set;
  }

  int raw_size = 0;
  Keypoint* raw = fast_detect(img, THRESHOLD, &raw_size);
  if (!raw || raw_size <= 0) {
    free(raw);
    return set;
  }

  int nms_size = 0;
  Keypoint* nms = nms_suppress(raw, raw_size, RADIUS, &nms_size);
  free(raw);
  if (!nms) {
    return set;
  }

  int top_size = 0;
  Keypoint* top = top_k_select(nms, nms_size, K, &top_size);
  free(nms);
  if (!top) {
    return set;
  }

  Descriptor* desc = brief_compute_descriptors(img, top, top_size);
  if (!desc) {
    free(top);
    return set;
  }

  set.keypoints = top;
  set.descriptors = desc;
  set.count = top_size;
  return set;
}

FeaturePyramid* build_feature_pyramid(const ImagePyramid* image_pyramid) {
  if (!image_pyramid) {
    return NULL;
  }

  FeaturePyramid* fp = (FeaturePyramid*)calloc(1, sizeof(FeaturePyramid));
  if (!fp) {
    return NULL;
  }

  for (int i = 0; i < image_pyramid->count; ++i) {
    fp->levels[i] = detect_level(image_pyramid->images[i]);
    ++fp->count;
  }
  return fp;
}

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

void free_feature_pyramid(FeaturePyramid* feature_pyramid) {
  if (!feature_pyramid) {
    return;
  }
  for (int i = 0; i < feature_pyramid->count; ++i) {
    free(feature_pyramid->levels[i].keypoints);
    free(feature_pyramid->levels[i].descriptors);
  }
  free(feature_pyramid);
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
