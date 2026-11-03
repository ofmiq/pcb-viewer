#ifndef FEATURE_PYRAMID_H
#define FEATURE_PYRAMID_H

#include "corners.h"
#include "image_pyramid.h"

typedef struct {
  Keypoint* keypoints;
  Descriptor* descriptors;
  int count;
} FeatureSet;

typedef struct {
  FeatureSet levels[PYRAMID_MAX_LEVELS];
  int count;
} FeaturePyramid;

FeaturePyramid* build_feature_pyramid(const ImagePyramid* image_pyramid);
void free_feature_pyramid(FeaturePyramid* feature_pyramid);

#endif  // FEATURE_PYRAMID_H
