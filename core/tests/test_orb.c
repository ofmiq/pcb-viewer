#include <stdio.h>

#include "debug.h"
#include "image.h"
#include "orb.h"
#include "preprocessing.h"

int main(void) {
  const char* input_path = "resources/png/standart.png";
  const char* output_path = "resources/output.png";

  PcbImage* img = image_load(input_path);

  if (!img) {
    printf("[CORE] Failed to load %s\n", input_path);
    return 1;
  }

  PcbImage* scaled_img = preprocess_downscale(img, 1200);

  if (!scaled_img) {
    printf("[CORE] Downscaling failed\n");
    image_free(img);
    return 1;
  }

  PcbImage* grayscaled_img = preprocess_grayscale(scaled_img);

  if (!grayscaled_img) {
    printf("[CORE] Grayscaling failed\n");
    image_free(img);
    image_free(scaled_img);
    return 1;
  }

  PcbImage* blurred_img = preprocess_gaussian_blur(grayscaled_img);

  if (!blurred_img) {
    printf("[CORE] Blurring failed\n");
    image_free(img);
    image_free(scaled_img);
    image_free(grayscaled_img);
    return 1;
  }

  int size = 0;
  Keypoint* keypoints;
  double keypoints_time = TIME_EXEC(keypoints = fast_detect(blurred_img, 40, &size));

  printf("[CORE] Num of keypoints: %d\n", size);

  int suppresed_size = 0;
  Keypoint* suppresed_keypoints;
  double suppressed_keypoints_time =
      TIME_EXEC(suppresed_keypoints = nms_suppress(keypoints, size, 3, &suppresed_size));

  printf("[CORE] Num of suppresed keypoints: %d\n", suppresed_size);

  PcbImage* rgb_image = grayscale_to_rgb(blurred_img);

  double drawing_time = TIME_EXEC(draw_keypoints(rgb_image, suppresed_keypoints, size));

  image_save_png(rgb_image, output_path);

  printf("[Keypoints]: %.3f ms\n", keypoints_time);
  printf("[Suppresed keypoints]: %.3f ms\n", suppressed_keypoints_time);
  printf("[Drawing]: %.3f ms\n", drawing_time);
  return 0;
}
