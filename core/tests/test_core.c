#include <stdio.h>
#include <time.h>

#include "../include/image.h"

int main() {
  struct timespec t1, t2, t3, t4;

  printf("[CORE] Image loading test\n");

  clock_gettime(CLOCK_MONOTONIC, &t1);
  PcbImage* img = image_load("resources/test.png");
  clock_gettime(CLOCK_MONOTONIC, &t2);

  if (!img) {
    printf("[CORE] Image loading failed\n");
    return 1;
  }

  printf("[CORE] Image loaded successfully\n");
  printf("[CORE] Width: %d\n Height: %d\n Channels: %d\n Stride: %d\n", img->width, img->height,
         img->channels, img->stride);

  clock_gettime(CLOCK_MONOTONIC, &t3);
  int flag = image_save_png(img, "resources/test_out.png");
  clock_gettime(CLOCK_MONOTONIC, &t4);

  if (flag == 0) {
    printf("[CORE] Image saved successfully\n");
  } else {
    printf("[CORE] Image saving failed\n");
  }

  image_free(img);

  double load_time = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_nsec - t1.tv_nsec) / 1e6;
  double save_time = (t4.tv_sec - t3.tv_sec) * 1000.0 + (t4.tv_nsec - t3.tv_nsec) / 1e6;

  printf("[CORE] load: %.3f ms, save: %.3f ms\n", load_time, save_time);

  return 0;
}
