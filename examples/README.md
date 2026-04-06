# examples/

Example programs demonstrating numerical instability problems that NSan detects.

| Directory / File | Status | Requires |
|------------------|--------|---------|
| `standalone_demo/` | **Working now** | `g++` only |
| `simple_computation.cpp` | Compiles standalone | `g++` or `clang++` |
| `matrix_determinant.cpp` | Not written yet | — |
| `iterative_solver.cpp` | Not written yet | — |

## Quick start

```bash
cd standalone_demo
g++ -std=c++14 -O1 nsan_demo.cpp -o nsan_demo.exe
./nsan_demo.exe
```
