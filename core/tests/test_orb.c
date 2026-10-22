#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "image.h"
#include "image_utils.h"
#include "orb.h"

#define THRESHOLD 30
#define RADIUS 3
#define K 2000

int main(void) {
  const char* input_path = "resources/png/standart.png";
  const char* output_path = "resources/output.png";

  PcbImage* img = image_load(input_path);
  if (!img) {
    printf("[CORE] Failed to load %s\n", input_path);
    return 1;
  }

  PcbImage* scaled_img = preprocess_downscale(img, 1200);
  image_free(img);

  if (!scaled_img) {
    printf("[CORE] Downscaling failed\n");
    return 1;
  }

  PcbImage* grayscaled_img = preprocess_grayscale(scaled_img);
  image_free(scaled_img);

  if (!grayscaled_img) {
    printf("[CORE] Grayscaling failed\n");
    return 1;
  }

  PcbImage* blurred_img = preprocess_gaussian_blur(grayscaled_img);
  image_free(grayscaled_img);

  if (!blurred_img) {
    printf("[CORE] Blurring failed\n");
    return 1;
  }

  int size = 0;
  Keypoint* keypoints;
  double keypoints_time = TIME_EXEC(keypoints = fast_detect(blurred_img, THRESHOLD, &size));

  printf("[CORE] Num of keypoints: %d\n", size);

  int suppresed_size = 0;
  Keypoint* suppresed_keypoints;
  double suppressed_keypoints_time =
      TIME_EXEC(suppresed_keypoints = nms_suppress(keypoints, size, RADIUS, &suppresed_size));

  free(keypoints);

  printf("[CORE] Num of suppresed keypoints: %d\n", suppresed_size);

  int top_k_size = 0;
  Keypoint* top_k_keypoints;
  double top_k_keypoints_time = TIME_EXEC(
      top_k_keypoints = top_k_select(suppresed_keypoints, suppresed_size, K, &top_k_size));

  free(suppresed_keypoints);

  PcbImage* rgb_image = grayscale_to_rgb(blurred_img);
  image_free(blurred_img);

  int drawn = suppresed_size < K ? suppresed_size : K;
  double drawing_time = TIME_EXEC(draw_keypoints(rgb_image, top_k_keypoints, drawn));

  image_save_png(rgb_image, output_path);

  printf("[Keypoints]: %.3f ms\n", keypoints_time);
  printf("[Suppresed keypoints]: %.3f ms\n", suppressed_keypoints_time);
  printf("[Top-K]: %.3f ms\n", top_k_keypoints_time);
  printf("[Drawing]: %.3f ms\n", drawing_time);

  image_free(rgb_image);
  free(top_k_keypoints);

  return 0;
}
