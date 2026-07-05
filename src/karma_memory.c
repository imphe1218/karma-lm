#include "karma.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool contains_word_ci(const uint8_t *text, int len, const char *word) {
    int wlen = (int)strlen(word);

    if (!text || !word || wlen <= 0 || len < wlen) {
        return false;
    }

    for (int i = 0; i <= len - wlen; ++i) {
        int ok = 1;

        for (int j = 0; j < wlen; ++j) {
            char a = (char)tolower((unsigned char)text[i + j]);
            char b = (char)tolower((unsigned char)word[j]);

            if (a != b) {
                ok = 0;
                break;
            }
        }

        if (ok) {
            return true;
        }
    }

    return false;
}

static bool contains_boundary_word_ci(
    const uint8_t *text,
    int len,
    const char *word
) {
    int wlen = (int)strlen(word);

    if (!text || !word || wlen <= 0 || len < wlen) {
        return false;
    }

    for (int i = 0; i <= len - wlen; ++i) {
        int ok = 1;

        for (int j = 0; j < wlen; ++j) {
            char a = (char)tolower((unsigned char)text[i + j]);
            char b = (char)tolower((unsigned char)word[j]);

            if (a != b) {
                ok = 0;
                break;
            }
        }

        if (!ok) {
            continue;
        }

        int left_ok =
            i == 0 ||
            !isalnum((unsigned char)text[i - 1]);

        int right_ok =
            i + wlen >= len ||
            !isalnum((unsigned char)text[i + wlen]);

        if (left_ok && right_ok) {
            return true;
        }
    }

    return false;
}

void karma_memory_init(KarmaMemory *mem) {
    if (!mem) {
        return;
    }

    memset(mem, 0, sizeof(*mem));
}

int karma_risk_score(uint32_t risk_flags) {
    int score = 0;

    if (risk_flags & KARMA_RISK_NUMBER) {
        score += 2;
    }

    if (risk_flags & KARMA_RISK_UNIT) {
        score += 2;
    }

    if (risk_flags & KARMA_RISK_NEGATION) {
        score += 3;
    }

    if (risk_flags & KARMA_RISK_CODE) {
        score += 3;
    }

    if (risk_flags & KARMA_RISK_QUOTE) {
        score += 2;
    }

    if (risk_flags & KARMA_RISK_DATE) {
        score += 2;
    }

    if (risk_flags & KARMA_RISK_EQUATION) {
        score += 3;
    }

    if (risk_flags & KARMA_RISK_NAME_HINT) {
        score += 1;
    }

    return score;
}

uint32_t karma_risk_classify_span(const uint8_t *text, int len) {
    uint32_t risk = KARMA_RISK_NONE;

    bool has_digit = false;
    bool has_alpha = false;
    bool has_symbol = false;
    bool has_decimal_point = false;
    bool quote_open = false;

    if (!text || len <= 0) {
        return risk;
    }

    for (int i = 0; i < len; ++i) {
        unsigned char c = text[i];

        if (isdigit(c)) {
            has_digit = true;
        }

        if (isalpha(c)) {
            has_alpha = true;
        }

        if (c == '"' || c == '\'') {
            quote_open = !quote_open;
        }

        if (
            c == '=' ||
            c == '<' ||
            c == '>' ||
            c == '+' ||
            c == '-' ||
            c == '*' ||
            c == '/' ||
            c == '^'
        ) {
            has_symbol = true;
        }

        if (c == '.') {
            int prev_digit =
                i > 0 &&
                isdigit((unsigned char)text[i - 1]);

            int next_digit =
                i + 1 < len &&
                isdigit((unsigned char)text[i + 1]);

            if (prev_digit && next_digit) {
                has_decimal_point = true;
            }
        }

        if (
            c == '_' ||
            c == '{' ||
            c == '}' ||
            c == '[' ||
            c == ']' ||
            c == ';' ||
            c == '#'
        ) {
            risk |= KARMA_RISK_CODE;
        }
    }

    if (has_digit) {
        risk |= KARMA_RISK_NUMBER;
    }

    if (quote_open || contains_word_ci(text, len, "\"")) {
        risk |= KARMA_RISK_QUOTE;
    }

    if (has_digit && has_alpha) {
        if (
            contains_boundary_word_ci(text, len, "v") ||
            contains_word_ci(text, len, "volt") ||
            contains_word_ci(text, len, "amp") ||
            contains_word_ci(text, len, "ohm") ||
            contains_word_ci(text, len, "hz") ||
            contains_word_ci(text, len, "kg") ||
            contains_word_ci(text, len, "m/s") ||
            contains_word_ci(text, len, "ms") ||
            contains_word_ci(text, len, "us") ||
            contains_word_ci(text, len, "ns")
        ) {
            risk |= KARMA_RISK_UNIT;
        }
    }

    if ((has_symbol && has_digit) || has_decimal_point) {
        risk |= KARMA_RISK_EQUATION;
    }

    if (
        contains_boundary_word_ci(text, len, "not") ||
        contains_boundary_word_ci(text, len, "never") ||
        contains_boundary_word_ci(text, len, "no") ||
        contains_boundary_word_ci(text, len, "unless") ||
        contains_boundary_word_ci(text, len, "except") ||
        contains_boundary_word_ci(text, len, "disable") ||
        contains_boundary_word_ci(text, len, "avoid") ||
        contains_word_ci(text, len, "do not") ||
        contains_word_ci(text, len, "don't")
    ) {
        risk |= KARMA_RISK_NEGATION;
    }

    if (len >= 8 && has_digit) {
        for (int i = 0; i <= len - 8; ++i) {
            if (
                isdigit((unsigned char)text[i]) &&
                isdigit((unsigned char)text[i + 1]) &&
                isdigit((unsigned char)text[i + 2]) &&
                isdigit((unsigned char)text[i + 3]) &&
                (text[i + 4] == '-' || text[i + 4] == '/') &&
                isdigit((unsigned char)text[i + 5]) &&
                isdigit((unsigned char)text[i + 6])
            ) {
                risk |= KARMA_RISK_DATE;
            }
        }
    }

    if (len > 1 && isupper((unsigned char)text[0])) {
        risk |= KARMA_RISK_NAME_HINT;
    }

    return risk;
}

bool karma_memory_store_exact(
    KarmaMemory *mem,
    const uint8_t *text,
    int len,
    uint32_t risk_flags
) {
    if (!mem || !text || len <= 0) {
        return false;
    }

    int pos;

    if (mem->exact_count < KARMA_EXACT_SPANS) {
        pos = mem->exact_count++;
    } else {
        pos = mem->exact_write_pos;
    }

    KarmaExactSpan *s = &mem->exact[pos];

    int copy_len = len;

    if (copy_len > KARMA_EXACT_SPAN_MAX) {
        copy_len = KARMA_EXACT_SPAN_MAX;
    }

    memcpy(s->text, text, (size_t)copy_len);
    s->length = copy_len;
    s->risk_flags = risk_flags;
    s->audit_hash = karma_fnv1a64(s->text, (size_t)s->length);

    mem->exact_write_pos = (pos + 1) % KARMA_EXACT_SPANS;

    return true;
}

void karma_memory_store_semantic(KarmaMemory *mem, const float *vec) {
    if (!mem || !vec) {
        return;
    }

    int slot = mem->semantic_count % KARMA_SEMANTIC_SLOTS;

    if (mem->semantic_count < KARMA_SEMANTIC_SLOTS) {
        for (int i = 0; i < KARMA_D_MODEL; ++i) {
            mem->semantic[slot][i] = vec[i];
        }

        mem->semantic_count++;
    } else {
        for (int i = 0; i < KARMA_D_MODEL; ++i) {
            mem->semantic[slot][i] =
                0.90f * mem->semantic[slot][i] +
                0.10f * vec[i];
        }
    }
}

static int text_contains_char_ci(
    const uint8_t *text,
    int len,
    unsigned char c
) {
    unsigned char lc = (unsigned char)tolower(c);

    for (int i = 0; i < len; ++i) {
        if ((unsigned char)tolower(text[i]) == lc) {
            return 1;
        }
    }

    return 0;
}

static int char_overlap_score(
    const uint8_t *memory_text,
    int memory_len,
    const uint8_t *query,
    int query_len
) {
    int score = 0;

    for (int q = 0; q < query_len; ++q) {
        unsigned char qc = (unsigned char)query[q];

        if (!isalnum(qc)) {
            continue;
        }

        if (text_contains_char_ci(memory_text, memory_len, qc)) {
            score++;
        }
    }

    return score;
}

static int token_overlap_score(
    const uint8_t *memory_text,
    int memory_len,
    const uint8_t *query,
    int query_len
) {
    int score = 0;
    int i = 0;

    while (i < query_len) {
        while (
            i < query_len &&
            !isalnum((unsigned char)query[i])
        ) {
            i++;
        }

        int start = i;

        while (
            i < query_len &&
            isalnum((unsigned char)query[i])
        ) {
            i++;
        }

        int token_len = i - start;

        if (token_len < 2) {
            continue;
        }

        char token[64];
        int copy_len = token_len;

        if (copy_len >= (int)sizeof(token)) {
            copy_len = (int)sizeof(token) - 1;
        }

        for (int j = 0; j < copy_len; ++j) {
            token[j] = (char)tolower((unsigned char)query[start + j]);
        }

        token[copy_len] = '\0';

        if (contains_word_ci(memory_text, memory_len, token)) {
            score += 4;
        }
    }

    return score;
}

static int exact_feature_overlap_score(
    const KarmaExactSpan *span,
    uint32_t query_risk
) {
    int score = 0;

    if ((span->risk_flags & KARMA_RISK_NUMBER) &&
        (query_risk & KARMA_RISK_NUMBER)) {
        score += 8;
    }

    if ((span->risk_flags & KARMA_RISK_UNIT) &&
        (query_risk & KARMA_RISK_UNIT)) {
        score += 8;
    }

    if ((span->risk_flags & KARMA_RISK_NEGATION) &&
        (query_risk & KARMA_RISK_NEGATION)) {
        score += 6;
    }

    if ((span->risk_flags & KARMA_RISK_CODE) &&
        (query_risk & KARMA_RISK_CODE)) {
        score += 6;
    }

    if ((span->risk_flags & KARMA_RISK_EQUATION) &&
        (query_risk & KARMA_RISK_EQUATION)) {
        score += 6;
    }

    if ((span->risk_flags & KARMA_RISK_DATE) &&
        (query_risk & KARMA_RISK_DATE)) {
        score += 6;
    }

    return score;
}

int karma_exact_span_recall_score(
    const KarmaExactSpan *span,
    const uint8_t *query,
    int query_len
) {
    if (!span || !query || query_len <= 0 || span->length <= 0) {
        return 0;
    }

    uint32_t query_risk = karma_risk_classify_span(query, query_len);

    int score = 0;

    score += char_overlap_score(
        span->text,
        span->length,
        query,
        query_len
    );

    score += token_overlap_score(
        span->text,
        span->length,
        query,
        query_len
    );

    score += exact_feature_overlap_score(span, query_risk);

    score += karma_risk_score(span->risk_flags) / 2;

    if (karma_memory_has_contradiction(NULL, span)) {
        score -= 0;
    }

    return score;
}

const KarmaExactSpan *karma_memory_recall_exact_scored(
    const KarmaMemory *mem,
    const uint8_t *query,
    int query_len,
    int *out_score
) {
    if (out_score) {
        *out_score = 0;
    }

    if (!mem || !query || query_len <= 0) {
        return NULL;
    }

    const KarmaExactSpan *best = NULL;
    int best_score = 0;

    for (int i = 0; i < mem->exact_count; ++i) {
        const KarmaExactSpan *s = &mem->exact[i];

        int score = karma_exact_span_recall_score(s, query, query_len);

        if (karma_memory_has_contradiction(mem, s)) {
            score -= 4;
        }

        if (score > best_score) {
            best_score = score;
            best = s;
        }
    }

    if (out_score) {
        *out_score = best_score;
    }

    return best_score >= 3 ? best : NULL;
}

const KarmaExactSpan *karma_memory_recall_exact(
    const KarmaMemory *mem,
    const uint8_t *query,
    int query_len
) {
    return karma_memory_recall_exact_scored(
        mem,
        query,
        query_len,
        NULL
    );
}

static bool extract_first_number(
    const uint8_t *text,
    int len,
    char *out,
    int out_size
) {
    if (!text || !out || out_size <= 0) {
        return false;
    }

    out[0] = '\0';

    for (int i = 0; i < len; ++i) {
        if (!isdigit((unsigned char)text[i])) {
            continue;
        }

        int j = i;
        int k = 0;

        while (
            j < len &&
            k < out_size - 1 &&
            (
                isdigit((unsigned char)text[j]) ||
                text[j] == '.'
            )
        ) {
            out[k++] = (char)text[j++];
        }

        out[k] = '\0';

        return k > 0;
    }

    return false;
}

bool karma_exact_spans_contradict(
    const KarmaExactSpan *a,
    const KarmaExactSpan *b
) {
    if (!a || !b) {
        return false;
    }

    if (a == b) {
        return false;
    }

    char na[64];
    char nb[64];

    bool a_num = extract_first_number(
        a->text,
        a->length,
        na,
        (int)sizeof(na)
    );

    bool b_num = extract_first_number(
        b->text,
        b->length,
        nb,
        (int)sizeof(nb)
    );

    if (a_num && b_num && strcmp(na, nb) != 0) {
        bool same_unit_context =
            ((a->risk_flags | b->risk_flags) & KARMA_RISK_UNIT) != 0;

        bool same_action_context =
            contains_word_ci(a->text, a->length, "use") &&
            contains_word_ci(b->text, b->length, "use");

        if (same_unit_context || same_action_context) {
            return true;
        }
    }

    if (
        ((a->risk_flags ^ b->risk_flags) & KARMA_RISK_NEGATION) != 0
    ) {
        int overlap = token_overlap_score(
            a->text,
            a->length,
            b->text,
            b->length
        );

        if (overlap >= 8) {
            return true;
        }
    }

    return false;
}

bool karma_memory_has_contradiction(
    const KarmaMemory *mem,
    const KarmaExactSpan *span
) {
    if (!mem || !span) {
        return false;
    }

    for (int i = 0; i < mem->exact_count; ++i) {
        const KarmaExactSpan *other = &mem->exact[i];

        if (other == span) {
            continue;
        }

        if (karma_exact_spans_contradict(span, other)) {
            return true;
        }
    }

    return false;
}

static void print_risk(uint32_t r) {
    if (r == KARMA_RISK_NONE) {
        printf("NONE ");
    }

    if (r & KARMA_RISK_NUMBER) {
        printf("NUMBER ");
    }

    if (r & KARMA_RISK_UNIT) {
        printf("UNIT ");
    }

    if (r & KARMA_RISK_NEGATION) {
        printf("NEGATION ");
    }

    if (r & KARMA_RISK_CODE) {
        printf("CODE ");
    }

    if (r & KARMA_RISK_QUOTE) {
        printf("QUOTE ");
    }

    if (r & KARMA_RISK_DATE) {
        printf("DATE ");
    }

    if (r & KARMA_RISK_EQUATION) {
        printf("EQUATION ");
    }

    if (r & KARMA_RISK_NAME_HINT) {
        printf("NAME_HINT ");
    }
}

void karma_memory_print(const KarmaMemory *mem) {
    if (!mem) {
        return;
    }

    printf("KARMA memory:\n");
    printf(
        " semantic slots used: %d / %d\n",
        mem->semantic_count,
        KARMA_SEMANTIC_SLOTS
    );

    printf(
        " exact spans used: %d / %d\n",
        mem->exact_count,
        KARMA_EXACT_SPANS
    );

    for (int i = 0; i < mem->exact_count; ++i) {
        const KarmaExactSpan *s = &mem->exact[i];

        printf(
            " exact[%03d] hash=%016llx risk_score=%d risk=",
            i,
            (unsigned long long)s->audit_hash,
            karma_risk_score(s->risk_flags)
        );

        print_risk(s->risk_flags);

        printf("text=\"%.*s\"", s->length, s->text);

        if (karma_memory_has_contradiction(mem, s)) {
            printf(" CONTRADICTION_CANDIDATE");
        }

        printf("\n");
    }
}