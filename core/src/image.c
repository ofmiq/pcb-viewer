#include "image.h"

#include <stdlib.h>

#include "stb_image.h"
#include "stb_image_write.h"

PcbImage* image_load(const char* filename) {
  PcbImage* img = (PcbImage*)malloc(sizeof(PcbImage));
  if (!img) {
    return NULL;
  }

  img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
  if (!img->data) {
    free(img);
    return NULL;
  }

  img->stride = img->width * img->channels;

  return img;
}

void image_free(PcbImage* img) {
  if (!img) {
    return;
  }

  if (img->data) {
    stbi_image_free(img->data);
  }
  free(img);
}

int image_save_png(const PcbImage* img, const char* filename) {
  if (!img || !img->data) {
    return 0;
  }
  stbi_write_png_compression_level = 0;
  stbi_write_force_png_filter = 0;
  return stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->stride);
}

PcbImage* preprocess_grayscale(const PcbImage* src) {
  if (!src || !src->data || src->channels < 3) {
    return NULL;
  }

  int width = src->width;
  int height = src->height;
  int channels = src->channels;
  int total_pixels = width * height;

  PcbImage* img = (PcbImage*)malloc(sizeof(PcbImage));
  if (!img) {
    return NULL;
  }

  img->width = width;
  img->height = height;
  img->channels = 1;
  img->stride = width;

  img->data = (uint8_t*)malloc(total_pixels);
  if (!img->data) {
    free(img);
    return NULL;
  }

  for (int i = 0; i < total_pixels; ++i) {
    const uint8_t* p = src->data + i * channels;

    uint8_t r = p[0];
    uint8_t g = p[1];
    uint8_t b = p[2];

    double y = 0.299 * r + 0.587 * g + 0.114 * b;

    int value = (int)(y + 0.5f);
    if (value < 0) {
      value = 0;
    }
    if (value > 255) {
      value = 255;
    }

    img->data[i] = (uint8_t)value;
  }

  return img;
}

PcbImage* preprocess_downscale(const PcbImage* src, const int target_width) {
  if (!src || !src->data || src->channels != 1) {
    return NULL;
  }

  int src_width = src->width;
  int src_height = src->height;

  double scale = (double)target_width / src_width;

  int out_width = target_width;
  int out_height = src_height * scale + 0.5;
  if (out_height < 1) {
    out_height = 1;
  }

  int* x_start = (int*)malloc(out_width * sizeof(int));
  int* x_end = (int*)malloc(out_width * sizeof(int));
  int* y_start = (int*)malloc(out_height * sizeof(int));
  int* y_end = (int*)malloc(out_height * sizeof(int));

  if (!x_start || !x_end || !y_start || !y_end) {
    free(x_start);
    free(x_end);
    free(y_start);
    free(y_end);
    return NULL;
  }

  for (int ox = 0; ox < out_width; ++ox) {
    int start = ox / scale;
    int end = (ox + 1) / scale;

    if (end <= start) {
      end = start + 1;
    }
    if (end > src_width) {
      end = src_width;
    }
    if (start >= end) {
      start = end - 1;
    }

    x_start[ox] = start;
    x_end[ox] = end;
  }

  for (int oy = 0; oy < out_height; ++oy) {
    int start = oy / scale;
    int end = (oy + 1) / scale;

    if (end <= start) {
      end = start + 1;
    }
    if (end > src_height) {
      end = src_height;
    }
    if (start >= end) {
      start = end - 1;
    }

    y_start[oy] = start;
    y_end[oy] = end;
  }

  PcbImage* dst = (PcbImage*)malloc(sizeof(PcbImage));
  if (!dst) {
    free(x_start);
    free(x_end);
    free(y_start);
    free(y_end);
    return NULL;
  }

  dst->data = (uint8_t*)malloc(out_width * out_height);
  if (!dst->data) {
    free(dst);
    free(x_start);
    free(x_end);
    free(y_start);
    free(y_end);
    return NULL;
  }

  dst->width = out_width;
  dst->height = out_height;
  dst->channels = 1;
  dst->stride = out_width;

  for (int oy = 0; oy < out_height; ++oy) {
    int sy_start = y_start[oy];
    int sy_end = y_end[oy];
    uint8_t* dst_row = dst->data + oy * out_width;

    for (int ox = 0; ox < out_width; ++ox) {
      int sx_start = x_start[ox];
      int sx_end = x_end[ox];

      int sum = 0;
      int count = 0;

      for (int sy = sy_start; sy < sy_end; ++sy) {
        const uint8_t* src_row = src->data + sy * src_width;

        for (int sx = sx_start; sx < sx_end; ++sx) {
          sum += src_row[sx];
          count++;
        }
      }

      dst_row[ox] = sum / count;
    }
  }

  free(x_start);
  free(x_end);
  free(y_start);
  free(y_end);

  return dst;
}

PcbImage* preprocess_gaussian_blur(const PcbImage* src) {
  if (!src || !src->data || src->channels != 1) {
    return NULL;
  }

  int width = src->width;
  int height = src->height;
  int total_pixels = width * height;

  PcbImage* dst = (PcbImage*)malloc(sizeof(PcbImage));
  if (!dst) {
    return NULL;
  }

  dst->data = (uint8_t*)malloc(total_pixels);
  if (!dst->data) {
    free(dst);
    return NULL;
  }

  dst->width = width;
  dst->height = height;
  dst->channels = 1;
  dst->stride = width;

  int* tmp = (int*)malloc(total_pixels * sizeof(int));
  if (!tmp) {
    free(dst->data);
    free(dst);
    return NULL;
  }

  for (int y = 0; y < height; ++y) {
    const uint8_t* src_row = src->data + y * width;
    int* tmp_row = tmp + y * width;

    tmp_row[0] = src_row[0] + 2 * src_row[0] + src_row[1];

    for (int x = 1; x < width - 1; ++x) {
      tmp_row[x] = src_row[x - 1] + 2 * src_row[x] + src_row[x + 1];
    }

    tmp_row[width - 1] = src_row[width - 2] + 2 * src_row[width - 1] + src_row[width - 1];
  }

  uint8_t* dst_row = dst->data;
  const int* tmp_row0 = tmp;
  const int* tmp_row1 = tmp + width;
  for (int x = 0; x < width; ++x) {
    dst_row[x] = (tmp_row0[x] + 2 * tmp_row0[x] + tmp_row1[x] + 8) / 16;
  }

  for (int y = 1; y < height - 1; ++y) {
    const int* above_row = tmp + (y - 1) * width;
    const int* center_row = tmp + y * width;
    const int* below_row = tmp + (y + 1) * width;
    dst_row = dst->data + y * width;

    for (int x = 0; x < width; ++x) {
      int val = above_row[x] + 2 * center_row[x] + below_row[x] + 8;
      dst_row[x] = val / 16;
    }
  }

  dst_row = dst->data + (height - 1) * width;
  const int* tmp_prev_bot_row = tmp + (height - 2) * width;
  const int* tmp_bot_row = tmp + (height - 1) * width;
  for (int x = 0; x < width; ++x) {
    dst_row[x] = (tmp_prev_bot_row[x] + 2 * tmp_bot_row[x] + tmp_bot_row[x] + 8) / 16;
  }

  free(tmp);
  return dst;
}
