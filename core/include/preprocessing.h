#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image.h"
#include "orb.h"

PcbImage* preprocess_grayscale(const PcbImage* src);
PcbImage* preprocess_downscale(const PcbImage* src, const int target_width);
PcbImage* preprocess_gaussian_blur(const PcbImage* src);
PcbImage* grayscale_to_rgb(const PcbImage* src);
void draw_keypoints(PcbImage* src, const Keypoint* keypoints, int count);

#endif  // PREPROCESSING_H
