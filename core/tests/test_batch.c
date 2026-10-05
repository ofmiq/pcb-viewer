#include <stdio.h>
#include <time.h>

#include "image.h"

double get_elapsed_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main(void) {
  const char* filenames[] = {"high_iso_noisy.png", "reflection.png", "standart.png"};
  const int num_files = sizeof(filenames) / sizeof(filenames[0]);

  printf("[CORE] Starting image processing...\n\n");

  for (int i = 0; i < num_files; i++) {
    const char* filename = filenames[i];
    printf("Processing: %s\n", filename);

    char input_path[256];
    char gray_path[256];
    char scale_path[256];
    char blur_path[256];

    snprintf(input_path, sizeof(input_path), "resources/png/%s", filename);
    snprintf(gray_path, sizeof(gray_path), "resources/grayscaled/%s", filename);
    snprintf(scale_path, sizeof(scale_path), "resources/downscaled/%s", filename);
    snprintf(blur_path, sizeof(blur_path), "resources/blurred/%s", filename);

    struct timespec start_t, end_t;

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    PcbImage* img = image_load(input_path);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double load_t = get_elapsed_ms(start_t, end_t);

    if (!img) {
      printf("[CORE] Failed to load %s\n\n", input_path);
      continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    PcbImage* grayscaled_img = preprocess_grayscale(img);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double gray_t = get_elapsed_ms(start_t, end_t);

    if (!grayscaled_img) {
      printf("[CORE] Grayscaling failed for %s\n\n", filename);
      image_free(img);
      continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    image_save_png(grayscaled_img, gray_path);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double save_gray_t = get_elapsed_ms(start_t, end_t);

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    PcbImage* downscaled_img = preprocess_downscale(grayscaled_img, 1200);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double scale_t = get_elapsed_ms(start_t, end_t);

    if (!downscaled_img) {
      printf("[CORE] Blurring failed for %s\n\n", filename);
      image_free(img);
      image_free(grayscaled_img);
      continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    image_save_png(downscaled_img, scale_path);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double save_scale_t = get_elapsed_ms(start_t, end_t);

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    PcbImage* blurred_img = preprocess_gaussian_blur(downscaled_img);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double blur_t = get_elapsed_ms(start_t, end_t);

    if (!blurred_img) {
      printf("[CORE] Blurring failed for %s\n\n", filename);
      image_free(img);
      image_free(grayscaled_img);
      image_free(downscaled_img);
      continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_t);
    image_save_png(blurred_img, blur_path);
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double save_blur_t = get_elapsed_ms(start_t, end_t);

    printf("[Load]: %.3f ms\n", load_t);
    printf("[Gray]: %.3f ms (Save: %.3f ms)\n", gray_t, save_gray_t);
    printf("[Scale]: %.3f ms (Save: %.3f ms)\n", scale_t, save_scale_t);
    printf("[Blur]: %.3f ms (Save: %.3f ms)\n", blur_t, save_blur_t);
    printf("[Total Preproc]: %.3f ms\n\n", gray_t + blur_t);

    image_free(img);
    image_free(grayscaled_img);
    image_free(blurred_img);
  }

  printf("[CORE] Processing complete\n");
  return 0;
}
