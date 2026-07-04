#include "karma.h"

void karma_rng_seed(KarmaRng *rng, uint64_t seed) {
    rng->state = seed ? seed : 0x9e3779b97f4a7c15ULL;
}

uint32_t karma_rng_u32(KarmaRng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return (uint32_t)((x * 2685821657736338717ULL) >> 32);
}

float karma_rng_f32(KarmaRng *rng) {
    return (karma_rng_u32(rng) >> 8) * (1.0f / 16777216.0f);
}

float karma_rng_uniform(KarmaRng *rng, float lo, float hi) {
    return lo + (hi - lo) * karma_rng_f32(rng);
}
