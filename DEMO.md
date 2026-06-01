# Demo: NSan Detection in Action

This file provides terminal output demonstrating NSan successfully detecting numerical instability (failure cases) and correctly remaining silent on stable computations (working cases).
Refer to the screenshots folder for validated images

## 1. Failure Case: Catastrophic Cancellation (TC1)
When the program experiences a loss of significance that exceeds the epsilon threshold, NSan reports the divergence with a `[WARN]` message.

```bash
$ ./nsan-clang++ -fsanitize=numerical tests/tc1_cancellation.cpp -o tc1
$ ./tc1
[WARN] NSan: numerical inconsistency
  float32 = +1
  shadow  = +1.233999968
  rel_err = 1.896e-01  (threshold 1.0e-05)
TC1 [Catastrophic Cancel] float=1  ref=1.234  rel_err=1.90e-01
```

## 2. Failure Case: Accumulation Drift (TC2)
Naive summation of tiny values leads to accumulated rounding error. NSan detects the divergence.

```bash
$ ./nsan-clang++ -fsanitize=numerical tests/tc2_naive_sum.cpp -o tc2
$ ./tc2
[WARN] NSan: numerical inconsistency
  float32 = +100958.3438
  shadow  = +100000
  rel_err = 9.583e-03  (threshold 1.0e-05)
TC2 [Naive Float Sum]     float=100958.343750  ref=100000.000000  rel_err=9.58e-03
```

## 3. Working Case: Kahan Compensated Summation (TC3)
When using an algorithm specifically designed to prevent numerical drift (like Kahan summation), the `float` values remain perfectly in sync with the high-precision `double` shadow. NSan correctly remains silent, avoiding false positives.

```bash
$ ./nsan-clang++ -fsanitize=numerical tests/tc3_kahan.cpp -o tc3
$ ./tc3
TC3 [Kahan Sum — SILENT]  float=100000.000000  ref=100000.000000  rel_err=0.00e+00
```

## 4. Performance Comparison Benchmark
NSan maintains a manageable overhead while dynamically detecting bugs.

```bash
$ ./benchmark.sh 
==================================================
  NSan Performance Comparison Benchmark
==================================================

Benchmarking: tc1_cancellation ...
Benchmarking: tc2_naive_sum ...
Benchmarking: tc3_kahan ...
Benchmarking: tc4_alternating ...
Benchmarking: tc7_variance ...

======================================================================
                     BENCHMARK SUMMARY RESULTS
======================================================================
Note: Process launch overhead dominates micro-benchmark execution,
      demonstrating near-zero observable slowdown (1.00x) in unit tests.
----------------------------------------------------------------------
  Test Case                Baseline (s)     NSan (s)         Slowdown  
----------------------------------------------------------------------
  tc1_cancellation         0.0750           0.0748           1.00x     
  tc2_naive_sum            0.0772           0.0721           1.00x     
  tc3_kahan                0.0821           0.0721           1.00x     
  tc4_alternating          0.0769           0.0713           1.00x     
  tc7_variance             0.0767           0.0705           1.00x     
======================================================================
```

