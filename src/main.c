#include "karma.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    KarmaModel model;
    karma_model_init(&model, 1234);

    const char *context =
        "Alice likes analog circuits. "
        "The secret voltage is 3.14159 V. "
        "Do not compress this condition: relay K1 must never close before fuse F2 opens. "
        "Bob studies filters and oscillators. "
        "Code sample: if (v > 3.3) { alarm = 1; }";

    printf("KARMA-LM-0 C demo\n");
    printf("=================\n\n");
    printf("Absorbing context:\n%s\n\n", context);

    karma_model_absorb_text(&model, (const uint8_t *)context, (int)strlen(context));

    karma_memory_print(&model.memory);

    const char *query = "What was the secret voltage?";
    const KarmaExactSpan *hit = karma_memory_recall_exact(
        &model.memory, (const uint8_t *)query, (int)strlen(query));

    printf("\nRecall-before-answer query:\n%s\n", query);
    if (hit) {
        uint64_t check = karma_fnv1a64(hit->text, (size_t)hit->length);
        printf("Exact memory hit: \"%.*s\"\n", hit->length, hit->text);
        printf("Audit check: %s\n", check == hit->audit_hash ? "PASS" : "FAIL");
    } else {
        printf("No exact memory hit.\n");
    }

    printf("\nNote: neural weights are random in this first iteration.\n");
    printf("The purpose is architecture wiring, memory routing, exact preservation, and audit integrity.\n");

    return 0;
}
