// nsan_demo.cpp - NSan Standalone Demo
//
// Shows what NSan detects: floating-point precision loss.
// This file manually instruments operations the way NSan's LLVM pass would --
// but here you can SEE it happen, without needing LLVM installed.
//
// Build: g++ -std=c++14 -O1 nsan_demo.cpp -o nsan_demo
// Run:   ./nsan_demo

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
static void enable_ansi() {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(h, &mode);
  SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
static void enable_ansi() {}
#endif

// ---------- Colour helpers ---------------------------------------------------
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN   "\033[1;36m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define RESET  "\033[0m"

// ---------- NSan-style runtime check ----------------------------------------
// In the real NSan, this function lives in nsan_runtime.cpp and is called
// automatically by instrumented code. Here we call it manually to show how it
// works.

static int g_checks = 0, g_warnings = 0;

// Threshold: warn when relative error exceeds this.
// ~1e-4 means "the float result is off by more than 0.01% from the reference"
static double NSAN_REL_EPSILON = 1e-4;

static bool nsan_check(float original, long double shadow, const char *label) {
  ++g_checks;
  double orig_d  = static_cast<double>(original);
  double shad_d  = static_cast<double>(shadow);   // downcast only for display
  double abs_err = fabs(orig_d - shad_d);
  double rel_err = (fabs(shad_d) > 1e-15) ? abs_err / fabs(shad_d) : abs_err;

  const char *ep = getenv("NSAN_REL_EPSILON");
  double thr = ep ? atof(ep) : NSAN_REL_EPSILON;

  bool warn = (rel_err > thr);
  if (warn) {
    ++g_warnings;
    printf(RED "  [WARN]" RESET "  %-32s\n"
           "          float32  = %+.8g\n"
           "          shadow   = %+.8g\n"
           "          rel_err  = " YELLOW "%.2e" RESET
           "  (threshold %.2e)\n",
           label, orig_d, shad_d, rel_err, thr);
  }
  return warn;
}

static void section(const char *title) {
  printf(BOLD CYAN "\n-- %s ", title);
  for (int i = (int)strlen(title); i < 48; ++i) putchar('-');
  printf(RESET "\n");
}

// ---------- Test 1: Catastrophic cancellation --------------------------------
static void test_cancellation() {
  section("Test 1: Catastrophic Cancellation");
  printf(DIM "  (a + b) - a  should equal b.\n"
             "  With large a and tiny b, float loses b during a+b.\n\n" RESET);

  float       a = 1e7f,  b = 1.234f;
  long double a_ld = 1e7L, b_ld = 1.234L;

  float       result = (a + b) - a;
  long double ref    = (a_ld + b_ld) - a_ld;

  printf("  a = 1e7,  b = 1.234\n");
  printf("  Expected         : %.6g\n", (double)b);
  printf("  float32 result   : " BOLD "%.6g" RESET "\n", (double)result);
  printf("  reference result : " BOLD "%.6g" RESET "\n\n", (double)ref);

  bool w = nsan_check(result, ref, "(a+b)-a");
  if (!w) printf(GREEN "  [PASS]\n" RESET);

  if (result == 0.0f)
    printf(RED "\n  => float COMPLETELY lost b (rounded away in a+b)!\n" RESET);
  else if (fabsf(result - b) > 0.01f)
    printf(YELLOW "\n  => float degraded b: %.4g became %.4g\n" RESET,
           (double)b, (double)result);
  else
    printf(GREEN "\n  => float preserved b.\n" RESET);
}

// ---------- Test 2: Naive vs Kahan summation ---------------------------------
static void test_summation() {
  section("Test 2: Naive vs Kahan Summation");
  printf(DIM "  Sum N large values. Naive accumulates rounding error;\n"
             "  Kahan correction keeps the float close to the reference.\n\n"
         RESET);

  // Values near 50000 stress float32: each ~50000, N=10000 -> sum ~5e8
  // float32 mantissa is 23 bits => ~7 decimal digits; 5e8 has 9 digits so
  // the fractional part is lost in naive summation.
  const int N = 10000;
  std::vector<float> values(N);
  srand(42);
  for (int i = 0; i < N; ++i)
    values[i] = 50000.0f + static_cast<float>(rand() % 1000) / 1000.0f;

  // Reference in long double
  long double ref = 0.0L;
  for (int i = 0; i < N; ++i) ref += static_cast<long double>(values[i]);

  // Naive
  float naive = 0.0f;
  for (int i = 0; i < N; ++i) naive += values[i];

  // Kahan
  float kahan = 0.0f, c = 0.0f;
  for (int i = 0; i < N; ++i) {
    float y = values[i] - c;
    float t = kahan + y;
    c = (t - kahan) - y;
    kahan = t;
  }

  double naive_err = fabs((double)naive - (double)ref) / fabs((double)ref);
  double kahan_err = fabs((double)kahan - (double)ref) / fabs((double)ref);

  printf("  N = %d,  each value in range [50000.000, 50000.999]\n\n", N);
  printf("  Reference (long dbl) : %.2f\n", (double)ref);
  printf("  Naive sum (float32)  : %.2f\n", (double)naive);
  printf("  Kahan sum (float32)  : %.2f\n\n", (double)kahan);

  printf("  Naive rel. error : ");
  if (naive_err > 1e-4) printf(RED   "%.2e  <-- SIGNIFICANT\n" RESET, naive_err);
  else                   printf(GREEN "%.2e  OK\n" RESET, naive_err);

  printf("  Kahan rel. error : ");
  if (kahan_err > 1e-4) printf(RED   "%.2e  <-- SIGNIFICANT\n" RESET, kahan_err);
  else                   printf(GREEN "%.2e  OK\n" RESET, kahan_err);

  printf("\n  NSan checks on final results:\n");
  bool nw = nsan_check(naive, ref, "naive sum (final)");
  if (!nw) printf(GREEN "  [PASS]  naive sum\n" RESET);

  bool kw = nsan_check(kahan, ref, "kahan sum (final)");
  if (!kw) printf(GREEN "  [PASS]  kahan sum\n" RESET);
}

// ---------- Test 3: Iterative precision loss ---------------------------------
static void test_iterative() {
  section("Test 3: Iterative Precision Loss (per-step)");
  printf(DIM "  Add delta=0.001 to x=1e6, ten thousand times.\n"
             "  float32 can't represent all increments -> drift accumulates.\n\n"
         RESET);

  float       x    = 1e6f;
  long double x_ld = 1e6L;
  float       delta    = 0.001f;
  long double delta_ld = 0.001L;

  int first_warn = -1, warn_count = 0;

  for (int i = 0; i < 10000; ++i) {
    x    += delta;
    x_ld += delta_ld;

    double err = fabs((double)x - (double)x_ld) / fabs((double)x_ld);
    if (err > NSAN_REL_EPSILON) {
      ++warn_count;
      ++g_checks; ++g_warnings;
      if (first_warn < 0) {
        first_warn = i;
        printf(RED "  [WARN]" RESET "  Divergence first detected at step %d\n", i);
        printf("          x (float32)  = %.10g\n", (double)x);
        printf("          x (ref)      = %.10g\n", (double)x_ld);
        printf("          rel error    = " YELLOW "%.2e" RESET "\n\n",
               err);
      }
    } else {
      ++g_checks;
    }
  }

  if (warn_count == 0)
    printf(GREEN "  [PASS]  No significant divergence detected.\n" RESET);
  else
    printf(YELLOW "  => %d / 10000 steps exceeded threshold.\n" RESET,
           warn_count);

  printf("  Final float32  : %.4f\n", (double)x);
  printf("  Final reference: %.4f\n", (double)x_ld);
  double final_err = fabs((double)x - (double)x_ld) / fabs((double)x_ld);
  printf("  Total rel error: %.2e\n", final_err);
}

// ---------- Test 4: Polynomial evaluation ------------------------------------
static void test_polynomial() {
  section("Test 4: Polynomial at Near-Root (Ill-Conditioned)");
  printf(DIM "  p(x) = x^2 - x - 1  at x = golden ratio\n"
             "  True answer = 0. Precision determines how close we get.\n\n"
         RESET);

  double      phi_d  = (1.0  + sqrt(5.0))  / 2.0;
  float       phi_f  = static_cast<float>(phi_d);
  long double phi_ld = (1.0L + sqrtl(5.0L)) / 2.0L;

  float       r_f  = phi_f  * phi_f  - phi_f  - 1.0f;
  double      r_d  = phi_d  * phi_d  - phi_d  - 1.0;
  long double r_ld = phi_ld * phi_ld - phi_ld - 1.0L;

  printf("  phi = golden ratio = (1+sqrt(5))/2\n\n");
  printf("  p(phi) with float32  : %+.6e  (|err| = %.2e)\n",
         (double)r_f,  (double)fabsf(r_f));
  printf("  p(phi) with double   : %+.6e  (|err| = %.2e)\n",
         r_d, fabs(r_d));
  printf("  p(phi) with long dbl : %+.6e  (|err| = %.2e)\n\n",
         (double)r_ld, (double)fabsl(r_ld));

  printf("  NSan check (float32 vs reference):\n");
  // ref ≈ 0, float result ≈ 1.19e-7; rel error is huge near zero
  // Use absolute difference here instead
  ++g_checks;
  double abs_diff = fabs((double)r_f - (double)r_ld);
  if (abs_diff > 1e-6) {
    ++g_warnings;
    printf(RED "  [WARN]" RESET
           "  float32 polynomial result is %.2e from reference\n", abs_diff);
  } else {
    printf(GREEN "  [PASS]  difference within 1e-6\n" RESET);
  }
}

// ---------- Main -------------------------------------------------------------
int main() {
  enable_ansi();

  printf(BOLD CYAN
    "\n+======================================================+\n"
    "|       NSan -- Numerical Stability Sanitizer          |\n"
    "|            Standalone Runtime Demo                   |\n"
    "+======================================================+\n"
    RESET);
  printf(DIM
    "\n  Simulates what NSan's LLVM pass injects into your binary.\n"
    "  Each float op is shadowed in long double precision.\n"
    "  A [WARN] fires when the two results diverge meaningfully.\n"
    RESET);

  test_cancellation();
  test_summation();
  test_iterative();
  test_polynomial();

  // Summary
  printf(BOLD CYAN
    "\n+======================================================+\n"
    "|                     Summary                          |\n"
    "+======================================================+\n"
    RESET);
  printf("  Checks performed : %d\n", g_checks);
  if (g_warnings > 0)
    printf(RED   "  Warnings fired   : %d\n" RESET, g_warnings);
  else
    printf(GREEN "  Warnings fired   : 0 -- all clean\n" RESET);

  printf(DIM
    "\n  Environment knobs (export before running):\n"
    "    NSAN_REL_EPSILON=1e-6    ./nsan_demo.exe   # stricter\n"
    "    NSAN_REL_EPSILON=1e-2    ./nsan_demo.exe   # permissive\n"
    RESET "\n");

  return (g_warnings > 0) ? 1 : 0;
}
