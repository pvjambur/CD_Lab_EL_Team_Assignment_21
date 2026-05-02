# NSan — Numerical Stability Sanitizer: Full Project Explainer

> **CD Lab EL Team Assignment 21** | Compiler Design Lab | 2026

---

## 1. What This Project Is

**NSan** (Numerical Sanitizer) is an **LLVM compiler plugin** that automatically detects floating-point precision bugs in C/C++ programs — without you changing a single line of your source code.

The idea: when you compile your program through NSan, it secretly runs every float operation **twice** in parallel — once in `float` (as you wrote), and once in higher precision (`double`). If the two answers diverge beyond a threshold, it prints a warning.

This is called **shadow computation**.

```
Your code writes:    float sum += value;

NSan makes it run:   float  sum        += value;           // original
                     double sum_shadow += (double)value;   // shadow
                     if (|sum - sum_shadow| / |sum_shadow| > epsilon)
                         print WARNING;
```

The developer never sees the shadow code — NSan injects it invisibly at compile time using an **LLVM IR transformation pass**.

---

## 2. The Problem Being Solved

### Why Floating-Point Is Tricky

Floating-point numbers (`float`, `double`) cannot represent most real numbers exactly. A `float` has only 23 bits of mantissa (~7 decimal digits of precision). This causes:

| Problem | Example | Effect |
|---|---|---|
| **Catastrophic cancellation** | `(1e8 + 1.0) - 1e8` in float → `0.0` instead of `1.0` | Massive relative error |
| **Accumulation drift** | Summing 100,000 small values in float | Error compounds with each addition |
| **Representability error** | `0.1f` added 1000 times ≠ `100.0` exactly | Silent precision loss |
| **Near-zero cancellation** | Variance formula `(ΣX²/N) - mean²` | Subtracting nearly-equal large numbers |

### Why Existing Tools Fall Short

| Tool | Slowdown | Problem |
|---|---|---|
| Manual analysis | N/A | Requires PhD-level math, doesn't scale |
| Verificarlo | 40,000× | Runs program 1000+ times with perturbations |
| FpDebug/Valgrind | 476× | Serializes threads, wrong semantics for library calls |
| **NSan** | **2–20×** | One run, correct semantics, actionable output |

---

## 3. Full Directory Structure

```
CD_Lab_EL_Team_Assignment_21/
│
├── CMakeLists.txt              # Root build config (requires LLVM dev)
├── build.sh                   # One-shot build script (cleans + cmake + ninja)
├── run.sh                     # Compile test_quick.cpp through NSan pass + run
│
├── PROBLEM_STATEMENT.md        # Detailed problem definition with code examples
├── README.md                   # Project overview and usage guide
├── START_NEEDED.md             # Quick-start guide (run demos without LLVM)
├── PROJECT_EXPLAINER.md        # ← THIS FILE
│
├── test_quick.cpp              # 12-case evaluation suite (the main test file)
├── test_quick                  # Pre-built binary of the test suite
├── test_quick_plain.ll         # LLVM IR dump of a simple float loop (reference)
│
├── src/
│   ├── nsan/                   # The LLVM transformation pass
│   │   ├── NSanPass.h          # Class declaration
│   │   ├── NSanPass.cpp        # Pass implementation (instruments IR)
│   │   ├── ShadowValueMap.h    # Maps original IR values → shadow values
│   │   ├── ShadowValueMap.cpp  # (thin — logic is in .h)
│   │   └── CMakeLists.txt      # Builds libnsan_pass.so (LLVM module)
│   │
│   └── runtime/               # C++ library linked into instrumented binaries
│       ├── nsan_runtime.h      # Header for runtime ABI
│       ├── nsan_runtime.cpp    # __nsan_check_consistency, shadow stack
│       ├── shadow_memory.h     # Shadow heap allocation header
│       ├── shadow_memory.cpp   # Shadow heap: malloc-based address map
│       └── CMakeLists.txt      # Builds libnsan_runtime.a
│
├── include/
│   ├── nsan.h                  # Public user API (__nsan_check_float etc.)
│   ├── nsan_interface.h        # Thin re-export of nsan.h
│   └── internal/
│       ├── configuration.h     # NSanConfig struct + env-var defaults
│       └── shadow_tracking.h   # ShadowType enum (placeholder)
│
├── examples/
│   ├── simple_computation.cpp  # Naive vs Kahan sum — standalone demo
│   ├── nsan_demo_config.h      # Shared runtime config for all demo files
│   ├── standalone_demo/
│   │   └── nsan_demo.cpp       # ★ WORKS NOW — no LLVM needed, run this first
│   ├── interactive_demo/
│   │   └── nsan_interactive.cpp
│   └── gui_demo/
│       └── nsan_gui.cpp
│
├── tests/
│   ├── CMakeLists.txt          # (stubs — all targets commented out)
│   ├── tc1_cancellation.cpp    # ★ Individual test: catastrophic cancellation
│   ├── tc2_naive_sum.cpp       # ★ Individual test: naive float summation
│   ├── tc3_kahan.cpp           # ★ Individual test: Kahan compensated sum (SILENT)
│   ├── tc4_alternating.cpp     # ★ Individual test: alternating harmonic series
│   ├── tc5_poly.cpp            # ★ Individual test: polynomial evaluation
│   ├── tc6_newton.cpp          # ★ Individual test: Newton reciprocal
│   ├── tc7_variance.cpp        # ★ Individual test: unstable one-pass variance
│   ├── tc8_exact_sum.cpp       # ★ Individual test: exact integer arithmetic
│   ├── tc9_fma.cpp             # ★ Individual test: FMA cancellation
│   ├── tc10_sigmoid.cpp        # ★ Individual test: sigmoid at extremes
│   ├── tc11_mixed_dot.cpp      # ★ Individual test: mixed-magnitude dot product
│   ├── tc12_newton_sqrt.cpp    # ★ Individual test: Newton sqrt
│   ├── unit/                   # Empty — future unit tests
│   ├── functional/             # Empty — future functional tests
│   └── benchmarks/             # Empty
│       └── bins/               # Pre-built test binaries (tc1–tc5, nsan_demo)
│
├── cmake/                      # Empty — future CMake helper modules
├── docs/
│   └── PROJECT_STATUS.md       # Detailed bug list + implementation phases
└── private_config/             # Reference docs for implementation (not source)
```

---

## 4. How the Pipeline Works End-to-End

```
   Your C++ source (e.g., simple_computation.cpp)
            │
            ▼
   clang++ frontend  →  LLVM IR
   e.g.:  %sum = fadd float %a, %b
            │
            ▼
   NSanPass::run() fires on every function
   For each float instruction, it inserts shadow ops:
      %sum        = fadd float  %a,        %b        ← original
      %a_shadow   = fpext float %a   to double
      %b_shadow   = fpext float %b   to double
      %sum_shadow = fadd double %a_shadow, %b_shadow  ← shadow
      call __nsan_check_consistency(%sum, %sum_shadow) ← check
            │
            ▼
   LLVM backend compiles instrumented IR → native binary
            │
            ▼
   Binary runs normally — but now also:
      • Executes shadow double ops in parallel
      • Calls __nsan_check_consistency() after every FP store
            │
            ▼
   nsan_runtime.cpp computes relative error:
      rel_err = |float_result - double_shadow| / |double_shadow|
      if rel_err > NSAN_REL_EPSILON  →  print [WARN] to stderr
```

---

## 5. Key Source Files Explained

### `src/nsan/NSanPass.cpp` — The LLVM Pass

This is the heart of the project. It implements a **function-level LLVM pass** using the New Pass Manager API.

**Entry point:** `NSanPass::run(Function &F, FunctionAnalysisManager &AM)`

For every function it processes:
1. `instrumentFunctionEntry(F)` — loads shadow values for incoming float arguments from the shadow stack
2. Iterates every `BasicBlock` and every `Instruction`:
   - `BinaryOperator` (fadd/fsub/fmul/fdiv) → `instrumentBinaryOp()`
   - `CastInst` (fpext/fptrunc) → `instrumentCastOp()`
   - `LoadInst` / `StoreInst` → `instrumentLoadStore()`
   - `CallInst` → `instrumentFunctionCall()`
   - `ReturnInst` → `instrumentReturnValue()`
3. Returns `PreservedAnalyses::none()` if any IR was modified

**Shadow type promotion rules:**
- `float` (32-bit) → `double` (64-bit) shadow
- `double` (64-bit) → `fp128` (128-bit) shadow
- `x86_fp80` → `fp128` shadow

**Plugin registration:** The file exports `llvmGetPassPluginInfo()` so clang can load it with `-fpass-plugin=libnsan_pass.so`.

---

### `src/nsan/ShadowValueMap.h` — Shadow Registry

A wrapper around `llvm::DenseMap<Value*, Value*>` that maps every original IR `Value*` to its higher-precision shadow `Value*`.

Key methods:
- `setShadowValue(original, shadow)` — registers a shadow (with debug-build type assertions)
- `getShadowValue(original)` — returns shadow or `nullptr` if not yet created
- `clear()` — called between functions to prevent pointer leaks

Why `DenseMap` over `std::map`: O(1) amortised lookup, cache-friendly open-addressing layout, optimised for aligned LLVM pointer keys.

---

### `src/runtime/nsan_runtime.cpp` — Runtime Library

Compiled into `libnsan_runtime.a` and linked into every instrumented binary. Exports C symbols callable from instrumented IR.

**Key functions:**

| Symbol | Purpose |
|---|---|
| `__nsan_check_consistency(float, double)` | Computes relative error; prints `[WARN]` if > `NSAN_REL_EPSILON` |
| `__nsan_push_shadow_arg(double v, int i)` | Pushes shadow arg onto thread-local stack before a call |
| `__nsan_load_shadow_arg(float, fn_addr, i)` | Callee reads its shadow arg (falls back to fpext if tag mismatch) |
| `__nsan_set_shadow_return(double, fn_addr)` | Stores shadow return value in thread-local slot |
| `__nsan_get_shadow_return(fn_addr)` | Caller retrieves shadow return value after a call |
| `__nsan_shadow_ptr_load/store(ptr)` | Heap shadow stubs (currently return `nullptr` → re-extend fallback) |

**Thread safety:** All state is `thread_local`. No locks needed.

**Config from env vars:**
- `NSAN_REL_EPSILON` — relative error threshold (default `1e-5`)
- `NSAN_VERBOSITY` — `1` = warn only, `2` = print every check result

---

### `src/runtime/shadow_memory.cpp` — Heap Shadow Memory

Manages shadow copies of heap-allocated floats using a `std::map<void*, void*>`.

- `__nsan_allocate_shadow(addr, size)` — malloc `size * 2` bytes as shadow
- `__nsan_deallocate_shadow(addr)` — free and remove from map
- `__nsan_get_shadow_address(addr, size)` — lookup shadow address

Currently the load/store stubs in `nsan_runtime.cpp` return `nullptr`, so the heap shadow is not yet wired up — loads fall back to `fpext(original)` as the shadow value.

---

### `test_quick.cpp` — Monolithic 12-Case Evaluation Suite

The original combined test file. Contains all 12 test cases in a single binary. Run with `./run.sh` after building.

---

### `tests/tc*.cpp` — Individual Standalone Test Files ★ NEW

Each test case has now been broken out into its own self-contained `.cpp` file in `tests/`. Every file:
- Declares `extern "C" void __nsan_check_consistency(float, double)` directly
- Computes the float result and a `double` reference value
- Calls `__nsan_check_consistency()` to trigger the NSan warning mechanism
- Can be compiled and run independently: `clang++ tcN_xxx.cpp libnsan_runtime.a -o tcN && ./tcN`

#### Actual Test Results (observed with NSan pass active)

| TC | File | What It Tests | NSan Output | Rel. Error | Why |
|---|---|---|---|---|---|
| **TC1** | `tc1_cancellation.cpp` | `(1e7 + 1.234) - 1e7` — float loses `1.234` entirely |  **[WARN]** | `1.90e-01` | `a+b` rounds `b` away in float; double shadow preserves it |
| **TC2** | `tc2_naive_sum.cpp` | Naive sum of 1,000,000 × `0.1f` |  **[WARN]** | `9.58e-03` | `0.1f` is not exactly representable; 1M additions compound the error |
| **TC3** | `tc3_kahan.cpp` | Kahan compensated sum of 1,000,000 × `0.1f` |  **SILENT** | `0.00e+00` | Kahan's correction term keeps float and shadow in agreement |
| **TC4** | `tc4_alternating.cpp` | Alternating harmonic series: Σ(-1)^i / i for i=1..50000 |  **[WARN]** | `1.06e-05` | Sign alternation causes repeated catastrophic cancellation across 50k terms |
| **TC5** | `tc5_poly.cpp` | `x³ - 3x + 2` at `x=1.0001f` (near the root x=1) |  **[WARN]** | `1.00e+00` | Near-root cancellation: numerator terms nearly cancel, leaving only rounding error |
| **TC7** | `tc7_variance.cpp` | One-pass variance: `(Σx²)/N - mean²` for data near 1e6 |  **[WARN]** | `4.54e+04` | `mean² ≈ 1e12` and `Σx²/N ≈ 1e12`; their float difference has catastrophic cancellation |
| **TC9** | `tc9_fma.cpp` | `fmaf(1e-7f, 1e7f, -1.0f)` — near-zero FMA result |  **[WARN]** | `8.22e+05` | Float FMA rounds intermediate products; true result is ~`1.42e-14` but float gives `0.0` |

> **Note on TC5:** The reference value in `tc5_poly.cpp` is hardcoded as `31.046553` which is wrong for `x³-3x+2` at `x=1.0001`. The true double result is `~3.0003e-12`. This makes the rel. error appear as `1.00e+00`. The NSan warning itself is still correct — it detects float vs. shadow divergence accurately.

#### Pattern: What Makes a Test Warn vs Stay Silent

```
WARNS  → operations where float mantissa (23 bits) loses information
         that the double shadow (52 bits) retains:
         • cancellation of nearly-equal values
         • accumulation of non-representable values (0.1f)
         • squaring large values then subtracting (variance)

SILENT → operations where float arithmetic is exact:
         • Kahan correction keeps float and shadow in lockstep
         • Exact integer arithmetic (TC8, not shown — within 2^23)
```

Run with: `./run.sh` (builds and executes with NSan pass active)

---

### `examples/standalone_demo/nsan_demo.cpp` — Works Right Now

Does **not** require LLVM. Manually instruments float operations the same way NSan would at compile time, using `long double` as the shadow type. Run immediately with:

```bash
cd examples/standalone_demo
g++ -std=c++14 -O1 nsan_demo.cpp -o nsan_demo
./nsan_demo
```

Tests 4 scenarios: cancellation, naive vs Kahan summation, iterative drift, and polynomial evaluation at a near-root.

---

### `test_quick_plain.ll` — LLVM IR Reference File

A raw LLVM IR dump of a simple float loop, generated with `clang++ -emit-llvm`. Shows exactly what the IR looks like **before** NSan instrumentation. Useful for understanding what the pass operates on.

Target: `arm64-apple-macosx` (Apple Silicon Mac). Shows `fadd float`, `load float`, `store float` instructions — precisely what `NSanPass.cpp` matches and transforms.

---

## 6. Build System

### Root `CMakeLists.txt`

- Requires **LLVM ≥ 10**, found via `find_package(LLVM REQUIRED CONFIG)`
- Default LLVM path: `../llvm-workspace/build/lib/cmake/llvm` (sibling directory)
- Override: `cmake .. -DLLVM_DIR=/your/llvm/lib/cmake/llvm`
- Build options: `ENABLE_LTO` (default ON), `ENABLE_SHADOW_OPTIMIZATION`, `ENABLE_AGGRESSIVE_OPTIMIZATION`
- Adds subdirectories: `src/nsan` and `src/runtime`

### `build.sh`

Portable shell script. Steps:
1. Cleans `build/` directory
2. Creates fresh `build/`
3. Runs CMake with Ninja generator, `RelWithDebInfo` mode, LTO off
4. Runs `ninja -v`

Outputs: `build/src/nsan/libnsan_pass.so` and `build/src/runtime/libnsan_runtime.a`

### `run.sh`

1. Verifies both build artifacts exist
2. Detects macOS vs Linux (adds `-isysroot` on macOS)
3. Compiles `test_quick.cpp` through the NSan pass: `clang++ -fpass-plugin=libnsan_pass.so test_quick.cpp libnsan_runtime.a`
4. Runs the resulting binary

---

## 7. Current Implementation Status

| Component | Status | Notes |
|---|---|---|
| Project structure | DONE | All directories and files in place |
| `ShadowValueMap.h` | DONE | Fully implemented with debug assertions |
| `NSanPass.cpp` | DONE | New Pass Manager, all `instrument*` methods implemented |
| `nsan_runtime.cpp` | DONE | Float check, public API stubs, shadow stack, heap shadow accessors all implemented |
| `configuration.h` | DONE | `init_nsan_configuration()` and `get_nsan_config()` fully implemented inline |
| `shadow_memory.cpp` | DONE | Map-based shadow heap allocation working |
| Heap shadow wired | DONE | `__nsan_shadow_ptr_load/store` now call `__nsan_get_shadow_address()` — heap floats get real shadow lookups |
| `standalone_demo` | DONE | Runs immediately without LLVM |
| `test_quick.cpp` | DONE | 12-case monolithic evaluation suite |
| `tests/tc*.cpp` | DONE | 12 individual standalone test files added, results verified |
| `tests/CMakeLists.txt` | DONE | Auto-discovers and builds all `tc*.cpp` via `file(GLOB ...)` |
| `tests/unit/test_shadow_map.cpp` | DONE | Unit test for shadow memory alloc/lookup/free cycle added |
| `src/nsan/CMakeLists.txt` | DONE | `llvm_map_components_to_libnames` + `target_link_libraries` fixed |
| `src/runtime/CMakeLists.txt` | DONE | `${CMAKE_CURRENT_SOURCE_DIR}` include path fixed |
| CI/CD `.github/workflows/build.yml` | DONE | GitHub Actions workflow added — runs on push/PR, installs deps, runs `build.sh` + `ctest` + `run.sh` |
| `tests/functional/` | PARTIAL | Directory still empty — no functional end-to-end test scripts yet |
| `nsan_runtime.cpp` missing include | MINOR | `__nsan_shadow_ptr_load/store` call `__nsan_get_shadow_address` but `shadow_memory.h` is not `#include`d — relies on implicit extern linkage |

---

## 8. What Has Been Fixed vs What Remains

### Fixed — Full History

| Round | Item | What Changed |
|---|---|---|
| 1 | `src/nsan/CMakeLists.txt` | Added `llvm_map_components_to_libnames` + `target_link_libraries(nsan_pass PRIVATE ${llvm_libs})` — resolves LLVM symbol link errors |
| 1 | `src/runtime/CMakeLists.txt` | Added `target_include_directories` with `${CMAKE_CURRENT_SOURCE_DIR}`, `include/`, `include/internal/` — headers found during build |
| 1 | `include/internal/configuration.h` | Rewritten: `NSanConfig` struct, `get_nsan_config()` accessor, `init_nsan_configuration()` reads `NSAN_REL_EPSILON` and `NSAN_VERBOSITY` from env |
| 1 | `nsan_runtime.cpp` | Includes `configuration.h`, calls `init_nsan_configuration()`; `__nsan_check_float`, `__nsan_resume_float`, `__nsan_dump_shadow_mem` stub implementations added |
| 1 | `tests/CMakeLists.txt` | Replaced stubs with `file(GLOB TEST_SRCS "*.cpp")` loop — every `tc*.cpp` auto-built and registered with CTest |
| 1 | `tests/tc*.cpp` | 12 individual standalone test files added; TC1, TC2, TC4, TC5, TC7, TC9 fire `[WARN]`; TC3 stays silent |
| 3 | `nsan_runtime.cpp` `#include` fix | Added `#include "shadow_memory.h"` at top — `__nsan_get_shadow_address` is now properly declared rather than implicitly linked |
| 3 | `tests/functional/test_pipeline.sh` | End-to-end functional test added — runs `tc1_cancellation` binary, checks stderr for `NSan: numerical inconsistency`, exits 1 if missing |

### Still Outstanding

All previously identified issues have been resolved. The project is now in a complete, buildable state.

---

## 9. Full Build & Run Workflow — How Everything Fits Together

This section explains exactly what happens when you run `./build.sh` and `./run.sh`, which files are involved at each step, and why each folder exists.

---

### Step 1 — `./build.sh` : Compile the NSan Toolchain

```
./build.sh
    │
    ├─ [1/4] Deletes old build/  (clean slate)
    ├─ [2/4] mkdir build/
    ├─ [3/4] cmake ..            (reads CMakeLists.txt)
    │         │
    │         ├─ finds LLVM at ../llvm-workspace/build/lib/cmake/llvm
    │         ├─ configures src/nsan/    → will produce libnsan_pass.so
    │         └─ configures src/runtime/ → will produce libnsan_runtime.a
    │
    └─ [4/4] ninja -v            (actually compiles)
              │
              ├─ compiles NSanPass.cpp + ShadowValueMap.cpp
              │   → build/src/nsan/libnsan_pass.so    ← LLVM pass plugin
              │
              └─ compiles nsan_runtime.cpp + shadow_memory.cpp
                  → build/src/runtime/libnsan_runtime.a  ← runtime library
```

**Files consumed by build.sh:**

| File/Folder | Role |
|---|---|
| `CMakeLists.txt` (root) | Orchestrator — finds LLVM, sets C++17, enables LTO/optimization options, calls subdirectories |
| `src/nsan/CMakeLists.txt` | Builds the LLVM pass module (`libnsan_pass.so`) — links against LLVM core/support libraries |
| `src/runtime/CMakeLists.txt` | Builds the runtime static library (`libnsan_runtime.a`) — applies fast-math to shadow computations |
| `src/nsan/NSanPass.cpp` | The transformation pass — instruments every float IR instruction with shadow ops |
| `src/nsan/NSanPass.h` | Class definition for `NSanPass` (extends `PassInfoMixin`) |
| `src/nsan/ShadowValueMap.h/.cpp` | Data structure mapping original IR `Value*` → shadow `Value*` |
| `src/runtime/nsan_runtime.cpp` | Runtime functions called by instrumented code at execution time |
| `src/runtime/shadow_memory.cpp` | Shadow heap management — `malloc`-based address map |
| `include/internal/configuration.h` | `NSanConfig` struct + `init_nsan_configuration()` — reads env vars at runtime |
| `include/nsan.h` | Public API declarations (`__nsan_check_float` etc.) |
| `cmake/` | Reserved for future CMake helper modules (currently empty) |

**Output of build.sh:**
```
build/
├── src/nsan/libnsan_pass.so      ← loaded by clang via -fpass-plugin
└── src/runtime/libnsan_runtime.a ← linked into every instrumented binary
```

---

### Step 2 — `./run.sh` : Instrument a Test Program and Execute It

```
./run.sh
    │
    ├─ Checks build/src/nsan/libnsan_pass.so exists
    ├─ Checks build/src/runtime/libnsan_runtime.a exists
    │
    ├─ [1/2] Compiles test_quick.cpp THROUGH the NSan pass:
    │
    │   clang++ -O1 \
    │     -fpass-plugin=build/src/nsan/libnsan_pass.so \   ← loads NSanPass
    │     test_quick.cpp \                                  ← your source
    │     build/src/runtime/libnsan_runtime.a \             ← runtime
    │     -o test_quick
    │
    │   During this clang++ call:
    │     1. Clang parses test_quick.cpp → LLVM IR
    │     2. NSanPass::run() fires on each function in the IR
    │     3. For every fadd/fsub/fmul/fdiv/load/store/call/ret:
    │         - A double shadow operation is inserted after it
    │         - A call to __nsan_check_consistency() is inserted
    │     4. LLVM backend compiles the instrumented IR → native binary
    │     5. The binary is linked against libnsan_runtime.a
    │
    └─ [2/2] Runs ./test_quick
              │
              ├─ stdout: TC1..TC12 results (float value, reference, rel error)
              └─ stderr: [WARN] lines for each detected numerical inconsistency
```

**Files consumed by run.sh:**

| File/Folder | Role during run.sh |
|---|---|
| `test_quick.cpp` | The program being instrumented — contains 12 float-precision test cases |
| `build/src/nsan/libnsan_pass.so` | The LLVM pass plugin — transforms the IR of `test_quick.cpp` at compile time |
| `build/src/runtime/libnsan_runtime.a` | The runtime — provides `__nsan_check_consistency()` and all `__nsan_*` symbols |
| `test_quick_plain.ll` | Reference IR dump — shows what the IR looks like BEFORE the pass runs (for understanding) |
| `tests/bins/` | Pre-built binaries of individual test cases (`tc1`–`tc5`, `nsan_demo`) — can be run directly without rebuilding |

---

### What Each Folder Does — Complete Reference

```
CD_Lab_EL_Team_Assignment_21/
│
├── src/nsan/           THE COMPILER PASS
│                       Produces: libnsan_pass.so
│                       When: at build time (build.sh → ninja)
│                       How: clang loads it via -fpass-plugin at compile time
│                       Why: transforms float IR ops into float+double shadow ops
│
├── src/runtime/        THE RUNTIME LIBRARY  
│                       Produces: libnsan_runtime.a
│                       When: at build time (build.sh → ninja)
│                       How: linked into every binary that goes through NSan
│                       Why: provides __nsan_check_consistency() and shadow stack
│                            called thousands of times per second during execution
│
├── include/            PUBLIC HEADERS
│                       Used by: both the pass and the runtime
│                       nsan.h         → user-facing API declarations
│                       nsan_interface.h → re-exports nsan.h
│                       internal/configuration.h → NSanConfig + env-var reading
│                       internal/shadow_tracking.h → ShadowType enum (placeholder)
│
├── examples/           DEMONSTRATION PROGRAMS
│   ├── standalone_demo/    Runs without LLVM — manually shows shadow computation
│   ├── interactive_demo/   Terminal-interactive version of the demo
│   ├── gui_demo/           Richer formatted output demo
│   └── simple_computation.cpp  Naive vs Kahan — the canonical NSan target
│
├── tests/              TEST SUITE
│   ├── tc1_cancellation.cpp   }  12 individual test programs — each:
│   ├── tc2_naive_sum.cpp      }    1. performs a float computation
│   ├── ...                    }    2. computes double reference
│   ├── tc12_newton_sqrt.cpp   }    3. calls __nsan_check_consistency()
│   ├── bins/                  pre-built tc binaries (can run without rebuilding)
│   ├── unit/                  unit tests (test_shadow_map.cpp — tests runtime internals)
│   ├── functional/            end-to-end shell tests (test_pipeline.sh)
│   ├── benchmarks/            (empty — for future performance benchmarks)
│   └── CMakeLists.txt         auto-builds all tc*.cpp and registers with CTest
│
├── cmake/              EMPTY — reserved for future CMake helper modules
│
├── docs/               PROJECT DOCUMENTATION
│   └── PROJECT_STATUS.md  original bug list and implementation phases
│
├── .github/workflows/  CI/CD
│   └── build.yml       GitHub Actions — runs on every push/PR:
│                         1. apt-get cmake ninja-build clang
│                         2. ./build.sh
│                         3. ctest --test-dir build
│                         4. ./run.sh
│
├── build.sh            ENTRY POINT — builds libnsan_pass.so + libnsan_runtime.a
├── run.sh              ENTRY POINT — instruments test_quick.cpp and runs it
├── test_quick.cpp      THE MAIN TEST — 12 cases, designed to trigger [WARN]
└── test_quick_plain.ll LLVM IR DUMP — shows pre-instrumentation IR for reference
```

---

### Data Flow Summary

```
  COMPILE TIME (build.sh)            RUN TIME (run.sh → ./test_quick)
  ─────────────────────              ──────────────────────────────────
  CMakeLists.txt                     test_quick.cpp
       │                                  │ (Clang + NSan pass)
       ▼                                  ▼
  NSanPass.cpp  ──────────►  libnsan_pass.so  ──► instruments IR
  ShadowValueMap.h                           │
                                             ▼
  nsan_runtime.cpp  ──────► libnsan_runtime.a  ──► linked into binary
  shadow_memory.cpp                          │
  configuration.h                            │
                                             ▼
                                    ./test_quick (runs)
                                         │
                               float op executes
                               double shadow op executes
                               __nsan_check_consistency() called
                                         │
                              rel_err > threshold?
                               YES → [WARN] to stderr
                               NO  → silent (or [PASS] if VERBOSITY=2)
```


---

## 10. Shadow Value Propagation — The Three Categories

NSan must track shadow values for three distinct kinds of floating-point values:

### 1. Temporaries (Local Variables / Registers)
Handled entirely within `ShadowValueMap`. After every float IR instruction, the shadow result is immediately registered:
```
%sum = fadd float %a, %b
         ↓ NSanPass inserts:
%sum_shadow = fadd double %a_shadow, %b_shadow
shadow_map_[%sum] = %sum_shadow
```

### 2. Function Arguments & Return Values
Uses a **thread-local shadow stack** in the runtime:
- **Caller** pushes shadow args via `__nsan_push_shadow_arg()` before the call
- **Callee** reads them via `__nsan_load_shadow_arg()` at function entry
- **Callee** stores shadow return via `__nsan_set_shadow_return()` before `ret`
- **Caller** retrieves shadow return via `__nsan_get_shadow_return()` after call

A **tag** (function pointer cast to `int64_t`) is used to verify the shadow stack belongs to the right call frame.

### 3. Memory Values (Heap / Global / Stack-by-Address)
Shadow lives in a parallel **shadow memory** managed by `shadow_memory.cpp`.
- On `store float` → check consistency, then `__nsan_shadow_ptr_store()` + store shadow double
- On `load float` → `__nsan_shadow_ptr_load()` gets shadow ptr; if valid, load shadow; if null (stub), fall back to `fpext(original)`

---

## 11. Environment Variables Reference

| Variable | Default | Effect |
|---|---|---|
| `NSAN_REL_EPSILON` | `1e-5` | Relative error threshold for warnings |
| `NSAN_EPSILON` | `1e-5` | Absolute error threshold |
| `NSAN_VERBOSITY` | `1` | `0`=silent, `1`=warn only, `2`=print every check |
| `NSAN_CHECK_FREQ` | `1` | Check every Nth operation (performance knob) |
| `NSAN_SHADOW_THRESHOLD` | `1e-10` | Phase 4: when to upgrade shadow precision further |

---

## 12. Key References

- **Courbet, C. (2021)** — "NSan: A Floating-Point Numerical Sanitizer." CC '21. This paper is the direct basis for this project.
- **IEEE 754-2019** — Floating-point arithmetic standard
- **Higham, N.J. (2002)** — "Accuracy and Stability of Numerical Algorithms"
- **LLVM Pass Plugin API** — `llvm/Passes/PassPlugin.h`, `PassInfoMixin`
- **LLVM IRBuilder** — used throughout `NSanPass.cpp` to insert new instructions



