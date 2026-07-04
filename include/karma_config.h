#ifndef KARMA_CONFIG_H
#define KARMA_CONFIG_H

#include <stdint.h>

/*
    KARMA-LM-0 configuration.

    This first iteration is intentionally tiny:
    - byte-level vocabulary
    - one recurrent layer
    - fixed-size semantic memory
    - fixed-size exact episodic memory
    - FNV-1a audit hashes
*/

#define KARMA_VOCAB_SIZE 256
#define KARMA_D_MODEL 64
#define KARMA_SEMANTIC_SLOTS 32
#define KARMA_EXACT_SPANS 128
#define KARMA_EXACT_SPAN_MAX 256
#define KARMA_MAX_TEXT 4096

#endif
