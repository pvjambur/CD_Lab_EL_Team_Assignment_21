# include/

Public and internal headers.

| File/Dir | Who uses it |
|----------|-------------|
| `nsan.h` | User code — explicit NSan API (`__nsan_check_float`, etc.) |
| `nsan_interface.h` | Instrumented binaries — full set of injected call signatures |
| `internal/configuration.h` | Runtime — default epsilon/verbosity constants |
| `internal/shadow_tracking.h` | Runtime — shadow type encoding enum |
