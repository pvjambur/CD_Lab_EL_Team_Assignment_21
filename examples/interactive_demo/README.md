# interactive_demo

Full interactive terminal app for NSan. Menu-driven, colour-coded, no extra libraries.

## Build & Run

```bash
g++ -std=c++14 -O1 nsan_interactive.cpp -o nsan_interactive.exe
./nsan_interactive.exe
```

## Menu Options

| Option | What it does |
|--------|-------------|
| `[1]` Pre-built Tests | Runs catastrophic cancellation + summation demos |
| `[2]` Custom Computation | Enter your own A, B — pick `+`,`-`,`*`,`/`, or `(A+B)-A` |
| `[3]` Summation Explorer | Choose N and value range, compare naive vs Kahan |
| `[4]` View Log | Table of all computations done this session |
| `[5]` Settings | Change the warning threshold (default 1e-4) |
| `[6]` How NSan Works | Shows the actual LLVM IR transformation NSan injects |
| `[7]` About | Project explanation |
