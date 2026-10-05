#include <stdio.h>
#include <time.h>

#include "image.h"

static double get_elapsed_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main(void) {
  const char* input_path = "resources/png/standart.png";
  const char* output_path = "resources/pipeline_out.png";

  printf("[CORE] Full pipeline test: %s\n\n", input_path);

  struct timespec pipeline_start, pipeline_end;
  struct timespec stage_start, stage_end;

  clock_gettime(CLOCK_MONOTONIC, &pipeline_start);

  stage_start = pipeline_start;
  PcbImage* img = image_load(input_path);
  clock_gettime(CLOCK_MONOTONIC, &stage_end);
  double load_t = get_elapsed_ms(stage_start, stage_end);

  if (!img) {
    printf("[CORE] Failed to load %s\n", input_path);
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &stage_start);
  PcbImage* grayscaled_img = preprocess_grayscale(img);
  clock_gettime(CLOCK_MONOTONIC, &stage_end);
  double gray_t = get_elapsed_ms(stage_start, stage_end);

  if (!grayscaled_img) {
    printf("[CORE] Grayscaling failed\n");
    image_free(img);
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &stage_start);
  PcbImage* scaled_img = preprocess_downscale(grayscaled_img, 1200);
  clock_gettime(CLOCK_MONOTONIC, &stage_end);
  double scaled_t = get_elapsed_ms(stage_start, stage_end);

  if (!scaled_img) {
    printf("[CORE] Downscaling failed\n");
    image_free(img);
    image_free(grayscaled_img);
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &stage_start);
  PcbImage* blurred_img = preprocess_gaussian_blur(scaled_img);
  clock_gettime(CLOCK_MONOTONIC, &stage_end);
  double blur_t = get_elapsed_ms(stage_start, stage_end);

  if (!blurred_img) {
    printf("[CORE] Blurring failed\n");
    image_free(img);
    image_free(grayscaled_img);
    image_free(scaled_img);
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &stage_start);
  int saved = image_save_png(blurred_img, output_path);
  clock_gettime(CLOCK_MONOTONIC, &stage_end);
  double save_t = get_elapsed_ms(stage_start, stage_end);

  clock_gettime(CLOCK_MONOTONIC, &pipeline_end);
  double total_t = get_elapsed_ms(pipeline_start, pipeline_end);

  printf("[Load]:  %.3f ms\n", load_t);
  printf("[Gray]:  %.3f ms\n", gray_t);
  printf("[Scale]: %.3f ms\n", scaled_t);
  printf("[Blur]:  %.3f ms\n", blur_t);
  printf("[Save]:  %.3f ms\n", save_t);
  printf("[TOTAL PIPELINE]: %.3f ms\n", total_t);

  if (!saved) {
    printf("[CORE] Warning: save failed\n");
  }

  image_free(img);
  image_free(grayscaled_img);
  image_free(scaled_img);
  image_free(blurred_img);

  return 0;
}
