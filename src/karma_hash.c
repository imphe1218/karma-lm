#include "karma.h"

uint64_t karma_fnv1a64(const uint8_t *data, size_t n) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint64_t)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}
