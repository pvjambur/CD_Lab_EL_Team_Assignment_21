# NSan Design

## Approach: Shadow Computation
NSan automatically detects floating-point precision bugs in C/C++ programs using **shadow computation**. For every floating-point instruction, an equivalent operation in a higher-precision type (a "shadow" value) is executed in parallel. 
- Original `float` operations are shadowed by `double`.
- Original `double` operations are shadowed by `fp128`.

At key observation points (stores to memory, function returns, comparisons), the framework computes the relative error between the original floating-point result and its higher-precision shadow. If this divergence exceeds a configured threshold, it emits a warning, allowing the developer to pinpoint exactly where catastrophic cancellation or significant precision loss occurred.

## Alternative Solutions Considered

### 1. Manual Numerical Analysis
- **Approach**: Statically analyzing the mathematical implications of precision loss in code.
- **Drawbacks**: Requires advanced domain knowledge (numerical analysis), scales very poorly with large codebases, and is prone to missing edge cases.

### 2. Probabilistic Methods (e.g., Verificarlo)
- **Approach**: Modifies operations slightly (perturbation) and runs the program multiple times (often thousands of times) to observe statistical variance in the results.
- **Drawbacks**: Extremely slow. Verificarlo has an overhead of roughly 40,000×, making it impractical for large programs or regular test suites. Statistical results can also be hard to trace back to exact bug origins.

### 3. Dynamic Binary Instrumentation (e.g., FpDebug/Valgrind)
- **Approach**: Uses Valgrind to intercept instructions at runtime and simulate higher-precision operations.
- **Drawbacks**: Extremely high overhead (roughly 476×). Due to running in a serialized thread environment, it does not scale for multi-threaded applications and struggles with standard math library semantics.

### Why Shadow Computation via LLVM Pass?
By integrating as an LLVM pass:
1. **Performance**: Instrumentation relies on native hardware (e.g. `double` instructions are very fast) leading to a relatively low overhead (typically 2-20×).
2. **Seamless**: The developer only needs to compile with `-fsanitize=numerical` without changing source code.
3. **Accuracy**: Standard library functions and operations map correctly with low false-positive rates since we only check *observable* values rather than every intermediate step.
