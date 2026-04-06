# src/nsan/

The LLVM instrumentation pass.

**What it does:** walks every function in LLVM IR, finds floating-point operations
(`fadd`, `fsub`, `fmul`, `fdiv`), and inserts a parallel shadow computation in
higher precision + a call to `__nsan_check_consistency_float()`.

**Status:** skeleton only — `runOnFunction()` is declared but not implemented.

## Key files

| File | Role |
|------|------|
| `NSanPass.cpp` | Pass entry point — implement `runOnFunction()` here first |
| `NSanPass.h` | Class declaration — all methods to implement |
| `ShadowValueMap.h` | Maps original IR values to their shadow values (complete) |
| `ShadowValueMap.cpp` | Placeholder (logic lives in the header) |
| `CMakeLists.txt` | Builds `libnsan_pass.so` — the loadable LLVM plugin |

## Build requirement

Needs LLVM dev libraries (`llvm-dev`, `clang`). Not buildable on plain Windows
without WSL or a Linux VM. See `docs/PROJECT_STATUS.md` for build instructions.
