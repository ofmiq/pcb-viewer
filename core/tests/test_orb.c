#include <stdio.h>

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
  Keypoint* keypoints = fast_detect(blurred_img, 20, &size);

  printf("Keypoints found: %d", size);

  PcbImage* rgb_image = grayscale_to_rgb(blurred_img);

  draw_keypoints(rgb_image, keypoints, size);

  image_save_png(rgb_image, output_path);

  return 0;
}
