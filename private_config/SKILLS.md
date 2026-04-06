# NSan Project Skills and Competencies

This file defines the skills, expertise areas, and development patterns Claude should apply when working on the NSan (Numerical Stability Sanitizer) project.

---

## Skill Categories

### 1. LLVM Compiler Infrastructure

#### Expertise Required
- **LLVM IR (Intermediate Representation)** understanding
  - Floating-point instruction types (fadd, fsub, fmul, fdiv, fcmp)
  - Vector operations and type conversions
  - Call instruction handling and function boundaries
  
- **LLVM Pass Architecture**
  - Module passes vs. Function passes
  - IR transformation patterns
  - Instruction visitor patterns
  - CallGraph analysis for interprocedural tracking

- **Type System in LLVM**
  - Float (32-bit), Double (64-bit), x86_fp80, fp128 (quad)
  - Vector types (<4 x float>, etc.)
  - Type casting and extension operations

#### Application to NSan
```cpp
// Understanding this LLVM IR:
%sum = fadd float %a, %b        // Floating-point add
%conv = fpext float %x to double // Precision extension

// We must transform it to:
%sum = fadd float %a, %b
%sum_shadow = fadd double %a_shadow, %b_shadow
%conv = fpext float %x to double
%conv_shadow = fpext double %x_shadow to fp128
```

#### Key Files to Master
- `llvm/IR/Instruction.h` - Instruction definitions
- `llvm/IR/IntrinsicInst.h` - Intrinsic function handling
- `llvm/Pass.h` - Pass infrastructure
- `llvm/Transforms/Utils/Local.h` - IR utilities

---

### 2. Floating-Point Arithmetic & Numerical Analysis

#### Expertise Required
- **IEEE 754 Standard**
  - Representation: sign, exponent, mantissa
  - Rounding modes (nearest, directed)
  - Special values (infinity, NaN, denormals)
  - Precision and ULP (Units in Last Place)

- **Error Metrics**
  - Absolute error: |computed - ideal|
  - Relative error: |computed - ideal| / |ideal|
  - ULP distance for comparison
  - Epsilon selection strategies

- **Common Numerical Issues**
  - Cancellation (subtraction of near-equal numbers)
  - Accumulation (repeated operations)
  - Underflow/Overflow
  - Loss of significance

- **Compensated Algorithms**
  - Kahan summation (compensated addition)
  - Compensated multiplication
  - Two-product algorithm
  - Horner's scheme variations

#### Application to NSan
```cpp
// When comparing float vs. double shadow:
float x = compute();           // Single precision
double x_shadow = ...;         // Double precision shadow

// Don't use: if (x == x_shadow)  ← Will always be false!
// Instead use relative epsilon comparison:
double rel_error = fabs(x - x_shadow) / fabs(x_shadow);
if (rel_error > EPSILON) {
    // Report error
}
```

#### Key Concepts to Implement
- Epsilon tolerance configuration (absolute and relative)
- Precision loss tracking across computations
- Shadow value validity checking
- Error accumulation patterns

---

### 3. C++ Programming (Advanced)

#### Expertise Required
- **Memory Management**
  - Manual memory layout understanding (for shadow memory)
  - Pointer arithmetic and aliasing
  - Custom allocators
  - Thread-local storage (thread_local keyword)

- **Concurrency**
  - Thread-local variables for shadow stack/return slot
  - Lock-free data structures (if used)
  - Race condition prevention
  - Synchronization primitives

- **Intrinsics and Low-Level Code**
  - Compiler intrinsics for atomic operations
  - Inline assembly when necessary
  - Bit manipulation for type punning detection
  - Memory fence instructions

- **Modern C++ Features**
  - RAII (Resource Acquisition Is Initialization)
  - Smart pointers (std::unique_ptr, std::shared_ptr)
  - move semantics
  - Templates for generic instrumentation

#### Application to NSan
```cpp
// Shadow memory management with custom allocator:
class ShadowMemoryManager {
public:
    void* allocate_shadow(size_t size, void* original_addr) {
        // Allocate shadow memory in separate address space
        // Map original_addr -> shadow_addr
    }
    
    void deallocate_shadow(void* shadow_addr) {
        // Clean up shadow memory
    }
    
private:
    std::map<void*, void*> memory_map;  // Track all allocations
};

// Thread-local shadow stack:
thread_local struct {
    double shadow_params[MAX_ARGS];
    int param_count;
} SHADOW_STACK;
```

---

### 4. Instrumentation Patterns

#### Expertise Required
- **Code Transformation Strategies**
  - Visitor pattern for IR traversal
  - Instruction rewriting techniques
  - Maintaining instruction ordering
  - Handling control flow

- **Shadow Value Mapping**
  - Maintaining correspondence between original and shadow values
  - Variable renaming strategies
  - Unique identifier generation for tracking

- **Memory Instrumentation**
  - Load/store interception
  - Shadow memory layout design
  - Type tracking in untyped memory
  - Malloc/free hook integration

#### Application to NSan
```cpp
// Pattern: Instrument floating-point operations

bool isFloatingPointOp(const Instruction* I) {
    return I->getType()->isFloatingPointTy() ||
           (I->getType()->isVectorTy() && 
            cast<VectorType>(I->getType())
              ->getElementType()->isFloatingPointTy());
}

void instrumentBinaryOp(BinaryOperator* Op) {
    Value* LHS = Op->getOperand(0);
    Value* RHS = Op->getOperand(1);
    
    // Get shadow values
    Value* LHS_shadow = getShadowValue(LHS);
    Value* RHS_shadow = getShadowValue(RHS);
    
    // Create shadow operation
    BinaryOperator* ShadowOp = BinaryOperator::Create(
        Op->getOpcode(),
        LHS_shadow, RHS_shadow,
        "shadow_" + Op->getName(),
        Op
    );
    
    // Map result to shadow
    mapShadowValue(Op, ShadowOp);
}
```

---

### 5. Testing & Validation

#### Expertise Required
- **Test Design**
  - Unit tests for instrumentation correctness
  - Numerical test suites
  - Benchmark programs
  - Edge case identification

- **Performance Testing**
  - Overhead measurement
  - Scalability analysis
  - Memory profiling
  - Cache behavior impact

- **Debugging Instrumentation**
  - Adding debug output
  - Assertion strategies
  - Shadow value verification
  - Execution tracing

#### Application to NSan
```cpp
// Test: Verify Kahan summation doesn't trigger warnings
TEST(NSanTests, KahanSummationNoWarning) {
    std::vector<float> values(1000000);
    // Fill with random values
    
    float result = KahanSum(values);
    // Compile with: -fsanitize=numerical
    // Expect: No warnings
    // Assert: Result is numerically stable
}

// Test: Verify naive summation triggers warning
TEST(NSanTests, NaiveSummationWarning) {
    std::vector<float> values(10000000);
    // Fill with values that cause cancellation
    
    float result = NaiveSum(values);
    // Compile with: -fsanitize=numerical
    // Expect: Warning about relative error > 1e-5
}
```

---

### 6. Documentation & Clarity

#### Expertise Required
- **Technical Writing**
  - Explaining compiler concepts clearly
  - Documenting IR transformations
  - Error message clarity
  - API documentation

- **Code Comments**
  - Explaining complex instrumentation logic
  - Documenting LLVM pass behavior
  - Runtime library comments
  - Configuration options

#### Application to NSan
```cpp
// Good documentation example:

/// Instruments a floating-point binary operation with shadow computation.
/// 
/// For an operation like: %sum = fadd float %a, %b
/// We emit:              %sum_shadow = fadd double %a_shadow, %b_shadow
///
/// The shadow operation uses the next higher precision type:
/// - float (32-bit) → double (64-bit)
/// - double (64-bit) → fp128 (128-bit quad)
///
/// @param Op The binary operation to instrument
/// @param IBuilder IR builder positioned after Op
void instrumentFloatBinaryOp(BinaryOperator* Op, IRBuilder<>& IBuilder);
```

---

## Development Patterns to Follow

### Pattern 1: Shadow Value Tracking

**When**: Creating shadow values for any floating-point computation

**How**:
```cpp
// Step 1: Check if value is floating-point
if (!isFloatingPointType(Value)) return;

// Step 2: Get or create shadow value
Value* Shadow = getShadowValue(Value);
if (!Shadow) {
    Shadow = createShadowValue(Value);
    storeShadowValue(Value, Shadow);
}

// Step 3: Use shadow in subsequent operations
// (Shadow operations happen in parallel)
```

### Pattern 2: Consistency Checking

**When**: Value escapes a function (call, return, store)

**How**:
```cpp
// Step 1: Get both values
Value* Original = ...;
Value* Shadow = getShadowValue(Original);

// Step 2: Emit comparison code
// (Instrumentation hook into runtime)
CallInst::Create(
    getCheckFunction(Original->getType()),
    {Original, Shadow},
    "",
    BeforeInst
);

// Step 3: Runtime will compare and report if inconsistent
```

### Pattern 3: Memory Instrumentation

**When**: Loading or storing floating-point values to memory

**How**:
```cpp
// For: store float %v, float* %addr
// Emit:
//   %shadow = getShadowValue(%v)
//   store double %shadow, double* %shadow_addr
//   (update shadow type memory)

// For: load float %v from float* %addr
// Emit:
//   %shadow = load double from %shadow_addr
//   (if shadow type memory indicates invalid, re-extend)
```

---

## Areas of Focus During Development

### Priority 1: Core Instrumentation (Essential)
- [ ] Floating-point operation detection and instrumentation
- [ ] Shadow value propagation through IR
- [ ] Parameter/return value handling via shadow stack
- [ ] Memory shadow tracking and type information

### Priority 2: Runtime Support (Essential)
- [ ] Shadow memory allocation/deallocation
- [ ] Consistency checking implementation
- [ ] Diagnostic message generation
- [ ] Configuration system (epsilon, verbosity, etc.)

### Priority 3: Testing & Validation (Important)
- [ ] Unit tests for instrumentation correctness
- [ ] Numerical benchmark suite
- [ ] Performance benchmarking
- [ ] False positive reduction validation

### Priority 4: Advanced Features (Enhancement)
- [ ] Adaptive threshold tuning
- [ ] Error chain analysis
- [ ] JSON diagnostic output
- [ ] Integration with development tools

---

## Common Pitfalls to Avoid

### ❌ Don't Instrument Non-Floating-Point Operations
```cpp
// Wrong: This shadows an integer add (makes no sense)
%sum = add i32 %a, %b
%sum_shadow = add i64 %a_shadow, %b_shadow

// Right: Only instrument floating-point ops
%fsum = fadd float %fa, %fb
%fsum_shadow = fadd double %fa_shadow, %fb_shadow
```

### ❌ Don't Compare Floats with ==
```cpp
// Wrong: Will always be false due to precision difference
if (original == shadow) { }

// Right: Use epsilon comparison
double rel_error = fabs(original - shadow) / fabs(shadow);
if (rel_error > EPSILON) { warn(); }
```

### ❌ Don't Assume Shadow Memory is Always Valid
```cpp
// Wrong: Unconditionally use shadow memory
double shadow = load_shadow_memory(addr);

// Right: Check shadow type validity first
if (is_shadow_type_valid(addr, DOUBLE_TYPE)) {
    double shadow = load_shadow_memory(addr);
} else {
    double shadow = (double)original;  // Re-extend
}
```

### ❌ Don't Ignore Type Punning
```cpp
// Wrong: Assume shadow reflects original after type punning
float* f = ...;
*(int*)f = 0x80000000;  // Flip sign bit
// shadow_f still has old value!

// Right: Invalidate shadow when type changes
void store_to_untyped_memory(void* addr, int value) {
    mark_shadow_type_invalid(addr, sizeof(int));
}
```

### ❌ Don't Break Thread Safety
```cpp
// Wrong: Global shadow stack (race conditions!)
static double SHADOW_PARAMS[100];

// Right: Thread-local shadow stack
thread_local double SHADOW_PARAMS[100];
```

---

## Code Style Guidelines

### LLVM Style (for IR manipulation)
```cpp
// Use LLVM naming conventions
void instrumentFloatingPointOp(BinaryOperator *Op, IRBuilder<> &B) {
    // 2 spaces for indentation
    // LLVM uses * with type, not with variable
    
    Type *OrigType = Op->getType();
    Type *ShadowType = getShadowType(OrigType);
    
    // Descriptive variable names
    Value *LHShadow = getShadowValue(Op->getOperand(0));
    Value *RHShadow = getShadowValue(Op->getOperand(1));
}
```

### Runtime Library Style
```cpp
// Clear separation of concerns
namespace nsan {
    namespace runtime {
        // Shadow memory management
        void* __nsan_allocate_shadow(size_t size);
        void __nsan_deallocate_shadow(void* ptr);
        
        // Consistency checking
        void __nsan_check_consistency_float(float f, double d);
        void __nsan_check_consistency_double(double d, __float128 q);
    }
}
```

---

## Success Criteria

### For Code Implementation
- ✅ Compiles without warnings
- ✅ Passes all test cases
- ✅ Performance within 2-20x overhead
- ✅ Handles edge cases (type punning, untyped stores, etc.)
- ✅ Clear, documented code

### For Debugging
- ✅ Pinpoints exact instruction causing error
- ✅ Reports relative and absolute error
- ✅ Shows full stack trace with symbols
- ✅ Actionable suggestions for fixing

### For Integration
- ✅ Works seamlessly with existing LLVM passes
- ✅ Doesn't interfere with optimizations
- ✅ Supports multi-threaded applications
- ✅ Minimal false positive rate

---

## Resources & References

### LLVM Documentation
- [LLVM Language Reference](https://llvm.org/docs/LangRef/)
- [LLVM IR Tutorial](https://llvm.org/docs/tutorial/)
- [Writing an LLVM Pass](https://llvm.org/docs/WritingAnLLVMPass/)
- [Sanitizer Design Document](https://github.com/google/sanitizers/wiki)

### Numerical Computing
- [IEEE 754-2019 Standard](https://en.wikipedia.org/wiki/IEEE_754)
- [Higham - Accuracy and Stability of Numerical Algorithms](https://people.maths.ox.ac.uk/higham/NA-large-print.html)
- [What Every Programmer Should Know About Floating-Point](https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html)

### Related Tools
- [Verificarlo](https://github.com/verificarlo/verificarlo) - Monte Carlo Arithmetic
- [FpDebug](https://github.com/LLNL/FpDebug) - Floating-point debugging
- [VERROU](https://github.com/edf-hpc/verrou) - Dynamic instrumentation

---

## Version Control & Collaboration

### Commit Message Format
```
[COMPONENT] Short description (50 chars max)

Detailed explanation of what changed and why.
- List specific changes
- Reference related issues/PRs
- Mention performance impact if significant

Tests: Added test case for X
Performance: ~2.5x overhead on benchmarks/test_suite.cpp
```

### Component Tags
- `[PASS]` - LLVM transformation pass
- `[RUNTIME]` - Runtime library code
- `[TEST]` - Test suite additions
- `[DOC]` - Documentation updates
- `[INFRA]` - Build system and configuration

---

**Last Updated**: 2026 | **Skill Version**: 1.0 | **For**: Claude Code Integration
