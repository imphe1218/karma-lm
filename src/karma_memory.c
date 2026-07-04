#include "karma.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool contains_word_ci(const uint8_t *text, int len, const char *word) {
    int wlen = (int)strlen(word);
    if (wlen <= 0 || len < wlen) return false;

    for (int i = 0; i <= len - wlen; ++i) {
        int ok = 1;
        for (int j = 0; j < wlen; ++j) {
            char a = (char)tolower(text[i + j]);
            char b = (char)tolower((unsigned char)word[j]);
            if (a != b) {
                ok = 0;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

void karma_memory_init(KarmaMemory *mem) {
    memset(mem, 0, sizeof(*mem));
}

uint32_t karma_risk_classify_span(const uint8_t *text, int len) {
    uint32_t risk = KARMA_RISK_NONE;

    bool has_digit = false;
    bool has_alpha = false;
    bool has_symbol = false;
    bool quote_open = false;

    for (int i = 0; i < len; ++i) {
        unsigned char c = text[i];

        if (isdigit(c)) has_digit = true;
        if (isalpha(c)) has_alpha = true;

        if (c == '"' || c == '\'') quote_open = !quote_open;

        if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' ||
            c == '*' || c == '/' || c == '^') {
            has_symbol = true;
        }

        if (c == '_' || c == '{' || c == '}' || c == '[' || c == ']' ||
            c == ';' || c == '#') {
            risk |= KARMA_RISK_CODE;
        }
    }

    if (has_digit) risk |= KARMA_RISK_NUMBER;
    if (quote_open || contains_word_ci(text, len, "\"")) risk |= KARMA_RISK_QUOTE;

    if (has_digit && has_alpha) {
        if (contains_word_ci(text, len, "v") ||
            contains_word_ci(text, len, "volt") ||
            contains_word_ci(text, len, "amp") ||
            contains_word_ci(text, len, "ohm") ||
            contains_word_ci(text, len, "hz") ||
            contains_word_ci(text, len, "kg") ||
            contains_word_ci(text, len, "m/s")) {
            risk |= KARMA_RISK_UNIT;
        }
    }

    if (has_symbol && has_digit) risk |= KARMA_RISK_EQUATION;

    if (contains_word_ci(text, len, "not") ||
        contains_word_ci(text, len, "never") ||
        contains_word_ci(text, len, "no ") ||
        contains_word_ci(text, len, "unless") ||
        contains_word_ci(text, len, "except")) {
        risk |= KARMA_RISK_NEGATION;
    }

    if (len >= 8 && has_digit) {
        for (int i = 0; i <= len - 8; ++i) {
            if (isdigit(text[i]) && isdigit(text[i+1]) && isdigit(text[i+2]) && isdigit(text[i+3]) &&
                (text[i+4] == '-' || text[i+4] == '/') &&
                isdigit(text[i+5]) && isdigit(text[i+6])) {
                risk |= KARMA_RISK_DATE;
            }
        }
    }

    if (len > 1 && isupper(text[0])) risk |= KARMA_RISK_NAME_HINT;

    return risk;
}

bool karma_memory_store_exact(KarmaMemory *mem, const uint8_t *text, int len, uint32_t risk_flags) {
    if (!mem || !text || len <= 0) return false;

    int pos;
    if (mem->exact_count < KARMA_EXACT_SPANS) {
        pos = mem->exact_count++;
    } else {
        pos = mem->exact_write_pos;
    }

    KarmaExactSpan *s = &mem->exact[pos];
    int copy_len = len;
    if (copy_len > KARMA_EXACT_SPAN_MAX) copy_len = KARMA_EXACT_SPAN_MAX;

    memcpy(s->text, text, (size_t)copy_len);
    s->length = copy_len;
    s->risk_flags = risk_flags;
    s->audit_hash = karma_fnv1a64(s->text, (size_t)s->length);

    mem->exact_write_pos = (pos + 1) % KARMA_EXACT_SPANS;
    return true;
}

void karma_memory_store_semantic(KarmaMemory *mem, const float *vec) {
    if (!mem || !vec) return;

    int slot = mem->semantic_count % KARMA_SEMANTIC_SLOTS;

    if (mem->semantic_count < KARMA_SEMANTIC_SLOTS) {
        for (int i = 0; i < KARMA_D_MODEL; ++i) mem->semantic[slot][i] = vec[i];
        mem->semantic_count++;
    } else {
        /* Simple exponential moving average update. */
        for (int i = 0; i < KARMA_D_MODEL; ++i) {
            mem->semantic[slot][i] = 0.90f * mem->semantic[slot][i] + 0.10f * vec[i];
        }
    }
}

const KarmaExactSpan *karma_memory_recall_exact(const KarmaMemory *mem, const uint8_t *query, int query_len) {
    if (!mem || !query || query_len <= 0) return NULL;

    const KarmaExactSpan *best = NULL;
    int best_score = 0;

    for (int i = 0; i < mem->exact_count; ++i) {
        const KarmaExactSpan *s = &mem->exact[i];
        int score = 0;

        for (int q = 0; q < query_len; ++q) {
            unsigned char qc = (unsigned char)tolower(query[q]);
            if (!isalnum(qc)) continue;

            for (int j = 0; j < s->length; ++j) {
                unsigned char sc = (unsigned char)tolower(s->text[j]);
                if (qc == sc) {
                    score++;
                    break;
                }
            }
        }

        if (score > best_score) {
            best_score = score;
            best = s;
        }
    }

    return best_score >= 3 ? best : NULL;
}

static void print_risk(uint32_t r) {
    if (r & KARMA_RISK_NUMBER) printf("NUMBER ");
    if (r & KARMA_RISK_UNIT) printf("UNIT ");
    if (r & KARMA_RISK_NEGATION) printf("NEGATION ");
    if (r & KARMA_RISK_CODE) printf("CODE ");
    if (r & KARMA_RISK_QUOTE) printf("QUOTE ");
    if (r & KARMA_RISK_DATE) printf("DATE ");
    if (r & KARMA_RISK_EQUATION) printf("EQUATION ");
    if (r & KARMA_RISK_NAME_HINT) printf("NAME_HINT ");
}

void karma_memory_print(const KarmaMemory *mem) {
    printf("KARMA memory:\n");
    printf("  semantic slots used: %d / %d\n", mem->semantic_count, KARMA_SEMANTIC_SLOTS);
    printf("  exact spans used:    %d / %d\n", mem->exact_count, KARMA_EXACT_SPANS);

    for (int i = 0; i < mem->exact_count; ++i) {
        const KarmaExactSpan *s = &mem->exact[i];
        printf("  exact[%03d] hash=%016llx risk=", i, (unsigned long long)s->audit_hash);
        print_risk(s->risk_flags);
        printf("text=\"%.*s\"\n", s->length, s->text);
    }
}
