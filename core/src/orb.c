#include "orb.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "image.h"
#include "preprocessing.c"

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

static PcbImage* prepare_level(const PcbImage* rgb, int target_width) {
  if (!rgb || target_width <= 0) {
    return NULL;
  }

  PcbImage* scaled = NULL;
  const PcbImage* src = rgb;

  if (target_width != rgb->width) {
    scaled = preprocess_downscale(rgb, target_width);
    if (!scaled) {
      return NULL;
    }
    src = scaled;
  }

  PcbImage* gray = preprocess_grayscale(src);
  if (scaled) {
    image_free(scaled);
  }
  if (!gray) {
    return NULL;
  }

  PcbImage* blurred = preprocess_gaussian_blur(gray);
  image_free(gray);
  return blurred;
}

ImagePyramid* build_image_pyramid(const PcbImage* src) {
  if (!src) {
    return NULL;
  }

  ImagePyramid* image_pyramid = (ImagePyramid*)calloc(1, sizeof(ImagePyramid));
  if (!image_pyramid) {
    return NULL;
  }

  double scale = 1.0;
  for (int i = 0; i < PYRAMID_MAX_LEVELS; ++i) {
    int target_width = (int)(src->width * scale + 0.5);
    if (target_width < 4 * BRIEF_HALF_PATCH) {
      break;
    }

    PcbImage* level = prepare_level(src, target_width);
    if (!level) {
      free_image_pyramid(image_pyramid);
      return NULL;
    }

    image_pyramid->images[i] = level;
    image_pyramid->scale_factors[i] = scale;
    ++image_pyramid->count;

    scale *= PYRAMID_SCALE_STEP;
  }

  if (image_pyramid->count == 0) {
    free(image_pyramid);
    return NULL;
  }
  return image_pyramid;
}

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

void free_image_pyramid(ImagePyramid* image_pyramid) {
  if (!image_pyramid) {
    return;
  }
  for (int i = 0; i < image_pyramid->count; ++i) {
    image_free(image_pyramid->images[i]);
  }
  free(image_pyramid);
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
