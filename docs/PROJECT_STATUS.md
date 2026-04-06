# NSan: Project Status & Implementation Guide

**Date**: 2026-04-06 | **Phase**: Pre-Phase 1 (Structure Complete, Code Stubs Only)

---

## 1. How This Project Actually Works

This is the most important section — understanding the execution flow end-to-end.

### What NSan Is

NSan is a **compiler plugin + runtime library**. It does not run alone. It hooks into LLVM's compilation pipeline and transforms your program *before* it becomes machine code.

### Full Pipeline

```
Your C++ file (e.g. simple_computation.cpp)
        │
        ▼
   clang++ frontend
        │  (parses C++ → LLVM IR)
        ▼
  LLVM IR (intermediate)
  e.g.:  %sum = fadd float %a, %b
        │
        ▼
  NSanPass runs (YOUR CODE — src/nsan/NSanPass.cpp)
        │  Transforms IR: adds shadow double computation after each float op
        │  e.g.:  %sum        = fadd float  %a,        %b
        │         %a_shadow   = fpext float %a to double
        │         %b_shadow   = fpext float %b to double
        │         %sum_shadow = fadd double %a_shadow, %b_shadow
        │         call __nsan_check_consistency_float(%sum, %sum_shadow)
        ▼
  LLVM backend → machine code binary
        │
        ▼
  Run the binary
        │  Each float op now also runs a double shadow op
        │  __nsan_check_consistency_float() fires after each one
        ▼
  nsan_runtime.cpp runs at each check:
        │  compares float result vs double shadow
        │  if relative error > threshold → print warning to stderr
        ▼
  Output: "NSan Warning: Numerical inconsistency detected at ..."
```

### How to Actually Compile with the Pass (when implemented)

The `-fsanitize=numerical` flag in examples does NOT currently exist in clang.
The real invocation to load a custom LLVM pass is:

```bash
# Step 1: Build NSan (produces libnsan_pass.so and libnsan_runtime.a)
mkdir build && cd build && cmake .. && make

# Step 2: Compile your program through the NSan pass
clang++ -O1 \
  -Xclang -load -Xclang ./build/src/nsan/libnsan_pass.so \
  simple_computation.cpp \
  -L./build/src/runtime -lnsan_runtime \
  -o simple_instrumented

# Step 3: Run — NSan checks happen automatically
./simple_instrumented
NSAN_VERBOSITY=2 ./simple_instrumented   # more detail
NSAN_EPSILON=1e-6 ./simple_instrumented  # stricter threshold
```

### What "Verification" Means Here

There are two layers:

| Layer | When | Who does it | What it checks |
|-------|------|-------------|----------------|
| **Compile-time** | During `clang++` | NSanPass.cpp (you write this) | Transforms IR to add shadow math |
| **Runtime** | When binary executes | nsan_runtime.cpp | Compares float vs. double shadow — reports errors |

There is no separate "verification tool" to run. The instrumented binary self-checks as it runs.

---

## 2. What Is Currently Present (Stubs Only)

### Files Present and Their State

| File | State | Notes |
|------|-------|-------|
| `CMakeLists.txt` (root) | Functional but has issues (see §3) | Correct structure, minor bugs |
| `src/nsan/NSanPass.cpp` | **Stub** — only registers the pass | `runOnFunction()` is declared but NOT defined |
| `src/nsan/NSanPass.h` | Good skeleton | All methods declared, none implemented |
| `src/nsan/ShadowValueMap.cpp` | Near-empty | Only `#include` — all logic is in the header |
| `src/nsan/ShadowValueMap.h` | **Fully implemented** | This one is complete and correct |
| `src/nsan/CMakeLists.txt` | Missing LLVM link | Will fail to build — no `llvm_map_components_to_libnames` |
| `src/runtime/nsan_runtime.cpp` | Partially implemented | `__nsan_check_consistency_float` works; `__nsan_check_consistency_double` is TODO |
| `src/runtime/shadow_memory.cpp` | Implemented | Basic map-based shadow allocation works |
| `src/runtime/nsan_runtime.h` | Has issues (see §3) | `func_addr` type is wrong |
| `src/runtime/shadow_memory.h` | Clean | Fine as-is |
| `src/runtime/CMakeLists.txt` | Missing self-include path | Headers in same dir not on include path |
| `include/nsan.h` | Clean | Public API declared correctly |
| `include/nsan_interface.h` | Near-empty | Just re-includes nsan.h — placeholder only |
| `include/internal/configuration.h` | Declares but has no impl | `init_nsan_configuration()` never defined anywhere |
| `include/internal/shadow_tracking.h` | Minimal | ShadowType enum exists but is not used anywhere yet |
| `tests/CMakeLists.txt` | All commented out | No tests wired up |
| `tests/unit/` | Empty | No test files |
| `tests/functional/` | Empty | No test files |
| `tests/benchmarks/` | Empty | No benchmark files |
| `examples/simple_computation.cpp` | **Fully working** | Compiles standalone, demonstrates the problem NSan should detect |
| `.github/workflows/` | Empty | No CI/CD configured |
| `cmake/` | Empty | LLVMConfig.cmake not written |
| `docs/` | Empty (until this file) | No architecture or API docs |

---

## 3. Bugs and Issues Found

### CMake Issues

**Bug 1 — `src/nsan/CMakeLists.txt` will not link LLVM properly**
```cmake
# Current (broken):
add_library(nsan_pass MODULE NSanPass.cpp ShadowValueMap.cpp)

# Needs these additions:
llvm_map_components_to_libnames(llvm_libs support core irreader passes)
target_link_libraries(nsan_pass PRIVATE LLVMCore LLVMSupport)
```
Without linking against LLVM libraries, the MODULE will have unresolved symbols at load time.

**Bug 2 — `enable_testing()` ordering in root CMakeLists.txt**

`enable_testing()` is called *after* `add_subdirectory(tests)`. Move it before the subdirectory additions. Low severity but non-standard.

**Bug 3 — `-fno-rtti -fno-exceptions` applied globally**

These flags are needed for the LLVM pass (which is correct — LLVM itself is built without RTTI/exceptions). But `nsan_runtime` is a normal C++ library and globally disabling exceptions is not a problem here. However, if tests use Google Test (which requires RTTI), this will break. The flags should be scoped to the pass target only.

**Bug 4 — `src/runtime/CMakeLists.txt` missing self-include path**
```cmake
# Missing:
target_include_directories(nsan_runtime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}   # <-- needed for nsan_runtime.h, shadow_memory.h
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/internal
)
```

### C++ Code Issues

**Bug 5 — `using namespace llvm;` in headers (NSanPass.h, ShadowValueMap.h)**

Never put `using namespace` in a header. Any file that includes these headers will have the entire `llvm` namespace injected, causing potential name collisions. Use `llvm::Value`, `llvm::Function`, etc. explicitly, or restrict `using` to `.cpp` files.

**Bug 6 — `runOnFunction()` declared but never defined**

`NSanPass.h` declares `bool runOnFunction(Function &F) override;` and `NSanPass.cpp` does not define it. The project will not link until this is implemented.

**Bug 7 — Wrong type for `func_addr` in `nsan_runtime.h`**
```cpp
// Current (wrong):
void __nsan_push_shadow_parameters(int func_addr, ...);

// Should be:
void __nsan_push_shadow_parameters(void *func_addr, ...);
// or better, use a function pointer:
void __nsan_push_shadow_parameters(uintptr_t func_addr, ...);
```
Using `int` to hold a pointer is UB on 64-bit platforms.

**Bug 8 — `configuration.h` declares `init_nsan_configuration()` but nothing implements it**

`nsan_runtime.cpp` sets its own static globals for epsilon/verbosity, but these are never connected to environment variables at startup. `init_nsan_configuration()` is declared but has no `.cpp` anywhere.

**Bug 9 — `shadow_memory.cpp` allocates `size * 2` bytes without a comment**

For `float` (4 bytes) → `double` (8 bytes), you need `size * 2` bytes. But for `double` → `long double`/`fp128`, you may need more. This should be parameterised or at least commented clearly.

**Bug 10 — `__nsan_check_float`, `__nsan_dump_shadow_mem`, `__nsan_resume_float` declared in `nsan.h` and `nsan_runtime.h` but never implemented**

These user-callable functions exist in both headers but have zero implementation in any `.cpp` file.

**Note on `simple_computation.cpp`:**
The comment says `clang++ -fsanitize=numerical simple_computation.cpp` but this flag does not exist. It should say:
```bash
# Without NSan (standalone demo):
clang++ simple_computation.cpp -o simple && ./simple

# With NSan (once implemented):
clang++ -Xclang -load -Xclang ./build/src/nsan/libnsan_pass.so \
  simple_computation.cpp -L./build/src/runtime -lnsan_runtime -o simple
```

### New Pass Manager (Important)

The current pass uses the **legacy pass manager** API (`FunctionPass`, `RegisterPass<>`). LLVM 14+ prefers the **new pass manager**. While the legacy PM still works, for future-proofing the pass should also register itself as a `PassPlugin`:

```cpp
// Add to NSanPass.cpp for new PM compatibility:
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

llvm::PassPluginLibraryInfo getNSanPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "NSan", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(/* ... */);
          }};
}
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getNSanPluginInfo();
}
```

---

## 4. What Needs To Be Implemented (in order)

### Phase 1 — Make it compile (Week 1-2)

1. **Fix `src/nsan/CMakeLists.txt`** — add LLVM link libraries
2. **Fix `src/runtime/CMakeLists.txt`** — add `${CMAKE_CURRENT_SOURCE_DIR}` to includes
3. **Implement `runOnFunction()` in NSanPass.cpp** — iterate over instructions, detect FP ops, call instrumentation methods (patterns are in `private_config/SKILLS.md`)
4. **Fix `using namespace llvm` in headers** — replace with explicit `llvm::` prefix

### Phase 2 — Shadow computation works (Week 3-4)

5. **Implement `instrumentBinaryOp()`** — see pattern in `private_config/SKILLS.md`
6. **Implement `getShadowValue()` / `createShadowValue()`** — use `IRBuilder::CreateFPExt()`
7. **Implement `getShadowType()`** — map float→double, double→fp128
8. **Wire `insertCheck()`** — insert calls to `__nsan_check_consistency_float` into IR

### Phase 3 — Runtime is complete (Week 5-6)

9. **Implement `init_nsan_configuration()`** — read `NSAN_EPSILON`, `NSAN_REL_EPSILON`, `NSAN_VERBOSITY` from `getenv()` and update the runtime statics
10. **Implement `__nsan_check_consistency_double()`** — same pattern as `_float` version
11. **Implement `__nsan_check_float()`, `__nsan_dump_shadow_mem()`, `__nsan_resume_float()`** — user-facing functions
12. **Implement `__nsan_on_load()` / `__nsan_on_store()`** — load/store shadow propagation

### Phase 4 — Tests and CI (Week 7-10)

13. **Write unit tests** (`tests/unit/`) using a test framework (GTest or plain executable)
14. **Wire up `tests/CMakeLists.txt`** — uncomment and configure targets
15. **Write `.github/workflows/build.yml`** — CI on Linux with LLVM installed
16. **Validate with `simple_computation.cpp`** — NaiveSum should trigger warning; KahanSum should not

---

## 5. Files That Do Not Exist Yet (referenced but missing)

These are mentioned in `private_config/` docs but not present in the repo:

| Missing File | Referenced In | Priority |
|---|---|---|
| `src/runtime/diagnostics.cpp` | INSTRUCTIONS.md Phase 4 | Medium |
| `src/runtime/configuration.cpp` | INSTRUCTIONS.md Phase 4 | High (fixes Bug 8) |
| `tests/unit/test_instrumentation.cpp` | PROJECT_STRUCTURE_GUIDE.md | High |
| `tests/unit/test_shadow_values.cpp` | PROJECT_STRUCTURE_GUIDE.md | High |
| `tests/unit/test_memory.cpp` | PROJECT_STRUCTURE_GUIDE.md | Medium |
| `tests/functional/test_naive_summation.cpp` | PROJECT_STRUCTURE_GUIDE.md | High |
| `tests/functional/test_kahan_summation.cpp` | PROJECT_STRUCTURE_GUIDE.md | High |
| `tests/benchmarks/benchmark_compensated_sum.cpp` | PROJECT_STRUCTURE_GUIDE.md | Low |
| `examples/matrix_determinant.cpp` | PROJECT_STRUCTURE_GUIDE.md | Low |
| `examples/iterative_solver.cpp` | PROJECT_STRUCTURE_GUIDE.md | Low |
| `.github/workflows/build.yml` | PROJECT_STRUCTURE_GUIDE.md | High |
| `docs/architecture.md` | PROJECT_STRUCTURE_GUIDE.md | Medium |
| `docs/api_reference.md` | PROJECT_STRUCTURE_GUIDE.md | Medium |
| `CONTRIBUTING.md` | PROJECT_STRUCTURE_GUIDE.md | Low |
| `CHANGELOG.md` | PROJECT_STRUCTURE_GUIDE.md | Low |
| `ROADMAP.md` | PROJECT_STRUCTURE_GUIDE.md | Low |

---

## 6. What the `private_config/` Docs Are For

The docs in `private_config/` are your **reference manuals** — not code. They explain:

| File | Purpose |
|------|---------|
| `INSTRUCTIONS.md` | Step-by-step implementation walkthrough per phase |
| `SKILLS.md` | Copy-paste code patterns for LLVM transformations |
| `QUICK_REFERENCE.md` | Cheatsheet for LLVM IR, build commands, debug tips |
| `PROJECT_STRUCTURE_GUIDE.md` | Where every file goes and what to create |
| `INDEX.md` | Navigation map to all docs |
| `REPO_SETUP_GUIDE.md` | Git workflow, branching, commit format |
| `CLAUDE_CODE_INTEGRATION.md` | How to use Claude Code with this project |

You should keep these in `private_config/` (they're already `.gitignore`d or should be) as they're AI-guidance files, not part of the source deliverable.

---

## 7. Summary Scorecard

| Category | Status |
|----------|--------|
| Project structure | Complete |
| CMake build system | Has bugs, will not build as-is |
| LLVM pass framework | Declared only, not implemented |
| Runtime consistency checking | ~40% done (float check works, rest missing) |
| Shadow memory | Basic version works |
| Configuration system | Declared, not connected |
| Public API | Declared, not implemented |
| Tests | 0% — all files missing |
| CI/CD | 0% — workflow file empty |
| Examples | 1 good standalone example |
| Documentation | This file + private_config reference |

**Current build status: Will NOT compile** due to missing `runOnFunction()` definition and LLVM link issue in CMakeLists.
