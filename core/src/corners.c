#include "corners.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "brief_patern.h"

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

static double compute_keypoint_orientation(const PcbImage* img, Keypoint* kp) {
  int radius = BRIEF_HALF_PATCH;
  int cx = (int)round(kp->x);
  int cy = (int)round(kp->y);

  double m01 = 0.0;
  double m10 = 0.0;

  if (cx - radius < 0 || cx + radius >= img->width || cy - radius < 0 ||
      cy + radius >= img->height) {
    return 0.0;
  }

  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy <= radius * radius) {
        int px = cx + dx;
        int py = cy + dy;
        uint8_t intensity = img->data[py * img->width + px];

        m10 += (double)dx * (double)intensity;
        m01 += (double)dy * (double)intensity;
      }
    }
  }

  return atan2(m01, m10);
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

  int height = img->height;
  int width = img->width;

  Descriptor* descriptors = calloc(count, sizeof(Descriptor));
  if (!descriptors) {
    return NULL;
  }

  for (int i = 0; i < count; ++i) {
    Keypoint* kp = &keypoints[i];

    double theta = compute_keypoint_orientation(img, kp);
    kp->angle = theta;

    double cos_t = cos(theta);
    double sin_t = sin(theta);

    int cx_round = round(kp->x);
    int cy_round = round(kp->y);

    for (int p = 0; p < BRIEF_DESCRIPTOR_BYTES * 8; ++p) {
      double orig_x1 = BRIEF_PATTERN[p * 4 + 0];
      double orig_y1 = BRIEF_PATTERN[p * 4 + 1];
      double orig_x2 = BRIEF_PATTERN[p * 4 + 2];
      double orig_y2 = BRIEF_PATTERN[p * 4 + 3];

      int x1 = round(orig_x1 * cos_t - orig_y1 * sin_t);
      int y1 = round(orig_x1 * sin_t + orig_y1 * cos_t);
      int x2 = round(orig_x2 * cos_t - orig_y2 * sin_t);
      int y2 = round(orig_x2 * sin_t + orig_y2 * cos_t);

      int px1 = cx_round + x1;
      int py1 = cy_round + y1;
      int px2 = cx_round + x2;
      int py2 = cy_round + y2;

      if (px1 >= 0 && px1 < width && py1 >= 0 && py1 < height && px2 >= 0 && px2 < width &&
          py2 >= 0 && py2 < height) {
        uint8_t p1_val = img->data[py1 * width + px1];
        uint8_t p2_val = img->data[py2 * width + px2];

        if (p1_val < p2_val) {
          int byte_idx = p / 8;
          int bit_idx = p % 8;
          descriptors[i].bytes[byte_idx] |= (1 << bit_idx);
        }
      }
    }
  }

  return descriptors;
}
