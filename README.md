# KARMA-LM C prototype

First portable C17 iteration of KARMA-LM: Kernel-Augmented Recurrent Memory Architecture with Loss-Managed Memory.

This prototype is intentionally small. It demonstrates:

- byte-level input
- tiny recurrent backbone
- semantic memory slots
- exact episodic memory
- rule-based compression-risk classifier
- audit/checksum hashes
- recall-before-answer behavior

The neural weights are random in this iteration. Training comes next.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/karma_demo
```

On Windows MSYS2, the executable may be:

```bash
./build/karma_demo.exe
```

## Test

```bash
./build/test_memory
```

or on Windows:

```bash
./build/test_memory.exe
```

## Next milestone

Add a tiny training loop for next-byte prediction:

- cross-entropy loss
- backpropagation through recurrent layer
- Adam optimizer
- checkpoint save/load
- synthetic memory preservation dataset

---
Version: `v0.0.2-pre`

## What Changed in v0.0.2-pre

`v0.0.2-pre` improves the memory reliability layer before adding training.

New or improved behavior:

- explicit risk scoring from risk flags
- stronger exact-detail recall scoring
- exact-memory preference for risky facts
- contradiction-candidate detection
- regression tests for:
  - numbers
  - units
  - decimals
  - negations
  - code-like spans
  - audit-hash preservation
  - exact recall before answer