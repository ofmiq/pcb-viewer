#include "image_pyramid.h"

#include <stdlib.h>

#include "preprocessing.h"

static PcbImage* prepare_level(const PcbImage* rgb, int target_width) {
  if (!rgb || target_width <= 0) {
    return NULL;
  }

  PcbImage* scaled = NULL;
  const PcbImage* src = rgb;

  if (target_width != rgb->width) {
    scaled = preprocess_downscale(rgb, target_width);
    if (!scaled) {
      return NULL;
    }
    src = scaled;
  }

  PcbImage* gray = preprocess_grayscale(src);
  if (scaled) {
    image_free(scaled);
  }
  if (!gray) {
    return NULL;
  }

  PcbImage* blurred = preprocess_gaussian_blur(gray);
  image_free(gray);
  return blurred;
}

ImagePyramid* build_image_pyramid(const PcbImage* src) {
  if (!src) {
    return NULL;
  }

  ImagePyramid* image_pyramid = (ImagePyramid*)calloc(1, sizeof(ImagePyramid));
  if (!image_pyramid) {
    return NULL;
  }

  double scale = 1.0;
  for (int i = 0; i < PYRAMID_MAX_LEVELS; ++i) {
    int target_width = (int)(src->width * scale + 0.5);
    if (target_width < 4 * BRIEF_HALF_PATCH) {
      break;
    }

    PcbImage* level = prepare_level(src, target_width);
    if (!level) {
      free_image_pyramid(image_pyramid);
      return NULL;
    }

    image_pyramid->images[i] = level;
    image_pyramid->scale_factors[i] = scale;
    ++image_pyramid->count;

    scale *= PYRAMID_SCALE_STEP;
  }

  if (image_pyramid->count == 0) {
    free(image_pyramid);
    return NULL;
  }
  return image_pyramid;
}

void free_image_pyramid(ImagePyramid* image_pyramid) {
  if (!image_pyramid) {
    return;
  }
  for (int i = 0; i < image_pyramid->count; ++i) {
    image_free(image_pyramid->images[i]);
  }
  free(image_pyramid);
}
