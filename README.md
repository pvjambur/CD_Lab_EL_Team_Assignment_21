<div align="center">

# NSan — Numerical Stability Sanitizer

**An LLVM compiler plugin that automatically detects floating-point precision bugs at compile time.**

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue?style=for-the-badge&logo=cplusplus)](https://en.cppreference.com/)
[![LLVM](https://img.shields.io/badge/LLVM-10%2B-orange?style=for-the-badge&logo=llvm)](https://llvm.org/)
[![Build](https://img.shields.io/badge/Build-CMake%203.12%2B-brightgreen?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-LLVM-lightgrey?style=for-the-badge)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-12%20Cases-success?style=for-the-badge)](tests/)

> *"Find numerical instabilities before they find you."*

</div>

---

## What is NSan?

NSan instruments your C/C++ program at compile time. Every floating-point operation gets a **shadow twin** running in parallel at higher precision. When the two diverge past a configurable threshold, NSan reports exactly where, what the error is, and by how much — without changing a single line of your source code.

---

## Architecture — 3D System View

```mermaid
graph TB
    subgraph DEV["Developer Layer"]
        SRC["Source Code\n(.cpp / .c)"]
        FLAG["'-fsanitize=numerical'"]
    end

    subgraph COMP["Compiler Layer (LLVM / Clang)"]
        direction LR
        WRAP["nsan-clang++\nwrapper"]
        FRONT["Clang\nFrontend"]
        IR["LLVM IR"]
        PASS["NSan Pass\n(NSanPass.cpp)"]
        SHADOW_MAP["ShadowValueMap\n(DenseMap cache)"]
        INST["Instrumented\nLLVM IR"]
        BACK["LLVM\nBackend"]
        WRAP --> FRONT --> IR --> PASS --> INST --> BACK
        PASS <--> SHADOW_MAP
    end

    subgraph RT["Runtime Layer (libnsan_runtime.a)"]
        direction LR
        SMEM["shadow_memory\n(address → shadow)"]
        STACK["Shadow Stack\n(args / returns)"]
        CHECK["__nsan_check_consistency\n(rel error calc)"]
        SMEM <--> CHECK
        STACK <--> CHECK
    end

    subgraph OUT["Output Layer"]
        WARN["[WARN] stderr\nrel_error + location"]
        OK["SILENT\n(no false positives)"]
    end

    SRC --> WRAP
    FLAG --> WRAP
    BACK --> BIN["Native Binary\n+ Runtime linked"]
    BIN --> RT
    CHECK --> OUT

    style DEV fill:#1a1a2e,color:#e0e0e0,stroke:#4a4aff
    style COMP fill:#16213e,color:#e0e0e0,stroke:#4a90d9
    style RT fill:#0f3460,color:#e0e0e0,stroke:#e94560
    style OUT fill:#533483,color:#e0e0e0,stroke:#e94560
```

---

## Compilation Pipeline

```mermaid
flowchart LR
    A(["Source\n.cpp"]):::src
    B(["Clang\nFrontend"]):::tool
    C(["LLVM IR\n(unmodified)"]):::ir
    D(["NSan\nInstrumentation\nPass"]):::nsan
    E(["LLVM IR\n(instrumented)"]):::ir
    F(["LLVM\nBackend"]):::tool
    G(["Native Binary\n+ libnsan_runtime.a"]):::bin

    A -->|clang++| B --> C --> D --> E --> F --> G

    subgraph PASS_DETAIL["NSan Pass instruments:"]
        P1["FAdd / FSub / FMul / FDiv\n→ duplicate in shadow type"]
        P2["store float\n→ inject consistency check"]
        P3["function calls\n→ shadow stack push/pop"]
        P4["return values\n→ shadow return slot"]
    end
    D -.-> PASS_DETAIL

    classDef src    fill:#2d6a4f,color:#fff,stroke:#52b788
    classDef tool   fill:#1b4332,color:#fff,stroke:#52b788
    classDef ir     fill:#023e8a,color:#fff,stroke:#90e0ef
    classDef nsan   fill:#9b2226,color:#fff,stroke:#e63946
    classDef bin    fill:#6a0572,color:#fff,stroke:#c77dff
```

---

## Shadow Value System — 3D Internals

<p align="center">
  <img src="assets/nsan-shadow-3d.svg" width="760" alt="NSan shadow computation 3D diagram"/>
</p>

```mermaid
graph LR
    subgraph ORIG["Original Precision"]
        F1["float\n(32-bit)"]
        D1["double\n(64-bit)"]
        LD1["long double\n(80-bit)"]
    end

    subgraph SHADOW["Shadow Precision"]
        F2["double\n(64-bit)"]
        D2["fp128\n(128-bit)"]
        LD2["fp128\n(128-bit)"]
    end

    subgraph CHECK["Consistency Check"]
        REL["rel_error =\n|orig − shadow| / |shadow|"]
        THRESH["rel_error > ε ?"]
        YES["[WARN] emitted\nlocation + error %"]
        NO["Silent pass"]
    end

    F1 -- "promoted →" --> F2
    D1 -- "promoted →" --> D2
    LD1 -- "promoted →" --> LD2

    F2 --> REL
    D2 --> REL
    LD2 --> REL
    REL --> THRESH
    THRESH -- "yes" --> YES
    THRESH -- "no"  --> NO

    style ORIG   fill:#1d3557,color:#fff,stroke:#457b9d
    style SHADOW fill:#2d6a4f,color:#fff,stroke:#52b788
    style CHECK  fill:#6a0572,color:#fff,stroke:#c77dff
```

---

## Project Structure

```
CD_Lab_EL_Team_Assignment_21/
│
├── README.md               ← you are here
├── DESIGN.md               ← approach + alternatives
├── IMPLEMENTATION.md       ← LLVM pass internals
├── EVALUATION.md           ← benchmark results + test case analysis
├── DEMO.md                 ← demo video / screenshots
│
├── nsan-clang++            ← compiler wrapper (-fsanitize=numerical)
├── build.sh                ← one-shot build script
├── run.sh                  ← test runner
├── benchmark.sh            ← performance benchmarking
│
├── src/
│   ├── nsan/
│   │   ├── NSanPass.cpp        ← LLVM function pass (core instrumentation)
│   │   ├── NSanPass.h
│   │   ├── ShadowValueMap.cpp  ← DenseMap: Value* → shadow Value*
│   │   └── ShadowValueMap.h
│   └── runtime/
│       ├── nsan_runtime.cpp    ← __nsan_check_consistency + shadow stack
│       ├── nsan_runtime.h
│       ├── shadow_memory.cpp   ← address → shadow memory mapping
│       └── shadow_memory.h
│
├── include/
│   ├── nsan.h                  ← public API
│   ├── nsan_interface.h
│   └── internal/
│       ├── configuration.h
│       └── shadow_tracking.h
│
└── tests/
    ├── tc1_cancellation.cpp    ← catastrophic cancellation
    ├── tc2_naive_sum.cpp        ← naive summation drift
    ├── tc3_kahan.cpp            ← Kahan (no false positive)
    ├── tc4_alternating.cpp      ← alternating harmonic series
    ├── tc5_poly.cpp             ← polynomial near root
    ├── tc6_newton.cpp           ← Newton's method
    ├── tc7_variance.cpp         ← one-pass variance
    ├── tc8_exact_sum.cpp        ← exact summation
    ├── tc9_fma.cpp              ← FMA cancellation
    ├── tc10_sigmoid.cpp         ← sigmoid instability
    ├── tc11_mixed_dot.cpp       ← mixed-precision dot product
    └── tc12_newton_sqrt.cpp     ← Newton sqrt convergence
```

---

## Tool Stack

| Layer | Tool | Role |
|-------|------|------|
| **Compiler** | `clang++` / LLVM 10+ | Parses C/C++, emits LLVM IR |
| **Pass** | `NSanPass` (New Pass Manager) | Instruments every FP instruction |
| **Shadow Map** | `ShadowValueMap` (DenseMap) | Tracks `Value* → shadow Value*` locally |
| **Runtime** | `libnsan_runtime.a` | Consistency checks, shadow memory, shadow stack |
| **Wrapper** | `nsan-clang++` shell script | Translates `-fsanitize=numerical` to pass flags |
| **Build** | CMake 3.12+, C++17 | Builds pass shared lib + runtime static lib |
| **Test Runner** | `run.sh` | Compiles and runs all 12 test cases |
| **Benchmark** | `benchmark.sh` | Measures overhead across 5 representative cases |

---

## Performance — Why NSan Wins

| Tool | Mechanism | Overhead | Multi-thread? | False Positives |
|------|-----------|----------|---------------|-----------------|
| **NSan (double shadow)** | LLVM pass, native hw | **2 – 4×** | Yes | Low (observable only) |
| **NSan (quad shadow)** | LLVM pass, fp128 hw | **~17×** | Yes | Low |
| Verificarlo (MCA) | Stochastic, 1000× reruns | ~40,000× | No | High |
| FpDebug / Valgrind | Binary instrumentation | ~476× | No | Medium |
| Manual Analysis | Human effort | infinite | — | Variable |

> NSan is **1–4 orders of magnitude faster** than the nearest alternative while providing deterministic, source-accurate diagnostics.

---

## Test Case Results

| # | Test | Numerical Pattern | NSan Result | Expected |
|---|------|-------------------|-------------|----------|
| TC1 | `tc1_cancellation.cpp` | `(1e7 + 1.234) − 1e7` | `[WARN] rel=1.90e-01` | WARN |
| TC2 | `tc2_naive_sum.cpp` | Sum `0.1f` × 1,000,000 | `[WARN] rel=9.58e-03` | WARN |
| TC3 | `tc3_kahan.cpp` | Kahan compensated sum | **SILENT** | Silent |
| TC4 | `tc4_alternating.cpp` | Alternating harmonic series | `[WARN] rel=1.06e-05` | WARN |
| TC5 | `tc5_poly.cpp` | Poly near root `x=1` | `[WARN]` | WARN |
| TC6 | `tc6_newton.cpp` | Newton's method | `[WARN]` | WARN |
| TC7 | `tc7_variance.cpp` | One-pass variance | `[WARN] rel=4.54e+04` | WARN |
| TC8 | `tc8_exact_sum.cpp` | Exact summation | **SILENT** | Silent |
| TC9 | `tc9_fma.cpp` | FMA near-zero | `[WARN] rel=8.22e+05` | WARN |
| TC10 | `tc10_sigmoid.cpp` | Sigmoid instability | `[WARN]` | WARN |
| TC11 | `tc11_mixed_dot.cpp` | Mixed-precision dot product | `[WARN]` | WARN |
| TC12 | `tc12_newton_sqrt.cpp` | Newton sqrt convergence | `[WARN]` | WARN |

> TC3 and TC8 deliberately produce **no warnings** — validating NSan's false-positive suppression.

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

### Run All Tests

```bash
./run.sh
```

### Instrument Your Own Code

```bash
# Compile with NSan
./nsan-clang++ -fsanitize=numerical my_program.cpp -o my_program

# Run — precision errors surface immediately
./my_program

# Tune tolerance
NSAN_REL_EPSILON=1e-6 ./my_program

# Increase verbosity
NSAN_VERBOSITY=2 ./my_program
```

### Benchmark Overhead

```bash
./benchmark.sh   # compares instrumented vs baseline on tc1–tc4, tc7
```

---

## Runtime Controls

| Environment Variable | Default | Description |
|----------------------|---------|-------------|
| `NSAN_REL_EPSILON` | `1e-5` | Relative error threshold for `[WARN]` |
| `NSAN_EPSILON` | — | Absolute error threshold (alternative) |
| `NSAN_VERBOSITY` | `1` | `1` = warnings only, `2` = full shadow trace |

---

## Use Cases

```mermaid
mindmap
  root((NSan))
    Scientific Computing
      Physics simulations
      Molecular dynamics
      Climate models
    Machine Learning
      Neural network training
      Gradient computation
      Loss landscape analysis
    Numerical Libraries
      BLAS / LAPACK
      Linear solvers
      FFT implementations
    Financial Systems
      Risk calculations
      Portfolio optimization
      Derivative pricing
    Embedded and Control
      PID controllers
      Signal processing
      Sensor fusion
    Computer Graphics
      Ray tracing
      Matrix transforms
      Shader precision
```

---

## References

- Courbet, C. (2021). *NSan: A Floating-Point Numerical Sanitizer*. CC '21 — ACM SIGPLAN International Conference on Compiler Construction.
- IEEE 754-2019 — Floating-point arithmetic standard.
- Higham, N. J. (2002). *Accuracy and Stability of Numerical Algorithms* (2nd ed.). SIAM.
- [LLVM Sanitizer Framework](https://clang.llvm.org/docs/Sanitizers/)
- [Verificarlo](https://github.com/verificarlo/verificarlo)

---

<div align="center">

**CD Lab — EL Team Assignment 21** | Compiler Design | 2026

*Built on LLVM. Inspired by the CC '21 paper. Implemented from scratch.*

</div>
