# NSan Implementation Instructions

Complete technical guide for building the Numerical Stability Sanitizer from scratch.

---

## Table of Contents

1. [Project Setup](#project-setup)
2. [Architecture Overview](#architecture-overview)
3. [Component Breakdown](#component-breakdown)
4. [Implementation Steps](#implementation-steps)
5. [LLVM Pass Development](#llvm-pass-development)
6. [Runtime Library Development](#runtime-library-development)
7. [Testing Strategy](#testing-strategy)
8. [Performance Optimization](#performance-optimization)
9. [Integration Guide](#integration-guide)
10. [Troubleshooting](#troubleshooting)

---

## Project Setup

### Prerequisites

```bash
# LLVM Development (choose one method)

# Method 1: Install from package manager (Recommended for start)
sudo apt-get install llvm-dev clang-dev cmake

# Method 2: Build LLVM from source (For customization)
git clone https://github.com/llvm/llvm-project.git
cd llvm-project && mkdir build && cd build
cmake -G Ninja ../llvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang
ninja && ninja install

# Verify installation
llvm-config --version
clang++ --version
```

### Directory Structure

```
nsan-project/
├── CMakeLists.txt                  # Build configuration
├── README.md                        # Overview
├── PROBLEM_STATEMENT.md            # Problem explanation
├── SKILLS.md                       # Development guide
├── INSTRUCTIONS.md                 # This file
│
├── src/
│   ├── nsan/
│   │   ├── CMakeLists.txt
│   │   ├── NSanPass.cpp            # Main LLVM pass
│   │   ├── NSanPass.h
│   │   ├── ShadowValueMap.cpp      # Shadow tracking
│   │   └── ShadowValueMap.h
│   │
│   ├── runtime/
│   │   ├── CMakeLists.txt
│   │   ├── nsan_runtime.cpp        # Runtime support
│   │   ├── nsan_runtime.h
│   │   ├── shadow_memory.cpp       # Memory management
│   │   ├── shadow_memory.h
│   │   ├── diagnostics.cpp         # Error reporting
│   │   └── diagnostics.h
│   │
│   └── lib/
│       ├── InterflowInterface.cpp   # Compiler integration
│       └── SanitizerCommon.cpp      # Common utilities
│
├── include/
│   ├── nsan.h                      # Public API
│   ├── nsan_interface.h            # User-callable functions
│   └── internal/
│       ├── shadow_tracking.h
│       └── configuration.h
│
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_instrumentation.cpp
│   │   ├── test_shadow_values.cpp
│   │   └── test_memory.cpp
│   │
│   ├── functional/
│   │   ├── test_kahan_summation.cpp
│   │   ├── test_naive_summation.cpp
│   │   ├── test_matrix_ops.cpp
│   │   └── test_cancelation.cpp
│   │
│   └── benchmarks/
│       ├── benchmark_compensated_sum.cpp
│       └── benchmark_real_world.cpp
│
├── examples/
│   ├── simple_computation.cpp
│   ├── matrix_determinant.cpp
│   └── iterative_solver.cpp
│
├── docs/
│   ├── architecture.md
│   ├── ir_transformations.md
│   ├── memory_model.md
│   └── api_reference.md
│
└── cmake/
    ├── LLVMConfig.cmake
    └── FindLLVM.cmake
```

### Initial CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)
project(NSan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add main components
add_subdirectory(src/nsan)
add_subdirectory(src/runtime)
add_subdirectory(tests)

# Enable testing
enable_testing()
```

---

## Architecture Overview

### High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                    User Program (C/C++)                     │
│        (Contains floating-point computations)               │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│               Clang Frontend + Preprocessor                 │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      LLVM IR (Unoptimized)                  │
│         (Floating-point ops: fadd, fsub, fmul, etc.)       │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                    NSan LLVM Pass                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ 1. Detect floating-point operations                  │  │
│  │ 2. Create shadow value tracking                      │  │
│  │ 3. Insert parallel shadow computations               │  │
│  │ 4. Instrument function boundaries (params/returns)   │  │
│  │ 5. Instrument memory operations (loads/stores)       │  │
│  │ 6. Insert consistency check calls                    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   Instrumented LLVM IR                      │
│    (Each float op has parallel double shadow op)            │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│              LLVM Backend + Optimizer                       │
│       (Lowering to machine code, applying optimizations)    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                    Compiled Binary                          │
│  (Executable linked with NSan runtime library)              │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                  NSan Runtime Library                       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ • Shadow Memory Management                           │  │
│  │ • Shadow Stack for function parameters               │  │
│  │ • Consistency Checking                               │  │
│  │ • Diagnostic Generation & Reporting                  │  │
│  │ • Configuration Management                           │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
        ┌───────────────────┬───────────────────┐
        ↓                   ↓                   ↓
    stdout            stderr (warnings)    log files
```

### Data Flow: Shadow Values

```
Input: float x = a + b

Step 1: Original Computation
┌─────────────────────────────────────┐
│ %a = load float ...                 │
│ %b = load float ...                 │
│ %x = fadd float %a, %b              │ ← Original 32-bit op
│ store float %x ...                  │
└─────────────────────────────────────┘

Step 2: Shadow Computation (Instrumented)
┌──────────────────────────────────────┐
│ %a_shadow = fpext float %a to double │ ← Extend to 64-bit
│ %b_shadow = fpext float %b to double │ ← Extend to 64-bit
│ %x_shadow = fadd double %a_s %b_s   │ ← Shadow 64-bit op
│ store double %x_shadow ...           │ ← Shadow memory
└──────────────────────────────────────┘

Step 3: Consistency Check (At observable boundary)
┌────────────────────────────────────────┐
│ call __nsan_check_consistency_float(   │
│     %x,           (float result)        │
│     %x_shadow     (double shadow)       │
│ )                                       │
│ // Runtime will compare and warn if    │
│ // relative error > threshold          │
└────────────────────────────────────────┘
```

---

## Component Breakdown

### 1. LLVM Pass (NSanPass)

**Responsibility**: IR instrumentation

```cpp
class NSanPass : public FunctionPass {
    // Key methods to implement:
    
    // Main entry point - called for each function
    bool runOnFunction(Function& F) override;
    
    // Detect if value is floating-point
    bool isFloatingPointValue(Value* V);
    
    // Create shadow value for a FP value
    Value* getShadowValue(Value* V);
    Value* createShadowValue(Value* V);
    
    // Instrument different operation types:
    void instrumentBinaryOp(BinaryOperator* Op);
    void instrumentCastOp(CastInst* Cast);
    void instrumentLoadStore(Instruction* I);
    void instrumentFunctionCall(CallInst* Call);
    void instrumentReturnValue(ReturnInst* Ret);
    void instrumentFunctionEntry(Function& F);
    
    // Map tracking
    ShadowValueMap shadow_map_;
};
```

**Key Implementation Points**:

```cpp
bool NSanPass::runOnFunction(Function& F) {
    bool Changed = false;
    
    // 1. Instrument function entry (parameters)
    instrumentFunctionEntry(F);
    
    // 2. Walk through all instructions
    for (BasicBlock& BB : F) {
        for (Instruction& I : BB) {
            
            // Floating-point binary ops (fadd, fsub, fmul, fdiv)
            if (auto* BinOp = dyn_cast<BinaryOperator>(&I)) {
                if (BinOp->getType()->isFloatingPointTy()) {
                    instrumentBinaryOp(BinOp);
                    Changed = true;
                }
            }
            
            // Type casting (fpext, fptrunc, sitofp, etc.)
            else if (auto* Cast = dyn_cast<CastInst>(&I)) {
                if (I.getType()->isFloatingPointTy()) {
                    instrumentCastOp(Cast);
                    Changed = true;
                }
            }
            
            // Function calls with FP args/return
            else if (auto* Call = dyn_cast<CallInst>(&I)) {
                if (I.getType()->isFloatingPointTy() ||
                    hasFPArguments(Call)) {
                    instrumentFunctionCall(Call);
                    Changed = true;
                }
            }
            
            // Load/store to FP values
            else if (auto* Load = dyn_cast<LoadInst>(&I)) {
                if (Load->getType()->isFloatingPointTy()) {
                    // Instrument shadow load
                    Changed = true;
                }
            }
            else if (auto* Store = dyn_cast<StoreInst>(&I)) {
                if (Store->getValueOperand()->getType()
                    ->isFloatingPointTy()) {
                    // Instrument shadow store
                    Changed = true;
                }
            }
            
            // Return statements
            else if (auto* Ret = dyn_cast<ReturnInst>(&I)) {
                if (Ret->getReturnValue() &&
                    Ret->getReturnValue()->getType()
                    ->isFloatingPointTy()) {
                    instrumentReturnValue(Ret);
                    Changed = true;
                }
            }
        }
    }
    
    return Changed;
}
```

### 2. Shadow Memory System

**Responsibility**: Managing shadow values for memory-allocated floats

```cpp
class ShadowMemory {
public:
    // Allocation interface
    void* allocate_shadow(size_t size, void* original_addr);
    void deallocate_shadow(void* original_addr);
    
    // Access interface
    void* get_shadow_address(void* original_addr, Type type);
    bool is_shadow_valid(void* original_addr, Type type);
    
private:
    // Shadow memory layout:
    // Original app address space:  [0x00000000 - 0xFFFFFFFF]
    // Shadow memory address space: [0x100000000 - 0x1FFFFFFFF]
    //
    // Mapping: shadow_addr = original_addr + SHADOW_OFFSET
    
    static constexpr uintptr_t SHADOW_OFFSET = 0x100000000ULL;
    
    // Track allocations
    std::map<void*, ShadowAllocation> allocations_;
    
    // Shadow type memory (1 byte per original byte)
    // Tracks what type each byte represents
    std::map<uintptr_t, uint8_t> shadow_types_;
};

// Shadow type encoding (one byte per original byte):
enum ShadowType : uint8_t {
    UNKNOWN = 0,
    FLOAT_POSITION_0 = 'f0',      // First byte of float
    FLOAT_POSITION_1 = 'f1',      // Second byte of float
    FLOAT_POSITION_2 = 'f2',      // Third byte of float
    FLOAT_POSITION_3 = 'f3',      // Fourth byte of float
    DOUBLE_POSITION_0 = 'd0',     // First byte of double
    // ... etc for all positions
};
```

### 3. Runtime Library (nsan_runtime.h)

**Responsibility**: Execution-time checking and reporting

```cpp
// External C interface for LLVM-instrumented code to call

extern "C" {

// Consistency checking
void __nsan_check_consistency_float(float original, double shadow);
void __nsan_check_consistency_double(double original, 
                                     __float128 shadow);

// Memory operations
void __nsan_on_load(void* addr, size_t size);
void __nsan_on_store(void* addr, void* value, size_t size);

// Function boundaries
void __nsan_push_shadow_parameters(int func_addr, ...);
double __nsan_pop_shadow_return();
void __nsan_push_shadow_return(double shadow);

// Memory management hooks
void* __nsan_malloc(size_t size);
void __nsan_free(void* ptr);

// User-facing interface
void __nsan_check_float(float f);
void __nsan_dump_shadow_mem(void* addr, size_t size);
void __nsan_resume_float(float f);

}
```

---

## Implementation Steps

### Phase 1: Foundation (Weeks 1-2)

#### Step 1.1: Basic LLVM Pass Structure

Create minimal pass that loads and runs:

```cpp
// src/nsan/NSanPass.cpp
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

class NSanPass : public FunctionPass {
public:
    static char ID;
    NSanPass() : FunctionPass(ID) {}
    
    bool runOnFunction(Function& F) override {
        errs() << "Running NSan on function: " << F.getName() << "\n";
        return false;  // No changes yet
    }
};

char NSanPass::ID = 0;
static RegisterPass<NSanPass> X("nsan", "Numerical Sanitizer Pass");
```

**Deliverable**: Pass compiles and registers with LLVM

#### Step 1.2: Floating-Point Detection

Add ability to identify FP operations:

```cpp
bool NSanPass::runOnFunction(Function& F) {
    int fp_ops = 0;
    
    for (auto& BB : F) {
        for (auto& I : BB) {
            // Count floating-point operations
            if (auto* BinOp = dyn_cast<BinaryOperator>(&I)) {
                if (BinOp->getType()->isFloatingPointTy()) {
                    errs() << "Found FP op: " << I << "\n";
                    fp_ops++;
                }
            }
        }
    }
    
    errs() << "Total FP ops: " << fp_ops << "\n";
    return false;
}
```

**Test**: Compile a simple floating-point program and verify detection

#### Step 1.3: Basic Shadow Value Creation

Add shadow value tracking:

```cpp
class ShadowValueMap {
    std::map<Value*, Value*> mapping;
    
public:
    Value* getShadowValue(Value* V) {
        return mapping[V];
    }
    
    void mapShadowValue(Value* Original, Value* Shadow) {
        mapping[Original] = Shadow;
    }
    
    bool hasShadow(Value* V) {
        return mapping.count(V) > 0;
    }
};

// In NSanPass:
ShadowValueMap shadow_map;

Value* createShadowValue(Value* V, IRBuilder<>& B) {
    Type* OrigType = V->getType();
    Type* ShadowType = getShadowType(OrigType);  // One level higher precision
    
    // For now, just create a dummy shadow (we'll improve later)
    Value* Shadow = ConstantFP::get(ShadowType, 0.0);
    return Shadow;
}
```

**Deliverable**: Pass can create shadow types for FP values

---

### Phase 2: Core Instrumentation (Weeks 3-4)

#### Step 2.1: Instrument Binary Operations

```cpp
void NSanPass::instrumentBinaryOp(BinaryOperator* Op, IRBuilder<>& B) {
    B.SetInsertPoint(Op->getNextNode());  // Insert after original op
    
    Value* LHS = Op->getOperand(0);
    Value* RHS = Op->getOperand(1);
    
    // Get shadow operands (extend if necessary)
    Value* LHS_Shadow = getShadowValue(LHS);
    Value* RHS_Shadow = getShadowValue(RHS);
    
    if (!LHS_Shadow) {
        LHS_Shadow = B.CreateFPExt(LHS, getShadowType(LHS->getType()));
    }
    if (!RHS_Shadow) {
        RHS_Shadow = B.CreateFPExt(RHS, getShadowType(RHS->getType()));
    }
    
    // Create shadow operation
    Value* Result_Shadow = B.CreateBinOp(Op->getOpcode(),
                                        LHS_Shadow, RHS_Shadow,
                                        "shadow_" + Op->getName());
    
    // Track shadow value
    shadow_map.mapShadowValue(Op, Result_Shadow);
}
```

**Test**: Verify shadow operations are created in IR

#### Step 2.2: Instrument Function Parameters

```cpp
void NSanPass::instrumentFunctionEntry(Function& F) {
    if (F.arg_empty()) return;
    
    IRBuilder<> B(&F.getEntryBlock(), F.getEntryBlock().begin());
    
    for (auto& Arg : F.args()) {
        if (Arg.getType()->isFloatingPointTy()) {
            // Create shadow parameter (initially same as original)
            Value* Shadow = B.CreateFPExt(
                &Arg,
                getShadowType(Arg.getType()),
                "shadow_" + Arg.getName()
            );
            
            shadow_map.mapShadowValue(&Arg, Shadow);
        }
    }
}
```

#### Step 2.3: Instrument Return Values

```cpp
void NSanPass::instrumentReturnValue(ReturnInst* Ret, IRBuilder<>& B) {
    Value* RetVal = Ret->getReturnValue();
    if (!RetVal || !RetVal->getType()->isFloatingPointTy()) {
        return;
    }
    
    B.SetInsertPoint(Ret);
    
    Value* RetShadow = getShadowValue(RetVal);
    if (!RetShadow) {
        RetShadow = B.CreateFPExt(RetVal, getShadowType(RetVal->getType()));
    }
    
    // Call consistency check before return
    Function* CheckFunc = getCheckFunction(RetVal->getType());
    B.CreateCall(CheckFunc, {RetVal, RetShadow});
}
```

**Deliverable**: Basic instrumentation working for simple programs

---

### Phase 3: Runtime System (Weeks 5-6)

#### Step 3.1: Consistency Checking

```cpp
// src/runtime/nsan_runtime.cpp
#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" {

// Configuration (from environment)
static double NSAN_EPSILON = 1e-5;
static double NSAN_REL_EPSILON = 1e-5;

void __nsan_check_consistency_float(float original, double shadow) {
    // Convert original to double for comparison
    double original_d = (double)original;
    
    // Compute relative error
    double abs_error = fabs(original_d - shadow);
    double rel_error = (fabs(shadow) > 1e-15) 
        ? abs_error / fabs(shadow)
        : abs_error;
    
    // Check against thresholds
    if (abs_error > NSAN_EPSILON || rel_error > NSAN_REL_EPSILON) {
        // Report error (we'll implement diagnostics next)
        fprintf(stderr, 
            "NSan Warning: Numerical inconsistency detected\n"
            "  Original (float): %.15g\n"
            "  Shadow (double):  %.15g\n"
            "  Relative error:   %.2e\n",
            original_d, shadow, rel_error
        );
    }
}

void __nsan_check_consistency_double(double original, 
                                     __float128 shadow) {
    // Similar but for double/quad
    // ... implementation ...
}

}
```

#### Step 3.2: Shadow Memory Management

```cpp
// src/runtime/shadow_memory.cpp
#include <map>
#include <cstdlib>
#include <cstring>

static std::map<void*, void*> shadow_memory_map;
static constexpr size_t SHADOW_OFFSET = 0x100000000ULL;

extern "C" {

void* __nsan_allocate_shadow(void* original_addr, size_t size) {
    // Allocate shadow memory
    void* shadow = malloc(size * 2);  // 2x for potential expansion
    
    // Create mapping
    shadow_memory_map[original_addr] = shadow;
    
    return shadow;
}

void __nsan_deallocate_shadow(void* original_addr) {
    auto it = shadow_memory_map.find(original_addr);
    if (it != shadow_memory_map.end()) {
        free(it->second);
        shadow_memory_map.erase(it);
    }
}

void* __nsan_get_shadow_address(void* original_addr, size_t original_size) {
    auto it = shadow_memory_map.find(original_addr);
    if (it != shadow_memory_map.end()) {
        return it->second;
    }
    return nullptr;
}

}
```

#### Step 3.3: Hook malloc/free

Intercept memory allocation to maintain shadow memory:

```cpp
// Hook into malloc/free via compiler-rt library
void* malloc(size_t size) {
    void* ptr = __libc_malloc(size);
    __nsan_allocate_shadow(ptr, size);
    return ptr;
}

void free(void* ptr) {
    __nsan_deallocate_shadow(ptr);
    __libc_free(ptr);
}
```

**Deliverable**: Runtime can check consistency and manage shadow memory

---

### Phase 4: Advanced Features (Weeks 7-8)

#### Step 4.1: Diagnostic System

```cpp
// src/runtime/diagnostics.cpp
#include <cstdio>
#include <execinfo.h>
#include <cxxabi.h>

struct NumericalError {
    float original_f;
    double shadow_d;
    double relative_error;
    const char* source_file;
    int source_line;
    void* stack_trace[32];
    int stack_size;
};

void report_numerical_error(const NumericalError& error) {
    fprintf(stderr, "\n");
    fprintf(stderr, "==================================================\n");
    fprintf(stderr, "  NSan: Numerical Stability Error Detected\n");
    fprintf(stderr, "==================================================\n");
    fprintf(stderr, "  Original value (float):   %.15g\n", error.original_f);
    fprintf(stderr, "  Shadow value (double):    %.15g\n", error.shadow_d);
    fprintf(stderr, "  Relative error:           %.2e%%\n", 
            error.relative_error * 100);
    fprintf(stderr, "  Location: %s:%d\n", 
            error.source_file, error.source_line);
    fprintf(stderr, "\nStack Trace:\n");
    
    // Print stack trace with symbols
    for (int i = 0; i < error.stack_size; i++) {
        // Demangle and print
    }
}
```

#### Step 4.2: Configuration System

```cpp
// src/runtime/configuration.cpp
#include <cstdlib>
#include <cstring>

void init_nsan_configuration() {
    // Read from environment variables
    const char* epsilon_str = getenv("NSAN_EPSILON");
    if (epsilon_str) {
        NSAN_EPSILON = atof(epsilon_str);
    }
    
    const char* rel_epsilon_str = getenv("NSAN_REL_EPSILON");
    if (rel_epsilon_str) {
        NSAN_REL_EPSILON = atof(rel_epsilon_str);
    }
    
    const char* verbosity_str = getenv("NSAN_VERBOSITY");
    if (verbosity_str) {
        NSAN_VERBOSITY = atoi(verbosity_str);
    }
}
```

**Deliverable**: Good error messages and configuration support

---

### Phase 5: Testing & Validation (Weeks 9-10)

#### Step 5.1: Unit Tests

```cpp
// tests/unit/test_instrumentation.cpp
#include <gtest/gtest.h>
#include "nsan/nsan_pass.h"

TEST(NSanPassTests, IdentifiesFloatingPointOps) {
    // Create simple IR with float operations
    // Run NSan pass
    // Verify FP ops are detected
}

TEST(NSanPassTests, CreatesShadowValues) {
    // Create IR with float variable
    // Run NSan pass
    // Verify shadow value created
}

TEST(NSanPassTests, InstrumentsBinaryOps) {
    // Create IR: %sum = fadd float %a, %b
    // Run NSan pass
    // Verify shadowed version created
}
```

#### Step 5.2: Functional Tests

```cpp
// tests/functional/test_naive_summation.cpp
#include <vector>
#include <cstdio>

// Compile with: clang++ -fsanitize=numerical test_naive_summation.cpp
//               -o test_naive && NSAN_EPSILON=1e-5 ./test_naive

float NaiveSum(const std::vector<float>& values) {
    float sum = 0.0f;
    for (float v : values) {
        sum += v;
    }
    return sum;
}

int main() {
    std::vector<float> values(10000000);
    for (int i = 0; i < 10000000; i++) {
        values[i] = 1.0f / (i + 1);  // Harmonic series
    }
    
    float result = NaiveSum(values);
    printf("Sum: %.10f\n", result);
    
    // Expected: NSan warning about numerical instability
    return 0;
}
```

#### Step 5.3: Benchmarking

```cpp
// tests/benchmarks/benchmark.cpp
#include <chrono>
#include <vector>
#include <cstdio>

template<typename Func>
double benchmark(Func f, int iterations = 100) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count() / iterations;
}

int main() {
    std::vector<float> values(1000000);
    // ... fill values ...
    
    // Benchmark original
    double orig_time = benchmark([&]() { NaiveSum(values); });
    
    // Benchmark instrumented (with NSan)
    // double nsan_time = ... (will be 2-20x slower)
    
    printf("Original:     %.6f ms\n", orig_time * 1000);
    printf("NSan:         %.6f ms\n", nsan_time * 1000);
    printf("Overhead:     %.1fx\n", nsan_time / orig_time);
}
```

**Deliverable**: Comprehensive test suite passing

---

## LLVM Pass Development

### Understanding IR Transformations

**Input IR**:
```llvm
define float @add_floats(float %a, float %b) {
entry:
  %sum = fadd float %a, %b
  ret float %sum
}
```

**Output IR (After NSan Pass)**:
```llvm
define float @add_floats(float %a, float %b) {
entry:
  ; Extend parameters to shadow precision
  %a.shadow = fpext float %a to double
  %b.shadow = fpext float %b to double
  
  ; Original computation
  %sum = fadd float %a, %b
  
  ; Shadow computation
  %sum.shadow = fadd double %a.shadow, %b.shadow
  
  ; Consistency check
  call void @__nsan_check_consistency_float(float %sum, double %sum.shadow)
  
  ret float %sum
}
```

### Pass Development Checklist

- [ ] Pass inherits from FunctionPass
- [ ] Pass ID registered with LLVM
- [ ] `runOnFunction` method implemented
- [ ] Floating-point type detection working
- [ ] Shadow value creation working
- [ ] IR manipulation using IRBuilder correct
- [ ] Consistency check insertion correct
- [ ] No infinite loops in instrumentation
- [ ] Handles all floating-point types (float, double, fp80, fp128)
- [ ] Handles vector types (<4 x float>, etc.)
- [ ] Thread-safety considerations addressed

---

## Runtime Library Development

### Key Runtime Functions

```cpp
// Initialization
void __nsan_init() {
    init_nsan_configuration();
    init_shadow_memory();
}

// Parameter passing (shadow stack)
void __nsan_push_shadow_parameters(int function_addr, ...) {
    // Variable arguments: shadow values for each FP param
    // Store on thread-local shadow stack
}

// Return values
void __nsan_push_shadow_return(double shadow, int function_addr) {
    // Store shadow return value
}

// Consistency checking
void __nsan_check_consistency_float(float orig, double shadow) {
    // Compute error and report
}

// Memory hooks
void __nsan_on_malloc(void* ptr, size_t size) {
    // Allocate shadow memory
}

void __nsan_on_free(void* ptr) {
    // Deallocate shadow memory
}

// User API
void __nsan_check_float(float f) {
    // Allow explicit checks from user code
}
```

### Thread Safety

Critical: Use thread-local storage for shadow stack:

```cpp
thread_local struct {
    double shadow_params[MAX_PARAMS];
    double shadow_return;
    int param_count;
    uint32_t function_addr;
} SHADOW_STACK;
```

---

## Testing Strategy

### Unit Test Categories

1. **IR Transformation Tests**
   - Verify correct shadow IR generation
   - Check instrumentation doesn't break IR

2. **Runtime Tests**
   - Shadow memory allocation/deallocation
   - Consistency checking logic
   - Error reporting

3. **Integration Tests**
   - End-to-end compilation and execution
   - Real-world numerical algorithms

4. **Regression Tests**
   - Ensure false positives don't increase
   - Track performance metrics

### Running Tests

```bash
# Build with tests
mkdir build && cd build
cmake -DENABLE_TESTS=ON ..
make

# Run all tests
ctest

# Run specific test
ctest -R test_instrumentation

# Run with verbose output
ctest --verbose

# Run benchmarks
./tests/benchmarks/benchmark_compensated_sum
```

---

## Performance Optimization

### Optimization Strategies

1. **Reduce Checking Overhead**
   - Only check observable values (default behavior)
   - Skip checking internal computations
   - Use configured epsilon thresholds

2. **Optimize Shadow Computations**
   - Hardware supports double precision well
   - Double → quad is slower (software implementation)
   - Consider selective instrumentation

3. **Memory Efficiency**
   - Shadow memory 1:2 ratio (not more)
   - Reuse shadow allocations when possible
   - Efficient shadow type tracking

4. **Compiler Integration**
   - Allow LLVM to optimize shadow code
   - Keep shadow operations local (no library calls for float→double)
   - Vectorize shadow operations with SIMD

### Benchmarking Points

```
Metric                      Target        Current
─────────────────────────────────────────────────
Float→Double (isolated)     2-3x overhead  (should be)
Double→Quad (isolated)      15-20x overhead (should be)
Real-world app overhead     2-10x overhead  (depends on computation)
Memory usage                4x original     (3-4x is ok)
False positive rate         <5%             (baseline)
Detection rate              >90%            (coverage)
```

---

## Integration Guide

### Integrating with Clang

1. **Register Pass with LLVM**

```cpp
// In a separate initialization file
void initializeNSanPass(PassRegistry& Registry) {
    initializeNSanPassPass(Registry);
}
```

2. **Add to Clang Driver**

Clang automatically picks up passes registered with LLVM.

3. **Compiler Flags**

Users compile with:
```bash
clang++ -fsanitize=numerical program.cpp
```

### Integration with Build Systems

#### CMake
```cmake
# In project's CMakeLists.txt
find_package(NSAN REQUIRED)
target_link_libraries(my_app ${NSAN_LIBRARIES})
```

#### Make
```makefile
NSAN_CFLAGS = -fsanitize=numerical
NSAN_LDFLAGS = -lnsan_runtime
```

#### Bazel
```starlark
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    copts = ["-fsanitize=numerical"],
    linkopts = ["-lnsan_runtime"],
)
```

---

## Troubleshooting

### Common Issues

#### Issue 1: "undefined reference to `__nsan_check_consistency_float`"

**Cause**: Runtime library not linked

**Solution**:
```bash
# Link runtime library explicitly
clang++ program.cpp -fsanitize=numerical -lnsan_runtime -o program
```

#### Issue 2: Shadow stack overflow

**Cause**: Deeply nested function calls or too many parameters

**Solution**:
```cpp
// Increase shadow stack size in configuration
#define MAX_SHADOW_STACK_ENTRIES 10000
```

#### Issue 3: False positives in library functions

**Cause**: NSan doesn't understand library semantics

**Solution**:
```cpp
// Create suppression file nsan_suppress.txt:
fun:sin
fun:cos
// Then run with:
// NSAN_SUPPRESSIONS=nsan_suppress.txt ./program
```

#### Issue 4: Performance overhead too high

**Cause**: Checking too much or quad precision shadow

**Solution**:
```bash
# Use only double shadow (faster)
clang++ -fsanitize=numerical program.cpp -o program

# Configure epsilon to reduce checks
export NSAN_EPSILON=1e-3
./program

# Profile to see where time is spent
perf record ./program
perf report
```

#### Issue 5: Incorrect diagnostics location

**Cause**: Function inlining moves checks

**Solution**:
```bash
# Compile without optimizations for debugging
clang++ -O0 -g -fsanitize=numerical program.cpp -o program

# With debug info, NSan can report better locations
```

---

## Deliverables Checklist

### Phase 1 Deliverables
- [ ] Project repository set up with correct directory structure
- [ ] CMake build system working
- [ ] LLVM pass compiles and registers
- [ ] Floating-point operation detection working

### Phase 2 Deliverables
- [ ] Binary operations instrumented
- [ ] Function parameters tracked
- [ ] Return values checked
- [ ] Memory operations handled

### Phase 3 Deliverables
- [ ] Runtime library compiles
- [ ] Consistency checking working
- [ ] Shadow memory management functional
- [ ] malloc/free hooks integrated

### Phase 4 Deliverables
- [ ] Diagnostic system generating useful messages
- [ ] Configuration system (environment variables)
- [ ] Error reporting with source locations
- [ ] Stack trace generation

### Phase 5 Deliverables
- [ ] Unit test suite complete (>50 tests)
- [ ] Functional tests passing
- [ ] Benchmark suite working
- [ ] Documentation complete
- [ ] Performance within 2-20x overhead

---

## Next Steps

1. **Set up project repository** with structure from this guide
2. **Start with Phase 1** - get basic pass framework working
3. **Test constantly** - verify each component works
4. **Iterate** - refine based on test results
5. **Optimize** - profile and improve performance
6. **Document** - keep documentation updated with implementation

---

**Version**: 1.0 | **Last Updated**: 2026 | **For**: Compiler Lab Project

