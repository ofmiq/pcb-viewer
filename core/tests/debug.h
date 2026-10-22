#ifndef DEBUG
#define DEBUG

#include <time.h>

#define TIME_EXEC(expression)                 \
  ({                                          \
    struct timespec start_t, end_t;           \
    clock_gettime(CLOCK_MONOTONIC, &start_t); \
    expression;                               \
    clock_gettime(CLOCK_MONOTONIC, &end_t);   \
    get_elapsed_ms(start_t, end_t);           \
  })

double get_elapsed_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
}

#endif  // DEBUG
