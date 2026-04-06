# src/runtime/

The NSan runtime library — linked into every binary the pass instruments.

**What it does:** provides the `__nsan_check_consistency_float()` function that
the instrumented code calls after every float operation to compare the original
value against its double/long-double shadow.

**Status:** `__nsan_check_consistency_float` is implemented. Other functions
(shadow memory, function-boundary stack, user API) are declared but empty.

## Key files

| File | Role |
|------|------|
| `nsan_runtime.cpp` | Consistency checking — the core check is done |
| `nsan_runtime.h` | Full runtime API declaration |
| `shadow_memory.cpp` | Map-based shadow allocation for heap memory |
| `shadow_memory.h` | Shadow memory API |
| `CMakeLists.txt` | Builds `libnsan_runtime.a` (static) |
