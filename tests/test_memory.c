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

int main(void) {
    int fails = 0;

    KarmaMemory mem;
    karma_memory_init(&mem);

    const char *safe = "Alice likes circuits";
    const char *risky = "Voltage = 3.14159 V";

    uint32_t r0 = karma_risk_classify_span((const uint8_t *)safe, (int)strlen(safe));
    uint32_t r1 = karma_risk_classify_span((const uint8_t *)risky, (int)strlen(risky));

    fails += expect(r0 & KARMA_RISK_NAME_HINT, "capitalized name hint is detected");
    fails += expect(r1 & KARMA_RISK_NUMBER, "number risk is detected");
    fails += expect(r1 & KARMA_RISK_EQUATION, "equation risk is detected");

    karma_memory_store_exact(&mem, (const uint8_t *)risky, (int)strlen(risky), r1);

    const char *query = "voltage";
    const KarmaExactSpan *hit = karma_memory_recall_exact(
        &mem, (const uint8_t *)query, (int)strlen(query));

    fails += expect(hit != NULL, "exact recall finds voltage span");
    if (hit) {
        uint64_t check = karma_fnv1a64(hit->text, (size_t)hit->length);
        fails += expect(check == hit->audit_hash, "audit hash verifies exact span");
    }


    KarmaModel model;
    karma_model_init(&model, 42);
    const char *ctx = "The secret voltage is 3.14159 V.";
    karma_model_absorb_text(&model, (const uint8_t *)ctx, (int)strlen(ctx));
    const KarmaExactSpan *vh = karma_memory_recall_exact(
        &model.memory, (const uint8_t *)"secret voltage", (int)strlen("secret voltage"));
    fails += expect(vh != NULL, "decimal voltage span is stored exactly");
    if (vh) {
        fails += expect(strstr((const char *)vh->text, "3.14159") != NULL, "decimal number is not split");
    }

    return fails ? 1 : 0;
}
