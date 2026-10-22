#include <stdio.h>

#include "image.h"
#include "orb.h"
#include "preprocessing.h"

int main(void) {
  const char* input_path = "resources/png/standart.png";

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
  // for (int i = 0; i < size; ++i) {
  //   printf("point: %d, %d, %lf\n", keypoints[i].x, keypoints[i].y, keypoints[i].score);
  // }
  return 0;
}
