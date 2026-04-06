# gui_demo

Real Windows GUI application. Double-click the `.exe` or build and run.

## Build

```bash
g++ -std=c++14 -O1 nsan_gui.cpp -o nsan_gui.exe -lcomctl32 -mwindows
```

## Run

```bash
./nsan_gui.exe
# or just double-click nsan_gui.exe in Explorer
```

## Buttons

| Button | What it does |
|--------|-------------|
| Run All Tests | Cancellation + summation demos in one click |
| Cancellation Demo | Shows `(A+B)-A` precision loss |
| Summation (N=5000) | Naive vs Kahan accuracy comparison |
| Custom Computation | Input your own A, B, pick operation |
| Summation Explorer | Choose N and value range interactively |
| View LLVM IR | Popup showing what NSan injects at compile time |
| Settings | Change warning threshold |
| About NSan | Project explanation |
| Clear Log | Wipe the output panel and reset counters |
