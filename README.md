<div align="center">

<img src="assets/nsan-shadow-3d.svg" width="720" alt="NSan Shadow Computation"/>

# NSan — Numerical Stability Sanitizer

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)](https://en.cppreference.com/)
[![LLVM 10+](https://img.shields.io/badge/LLVM-10%2B-orange?style=flat-square)](https://llvm.org/)
[![CMake](https://img.shields.io/badge/CMake-3.12%2B-brightgreen?style=flat-square)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-LLVM-lightgrey?style=flat-square)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-12%20Cases-success?style=flat-square)](tests/)

*Compile-time detection of floating-point numerical instabilities in C/C++ programs.*

</div>

---

## Overview

NSan is an LLVM compiler plugin that instruments every floating-point operation at compile time. Each native FP value gets a **shadow twin** running in parallel at higher precision. At observable points — memory stores, function returns, comparisons — NSan measures the divergence between the two. If the relative error exceeds a configurable threshold, it reports the exact location and magnitude of the precision loss.

No source changes are required. Compile with `-fsanitize=numerical` and run your program normally.

**Why NSan over alternatives?**

| Tool | Mechanism | Overhead | Multi-threaded |
|------|-----------|----------|----------------|
| **NSan (double shadow)** | LLVM pass — native hardware | **2 – 4×** | Yes |
| **NSan (quad shadow)** | LLVM pass — fp128 | **~17×** | Yes |
| FpDebug / Valgrind | Binary instrumentation | ~476× | No |
| Verificarlo (MCA) | Stochastic, 1000 reruns | ~40,000× | No |

---

## How It Works

Every floating-point value is **promoted** to a higher-precision shadow type at the instrumentation layer:

```
float       →  double     (32-bit  →  64-bit)
double      →  fp128      (64-bit  →  128-bit)
long double →  fp128      (80-bit  →  128-bit)
```

The LLVM pass (`NSanPass`) duplicates every `FAdd`, `FSub`, `FMul`, `FDiv`, and cast operation. Local shadow values are tracked in a `ShadowValueMap` (DenseMap). At function boundaries the shadow stack (`__nsan_push_shadow_arg` / `__nsan_load_shadow_arg`) carries values across calls.

At each **observation point** the runtime checks:

```
rel_error = |native_result - shadow_result| / |shadow_result|

if rel_error > NSAN_REL_EPSILON:
    [WARN] NSan: numerical inconsistency
           float32  = <native value>
           shadow   = <shadow value>
           rel_err  = <magnitude>
```

If the algorithm is numerically stable the output is **silent** — no false positives.

---

## Project Structure

```
NSan/
├── src/
│   ├── nsan/
│   │   ├── NSanPass.cpp          LLVM function pass — core instrumentation
│   │   ├── NSanPass.h
│   │   ├── ShadowValueMap.cpp    DenseMap: Value* → shadow Value*
│   │   └── ShadowValueMap.h
│   └── runtime/
│       ├── nsan_runtime.cpp      __nsan_check_consistency, shadow stack
│       ├── nsan_runtime.h
│       ├── shadow_memory.cpp     application address → shadow memory map
│       └── shadow_memory.h
├── include/
│   ├── nsan.h                    public API
│   ├── nsan_interface.h
│   └── internal/
│       ├── configuration.h
│       └── shadow_tracking.h
├── tests/
│   ├── tc1_cancellation.cpp      catastrophic cancellation
│   ├── tc2_naive_sum.cpp          naive summation drift
│   ├── tc3_kahan.cpp              Kahan sum — false-positive control
│   ├── tc4_alternating.cpp        alternating harmonic series
│   ├── tc5_poly.cpp               polynomial near root
│   ├── tc6_newton.cpp             Newton reciprocal — false-positive control
│   ├── tc7_variance.cpp           one-pass variance (massive cancellation)
│   ├── tc8_exact_sum.cpp          exact integer sum — false-positive control
│   ├── tc9_fma.cpp                FMA near-zero cancellation
│   ├── tc10_sigmoid.cpp           sigmoid — false-positive control
│   ├── tc11_mixed_dot.cpp         mixed-precision dot product
│   └── tc12_newton_sqrt.cpp       Newton sqrt — false-positive control
├── examples/
│   ├── standalone_demo/           nsan_demo (prebuilt binary)
│   ├── interactive_demo/          nsan_interactive (prebuilt binary)
│   └── gui_demo/                  nsan_gui (prebuilt binary)
├── screenshots/                   build and run evidence
├── assets/
│   └── nsan-shadow-3d.svg         shadow computation diagram
├── nsan-clang++                   compiler wrapper (-fsanitize=numerical)
├── build.sh                       one-shot build
├── run.sh                         test runner
├── benchmark.sh                   performance benchmarking
├── DESIGN.md                      approach and alternatives considered
├── IMPLEMENTATION.md              LLVM pass internals
├── EVALUATION.md                  test case analysis and benchmark results
└── DEMO.md                        demo screenshots and walkthrough
```

---

## Tool Stack

| Layer | Component | Description |
|-------|-----------|-------------|
| Compiler | `clang++` + LLVM 10+ | Parses C/C++, emits LLVM IR |
| Pass | `NSanPass` (New Pass Manager) | Instruments every FP instruction in the IR |
| Shadow tracking | `ShadowValueMap` (DenseMap) | Per-function `Value* → shadow Value*` lookup |
| Runtime | `libnsan_runtime.a` | Consistency checks, shadow stack, shadow memory |
| Wrapper | `nsan-clang++` | Translates `-fsanitize=numerical` to plugin flags |
| Build | CMake 3.12+, C++17 | Produces `NSanPass.so` + `libnsan_runtime.a` |
| Tests | `run.sh` | Builds and runs all 12 test cases |
| Benchmark | `benchmark.sh` | Measures instrumented vs baseline overhead |

---

## Test Suite

| # | File | Pattern | Expected |
|---|------|---------|----------|
| TC1  | `tc1_cancellation.cpp`   | `(1e7 + 1.234) − 1e7`           | WARN — `rel=1.90e-01` |
| TC2  | `tc2_naive_sum.cpp`       | Sum `0.1f` × 1,000,000           | WARN — `rel=9.58e-03` |
| TC3  | `tc3_kahan.cpp`           | Kahan compensated sum             | **SILENT** |
| TC4  | `tc4_alternating.cpp`     | Alternating harmonic series       | WARN — `rel=1.06e-05` |
| TC5  | `tc5_poly.cpp`            | Polynomial near root `x=1`        | WARN |
| TC6  | `tc6_newton.cpp`          | Newton reciprocal iteration       | **SILENT** |
| TC7  | `tc7_variance.cpp`        | One-pass variance — `Σx²/N − μ²`  | WARN — `rel=4.54e+04` |
| TC8  | `tc8_exact_sum.cpp`       | Exact integer summation           | **SILENT** |
| TC9  | `tc9_fma.cpp`             | FMA near-zero `a·b + c ≈ 0`       | WARN — `rel=8.22e+05` |
| TC10 | `tc10_sigmoid.cpp`        | Sigmoid — stable input domain     | **SILENT** |
| TC11 | `tc11_mixed_dot.cpp`      | Mixed-magnitude dot product       | WARN |
| TC12 | `tc12_newton_sqrt.cpp`    | Newton √2 convergence             | **SILENT** |

SILENT cases (TC3, TC6, TC8, TC10, TC12) are false-positive controls — they validate that NSan does **not** warn on stable, well-designed algorithms.

---

## Quick Start

### Prerequisites

```bash
# Ubuntu / Debian
sudo apt-get install llvm-dev clang cmake build-essential

# macOS
brew install llvm cmake
```

### Build

```bash
git clone https://github.com/pvjambur/CD_Lab_EL_Team_Assignment_21.git
cd CD_Lab_EL_Team_Assignment_21
./build.sh
```

### Run Tests

```bash
./run.sh
```

### Instrument Your Own Code

```bash
./nsan-clang++ -fsanitize=numerical my_program.cpp -o my_program
./my_program
```

### Benchmark

```bash
./benchmark.sh
```

---

## Runtime Configuration

| Variable | Default | Effect |
|----------|---------|--------|
| `NSAN_REL_EPSILON` | `1e-5` | Relative error threshold for `[WARN]` |
| `NSAN_EPSILON` | — | Absolute error threshold (alternative) |
| `NSAN_VERBOSITY` | `1` | `1` = warnings only · `2` = full shadow trace |

---

## Use Cases

- **Scientific computing** — physics simulations, molecular dynamics, climate modelling
- **Machine learning** — gradient computation, loss landscape analysis, training stability
- **Numerical libraries** — BLAS/LAPACK validation, linear solver correctness
- **Financial systems** — risk calculations, derivative pricing, portfolio optimisation
- **Embedded and control** — PID controllers, signal processing, sensor fusion
- **Computer graphics** — ray tracing, shader precision, matrix transform chains

---

## References

- Courbet, C. (2021). *NSan: A Floating-Point Numerical Sanitizer.* CC '21 — ACM SIGPLAN International Conference on Compiler Construction.
- IEEE 754-2019 — Floating-point arithmetic standard.
- Higham, N. J. (2002). *Accuracy and Stability of Numerical Algorithms* (2nd ed.). SIAM.
- [LLVM Sanitizer Framework](https://clang.llvm.org/docs/Sanitizers/)

---

<div align="center">

CD Lab · EL Team · Assignment 21 · Compiler Design · 2026

</div>
