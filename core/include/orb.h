#ifndef ORB_H
#define ORB_H

#include <image.h>

#include "config.h"
#include "corners.h"
#include "matching.h"

typedef struct {
  PcbImage* images[PYRAMID_MAX_LEVELS];
  double scale_factors[PYRAMID_MAX_LEVELS];
  int count;
} ImagePyramid;

typedef struct {
  Keypoint* keypoints;
  Descriptor* descriptors;
  int count;
} FeatureSet;

typedef struct {
  FeatureSet levels[PYRAMID_MAX_LEVELS];
  int count;
} FeaturePyramid;

typedef struct {
  Match* matches;
  int count;
} LevelResult;

ImagePyramid* build_image_pyramid(const PcbImage* src);
FeaturePyramid* build_feature_pyramid(const ImagePyramid* image_pyramid);
LevelResult* compute_levels_results(const Keypoint* keypoints, const Descriptor* descriptors,
                                    int size, const FeaturePyramid* feature_pyramid);

void free_image_pyramid(ImagePyramid* image_pyramidp);
void free_feature_pyramid(FeaturePyramid* feature_pyramid);
void free_levels_results(LevelResult* leveles_result, int count);

#endif  // ORB_H
