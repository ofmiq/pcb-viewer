#include <stdio.h>
#include <time.h>

#include "image.h"

int main(void) {
  struct timespec load_start;
  struct timespec load_end;
  struct timespec preproc_start;
  struct timespec preproc_end;
  struct timespec save_start;
  struct timespec save_end;

  printf("[CORE] Image loading test\n");

  clock_gettime(CLOCK_MONOTONIC, &load_start);
  PcbImage* img = image_load("resources/test.png");
  clock_gettime(CLOCK_MONOTONIC, &load_end);

  if (!img) {
    printf("[CORE] Image loading failed\n");
    return 1;
  }

  printf("[CORE] Image loaded successfully\n");
  printf("[CORE] Width: %d\n Height: %d\n Channels: %d\n Stride: %d\n", img->width, img->height,
         img->channels, img->stride);

  clock_gettime(CLOCK_MONOTONIC, &preproc_start);
  PcbImage* grayscaled_image = preprocess_grayscale(img);
  if (!grayscaled_image) {
    printf("[CORE] Image grayscaling failed\n");
    return 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &preproc_end);

  clock_gettime(CLOCK_MONOTONIC, &save_start);
  int flag = image_save_png(grayscaled_image, "resources/test_out.png");
  clock_gettime(CLOCK_MONOTONIC, &save_end);

  if (flag == 1) {
    printf("[CORE] Image saved successfully\n");
  } else {
    printf("[CORE] Image saving failed\n");
  }

  image_free(img);
  image_free(grayscaled_image);

  double load_time = (load_end.tv_sec - load_start.tv_sec) * 1000.0 +
                     (load_end.tv_nsec - load_start.tv_nsec) / 1e6;
  double preproc_time = (preproc_end.tv_sec - preproc_start.tv_sec) * 1000.0 +
                        (preproc_end.tv_nsec - preproc_start.tv_nsec) / 1e6;
  double save_time = (save_end.tv_sec - save_start.tv_sec) * 1000.0 +
                     (save_end.tv_nsec - save_start.tv_nsec) / 1e6;

  printf("[CORE] load: %.3f ms, preproc: %.3f ms, save: %.3f ms\n", load_time, preproc_time,
         save_time);

  return 0;
}
