#include "karma.h"
#include <math.h>
#include <string.h>

float karma_dot(const float *a, const float *b, int n) {
    float s = 0.0f;
    for (int i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}

void karma_vec_zero(float *x, int n) {
    for (int i = 0; i < n; ++i) x[i] = 0.0f;
}

void karma_vec_copy(float *dst, const float *src, int n) {
    memcpy(dst, src, (size_t)n * sizeof(float));
}

void karma_vec_add_scaled(float *dst, const float *src, float scale, int n) {
    for (int i = 0; i < n; ++i) dst[i] += scale * src[i];
}

void karma_softmax(float *x, int n) {
    float maxv = x[0];
    for (int i = 1; i < n; ++i) if (x[i] > maxv) maxv = x[i];

    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        x[i] = expf(x[i] - maxv);
        sum += x[i];
    }

    if (sum > 0.0f) {
        float inv = 1.0f / sum;
        for (int i = 0; i < n; ++i) x[i] *= inv;
    }
}
