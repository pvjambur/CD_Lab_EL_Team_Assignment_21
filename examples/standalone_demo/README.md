# standalone_demo

Runnable demo — no LLVM required. Shows exactly what NSan detects.

## Build & Run

```bash
g++ -std=c++14 -O1 nsan_demo.cpp -o nsan_demo.exe
./nsan_demo.exe
```

Or with `make`:
```bash
make run
make run-verbose       # full per-op trace
make run-strict        # tighter threshold
```

## What it shows

| Test | What happens |
|------|-------------|
| Catastrophic cancellation | `(1e7 + 1.234) - 1e7` → float says `1`, truth is `1.234` |
| Naive vs Kahan sum | Naive sum of 10k large floats drifts 138ppm; Kahan stays exact |
| Iterative drift | 10k small additions to a large base — cumulative float error |
| Polynomial eval | `p(golden_ratio)` — float vs double vs long double accuracy |
