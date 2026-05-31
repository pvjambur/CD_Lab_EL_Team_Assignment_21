# NSan: Numerical Stability Sanitizer

## Project Overview

NSan is a floating-point numerical sanitizer designed to automatically detect and debug numerical instabilities in C/C++ programs. It uses compile-time instrumentation to augment floating-point computations with higher-precision shadow values, enabling developers to identify precision loss and numerical errors without requiring extensive manual analysis.

### Key Characteristics

- **Performance-Oriented**: 1-4 orders of magnitude faster than existing approaches (FpDebug, Verificarlo)
- **Practical Debugging**: Provides precise, actionable feedback with minimal false positives
- **Compiler Integration**: Implemented within the LLVM sanitizer framework for seamless integration
- **Scalable**: Supports multi-threaded applications (unlike Valgrind-based tools)
- **Production-Ready**: Can be run routinely as part of unit tests and on large applications

## Technical Stack

### Core Components

1. **LLVM Infrastructure**
   - LLVM IR (Intermediate Representation) instrumentation
   - Compile-time transformation passes
   - Native code generation for shadow computations
   - Version: LLVM 10+ recommended

2. **Programming Language**
   - C++ for implementation
   - C/C++ for target applications being analyzed

3. **Runtime Components**
   - Shadow memory management
   - Shadow stack for parameter/return value tracking
   - Runtime checking and diagnostic reporting

4. **Build System**
   - CMake 3.12+
   - Standard C++17 compiler

### Dependencies

- LLVM development libraries (llvm-dev)
- Clang compiler toolchain
- Standard C library with sanitizer support
- Thread-local storage support (for multi-threading)

## Project Structure

```
nsan-project/
├── README.md                 # Project overview
├── DESIGN.md                 # Approach and alternative solutions
├── IMPLEMENTATION.md         # LLVM-specific details
├── EVALUATION.md             # Metrics, comparisons, and test cases
├── DEMO.md                   # Video/screenshot demonstrations
├── src/
│   ├── nsan/                 # LLVM transformation pass
│   └── runtime/              # Runtime library
├── include/                  # Public API
├── tests/                    # 10+ test cases
├── examples/                 # Example standalone codes
├── CMakeLists.txt            # Root build config
├── build.sh                  # Build script
├── run.sh                    # Test runner
├── nsan-clang++              # Compiler wrapper
└── benchmark.sh              # Performance benchmarking script
```

## Getting Started

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install llvm-dev clang cmake

# macOS
brew install llvm cmake

# From source
# Clone LLVM and build with NSan pass
```

### Building the Project

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
make test  # Run test suite
```

### Basic Usage

```bash
# Compile with NSan instrumentation using our wrapper
./nsan-clang++ -fsanitize=numerical my_program.cpp -o my_program

# Run with default epsilon tolerance
./my_program

# Run with custom tolerance
NSAN_EPSILON=1e-6 ./my_program

# Generate detailed shadow memory output
NSAN_VERBOSITY=2 ./my_program
```

## Core Concepts

### Shadow Values

Every floating-point value maintains a parallel "shadow" in higher precision:
- `float` → `double` shadow
- `double` → `fp128` (quad) shadow
- `long double` → `fp128` shadow

This enables comparison for consistency checking without expensive runtime calls.

### Shadow Value Categories

1. **Temporary Values**: Local variables in functions
2. **Parameter Values**: Function arguments (stored on shadow stack)
3. **Return Values**: Function results (stored in shadow return slot)
4. **Memory Values**: Values in application memory (tracked in shadow memory)

### Consistency Checks

1. **Observable Value Consistency**: Checked at function calls, returns, and memory stores
2. **Branch Consistency**: Verified for floating-point comparisons
3. **Load Consistency**: Validated when loading from memory



## Performance Characteristics

### Benchmark Results (from research)

| Approach | Slowdown (1 sample) | Total Analysis |
|----------|-------------------|-----------------|
| Original Program | 1.0x | 1.0x |
| NSan (double shadow) | 2.3x | 2.3x |
| NSan (quad shadow) | 17.2x | 17.2x |
| FpDebug | 476.6x | 476.6x |
| Verificarlo (MCA) | 40.0x | 40,000x |

### Scalability
- Linear scaling with problem size
- Multi-threaded support (unlike Valgrind-based tools)
- Scales to large production applications

## Key Features

### 1. Practical Debuggability
- Pinpoints exact source locations of numerical errors
- Provides stack traces with symbol information
- Reports relative error percentages and ULP (Units in the Last Place) differences

### 2. False Positive Reduction
- Only checks observable values (values escaping functions)
- Understands semantics of standard library functions
- Tracks memory type information to avoid spurious warnings

### 3. User Control
```cpp
// Runtime flags
NSAN_EPSILON=1e-5        // Absolute tolerance
NSAN_REL_EPSILON=1e-3    // Relative tolerance
NSAN_VERBOSITY=2         // Diagnostic detail level

// Suppression files
// Disable warnings in specific functions/files
function_name
path/to/file.cpp
```

### 4. External Library Interaction
- Seamless integration with non-instrumented libraries
- Correct handling of math library functions
- Type-aware memory operations through shadow tagging

## Compilation Strategy

### LLVM IR Instrumentation Flow

```
Source Code (C/C++)
       ↓
   [Clang Frontend]
       ↓
   LLVM IR
       ↓
   [NSan Pass]
   - Add shadow value tracking
   - Instrument operations
   - Insert consistency checks
       ↓
   Instrumented IR
       ↓
   [LLVM Backend]
       ↓
   Native Code
       ↓
   Executable with NSan Runtime
```

## Applications & Use Cases

1. **Numerical Libraries**: BLAS, LAPACK, linear solvers
2. **Scientific Computing**: Physics simulations, molecular dynamics
3. **Machine Learning**: Neural network computations, training
4. **Financial Computing**: Portfolio optimization, risk calculations
5. **Computer Graphics**: Rendering algorithms, matrix operations
6. **Control Systems**: PID controllers, signal processing

## Contributing Guidelines

1. Follow LLVM coding standards
2. Add tests for new instrumentation patterns
3. Benchmark performance impact
4. Update documentation for new features
5. Ensure thread-safety of runtime components

## References

- Courbet, C. (2021). "NSan: A Floating-Point Numerical Sanitizer." CC '21: 30th ACM SIGPLAN International Conference on Compiler Construction.
- IEEE 754-2019: Floating-point arithmetic standard
- Higham, N. J. (2002). "Accuracy and Stability of Numerical Algorithms" (2nd ed.)
- Verificarlo: https://github.com/verificarlo/verificarlo
- LLVM Sanitizers: https://clang.llvm.org/docs/Sanitizers/

## License

This project is based on research from Google LLC and follows the same licensing model as LLVM.

## Support & Documentation

- **Design**: See `DESIGN.md`
- **Implementation**: See `IMPLEMENTATION.md`
- **Evaluation**: See `EVALUATION.md`
- **API Documentation**: See `include/nsan.h`

---

**Status**: Active Development | **Last Updated**: 2026 | **Maintainers**: Compiler Lab Team
