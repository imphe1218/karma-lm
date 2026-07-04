#include "karma.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static float karma_tanh_fast(float x) {
    return tanhf(x);
}

void karma_model_init(KarmaModel *model, uint64_t seed) {
    KarmaRng rng;
    karma_rng_seed(&rng, seed);
    memset(model, 0, sizeof(*model));

    const float scale = 0.05f;

    for (int t = 0; t < KARMA_VOCAB_SIZE; ++t) {
        for (int i = 0; i < KARMA_D_MODEL; ++i) {
            model->emb[t][i] = karma_rng_uniform(&rng, -scale, scale);
        }
    }

    for (int i = 0; i < KARMA_D_MODEL; ++i) {
        for (int j = 0; j < KARMA_D_MODEL; ++j) {
            model->wxh[i][j] = karma_rng_uniform(&rng, -scale, scale);
            model->whh[i][j] = karma_rng_uniform(&rng, -scale, scale);
        }
    }

    for (int i = 0; i < KARMA_D_MODEL; ++i) {
        for (int t = 0; t < KARMA_VOCAB_SIZE; ++t) {
            model->why[i][t] = karma_rng_uniform(&rng, -scale, scale);
        }
    }

    karma_memory_init(&model->memory);
    karma_model_reset_state(model);
}

void karma_model_reset_state(KarmaModel *model) {
    karma_vec_zero(model->h, KARMA_D_MODEL);
}

void karma_model_forward_byte(KarmaModel *model, uint8_t token, float logits[KARMA_VOCAB_SIZE]) {
    float new_h[KARMA_D_MODEL];

    for (int i = 0; i < KARMA_D_MODEL; ++i) {
        float s = 0.0f;

        for (int j = 0; j < KARMA_D_MODEL; ++j) {
            s += model->wxh[j][i] * model->emb[token][j];
            s += model->whh[j][i] * model->h[j];
        }

        /*
            Very simple semantic memory read:
            mix a small amount of the most recent semantic slot into state.
            Later this becomes K_retrieve(q, m_i).
        */
        if (model->memory.semantic_count > 0) {
            int slot = (model->memory.semantic_count - 1) % KARMA_SEMANTIC_SLOTS;
            s += 0.05f * model->memory.semantic[slot][i];
        }

        new_h[i] = karma_tanh_fast(s);
    }

    karma_vec_copy(model->h, new_h, KARMA_D_MODEL);

    for (int t = 0; t < KARMA_VOCAB_SIZE; ++t) {
        float s = 0.0f;
        for (int i = 0; i < KARMA_D_MODEL; ++i) {
            s += model->h[i] * model->why[i][t];
        }
        logits[t] = s;
    }
}

uint8_t karma_model_predict_next(KarmaModel *model, uint8_t token) {
    float logits[KARMA_VOCAB_SIZE];
    karma_model_forward_byte(model, token, logits);

    int best = 0;
    float bestv = logits[0];
    for (int i = 1; i < KARMA_VOCAB_SIZE; ++i) {
        if (logits[i] > bestv) {
            bestv = logits[i];
            best = i;
        }
    }
    return (uint8_t)best;
}

static int karma_is_span_boundary(const uint8_t *text, int len, int i) {
    if (i < 0 || i >= len) return 0;

    uint8_t c = text[i];

    if (c == '\n' || c == ';') return 1;

    if (c == '.') {
        /*
            Do not split decimal numbers such as 3.14159.
            This is an early example of loss-managed parsing:
            exact numeric form must survive chunking.
        */
        int prev_digit = (i > 0 && text[i - 1] >= '0' && text[i - 1] <= '9');
        int next_digit = (i + 1 < len && text[i + 1] >= '0' && text[i + 1] <= '9');
        if (prev_digit && next_digit) return 0;
        return 1;
    }

    return 0;
}

void karma_model_absorb_text(KarmaModel *model, const uint8_t *text, int len) {
    if (!model || !text || len <= 0) return;

    /*
        First pass: process text and build a crude chunk vector.
        In KARMA-LM language:
        - recurrent state h is the backbone state
        - chunk vector goes to semantic memory if low-risk
        - exact span goes to episodic memory if high-risk
    */
    int start = 0;
    float logits[KARMA_VOCAB_SIZE];

    for (int i = 0; i < len; ++i) {
        karma_model_forward_byte(model, text[i], logits);

        if (karma_is_span_boundary(text, len, i) || i == len - 1) {
            int end = (i == len - 1) ? (i + 1) : i;
            while (start < end && (text[start] == ' ' || text[start] == '\n' || text[start] == '\t')) start++;

            int span_len = end - start;
            if (span_len > 0) {
                uint32_t risk = karma_risk_classify_span(text + start, span_len);

                if (risk != KARMA_RISK_NONE) {
                    karma_memory_store_exact(&model->memory, text + start, span_len, risk);
                } else {
                    karma_memory_store_semantic(&model->memory, model->h);
                }
            }

            start = i + 1;
        }
    }
}
