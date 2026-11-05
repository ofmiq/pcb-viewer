#ifndef HOMOGRAPHY_H
#define HOMOGRAPHY_H

#include <gsl/gsl_linalg.h>

#include "corners.h"

typedef struct {
  gsl_matrix* T;
  gsl_matrix* T_inverse;
} Normalization;

int normalize_points(const Keypoint* points, int count, gsl_matrix* normalized_pts_out,
                     Normalization* normalization_out);

int denormalize_homography(const gsl_matrix* H_norm, Normalization* normalization1,
                           Normalization* normalization2, gsl_matrix* H_out);

int compute_dlt(const gsl_matrix* points1_norm, const gsl_matrix* points2_norm, gsl_matrix* H_out);

#endif  // HOMOGRAPHY_H
