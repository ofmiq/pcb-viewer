#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image.h"

PcbImage* preprocess_grayscale(const PcbImage* src);
PcbImage* preprocess_downscale(const PcbImage* src, const int targe_width);
PcbImage* preprocess_gaussian_blur(const PcbImage* src);

#endif  // PREPROCESSING_H
