#include "karma.h"

#include <stdio.h>
#include <string.h>

static int expect(int cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        return 1;
    }

    printf("PASS: %s\n", msg);
    return 0;
}

static int test_risk_classifier(void) {
    int fails = 0;

    const char *safe = "Alice likes circuits";
    const char *risky = "Voltage = 3.14159 V";
    const char *negated = "Do not enable the relay during startup.";
    const char *code = "GPIO17 HIGH for 20 ms, then LOW.";

    uint32_t r0 = karma_risk_classify_span(
        (const uint8_t *)safe,
        (int)strlen(safe)
    );

    uint32_t r1 = karma_risk_classify_span(
        (const uint8_t *)risky,
        (int)strlen(risky)
    );

    uint32_t r2 = karma_risk_classify_span(
        (const uint8_t *)negated,
        (int)strlen(negated)
    );

    uint32_t r3 = karma_risk_classify_span(
        (const uint8_t *)code,
        (int)strlen(code)
    );

    fails += expect(
        r0 & KARMA_RISK_NAME_HINT,
        "capitalized name hint is detected"
    );

    fails += expect(
        r1 & KARMA_RISK_NUMBER,
        "number risk is detected"
    );

    fails += expect(
        r1 & KARMA_RISK_EQUATION,
        "equation/decimal risk is detected"
    );

    fails += expect(
        r1 & KARMA_RISK_UNIT,
        "unit risk is detected"
    );

    fails += expect(
        r2 & KARMA_RISK_NEGATION,
        "negation risk is detected"
    );

    fails += expect(
        r3 & KARMA_RISK_CODE,
        "code-like risk is detected"
    );

    fails += expect(
        r3 & KARMA_RISK_UNIT,
        "timing unit risk is detected"
    );

    return fails;
}

static int test_exact_store_and_audit(void) {
    int fails = 0;

    KarmaMemory mem;
    karma_memory_init(&mem);

    const char *risky = "Voltage = 3.14159 V";

    uint32_t r = karma_risk_classify_span(
        (const uint8_t *)risky,
        (int)strlen(risky)
    );

    karma_memory_store_exact(
        &mem,
        (const uint8_t *)risky,
        (int)strlen(risky),
        r
    );

    const char *query = "voltage";

    const KarmaExactSpan *hit = karma_memory_recall_exact(
        &mem,
        (const uint8_t *)query,
        (int)strlen(query)
    );

    fails += expect(
        hit != NULL,
        "exact recall finds voltage span"
    );

    if (hit) {
        uint64_t check = karma_fnv1a64(
            hit->text,
            (size_t)hit->length
        );

        fails += expect(
            check == hit->audit_hash,
            "audit hash verifies exact span"
        );
    }

    return fails;
}

static int test_model_preserves_decimal_number(void) {
    int fails = 0;

    KarmaModel model;
    karma_model_init(&model, 42);

    const char *ctx = "The secret voltage is 3.14159 V.";

    karma_model_absorb_text(
        &model,
        (const uint8_t *)ctx,
        (int)strlen(ctx)
    );

    const KarmaExactSpan *vh = karma_memory_recall_exact(
        &model.memory,
        (const uint8_t *)"secret voltage",
        (int)strlen("secret voltage")
    );

    fails += expect(
        vh != NULL,
        "decimal voltage span is stored exactly"
    );

    if (vh) {
        fails += expect(
            strstr((const char *)vh->text, "3.14159") != NULL,
            "decimal number is not split"
        );
    }

    return fails;
}

static int test_recall_scoring(void) {
    int fails = 0;

    KarmaMemory mem;
    karma_memory_init(&mem);

    const char *generic = "Use low voltage.";
    const char *exact = "Use 3.3 V, not 5 V.";

    uint32_t rg = karma_risk_classify_span(
        (const uint8_t *)generic,
        (int)strlen(generic)
    );

    uint32_t re = karma_risk_classify_span(
        (const uint8_t *)exact,
        (int)strlen(exact)
    );

    karma_memory_store_exact(
        &mem,
        (const uint8_t *)generic,
        (int)strlen(generic),
        rg
    );

    karma_memory_store_exact(
        &mem,
        (const uint8_t *)exact,
        (int)strlen(exact),
        re
    );

    int score = 0;

    const KarmaExactSpan *hit = karma_memory_recall_exact_scored(
        &mem,
        (const uint8_t *)"What voltage should I use?",
        (int)strlen("What voltage should I use?"),
        &score
    );

    fails += expect(
        hit != NULL,
        "scored exact recall returns a hit"
    );

    if (hit) {
        fails += expect(
            strstr((const char *)hit->text, "3.3 V") != NULL,
            "scored recall prefers exact voltage span"
        );

        fails += expect(
            score > 0,
            "recall score is positive"
        );
    }

    return fails;
}

static int test_contradiction_detection(void) {
    int fails = 0;

    KarmaMemory mem;
    karma_memory_init(&mem);

    const char *a = "Use 3.3 V.";
    const char *b = "Use 5 V.";

    uint32_t ra = karma_risk_classify_span(
        (const uint8_t *)a,
        (int)strlen(a)
    );

    uint32_t rb = karma_risk_classify_span(
        (const uint8_t *)b,
        (int)strlen(b)
    );

    karma_memory_store_exact(
        &mem,
        (const uint8_t *)a,
        (int)strlen(a),
        ra
    );

    karma_memory_store_exact(
        &mem,
        (const uint8_t *)b,
        (int)strlen(b),
        rb
    );

    fails += expect(
        karma_exact_spans_contradict(&mem.exact[0], &mem.exact[1]),
        "different voltage instructions are contradiction candidates"
    );

    fails += expect(
        karma_memory_has_contradiction(&mem, &mem.exact[0]),
        "memory-level contradiction scan detects conflict"
    );

    return fails;
}

int main(void) {
    int fails = 0;

    printf("Running KARMA-LM-C v0.0.2-pre memory tests...\n\n");

    fails += test_risk_classifier();
    fails += test_exact_store_and_audit();
    fails += test_model_preserves_decimal_number();
    fails += test_recall_scoring();
    fails += test_contradiction_detection();

    if (fails) {
        printf("\n%d test failure(s).\n", fails);
        return 1;
    }

    printf("\nAll tests passed.\n");
    return 0;
}