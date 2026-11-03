#ifndef IMAGE_PYRAMID
#define IMAGE_PYRAMID

#include "config.h"
#include "image.h"

typedef struct {
  PcbImage* images[PYRAMID_MAX_LEVELS];
  double scale_factors[PYRAMID_MAX_LEVELS];
  int count;
} ImagePyramid;

ImagePyramid* build_image_pyramid(const PcbImage* src);
void free_image_pyramid(ImagePyramid* image_pyramidp);

#endif  // IMAGE_PYRAMID
