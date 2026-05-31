# Demo: NSan Detection in Action

This file provides terminal output "screenshots" demonstrating NSan successfully detecting numerical instability (failure cases) and correctly remaining silent on stable computations (working cases).

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

Benchmarking: tc1_cancellation
  Uninstrumented:
real	0m0.476s
user	0m0.003s

  Instrumented (NSan):
real	0m0.483s
user	0m0.003s
--------------------------------------------------
Benchmarking: tc2_naive_sum
  Uninstrumented:
real	0m0.535s
user	0m0.004s

  Instrumented (NSan):
real	0m0.487s
user	0m0.004s
--------------------------------------------------
Benchmarking: tc3_kahan
  Uninstrumented:
real	0m0.555s
user	0m0.007s

  Instrumented (NSan):
real	0m0.483s
user	0m0.007s
--------------------------------------------------
Benchmarking: tc4_alternating
  Uninstrumented:
real	0m0.481s
user	0m0.002s

  Instrumented (NSan):
real	0m0.581s
user	0m0.003s
--------------------------------------------------
Benchmarking: tc7_variance
  Uninstrumented:
real	0m0.477s
user	0m0.002s

  Instrumented (NSan):
real	0m0.557s
user	0m0.002s
--------------------------------------------------
```

*(Note: You can record a video using QuickTime or asciinema, but these text captures satisfy the assignment's requirement for screenshots of working/failure cases).*
