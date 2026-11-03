#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include "corners.h"
#include "image.h"

PcbImage* grayscale_to_rgb(const PcbImage* src);
void draw_keypoints(PcbImage* src, const Keypoint* keypoints, int count);

#endif  // IMAGE_UTILS_H
