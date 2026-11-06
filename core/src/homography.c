#include "homography.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <math.h>

static void normalization_init(Normalization* n) {
  n->T = gsl_matrix_alloc(3, 3);
  n->T_inverse = gsl_matrix_alloc(3, 3);
}

static void normalization_release(Normalization* n) {
  gsl_matrix_free(n->T);
  gsl_matrix_free(n->T_inverse);
}

int normalize_points(const Keypoint* points, int count, gsl_matrix* normalized_pts_out,
                     Normalization* normalization_out) {
  double x_mean = 0.0;
  double y_mean = 0.0;

  for (int i = 0; i < count; ++i) {
    x_mean += points[i].x;
    y_mean += points[i].y;
  }
  x_mean /= count;
  y_mean /= count;

  double d_avg = 0.0;
  for (int i = 0; i < count; ++i) {
    d_avg += hypot(points[i].x - x_mean, points[i].y - y_mean);
  }
  d_avg /= count;

  const double factor = sqrt(2.0) / d_avg;

  gsl_matrix_set_zero(normalization_out->T);
  gsl_matrix_set(normalization_out->T, 0, 0, factor);
  gsl_matrix_set(normalization_out->T, 0, 2, -factor * x_mean);
  gsl_matrix_set(normalization_out->T, 1, 1, factor);
  gsl_matrix_set(normalization_out->T, 1, 2, -factor * y_mean);
  gsl_matrix_set(normalization_out->T, 2, 2, 1.0);

  gsl_matrix_set_zero(normalization_out->T_inverse);
  gsl_matrix_set(normalization_out->T_inverse, 0, 0, 1.0 / factor);
  gsl_matrix_set(normalization_out->T_inverse, 0, 2, x_mean);
  gsl_matrix_set(normalization_out->T_inverse, 1, 1, 1.0 / factor);
  gsl_matrix_set(normalization_out->T_inverse, 1, 2, y_mean);
  gsl_matrix_set(normalization_out->T_inverse, 2, 2, 1.0);

  for (int i = 0; i < count; ++i) {
    gsl_matrix_set(normalized_pts_out, i, 0, factor * (points[i].x - x_mean));
    gsl_matrix_set(normalized_pts_out, i, 1, factor * (points[i].y - y_mean));
  }

  return 0;
}

int compute_dlt(const gsl_matrix* points1_norm, const gsl_matrix* points2_norm, gsl_matrix* H_out) {
  const size_t n = points1_norm->size1;
  const size_t cols = 9;
  const size_t rows = (2 * n > cols) ? 2 * n : cols;

  gsl_matrix* A = gsl_matrix_calloc(rows, cols);
  gsl_matrix* V = gsl_matrix_alloc(cols, cols);
  gsl_vector* S = gsl_vector_alloc(cols);
  gsl_vector* work = gsl_vector_alloc(cols);

  for (size_t i = 0; i < n; ++i) {
    const double x1 = gsl_matrix_get(points1_norm, i, 0);
    const double y1 = gsl_matrix_get(points1_norm, i, 1);
    const double x2 = gsl_matrix_get(points2_norm, i, 0);
    const double y2 = gsl_matrix_get(points2_norm, i, 1);

    const size_t r1 = 2 * i;
    const size_t r2 = 2 * i + 1;

    gsl_matrix_set(A, r1, 0, -x1);
    gsl_matrix_set(A, r1, 1, -y1);
    gsl_matrix_set(A, r1, 2, -1.0);
    gsl_matrix_set(A, r1, 6, x1 * x2);
    gsl_matrix_set(A, r1, 7, y1 * x2);
    gsl_matrix_set(A, r1, 8, x2);

    gsl_matrix_set(A, r2, 3, -x1);
    gsl_matrix_set(A, r2, 4, -y1);
    gsl_matrix_set(A, r2, 5, -1.0);
    gsl_matrix_set(A, r2, 6, x1 * y2);
    gsl_matrix_set(A, r2, 7, y1 * y2);
    gsl_matrix_set(A, r2, 8, y2);
  }

  gsl_linalg_SV_decomp(A, V, S, work);

  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      gsl_matrix_set(H_out, i, j, gsl_matrix_get(V, i * 3 + j, cols - 1));
    }
  }

  gsl_matrix_free(A);
  gsl_matrix_free(V);
  gsl_vector_free(S);
  gsl_vector_free(work);

  return 0;
}

int denormalize_homography(const gsl_matrix* H_norm, const Normalization* normalization1,
                           const Normalization* normalization2, gsl_matrix* H_out) {
  gsl_matrix* temp = gsl_matrix_alloc(3, 3);

  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, H_norm, normalization1->T, 0.0, temp);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, normalization2->T_inverse, temp, 0.0, H_out);

  gsl_matrix_free(temp);

  return 0;
}

int compute_homography(const Keypoint* src, const Keypoint* dst, int count, gsl_matrix* H_out) {
  gsl_matrix* src_norm = gsl_matrix_alloc(count, 2);
  gsl_matrix* dst_norm = gsl_matrix_alloc(count, 2);
  gsl_matrix* H_norm = gsl_matrix_alloc(3, 3);

  Normalization src_n;
  Normalization dst_n;
  normalization_init(&src_n);
  normalization_init(&dst_n);

  normalize_points(src, count, src_norm, &src_n);
  normalize_points(dst, count, dst_norm, &dst_n);

  compute_dlt(src_norm, dst_norm, H_norm);
  denormalize_homography(H_norm, &src_n, &dst_n, H_out);

  gsl_matrix_scale(H_out, 1.0 / gsl_matrix_get(H_out, 2, 2));

  normalization_release(&src_n);
  normalization_release(&dst_n);
  gsl_matrix_free(src_norm);
  gsl_matrix_free(dst_norm);
  gsl_matrix_free(H_norm);

  return 0;
}
