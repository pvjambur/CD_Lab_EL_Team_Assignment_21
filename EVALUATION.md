# Evaluation

## Metrics & Performance Overview
Based on evaluations of similar shadow-computation frameworks (like the original NSan CC '21 paper), overhead is strictly constrained compared to alternatives:
- **Execution Overhead**: Typically between 2× to 20× original program execution time, primarily dominated by branch consistency and shadow memory allocations.
- **Comparison**: This represents a drastic improvement over tools like Valgrind-based FpDebug (~476×) or Stochastic execution like Verificarlo (~40,000× due to multi-runs). The script `./benchmark.sh` dynamically evaluates this execution baseline across 5 test cases (tc1, tc2, tc3, tc4, tc7).
- **False Positive Mitigation**: Only comparing operations that become observable (stores to memory, return values, comparisons) effectively reduces false positives common in instruction-by-instruction verification.

## Test Cases


1. **TC1: Catastrophic Cancellation** (`tc1_cancellation.cpp`)
   - `(1e7 + 1.234) - 1e7`. Float evaluation yields `0.0`. NSan detects the difference and reports `1.90e-01` relative error. `[WARN]` triggers correctly.
2. **TC2: Naive Summation Accumulation** (`tc2_naive_sum.cpp`)
   - Summing `0.1f` 1,000,000 times compounds representability errors. Detected relative error is `9.58e-03`. `[WARN]` triggers correctly.
3. **TC3: Compensated Summation** (`tc3_kahan.cpp`)
   - Using Kahan summation properly corrects floating-point error, preserving value integrity. Relative error is 0. NSan remains `SILENT` — accurately avoiding false positives.
4. **TC4: Alternating Harmonic Series** (`tc4_alternating.cpp`)
   - Repeated cancellation across terms causing massive divergence. NSan detects `1.06e-05` error. `[WARN]` triggers correctly.
5. **TC5: Polynomial Evaluation Near Root** (`tc5_poly.cpp`)
   - Evaluates `x^3 - 3x + 2` very close to root `x=1`. Expected double result `~3e-12`, float rounds differently. `[WARN]` triggers correctly.
6. **TC7: One-Pass Variance** (`tc7_variance.cpp`)
   - Large `(Σx²)/N - mean²` cancellation. Generates massive error `4.54e+04`. `[WARN]` triggers correctly.
7. **TC9: Fused Multiply-Add Cancellation** (`tc9_fma.cpp`)
   - Near zero FMA result. Error `8.22e+05`. `[WARN]` triggers correctly.
*(Additional tests available in `tests/` directory)*

## Testing Execution
The `./run.sh` script actively builds and runs all `tc*.cpp` standalone files against the NSan plugin, emitting the results to `stdout` and `stderr`. This pipeline continuously validates both false negatives (e.g. naive summations missed) and false positives (e.g. alerting on Kahan summation incorrectly).
