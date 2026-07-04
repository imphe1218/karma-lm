#ifndef KARMA_H
#define KARMA_H

#include "karma_config.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KarmaRng {
    uint64_t state;
} KarmaRng;

void karma_rng_seed(KarmaRng *rng, uint64_t seed);
uint32_t karma_rng_u32(KarmaRng *rng);
float karma_rng_f32(KarmaRng *rng);
float karma_rng_uniform(KarmaRng *rng, float lo, float hi);

uint64_t karma_fnv1a64(const uint8_t *data, size_t n);

typedef enum KarmaRiskFlags {
    KARMA_RISK_NONE      = 0,
    KARMA_RISK_NUMBER    = 1 << 0,
    KARMA_RISK_UNIT      = 1 << 1,
    KARMA_RISK_NEGATION  = 1 << 2,
    KARMA_RISK_CODE      = 1 << 3,
    KARMA_RISK_QUOTE     = 1 << 4,
    KARMA_RISK_DATE      = 1 << 5,
    KARMA_RISK_EQUATION  = 1 << 6,
    KARMA_RISK_NAME_HINT = 1 << 7
} KarmaRiskFlags;

typedef struct KarmaExactSpan {
    uint8_t text[KARMA_EXACT_SPAN_MAX];
    int length;
    uint32_t risk_flags;
    uint64_t audit_hash;
} KarmaExactSpan;

typedef struct KarmaMemory {
    float semantic[KARMA_SEMANTIC_SLOTS][KARMA_D_MODEL];
    int semantic_count;

    KarmaExactSpan exact[KARMA_EXACT_SPANS];
    int exact_count;
    int exact_write_pos;
} KarmaMemory;

typedef struct KarmaModel {
    float emb[KARMA_VOCAB_SIZE][KARMA_D_MODEL];

    float wxh[KARMA_D_MODEL][KARMA_D_MODEL];
    float whh[KARMA_D_MODEL][KARMA_D_MODEL];
    float why[KARMA_D_MODEL][KARMA_VOCAB_SIZE];

    float h[KARMA_D_MODEL];

    KarmaMemory memory;
} KarmaModel;

void karma_memory_init(KarmaMemory *mem);
uint32_t karma_risk_classify_span(const uint8_t *text, int len);
bool karma_memory_store_exact(KarmaMemory *mem, const uint8_t *text, int len, uint32_t risk_flags);
void karma_memory_store_semantic(KarmaMemory *mem, const float *vec);
const KarmaExactSpan *karma_memory_recall_exact(const KarmaMemory *mem, const uint8_t *query, int query_len);
void karma_memory_print(const KarmaMemory *mem);

void karma_model_init(KarmaModel *model, uint64_t seed);
void karma_model_reset_state(KarmaModel *model);
void karma_model_forward_byte(KarmaModel *model, uint8_t token, float logits[KARMA_VOCAB_SIZE]);
void karma_model_absorb_text(KarmaModel *model, const uint8_t *text, int len);
uint8_t karma_model_predict_next(KarmaModel *model, uint8_t token);

float karma_dot(const float *a, const float *b, int n);
void karma_vec_zero(float *x, int n);
void karma_vec_copy(float *dst, const float *src, int n);
void karma_vec_add_scaled(float *dst, const float *src, float scale, int n);
void karma_softmax(float *x, int n);

#ifdef __cplusplus
}
#endif

#endif
