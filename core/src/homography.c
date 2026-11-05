#include "homography.h"

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_vector_double.h>
#include <math.h>

static void invert_matrix(const gsl_matrix* m, gsl_matrix* m_invert_out) {
  if (!m || !m_invert_out) {
    return;
  }

  int s;
  int n = m->size1;

  gsl_permutation* p = gsl_permutation_alloc(n);
  gsl_matrix* m_copy = gsl_matrix_alloc(n, n);
  gsl_matrix_memcpy(m_copy, m);

  gsl_linalg_LU_decomp(m_copy, p, &s);
  gsl_linalg_LU_invert(m_copy, p, m_invert_out);

  gsl_permutation_free(p);
  gsl_matrix_free(m_copy);
}

int normalize_points(const Keypoint* points, int count, gsl_matrix* normalized_pts_out,
                     Normalization* normalization_out) {
  if (!points || count <= 0 || !normalized_pts_out || !normalization_out) {
    return 1;
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

  return 0;
}

int compute_dlt(const gsl_matrix* points1_norm, const gsl_matrix* points2_norm, gsl_matrix* H_out) {
  if (!points1_norm || !points2_norm || !H_out) {
    return 1;
  }

  const int A_rows = 8;
  const int A_cols = 9;

  gsl_matrix* A = gsl_matrix_alloc(A_rows, A_cols);
  gsl_matrix_set_zero(A);

  for (int i = 0; i < 4; ++i) {
    double x1 = gsl_matrix_get(points1_norm, i, 0);
    double y1 = gsl_matrix_get(points1_norm, i, 1);
    double x2 = gsl_matrix_get(points2_norm, i, 0);
    double y2 = gsl_matrix_get(points2_norm, i, 1);

    int r1 = 2 * i;
    int r2 = 2 * i + 1;

    gsl_matrix_set(A, r1, 0, -x1);
    gsl_matrix_set(A, r1, 1, -y1);
    gsl_matrix_set(A, r1, 2, -1.0);
    gsl_matrix_set(A, r1, 3, 0.0);
    gsl_matrix_set(A, r1, 4, 0.0);
    gsl_matrix_set(A, r1, 5, 0.0);
    gsl_matrix_set(A, r1, 6, x1 * x2);
    gsl_matrix_set(A, r1, 7, y1 * x2);
    gsl_matrix_set(A, r1, 8, x2);

    gsl_matrix_set(A, r2, 0, 0.0);
    gsl_matrix_set(A, r2, 1, 0.0);
    gsl_matrix_set(A, r2, 2, 0.0);
    gsl_matrix_set(A, r2, 3, -x1);
    gsl_matrix_set(A, r2, 4, -y1);
    gsl_matrix_set(A, r2, 5, -1.0);
    gsl_matrix_set(A, r2, 6, x1 * y2);
    gsl_matrix_set(A, r2, 7, y1 * y2);
    gsl_matrix_set(A, r2, 8, y2);
  }

  gsl_matrix* V = gsl_matrix_alloc(A_cols, A_cols);
  gsl_vector* S = gsl_vector_alloc(A_cols);
  gsl_vector* work = gsl_vector_alloc(A_cols);

  gsl_linalg_SV_decomp(A, V, S, work);

  gsl_vector* h = gsl_vector_alloc(A_cols);

  gsl_matrix_get_col(h, V, A_cols - 1);

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      gsl_matrix_set(H_out, i, j, gsl_vector_get(h, i * 3 + j));
    }
  }

  gsl_matrix_free(A);
  gsl_matrix_free(V);
  gsl_vector_free(S);
  gsl_vector_free(work);
  gsl_vector_free(h);

  return 0;
}

int denormalize_homography(const gsl_matrix* H_norm, Normalization* normalization1,
                           Normalization* normalization2, gsl_matrix* H_out) {
  if (!H_norm || !normalization1 || !normalization2 || !H_out) {
    return 1;
  }

  gsl_matrix* temp = gsl_matrix_alloc(3, 3);

  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, H_norm, normalization1->T, 0.0, temp);

  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, normalization2->T_inverse, temp, 0.0, H_out);

  gsl_matrix_free(temp);

  return 0;
}
