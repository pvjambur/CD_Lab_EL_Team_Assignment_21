# NSan Project: Quick Reference Guide for Claude Code

This guide provides quick lookups and checklists for Claude Code to efficiently work on the NSan project.

---

## File Locations & Purpose

```
README.md              ← Project overview, features, use cases
PROBLEM_STATEMENT.md  ← What problem we're solving with examples
SKILLS.md             ← Development patterns & expertise required
INSTRUCTIONS.md       ← Detailed implementation steps (MAIN GUIDE)
This file             ← Quick reference
```

---

## Quick Command Reference

### Building the Project

```bash
# Setup
mkdir build && cd build
cmake ..

# Build everything
cmake --build . --config Release

# Run tests
ctest --verbose

# Build specific component
cmake --build . --target nsan_pass
cmake --build . --target nsan_runtime
```

### Compiling with NSan

```bash
# Basic usage
clang++ -fsanitize=numerical myprogram.cpp -o myprogram

# With debug info
clang++ -O0 -g -fsanitize=numerical myprogram.cpp -o myprogram

# With custom thresholds
NSAN_EPSILON=1e-6 ./myprogram
NSAN_REL_EPSILON=1e-4 ./myprogram
NSAN_VERBOSITY=2 ./myprogram
```

---

## Key Concepts (One-Liner Summary)

| Concept | Explanation |
|---------|-------------|
| **Shadow Value** | Higher-precision parallel computation (float→double) |
| **Shadow Stack** | Thread-local storage for function parameter shadows |
| **Shadow Memory** | Separate memory region for shadow values in heap |
| **Observable Value** | Values escaping functions (returns, calls, stores) |
| **Consistency Check** | Compare original vs. shadow for precision loss |
| **Relative Error** | (original - shadow) / shadow ratio |
| **Epsilon Threshold** | Configured tolerance for error reporting |
| **IR Instrumentation** | Transforming LLVM IR to add shadow operations |

---

## Implementation Roadmap

### Quick Milestone Checklist

**Week 1-2: Foundation**
- [ ] Project structure set up
- [ ] LLVM pass framework working
- [ ] Floating-point detection implemented
- [ ] Build system functioning

**Week 3-4: Core Instrumentation**
- [ ] Binary operations instrumented
- [ ] Function parameters tracked
- [ ] Return values instrumented
- [ ] Memory operations handled

**Week 5-6: Runtime**
- [ ] Runtime library compiles
- [ ] Consistency checking working
- [ ] Shadow memory management done
- [ ] malloc/free hooks integrated

**Week 7-8: Features**
- [ ] Diagnostics system complete
- [ ] Configuration system working
- [ ] Error reporting functional
- [ ] User API functions implemented

**Week 9-10: Testing**
- [ ] All unit tests passing
- [ ] Functional tests passing
- [ ] Benchmarks acceptable (<20x overhead)
- [ ] Documentation complete

---

## Code Patterns (Copy-Paste Ready)

### Pattern 1: Detect Floating-Point Instruction

```cpp
if (auto* BinOp = dyn_cast<BinaryOperator>(&I)) {
    if (BinOp->getType()->isFloatingPointTy()) {
        // Process FP operation
        errs() << "Found FP operation: " << *BinOp << "\n";
    }
}
```

### Pattern 2: Create Shadow Value

```cpp
Value* createShadowValue(Value* Original, IRBuilder<>& B) {
    Type* OrigType = Original->getType();
    Type* ShadowType = getShadowType(OrigType);
    
    // Extend to higher precision
    return B.CreateFPExt(Original, ShadowType, 
                        "shadow_" + Original->getName());
}
```

### Pattern 3: Instrument Binary Operation

```cpp
void instrumentBinaryOp(BinaryOperator* Op, IRBuilder<>& B) {
    B.SetInsertPoint(Op->getNextNode());
    
    Value* LHS_Shadow = createShadowValue(Op->getOperand(0), B);
    Value* RHS_Shadow = createShadowValue(Op->getOperand(1), B);
    
    Value* Result_Shadow = B.CreateBinOp(
        Op->getOpcode(),
        LHS_Shadow, RHS_Shadow,
        "shadow_" + Op->getName()
    );
    
    shadow_map.mapShadowValue(Op, Result_Shadow);
}
```

### Pattern 4: Insert Consistency Check

```cpp
void insertCheck(Value* Original, Value* Shadow, 
                 IRBuilder<>& B, Instruction* Before) {
    B.SetInsertPoint(Before);
    
    Function* CheckFunc = getCheckFunction(Original->getType());
    B.CreateCall(CheckFunc, {Original, Shadow});
}
```

### Pattern 5: Runtime Consistency Check

```cpp
extern "C" void __nsan_check_consistency_float(float orig, 
                                               double shadow) {
    double orig_d = (double)orig;
    double abs_err = fabs(orig_d - shadow);
    double rel_err = (fabs(shadow) > 1e-15) 
                     ? abs_err / fabs(shadow) 
                     : abs_err;
    
    if (rel_err > NSAN_REL_EPSILON) {
        fprintf(stderr, "NSan: Numerical error at ...\n");
    }
}
```

---

## Floating-Point Types in LLVM

```cpp
// Checking types
if (Type* FPType = dyn_cast<FloatingPointType>(T)) {
    // It's a floating-point type
}

// Type precision hierarchy
float   (32-bit)    ← lowest precision
double  (64-bit)    ← next level
x86_fp80 (80-bit)   ← extended precision (x86 only)
fp128   (128-bit)   ← quad precision, highest

// Type conversions
fpext float to double       // Safe extension
fptrunc double to float     // Precision loss
sitofp i32 to float         // Integer to float
fptosi float to i32         // Float to integer
```

---

## Common LLVM Operations

```cpp
// Creating IR Builder at specific location
IRBuilder<> B;
B.SetInsertPoint(Instruction*);           // After this instruction
B.SetInsertPoint(BasicBlock*, iterator);  // At this position

// Creating instructions
B.CreateFAdd(L, R);                    // Addition
B.CreateFSub(L, R);                    // Subtraction
B.CreateFMul(L, R);                    // Multiplication
B.CreateFDiv(L, R);                    // Division
B.CreateFExt(Val, TargetType);         // Extend precision
B.CreateFTrunc(Val, TargetType);       // Reduce precision
B.CreateCall(Function, Args);          // Call function

// Casting
dyn_cast<BinaryOperator>(Instr)        // Convert to BinOp
dyn_cast<CallInst>(Instr)              // Convert to Call
dyn_cast<ReturnInst>(Instr)            // Convert to Return
isa<LoadInst>(Instr)                   // Check type
```

---

## Error Messages Guide

| Message | Meaning | Fix |
|---------|---------|-----|
| "undefined reference to `__nsan_check_consistency_float`" | Runtime not linked | Link: `-lnsan_runtime` |
| "float operations with no shadow" | Shadow creation failed | Check `createShadowValue` |
| "shadow stack overflow" | Too many parameters/nesting | Increase `MAX_SHADOW_STACK` |
| "false positive: libm function" | Library semantics unknown | Use suppression file |
| "high overhead (>50x)" | Too much instrumentation | Use faster shadow type |

---

## Testing Patterns

### Unit Test Template

```cpp
#include <gtest/gtest.h>

TEST(NSanTests, TestName) {
    // Setup
    std::vector<float> values(1000);
    // ... initialize ...
    
    // Execute
    float result = computeResult(values);
    
    // Verify
    EXPECT_NEAR(result, expected, tolerance);
}
```

### Functional Test Template

```cpp
#include <cstdio>
#include <vector>

float NumericalTest(const std::vector<float>& values) {
    float sum = 0.0f;
    for (float v : values) {
        sum += v;  // ← Should trigger NSan warning
    }
    return sum;
}

int main() {
    std::vector<float> values(10000000);
    // ... fill with problematic data ...
    
    float result = NumericalTest(values);
    printf("Result: %.10f\n", result);
    
    // Expected: NSan warning in stderr
    return 0;
}
```

---

## Performance Checklist

When optimizing NSan performance:

- [ ] Profile with `perf` to identify bottlenecks
- [ ] Shadow computations are hardware-accelerated (double is fast)
- [ ] Quad precision is slow (software implementation)
- [ ] Only check observable values (not internal temporaries)
- [ ] Memory operations are cheap (<5% overhead)
- [ ] Consistency checking is main cost
- [ ] LLVM optimizes shadow code automatically
- [ ] Multi-threading scales linearly with cores

---

## Shadow Type Mapping

```
Original Type     Shadow Type      Precision Gain
─────────────────────────────────────────────────
float (f32)   →  double (f64)     2x precision
double (f64)  →  fp128 (f128)     2x precision
x86_fp80      →  fp128 (f128)     Slight gain
fp128         →  (not shadowed)   Already maximum
```

---

## Configuration Variables

```cpp
// Environment variables controlling NSan behavior

NSAN_EPSILON=1e-5           // Absolute error threshold
NSAN_REL_EPSILON=1e-5       // Relative error threshold  
NSAN_VERBOSITY=0|1|2        // 0=quiet, 1=warnings, 2=verbose
NSAN_SUPPRESSIONS=file.txt  // Suppression file path
NSAN_LOG_FILE=/tmp/nsan.log // Write output to file
NSAN_HALT_ON_ERROR=0        // 0=warn, 1=abort on error
```

---

## Class Relationships

```
NSanPass (LLVM Pass)
├─ runOnFunction(Function&)
├─ instrumentBinaryOp()
├─ instrumentFunctionCall()
├─ instrumentReturnValue()
└─ ShadowValueMap
   ├─ getShadowValue(Value*)
   ├─ mapShadowValue(Value*, Value*)
   └─ hasShadow(Value*)

NSan Runtime Library
├─ Shadow Memory
│  ├─ allocate_shadow()
│  ├─ deallocate_shadow()
│  └─ get_shadow_address()
├─ Consistency Checking
│  ├─ __nsan_check_consistency_float()
│  └─ __nsan_check_consistency_double()
└─ Diagnostics
   ├─ report_error()
   └─ format_message()
```

---

## Debugging Tips

### Print IR for inspection

```cpp
// At end of transformation
F.print(llvm::errs());

// Or dump a specific instruction
I->print(llvm::errs());
```

### Add debug output

```cpp
errs() << "Processing function: " << F.getName() << "\n";
errs() << "Instruction: " << *Instruction << "\n";
errs() << "Shadow created: " << *ShadowValue << "\n";
```

### Use gdb

```bash
# Compile with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Run with gdb
gdb ./myprogram
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable
```

### Check generated assembly

```bash
clang++ -S -emit-llvm myprogram.cpp -o myprogram.ll
llvm-dis myprogram.bc  # If compiled to bytecode
```

---

## Key Files to Modify/Create

### Starting Implementation

1. **src/nsan/NSanPass.cpp** (Main LLVM pass)
   - Entry point: `runOnFunction()`
   - Detects and instruments FP operations
   - ~300-500 lines initially

2. **src/runtime/nsan_runtime.cpp** (Runtime support)
   - Consistency checking logic
   - Error reporting
   - ~200-300 lines

3. **src/runtime/shadow_memory.cpp** (Memory management)
   - malloc/free hooking
   - Shadow address mapping
   - ~150-200 lines

4. **tests/** (Test suite)
   - Unit tests for IR transformation
   - Functional tests with real numerical code
   - Benchmarks

---

## Documentation To Update As You Go

- [ ] Keep README.md current with features
- [ ] Add API documentation to `include/nsan.h`
- [ ] Update INSTRUCTIONS.md with lessons learned
- [ ] Document any architectural decisions
- [ ] Add examples to `examples/` directory
- [ ] Create troubleshooting guide

---

## Before Submitting

- [ ] All tests passing: `ctest --verbose`
- [ ] No compiler warnings: `cmake --build . -- -Werror`
- [ ] Code formatted consistently
- [ ] Comments explain "why", not "what"
- [ ] Performance benchmarks documented
- [ ] All functionality tested
- [ ] README updated
- [ ] INSTRUCTIONS updated with lessons learned

---

## Quick Links to Documentation

- **How to write LLVM passes**: [LLVM Pass Guide](https://llvm.org/docs/WritingAnLLVMPass/)
- **IR Language reference**: [LLVM Language Reference](https://llvm.org/docs/LangRef/)
- **Floating-point reference**: [IEEE 754 Summary](https://en.wikipedia.org/wiki/IEEE_754)
- **Compiler-rt sanitizers**: [Sanitizer Design](https://github.com/google/sanitizers/wiki)

---

## Success Indicators

✅ Project is succeeding if:

- Naive summation triggers NSan warning
- Kahan summation does NOT trigger warning
- Performance overhead: 2-20x (acceptable)
- False positives: <5% of warnings
- Diagnostics include: location, values, error, stack trace
- Handles multi-threaded code correctly
- Integrates with existing LLVM infrastructure

---

**Last Updated**: 2026 | **Version**: 1.0 | **For**: Quick Reference While Coding
