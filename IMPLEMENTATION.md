# NSan Implementation

The implementation consists of two main components: an LLVM compiler instrumentation pass and a linked runtime library.

## LLVM Pass Details
The LLVM pass (`NSanPass`) operates at the function level. It iterates through all instructions in the IR using the LLVM New Pass Manager.

- **Type Promotion**: For every `float` value, a `double` shadow value is instantiated. `double` and `x86_fp80` are promoted to `fp128`.
- **Instruction Shadowing**: Binary operations (`FAdd`, `FSub`, `FMul`, `FDiv`) and Cast operations (`FPExt`, `FPTrunc`) are duplicated.
- **Local Temporaries Tracking**: A `DenseMap` based `ShadowValueMap` tracks the original `Value*` mapped to its shadow `Value*` for local temporaries, keeping lookups fast and cache-friendly.
- **Memory Operations**: On `store float`, we inject a call to `__nsan_check_consistency()`, comparing the original value and the shadow value. We then store the shadow value in parallel shadow memory via `__nsan_shadow_ptr_store`.
- **Function Boundaries**: 
  - To pass shadow arguments, we use a thread-local shadow stack managed by `__nsan_push_shadow_arg` and `__nsan_load_shadow_arg`.
  - A tag-based check (using the function address) verifies caller-callee alignment.
  - Return values are shadowed using a thread-local slot, managed by `__nsan_set_shadow_return` and `__nsan_get_shadow_return`.

## Runtime Library
The runtime library (`libnsan_runtime.a`) is linked into the final binary. It exposes C-ABI symbols (`extern "C"`) that the LLVM pass injects calls to.

- **Consistency Checking**: `__nsan_check_consistency` calculates the relative error between the original value and the shadow value. If `rel_error > NSAN_REL_EPSILON` (configurable via environment, default `1e-5`), it prints a `[WARN]` to stderr.
- **Shadow Memory Allocation**: Managed via a `std::map<void*, void*>` in `shadow_memory.cpp` mapping application addresses to shadow memory addresses. In a real-world high-performance implementation, this is often done using bit-masking/offset addressing mapping a fixed region, but here map-based allocation handles the tracking safely.

## Wrapper Script Integration
To satisfy the `-fsanitize=numerical` interface standard expected from LLVM sanitizers without modifying the clang driver, the project provides an `nsan-clang++` compiler wrapper script. This script intercepts `-fsanitize=numerical`, seamlessly replacing it with the actual `-fpass-plugin` and linking flags, tying the compiler and runtime together.
