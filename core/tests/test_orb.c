#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "debug.h"
#include "image.h"
#include "image_utils.h"
#include "orb.h"

PcbImage* standart_preprocess_pipeline(const PcbImage* img) {
  if (!img) {
    return NULL;
  }

  PcbImage* scaled = preprocess_downscale(img, 1200);
  if (!scaled) {
    return NULL;
  }

  PcbImage* gray = preprocess_grayscale(scaled);
  image_free(scaled);

  if (!gray) {
    return NULL;
  }

  PcbImage* blurred = preprocess_gaussian_blur(gray);
  image_free(gray);

  return blurred;
}

static Keypoint* find_keypoints(const PcbImage* img, int* out_size) {
  *out_size = 0;

  int raw_size = 0;
  Keypoint* raw = fast_detect(img, THRESHOLD, &raw_size);

  if (!raw || raw_size <= 0) {
    free(raw);
    return NULL;
  }

  int nms_size = 0;
  Keypoint* nms = nms_suppress(raw, raw_size, RADIUS, &nms_size);
  free(raw);

  if (!nms) {
    return NULL;
  }

  Keypoint* top = top_k_select(nms, nms_size, K, out_size);
  free(nms);

  return top;
}

int main() {
  int rc = 1;

  PcbImage* raw_standart = NULL;
  PcbImage* raw_cropped = NULL;
  PcbImage* standart_img = NULL;
  PcbImage* crop_base = NULL;

  Keypoint* standart_keypoints = NULL;
  Descriptor* standart_descriptors = NULL;

  ImagePyramid* img_pyramid = NULL;
  FeaturePyramid* feature_pyramid = NULL;
  LevelResult* levels_results = NULL;

  raw_standart = image_load("resources/png/standart.png");
  raw_cropped = image_load("resources/png/crop.png");

  if (!raw_standart || !raw_cropped) {
    printf("Failed to load images\n");
    goto cleanup;
  }

  const int standart_width = 1200;
  const double s0 = (double)standart_width / raw_standart->width;

  const double crop_nominal = raw_cropped->width * s0;
  const double extra = 1.0 / 0.706;
  const int crop_width = (int)(crop_nominal * extra + 0.5);

  printf("== INPUT PARAMETERS ==\n");
  printf("Standard original: %dx%d\n", raw_standart->width, raw_standart->height);
  printf("Crop original:     %dx%d\n", raw_cropped->width, raw_cropped->height);
  printf("Standard scale:    %.6f\n", s0);
  printf("Crop nominal:      %.2f px\n", crop_nominal);
  printf("Crop extra:        %.6f\n", extra);
  printf("Crop target width: %d px\n\n", crop_width);

  struct timespec global_start_t, global_end_t;
  clock_gettime(CLOCK_MONOTONIC, &global_start_t);

  double standart_pipeline_time =
      TIME_EXEC(standart_img = standart_preprocess_pipeline(raw_standart));

  if (!standart_img) {
    printf("Failed to preprocess standard image\n");
    goto cleanup;
  }

  int standart_size = 0;

  double standart_keypoints_time =
      TIME_EXEC(standart_keypoints = find_keypoints(standart_img, &standart_size));

  if (!standart_keypoints) {
    printf("Failed to find standard keypoints\n");
    goto cleanup;
  }

  double standart_descriptors_time =
      TIME_EXEC(standart_descriptors =
                    brief_compute_descriptors(standart_img, standart_keypoints, standart_size));

  if (!standart_descriptors) {
    printf("Failed to compute standard descriptors\n");
    goto cleanup;
  }

  crop_base = preprocess_downscale(raw_cropped, crop_width);

  if (!crop_base) {
    printf("Failed to downscale crop\n");
    goto cleanup;
  }

  image_free(raw_cropped);
  raw_cropped = NULL;

  double build_pyramid_time = TIME_EXEC(img_pyramid = build_image_pyramid(crop_base));

  if (!img_pyramid) {
    printf("Failed to build image pyramid\n");
    goto cleanup;
  }

  double build_feature_pyramid_time =
      TIME_EXEC(feature_pyramid = build_feature_pyramid(img_pyramid));

  if (!feature_pyramid) {
    printf("Failed to build feature pyramid\n");
    goto cleanup;
  }

  double compute_levels_results_time =
      TIME_EXEC(levels_results = compute_levels_results(standart_keypoints, standart_descriptors,
                                                        standart_size, feature_pyramid));

  if (!levels_results) {
    printf("Failed to compute level results\n");
    goto cleanup;
  }

  const double off_x = 60.0 * s0;
  const double off_y = 40.0 * s0;
  const double tolerance = 3.0;

  printf("\n========== PYRAMID VALIDATION ==========\n");

  for (int lvl = 0; lvl < feature_pyramid->count; ++lvl) {
    const PcbImage* level_img = img_pyramid->images[lvl];

    const Keypoint* cropped_keypoints = feature_pyramid->levels[lvl].keypoints;

    const Match* matches = levels_results[lvl].matches;
    const int matches_size = levels_results[lvl].count;

    const double k = crop_nominal / level_img->width;

    int correct = 0;

    for (int m = 0; m < matches_size; ++m) {
      const Match* match = &matches[m];

      if (match->query_idx < 0 || match->query_idx >= standart_size || match->train_idx < 0 ||
          match->train_idx >= feature_pyramid->levels[lvl].count) {
        continue;
      }

      const Keypoint* q = &standart_keypoints[match->query_idx];

      const Keypoint* t = &cropped_keypoints[match->train_idx];

      const double expected_x = t->x * k + off_x;
      const double expected_y = t->y * k + off_y;

      if (fabs(q->x - expected_x) <= tolerance && fabs(q->y - expected_y) <= tolerance) {
        ++correct;
      }
    }

    const double accuracy = matches_size > 0 ? 100.0 * correct / matches_size : 0.0;

    printf("\n== LEVEL %d (scale %.3f, %dx%d) ==\n", lvl, img_pyramid->scale_factors[lvl],
           level_img->width, level_img->height);

    printf("Keypoints:  %d\n", feature_pyramid->levels[lvl].count);

    printf("Matched:    %d\n", matches_size);
    printf("Inliers:    %d\n", correct);
    printf("Outliers:   %d\n", matches_size - correct);
    printf("Accuracy:   %.2f%%\n", accuracy);
  }

  clock_gettime(CLOCK_MONOTONIC, &global_end_t);

  printf("\n========== STANDARD TIME ==========\n");
  printf("Pipeline:    %.3f ms\n", standart_pipeline_time);
  printf("Keypoints:   %.3f ms\n", standart_keypoints_time);
  printf("Descriptors: %.3f ms\n", standart_descriptors_time);

  printf("\n========== PYRAMID TIME ==========\n");
  printf("Pipeline: %.3f ms\n", build_pyramid_time);
  printf("Feature:  %.3f ms\n", build_feature_pyramid_time);
  printf("Results:  %.3f ms\n", compute_levels_results_time);

  printf("\n========== TOTAL TIME ==========\n");
  printf("%.3f ms\n", get_elapsed_ms(global_start_t, global_end_t));

  rc = 0;

cleanup:
  free_levels_results(levels_results, feature_pyramid ? feature_pyramid->count : 0);

  free_feature_pyramid(feature_pyramid);
  free_image_pyramid(img_pyramid);

  image_free(crop_base);
  image_free(standart_img);

  free(standart_keypoints);
  free(standart_descriptors);

  image_free(raw_cropped);
  image_free(raw_standart);

  return rc;
}
