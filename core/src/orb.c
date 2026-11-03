#include "orb.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "brief_patern.h"
#include "config.h"
#include "image.h"
#include "image_utils.h"

// OFFSETS FOR TRAVERSAL BY CIRCLE
static const int CIRCLE_X_OFFSETS[16] = {0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3, -3, -3, -2, -1};
static const int CIRCLE_Y_OFFSETS[16] = {-3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3};

static inline PixelState classify_pixel(uint8_t center, uint8_t sample, int threshold) {
  if (sample > center + threshold) {
    return PIXEL_LIGHTER;
  }
  if (sample < center - threshold) {
    return PIXEL_DARKER;
  }
  return PIXEL_SIMILAR;
}

static inline double compute_fast_score(const PcbImage* img, int x, int y, uint8_t p, int threshold,
                                        PixelState target_state) {
  if (!img) {
    return 0.0;
  }

  double sum_light = 0.0;
  double sum_dark = 0.0;

  for (int i = 0; i < 16; i++) {
    int idx = (y + CIRCLE_Y_OFFSETS[i]) * img->width + (x + CIRCLE_X_OFFSETS[i]);
    int val = img->data[idx];

    if (val > p + threshold) {
      sum_light += (val - p);
    } else if (val < p - threshold) {
      sum_dark += (p - val);
    }
  }

  sum_light -= threshold;
  sum_dark -= threshold;

  if (target_state == PIXEL_LIGHTER) {
    return (sum_light > 0.0) ? sum_light : 0.0;
  } else {
    return (sum_dark > 0.0) ? sum_dark : 0.0;
  }
}

static int keypoints_comparator(const void* a, const void* b) {
  const Keypoint* k1 = (const Keypoint*)a;
  const Keypoint* k2 = (const Keypoint*)b;
  if (k1->score < k2->score) {
    return 1;
  }
  if (k1->score > k2->score) {
    return -1;
  }
  return 0;
}

Keypoint* fast_detect(const PcbImage* img, int threshold, int* count_out) {
  if (!img || threshold <= 0 || !count_out) {
    return NULL;
  }

  int capacity = 1024;
  int count = 0;

  Keypoint* keypoints = (Keypoint*)malloc(sizeof(Keypoint) * capacity);
  if (!keypoints) {
    return NULL;
  }

  int height = img->height;
  int width = img->width;

  for (int y = BRIEF_HALF_PATCH; y < img->height - BRIEF_HALF_PATCH; ++y) {
    for (int x = BRIEF_HALF_PATCH; x < img->width - BRIEF_HALF_PATCH; ++x) {
      int center_idx = y * width + x;
      uint8_t p = img->data[center_idx];

      int candidate_indices[4] = {center_idx - 3 * width, center_idx + 3, center_idx + 3 * width,
                                  center_idx - 3};

      int lighter = 0;
      int darker = 0;

      for (int i = 0; i < 4; ++i) {
        PixelState state = classify_pixel(p, img->data[candidate_indices[i]], threshold);
        if (state == PIXEL_LIGHTER) {
          ++lighter;
        }
        if (state == PIXEL_DARKER) {
          ++darker;
        }
      }

      if (lighter < 3 && darker < 3) {
        continue;
      }

      PixelState target_state = (lighter >= 3) ? PIXEL_LIGHTER : PIXEL_DARKER;
      int consecutive = 0;
      bool is_found = false;

      for (int i = 0; i < 16 + FAST_ARC_LENGTH - 1; ++i) {
        int idx = i % 16;
        int p_idx = (y + CIRCLE_Y_OFFSETS[idx]) * width + (x + CIRCLE_X_OFFSETS[idx]);

        PixelState state = classify_pixel(p, img->data[p_idx], threshold);

        if (state == target_state) {
          ++consecutive;
          if (consecutive >= FAST_ARC_LENGTH) {
            is_found = true;
            break;
          }
        } else {
          consecutive = 0;
        }
      }

      if (is_found) {
        if (count >= capacity) {
          capacity *= 2;
          Keypoint* temp = (Keypoint*)realloc(keypoints, sizeof(Keypoint) * capacity);
          if (!temp) {
            free(keypoints);
            return NULL;
          }
          keypoints = temp;
        }

        keypoints[count].x = x;
        keypoints[count].y = y;

        keypoints[count].score = compute_fast_score(img, x, y, p, threshold, target_state);

        ++count;
      }
    }
  }

  *count_out = count;

  qsort(keypoints, count, sizeof(Keypoint), keypoints_comparator);

  return keypoints;
}

Keypoint* nms_suppress(const Keypoint* keypoints, int count, int radius, int* out_count) {
  if (!keypoints || count <= 0 || !out_count) {
    if (out_count) {
      *out_count = 0;
    }
    return NULL;
  }

  bool* suppressed = (bool*)calloc(count, 1);
  if (!suppressed) {
    *out_count = 0;
    return NULL;
  }

  int kept_count = 0;
  int radius_squared = radius * radius;

  for (int i = 0; i < count; ++i) {
    if (suppressed[i]) {
      continue;
    }

    ++kept_count;

    for (int j = i + 1; j < count; ++j) {
      int dx = keypoints[i].x - keypoints[j].x;
      int dy = keypoints[i].y - keypoints[j].y;
      int distance_squared = dx * dx + dy * dy;
      if (distance_squared <= radius_squared) {
        suppressed[j] = true;
      }
    }
  }

  Keypoint* result = (Keypoint*)malloc(sizeof(Keypoint) * kept_count);
  if (!result) {
    free(suppressed);
    *out_count = 0;
    return NULL;
  }

  int idx = 0;
  for (int i = 0; i < count; ++i) {
    if (suppressed[i] == false) {
      result[idx++] = keypoints[i];
    }
  }

  *out_count = kept_count;

  free(suppressed);

  return result;
}

Keypoint* top_k_select(const Keypoint* keypoints, int count, int k, int* out_count) {
  if (!keypoints || count <= 0 || k <= 0 || !out_count) {
    return NULL;
  }

  k = (k > count) ? count : k;

  Keypoint* result = (Keypoint*)malloc(sizeof(Keypoint) * k);
  if (!result) {
    *out_count = 0;
    return NULL;
  }

  for (int i = 0; i < k; ++i) {
    result[i] = keypoints[i];
  }

  *out_count = k;

  return result;
}

Descriptor* brief_compute_descriptors(const PcbImage* img, Keypoint* keypoints, int count) {
  if (!img || !keypoints || count <= 0) {
    return NULL;
  }

  int half_patch = BRIEF_HALF_PATCH;
  int height = img->height;
  int width = img->width;

  Descriptor* descriptors = calloc(count, sizeof(Descriptor));
  if (!descriptors) {
    return NULL;
  }

  for (int i = 0; i < count; ++i) {
    int cx = keypoints[i].x;
    int cy = keypoints[i].y;

    if (cx - half_patch < 0 || cx + half_patch >= width || cy - half_patch < 0 ||
        cy + half_patch >= height) {
      continue;
    }

    for (int p = 0; p < BRIEF_DESCRIPTOR_BYTES * 8; ++p) {
      int x1 = cx + BRIEF_PATTERN[p * 4 + 0];
      int y1 = cy + BRIEF_PATTERN[p * 4 + 1];
      int x2 = cx + BRIEF_PATTERN[p * 4 + 2];
      int y2 = cy + BRIEF_PATTERN[p * 4 + 3];

      uint8_t p1_val = img->data[y1 * width + x1];
      uint8_t p2_val = img->data[y2 * width + x2];

      if (p1_val < p2_val) {
        int byte_idx = p / 8;
        int bit_idx = p % 8;
        descriptors[i].bytes[byte_idx] |= (1 << bit_idx);
      }
    }
  }

  return descriptors;
}

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
