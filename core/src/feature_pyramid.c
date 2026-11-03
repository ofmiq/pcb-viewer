#include "feature_pyramid.h"

#include <stdlib.h>

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
