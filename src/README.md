# src/

Source code for NSan. Two subdirectories:

| Directory | What it is |
|-----------|-----------|
| `nsan/` | The LLVM compiler pass — transforms float ops at compile time |
| `runtime/` | The runtime library — linked into instrumented binaries |

Neither compiles standalone yet. See `examples/standalone_demo/` for a working demo.
