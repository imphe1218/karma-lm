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
        "Bob studies filters and oscillators.\n"
        "Code sample: if (v > 3.3) { alarm = 1; }";

    printf("KARMA-LM C demo\n");
    printf("=================\n\n");

    printf("Absorbing context:\n%s\n\n", context);

    karma_model_absorb_text(
        &model,
        (const uint8_t *)context,
        (int)strlen(context)
    );

    karma_memory_print(&model.memory);

    const char *query = "What was the secret voltage?";
    int score = 0;

    const KarmaExactSpan *hit = karma_memory_recall_exact_scored(
        &model.memory,
        (const uint8_t *)query,
        (int)strlen(query),
        &score
    );

    printf("\nRecall-before-answer query:\n%s\n", query);

    if (hit) {
        uint64_t check = karma_fnv1a64(
            hit->text,
            (size_t)hit->length
        );

        printf("Exact memory hit: \"%.*s\"\n", hit->length, hit->text);
        printf("Recall score: %d\n", score);
        printf("Audit check: %s\n", check == hit->audit_hash ? "PASS" : "FAIL");

        if (karma_memory_has_contradiction(&model.memory, hit)) {
            printf("Warning: recalled span has contradiction candidate.\n");
        }
    } else {
        printf("No exact memory hit.\n");
    }

    printf("\nNote: neural weights are random in this iteration.\n");
    printf("The purpose is architecture wiring, memory routing, exact preservation, recall scoring, and audit integrity.\n");

    return 0;
}