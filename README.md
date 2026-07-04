# KARMA-LM-0 C prototype

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
