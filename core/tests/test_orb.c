#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "image.h"
#include "image_utils.h"
#include "orb.h"

#define THRESHOLD 30
#define RADIUS 3
#define K 2000

PcbImage* preprocess_pipeline(const char* filepath) {
  PcbImage* img = image_load(filepath);
  if (!img) return NULL;

  // PcbImage* scaled = preprocess_downscale(img, 1200);
  // image_free(img);
  // if (!scaled) return NULL;

  PcbImage* gray = preprocess_grayscale(img);
  image_free(img);
  if (!gray) return NULL;

  PcbImage* blurred = preprocess_gaussian_blur(gray);
  image_free(gray);

  return blurred;
}

Keypoint* find_keypoints(const PcbImage* img, int* out_size) {
  int raw_size = 0;
  Keypoint* keypoints = fast_detect(img, THRESHOLD, &raw_size);
  if (!keypoints) {
    *out_size = 0;
    return NULL;
  }

  int nms_size = 0;
  Keypoint* nms_keypoints = nms_suppress(keypoints, raw_size, RADIUS, &nms_size);
  free(keypoints);

  Keypoint* top_k_keypoints = top_k_select(nms_keypoints, nms_size, K, out_size);
  free(nms_keypoints);

  return top_k_keypoints;
}

int main() {
  const char* standart_path = "resources/png/standart.png";
  const char* cropped_path = "resources/png/crop.png";

  struct timespec global_start_t, global_end_t;
  clock_gettime(CLOCK_MONOTONIC, &global_start_t);

  PcbImage* standart_img;
  double standart_pipeline_time = TIME_EXEC(standart_img = preprocess_pipeline(standart_path));

  PcbImage* cropped_img;
  double cropped_pipeline_time = TIME_EXEC(cropped_img = preprocess_pipeline(cropped_path));

  if (!standart_img || !cropped_img) {
    printf("[CORE] Failed to load or preprocess images\n");
    if (standart_img) {
      image_free(standart_img);
    }
    if (cropped_img) {
      image_free(cropped_img);
    }
    return 1;
  }

  int standart_size = 0;
  int cropped_size = 0;

  Keypoint* standart_keypoints;
  double standart_keypoints_time =
      TIME_EXEC(standart_keypoints = find_keypoints(standart_img, &standart_size));
  Keypoint* cropped_keypoints;
  double cropped_keypoints_time =
      TIME_EXEC(cropped_keypoints = find_keypoints(cropped_img, &cropped_size));

  printf("[CORE] Standart keypoints: %d | Cropped keypoints: %d\n", standart_size, cropped_size);

  Descriptor* standart_descriptors;
  double standart_descriptors_time =
      TIME_EXEC(standart_descriptors =
                    brief_compute_descriptors(standart_img, standart_keypoints, standart_size));
  Descriptor* cropped_descriptors;
  double cropped_descriptors_time = TIME_EXEC(cropped_descriptors = brief_compute_descriptors(
                                                  cropped_img, cropped_keypoints, cropped_size));

  image_free(standart_img);
  image_free(cropped_img);

  int matches_size = 0;
  Match* matches;
  double match_time =
      TIME_EXEC(matches = brief_match_descriptors(
                    standart_keypoints, standart_descriptors, standart_size, cropped_keypoints,
                    cropped_descriptors, cropped_size, &matches_size));

  int correct_matches = 0;
  int offset_x = 60;
  int offset_y = 40;
  int tolerance = 1;

  for (int i = 0; i < matches_size; ++i) {
    int q_idx = matches[i].query_idx;
    int t_idx = matches[i].train_idx;

    int x_global = standart_keypoints[q_idx].x;
    int y_global = standart_keypoints[q_idx].y;

    int x_crop = cropped_keypoints[t_idx].x;
    int y_crop = cropped_keypoints[t_idx].y;

    int expected_x = x_crop + offset_x;
    int expected_y = y_crop + offset_y;

    if (abs(x_global - expected_x) <= tolerance && abs(y_global - expected_y) <= tolerance) {
      correct_matches++;
    } else {
      printf("Outlier! Global(%d,%d) vs CropMapped(%d,%d)\n", x_global, y_global, expected_x,
             expected_y);
    }
  }

  double accuracy = (matches_size > 0) ? ((double)correct_matches / matches_size) * 100.0 : 0.0;

  clock_gettime(CLOCK_MONOTONIC, &global_end_t);

  printf("== VALIDATION ==\n");
  printf("Total pairs matched: %d\n", matches_size);
  printf("Inliers:             %d\n", correct_matches);
  printf("Outliners:           %d\n", matches_size - correct_matches);
  printf("Accuracy:            %.3f%%\n", accuracy);

  printf("== TIME ==\n");
  printf("Pipeline:    Standart: %.3f ms; Cropped: %.3f ms\n", standart_pipeline_time,
         cropped_pipeline_time);
  printf("Keypoints:   Standart: %.3f ms; Cropped: %.3f ms\n", standart_keypoints_time,
         cropped_keypoints_time);
  printf("Descriptors: Standart: %.3f ms; Cropped: %.3f ms\n", standart_descriptors_time,
         cropped_descriptors_time);
  printf("Matching:    %.3f ms\n", match_time);
  printf("TOTAL TIME:  %.3f ms\n", get_elapsed_ms(global_start_t, global_end_t));

  free(matches);
  free(standart_keypoints);
  free(cropped_keypoints);
  free(standart_descriptors);
  free(cropped_descriptors);

  return 0;
}
