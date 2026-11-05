#ifndef HOMOGRAPHY_H
#define HOMOGRAPHY_H

#include <gsl/gsl_linalg.h>

#include "corners.h"

typedef struct {
  gsl_matrix* T;
  gsl_matrix* T_inverse;
} Normalization;

void normalize_points(const Keypoint* points, int count, gsl_matrix* normalized_pts_out,
                      Normalization* normalization_out);

#endif  // HOMOGRAPHY_H
