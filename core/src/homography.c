#include "homography.h"

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_permutation.h>
#include <math.h>

static void invert_matrix(const gsl_matrix* m, gsl_matrix* m_invert_out) {
  if (!m || !m_invert_out) {
    return;
  }

  int s;
  size_t n = m->size1;

  gsl_permutation* p = gsl_permutation_alloc(n);
  gsl_matrix* m_copy = gsl_matrix_alloc(n, n);
  gsl_matrix_memcpy(m_copy, m);

  gsl_linalg_LU_decomp(m_copy, p, &s);
  gsl_linalg_LU_invert(m_copy, p, m_invert_out);

  gsl_permutation_free(p);
  gsl_matrix_free(m_copy);
}

void normalize_points(const Keypoint* points, int count, gsl_matrix* normalized_pts_out,
                      Normalization* normalization_out) {
  if (!points || count <= 0 || !normalized_pts_out || !normalization_out) {
    return;
  }

  double x_sum = 0.0;
  double y_sum = 0.0;

  for (int i = 0; i < count; ++i) {
    x_sum += points[i].x;
    y_sum += points[i].y;
  }

  double x_mean = x_sum / count;
  double y_mean = y_sum / count;

  double d_sum = 0.0;
  for (int i = 0; i < count; ++i) {
    double dx = points[i].x - x_mean;
    double dy = points[i].y - y_mean;
    d_sum += sqrt(dx * dx + dy * dy);
  }

  double d_avg = d_sum / count;

  double factor = sqrt(2.0) / d_avg;

  gsl_matrix_set_zero(normalization_out->T);
  gsl_matrix_set(normalization_out->T, 0, 0, factor);
  gsl_matrix_set(normalization_out->T, 0, 2, -factor * x_mean);
  gsl_matrix_set(normalization_out->T, 1, 1, factor);
  gsl_matrix_set(normalization_out->T, 1, 2, -factor * y_mean);
  gsl_matrix_set(normalization_out->T, 2, 2, 1.0);

  invert_matrix(normalization_out->T, normalization_out->T_inverse);

  for (int i = 0; i < count; ++i) {
    double x_norm = factor * (points[i].x - x_mean);
    double y_norm = factor * (points[i].y - y_mean);

    gsl_matrix_set(normalized_pts_out, i, 0, x_norm);
    gsl_matrix_set(normalized_pts_out, i, 1, y_norm);
  }
}
