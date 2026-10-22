#include <stdio.h>

#include "debug.h"
#include "image.h"
#include "image_utils.h"

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

    // 1. Loading
    PcbImage* img = NULL;
    double load_t = TIME_EXEC(img = image_load(input_path));

    if (!img) {
      printf("[CORE] Failed to load %s\n\n", input_path);
      continue;
    }

    // 2. Downscaling
    PcbImage* downscaled_img = NULL;
    double scale_t = TIME_EXEC(downscaled_img = preprocess_downscale(img, 1200));

    if (!downscaled_img) {
      printf("[CORE] Downscaling failed for %s\n\n", filename);
      image_free(img);
      continue;
    }

    // 3. Saving downscaled image
    double save_scale_t = TIME_EXEC(image_save_png(downscaled_img, scale_path));

    // 4. Grayscaling
    PcbImage* grayscaled_img = NULL;
    double gray_t = TIME_EXEC(grayscaled_img = preprocess_grayscale(downscaled_img));

    if (!grayscaled_img) {
      printf("[CORE] Grayscaling failed for %s\n\n", filename);
      image_free(img);
      image_free(downscaled_img);
      continue;
    }

    // 5. Saving grayscaled image
    double save_gray_t = TIME_EXEC(image_save_png(grayscaled_img, gray_path));

    // 6. Gaussian blurring
    PcbImage* blurred_img = NULL;
    double blur_t = TIME_EXEC(blurred_img = preprocess_gaussian_blur(grayscaled_img));

    if (!blurred_img) {
      printf("[CORE] Blurring failed for %s\n\n", filename);
      image_free(img);
      image_free(downscaled_img);
      image_free(grayscaled_img);
      continue;
    }

    // 7. Saving blurred image
    double save_blur_t = TIME_EXEC(image_save_png(blurred_img, blur_path));

    // Stats
    printf("[Load]: %.3f ms\n", load_t);
    printf("[Scale]: %.3f ms (Save: %.3f ms)\n", scale_t, save_scale_t);
    printf("[Gray]: %.3f ms (Save: %.3f ms)\n", gray_t, save_gray_t);
    printf("[Blur]: %.3f ms (Save: %.3f ms)\n", blur_t, save_blur_t);
    printf("[Total Preproc]: %.3f ms\n\n", scale_t + gray_t + blur_t);

    image_free(img);
    image_free(downscaled_img);
    image_free(grayscaled_img);
    image_free(blurred_img);
  }

  printf("[CORE] Processing complete\n");
  return 0;
}
