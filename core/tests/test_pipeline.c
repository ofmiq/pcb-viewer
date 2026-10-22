#include <stdio.h>

#include "debug.h"
#include "image.h"
#include "image_utils.h"

int main(void) {
  const char* input_path = "resources/png/standart.png";
  const char* output_path = "resources/pipeline_out.png";

  printf("[CORE] Full pipeline test: %s\n\n", input_path);

  struct timespec pipeline_start, pipeline_end;
  clock_gettime(CLOCK_MONOTONIC, &pipeline_start);

  // 1. Loading
  PcbImage* img = NULL;
  double load_t = TIME_EXEC(img = image_load(input_path));

  if (!img) {
    printf("[CORE] Failed to load %s\n", input_path);
    return 1;
  }

  // 2. Downscaling
  PcbImage* scaled_img = NULL;
  double scaled_t = TIME_EXEC(scaled_img = preprocess_downscale(img, 1200));

  if (!scaled_img) {
    printf("[CORE] Downscaling failed\n");
    image_free(img);
    return 1;
  }

  // 3. Grayscaling
  PcbImage* grayscaled_img = NULL;
  double gray_t = TIME_EXEC(grayscaled_img = preprocess_grayscale(scaled_img));

  if (!grayscaled_img) {
    printf("[CORE] Grayscaling failed\n");
    image_free(img);
    image_free(scaled_img);
    return 1;
  }

  // 4. Gaussian blurring
  PcbImage* blurred_img = NULL;
  double blur_t = TIME_EXEC(blurred_img = preprocess_gaussian_blur(grayscaled_img));

  if (!blurred_img) {
    printf("[CORE] Blurring failed\n");
    image_free(img);
    image_free(scaled_img);
    image_free(grayscaled_img);
    return 1;
  }

  // 5. Saving
  int saved = 0;
  double save_t = TIME_EXEC(saved = image_save_png(blurred_img, output_path));

  clock_gettime(CLOCK_MONOTONIC, &pipeline_end);
  double total_t = get_elapsed_ms(pipeline_start, pipeline_end);

  // Stats
  printf("[Load]:  %.3f ms\n", load_t);
  printf("[Scale]: %.3f ms\n", scaled_t);
  printf("[Gray]:  %.3f ms\n", gray_t);
  printf("[Blur]:  %.3f ms\n", blur_t);
  printf("[Save]:  %.3f ms\n", save_t);
  printf("[TOTAL PIPELINE]: %.3f ms\n", total_t);

  if (!saved) {
    printf("[CORE] Warning: save failed\n");
  }

  image_free(img);
  image_free(scaled_img);
  image_free(grayscaled_img);
  image_free(blurred_img);

  return 0;
}
