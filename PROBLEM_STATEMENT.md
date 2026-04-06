# Problem Statement: Floating-Point Numerical Instability Detection

## Executive Summary

Floating-point arithmetic is ubiquitous in modern computing but fraught with subtle precision-related bugs. Traditional debugging tools struggle to identify numerical instabilities because they don't understand the mathematical implications of precision loss. **NSan** addresses this by automatically detecting inconsistencies between low-precision computations and their high-precision shadows, enabling developers to catch and fix numerical bugs before they cause problems in production.

---

## The Problem

### Why Numerical Stability Matters

When you write numerical code, you're balancing two competing demands:
1. **Accuracy**: Results should be mathematically correct
2. **Performance**: Code should run fast enough to be practical

This creates a fundamental tension: using lower precision types (float vs. double) improves performance through smaller memory footprint and faster hardware operations, but at the cost of lost precision.

### Example 1: Naive Summation (The Classic Problem)

Consider summing 10 million random floating-point numbers:

```cpp
// NAIVE APPROACH - Numerically Unstable
float NaiveSum(const vector<float>& values) {
    float sum = 0.0f;
    for (float v : values) {
        sum += v;                    // ← Precision lost here!
    }
    return sum;
}

// IMPROVED APPROACH - Kahan Summation
float KahanSum(const vector<float>& values) {
    float sum = 0.0f;
    float c = 0.0f;                 // Correction term
    for (float v : values) {
        float y = v - c;            // Extract correction
        float t = sum + y;          // Add to sum
        c = (t - sum) - y;          // Compute new correction
        sum = t;
    }
    return sum;
}
```

**The Problem**: In naive summation, when adding tiny values (1e-7) to a large sum (1e6), the low bits get truncated due to floating-point representation limits. This isn't a bug in the code—it's how floating-point math works—but the error accumulates.

**Numeric Results**:
- Naive Sum Relative Error: **3.6 × 10⁻⁵** ❌
- Kahan Sum Relative Error: **3.3 × 10⁻⁸** ✓

**Detection Challenge**: How does a developer know their summation is wrong without running high-precision versions in parallel?

---

### Example 2: Matrix Operations Gone Wrong

```cpp
// Computing determinant of a 3x3 matrix
float MatrixDeterminant(float m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Problem case:
float matrix[3][3] = {
    {1.0f, 2.0f, 3.0f},
    {4.0f, 5.0f, 6.0f},
    {7.0f, 8.0f, 9.0f}  // Singular matrix (det = 0)
};

float det = MatrixDeterminant(matrix);
// Expected: 0.0
// Got: -9.53674e-07 (due to cancellation)
// Problem: Subsequent division by det causes huge errors!
```

**The Issue**: 
- Mathematically, the determinant is exactly 0 (rows are linearly dependent)
- In floating-point, subtraction of nearly-equal numbers causes massive relative error
- This leads to near-divide-by-zero conditions in algorithms like Gaussian elimination

---

### Example 3: Iterative Algorithms Diverging

```cpp
// Iterative power method for eigenvalue computation
float PowerMethod(float A[N][N], float x[N], int iterations) {
    for (int i = 0; i < iterations; i++) {
        // x = A * x (matrix-vector multiplication)
        MatrixVectorMultiply(A, x, x);
        
        // Normalize (but precision loss accumulates!)
        float norm = ComputeNorm(x);
        for (int j = 0; j < N; j++) {
            x[j] /= norm;
        }
    }
    // Problem: After 1000 iterations, precision has degraded so much
    // that the algorithm diverges instead of converging to eigenvector
}
```

**The Problem**: 
- Each iteration loses a few bits of precision
- After many iterations, accumulated error dominates
- Algorithm behavior becomes unpredictable (diverges, oscillates, stalls)

---

### Example 4: Cancellation Error (The Silent Killer)

```cpp
// Quadratic formula: x = (-b ± √(b² - 4ac)) / 2a
float QuadraticRoot(float a, float b, float c) {
    float discriminant = b*b - 4*a*c;
    float sqrtDisc = sqrt(discriminant);
    
    // Problem when b > 0 and √disc ≈ b
    float x1 = (-b + sqrtDisc) / (2*a);  // Subtraction → LOSS!
    float x2 = (-b - sqrtDisc) / (2*a);  // Much safer
    
    return x1;  // Wrong answer!
}

// Better approach using cancellation-free formula:
float QuadraticRoot_Better(float a, float b, float c) {
    float discriminant = b*b - 4*a*c;
    float sqrtDisc = sqrt(discriminant);
    
    // Use different formula depending on sign of b
    float x = (b > 0) 
        ? (-2*c) / (b + sqrtDisc)
        : (-b - sqrtDisc) / (2*a);
    
    return x;  // Correct!
}
```

**The Issue**: When subtracting two nearly-equal numbers, most significant bits cancel, leaving only the error. This is the #1 source of numerical instability.

---

## Why Existing Tools Fall Short

### Current Detection Methods

#### 1. **Manual Numerical Analysis**
- **Time Consuming**: Requires PhD-level mathematics for complex algorithms
- **Error Prone**: Easy to miss edge cases
- **Not Scalable**: Only practical for critical code sections
```
Real development time: 80% mathematics, 20% coding ❌
```

#### 2. **Probabilistic Methods (Verificarlo, VERROU)**
- **Slow**: Requires running program 1000+ times with perturbations
- **Uninterpretable**: Statistical analysis is hard to understand
- **False Positives**: Produces many spurious warnings

```
Example performance on Kahan summation:
Original program:        3.3 ms
With Verificarlo:      132 ms (40x slower)
Full analysis (1000×):  132 seconds (40,000x slower!) ❌
```

#### 3. **Dynamic Analysis (FpDebug/Valgrind)**
- **Very Slow**: Valgrind serializes all threads
- **Doesn't Scale**: Can't handle large applications
- **Wrong Semantics**: Doesn't understand library function behavior

```
Example: For 'sin(float)' computation
FpDebug does: sin(sin(float_shadow))      ❌ Wrong!
NSan does:    sin(double_shadow)          ✓ Correct!
```

---

## The NSan Solution

### Core Insight: Shadow Computing

Instead of modifying computations (like Verificarlo does), NSan runs a **parallel shadow computation** in higher precision:

```
Original (float):     sum += v;
Shadow (double):      sum_shadow += v_shadow;

Then compare: Are sum and sum_shadow significantly different?
```

### Why This Works

1. **Efficient**: Shadow operations run on real hardware (double is fast)
2. **Correct**: Shadow semantics naturally match original semantics
3. **Low False Positives**: Only checks observable values leaving functions
4. **Actionable**: Pinpoints exact location where precision is lost

### Example: How NSan Detects Naive Summation Bug

```cpp
float NaiveSum(const vector<float>& values) {
    float sum = 0.0f;
    double sum_shadow = 0.0;          // ← NSan adds this
    
    for (float v : values) {
        double v_shadow = (double)v;  // ← NSan adds this
        
        // Original computation
        sum += v;
        
        // Shadow computation (NSan instrumentation)
        sum_shadow += v_shadow;
        
        // At each iteration, check if they're consistent
        // After 10M iterations: sum ≠ sum_shadow → WARNING! ⚠️
    }
    
    return sum;
    // Before return, NSan checks:
    // sum (float) vs sum_shadow (double) - they differ!
    // Relative error: 3.6 × 10⁻⁵ > threshold (1e-5) → REPORT ERROR
}
```

---

## NSan's Technical Approach

### Three Types of Instrumentation

#### 1. **Temporary Values** (Local Variables)
```cpp
// Original IR:
%sum = fadd float %a, %b

// Instrumented IR:
%sum = fadd float %a, %b
%sum_shadow = fadd double %a_shadow, %b_shadow
```

#### 2. **Function Parameters & Returns**
```cpp
// Shadow stack: Caller pushes shadow parameters before call
caller: push_shadow_stack(a_shadow, b_shadow)
call function(a, b)

// Callee: Reads shadow parameters from shadow stack
callee: a_shadow = pop_shadow_stack()
        b_shadow = pop_shadow_stack()

// Return: Shadow return slot mechanism
function_return: shadow_return_slot[address] = result_shadow
caller_resume: result_shadow = read_shadow_return_slot()
```

#### 3. **Memory Values**
```
Application Memory:      0x1000: [f0 f1 f2 f3]  (float = 4 bytes)
Shadow Memory:           0x5000: [s0 s1 s2 s3 s4 s5 s6 s7]  (double = 8 bytes)
Shadow Type Memory:      0x9000: [d0 d1 d2 d3]  (track types of each byte)

load from 0x1000  →  load shadow from 0x5000  +  check shadow_types[0x1000]
```

---

## Concrete Examples for Understanding

### Example A: Compensated Summation (What NSan Detects)

```cpp
#include <cstdio>
#include <vector>
#include <cmath>

float NaiveSum(const std::vector<float>& v) {
    float sum = 0.0f;
    for (float x : v) sum += x;
    return sum;
}

float KahanSum(const std::vector<float>& v) {
    float sum = 0.0f;
    float correction = 0.0f;
    for (float x : v) {
        float y = x - correction;
        float t = sum + y;
        correction = (t - sum) - y;
        sum = t;
    }
    return sum;
}

int main() {
    // Generate 10 million random floats
    std::vector<float> values(10000000);
    for (int i = 0; i < 10000000; i++) {
        values[i] = (float)rand() / RAND_MAX;  // Random in [0, 1]
    }
    
    float naive = NaiveSum(values);
    float kahan = KahanSum(values);
    
    printf("Naive:  %.10f\n", naive);
    printf("Kahan:  %.10f\n", kahan);
    // NSan detects that naive_shadow differs from naive by >1e-5
    // NSan remains silent for kahan (both are consistent)
}
```

**NSan Output**:
```
WARNING: NumericalSanitizer: inconsistent shadow results
Location: main.cpp:8 (return from NaiveSum)
Original value (float):  5000023.00 
Shadow value (double):   5000028.87
Relative error:          1.17e-05  (threshold: 1e-05)
Status:                  ERROR ❌

No warning for KahanSum ✓
```

### Example B: What NSan Gets Right

**Type Punning** (modifying float's binary representation):
```cpp
void ModifyFloat(float* f) {
    // Flip sign bit (very unusual, but legal C++)
    *((unsigned int*)f) ^= 0x80000000;
}

float Test() {
    float x = 3.14f;
    double x_shadow = 3.14;
    ModifyFloat(&x);      // x is now -3.14
    // x_shadow still 3.14 (no shadow update for non-float stores)
    
    // NSan detects this and marks shadow as invalid
    // Resume computation: x_shadow = (double)x
    return x;
}
```

**Result**: NSan correctly handles this edge case by tracking shadow type information in each memory byte.

---

## What We're Building

### Project Scope

**Core Implementation** (Required):
1. ✅ LLVM IR instrumentation pass for floating-point operations
2. ✅ Shadow value tracking for temps, parameters, memory
3. ✅ Consistency checking with configurable thresholds
4. ✅ Diagnostic reporting with source locations
5. ✅ Test suite with benchmark comparisons

**Enhanced Features** (Recommended Extensions):
1. 🔧 JSON diagnostic output for tool integration
2. 🔧 Adaptive epsilon tuning
3. 🔧 Error chain analysis
4. 🔧 Hot-spot profiling
5. 🔧 Performance analysis dashboard

**Optional Advanced Features** (Stretch Goals):
1. 📌 CUDA instrumentation
2. 📌 IDE plugin integration
3. 📌 Continuous integration pipeline
4. 📌 Machine learning-based false positive filtering

---

## Success Metrics

### Performance Requirements
- **Single-run overhead**: 2-20x (vs. 40,000x for Verificarlo)
- **Multi-threaded scalability**: Linear with core count
- **Memory overhead**: 4x original (reasonable for debugging)

### Accuracy Requirements
- **False positive rate**: <5% (vs. 100% for basic FpDebug)
- **Detection rate**: >90% of real numerical issues
- **Precision of error location**: Exact instruction level

### Usability Requirements
- **Zero recompilation for non-instrumented code**: ✓
- **Clear, actionable diagnostics**: ✓
- **Standard compiler integration**: ✓

---

## Key Takeaway

**NSan solves the numerical debugging problem** by:
1. Running shadow computation in parallel (high precision)
2. Comparing observable values for consistency
3. Reporting precise, actionable diagnostics
4. Achieving 1-4 orders of magnitude speedup over existing tools

This enables numerical stability checking to become a **routine part of the development process**, not an expensive after-thought.

---

## Further Reading

- IEEE 754-2019: Floating-Point Arithmetic Standard
- Higham, N. J. (2002): "Accuracy and Stability of Numerical Algorithms"
- Courbet, C. (2021): "NSan: A Floating-Point Numerical Sanitizer" (CC '21)
- Dawson, B. (2012): "Comparing Floating Point Numbers, 2012 Edition"
