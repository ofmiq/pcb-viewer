#include "orb.h"

#include <stdbool.h>
#include <stdlib.h>

#include "image.h"

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

static inline double compute_fast_score(PcbImage* img, int x, int y, uint8_t p, int threshold,
                                        PixelState target_state) {
  if (img == NULL) {
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

Keypoint* fast_detect(PcbImage* img, int threshold, int* count_out) {
  if (img == NULL) {
    return NULL;
  }

  int capacity = 1024;
  int count = 0;

  Keypoint* keypoints = (Keypoint*)malloc(sizeof(Keypoint) * capacity);
  if (keypoints == NULL) {
    return NULL;
  }

  int height = img->height;
  int width = img->width;

  for (int y = 3; y < height - 3; ++y) {
    for (int x = 3; x < width - 3; ++x) {
      int center_idx = y * width + x;
      uint8_t p = img->data[center_idx];

      int upper_limit = p + threshold;
      int lower_limit = p - threshold;

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
      bool is_found = 0;

      for (int i = 0; i < 24; ++i) {
        int idx = i % 16;
        int p_idx = (y + CIRCLE_Y_OFFSETS[idx]) * width + (x + CIRCLE_X_OFFSETS[idx]);

        PixelState state = classify_pixel(p, img->data[p_idx], threshold);

        if (state == target_state) {
          ++consecutive;
          if (consecutive >= 9) {
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
  return keypoints;
}
