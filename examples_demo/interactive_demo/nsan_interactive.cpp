// nsan_interactive.cpp  --  NSan Interactive Demo
//
// An interactive terminal application demonstrating the NSan
// Numerical Stability Sanitizer concept.
//
// Build:  g++ -std=c++14 -O1 nsan_interactive.cpp -o nsan_interactive.exe
// Run:    ./nsan_interactive.exe
//
// No external libraries required.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
static void enable_ansi() {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(h, &mode))
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
static void clear_screen() { system("cls"); }
#else
static void enable_ansi() {}
static void clear_screen() { system("clear"); }
#endif

// ── Colours ──────────────────────────────────────────────────────────────────
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define CYAN   "\033[1;36m"
#define WHITE  "\033[1;37m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define RESET  "\033[0m"

// ── Global state ──────────────────────────────────────────────────────────────
static double g_threshold  = 1e-4;
static int    g_checks     = 0;
static int    g_warnings   = 0;

struct LogEntry {
  std::string label;
  double      float_val;
  double      shadow_val;
  double      rel_err;
  bool        warned;
};
static std::vector<LogEntry> g_log;

// ── Core NSan check ───────────────────────────────────────────────────────────
static bool nsan_check(float original, long double shadow,
                        const std::string &label) {
  ++g_checks;
  double orig_d  = static_cast<double>(original);
  double shad_d  = static_cast<double>(shadow);
  double abs_err = fabs(orig_d - shad_d);
  double rel_err = (fabs(shad_d) > 1e-15) ? abs_err / fabs(shad_d) : abs_err;
  bool   warned  = (rel_err > g_threshold);

  if (warned) ++g_warnings;

  g_log.push_back({label, orig_d, shad_d, rel_err, warned});
  return warned;
}

// ── UI helpers ────────────────────────────────────────────────────────────────
static void hline(int w = 58) {
  printf(CYAN);
  for (int i = 0; i < w; ++i) putchar('-');
  printf(RESET "\n");
}

static void header(const char *title) {
  hline();
  printf(CYAN "|" RESET BOLD "  %-54s" RESET CYAN "|\n" RESET, title);
  hline();
}

static void status_bar() {
  printf(DIM
    "  Checks: %d  |  Warnings: %d  |  Threshold: %.0e\n"
    RESET, g_checks, g_warnings, g_threshold);
  hline();
}

static void wait_enter(const char *msg = "  Press Enter to continue...") {
  printf(DIM "%s" RESET, msg);
  // flush and consume newline
  fflush(stdout);
  int c;
  while ((c = getchar()) != '\n' && c != EOF) {}
}

static char get_choice() {
  printf(BOLD "\n  Choice: " RESET);
  fflush(stdout);
  char buf[8] = {};
  if (fgets(buf, sizeof(buf), stdin))
    return buf[0];
  return 0;
}

static double get_double(const char *prompt) {
  printf(BOLD "  %s: " RESET, prompt);
  fflush(stdout);
  char buf[64] = {};
  double v = 0.0;
  if (fgets(buf, sizeof(buf), stdin))
    v = atof(buf);
  return v;
}

static int get_int(const char *prompt) {
  return (int)get_double(prompt);
}

static void print_check_result(const std::string &label,
                                float orig, long double ref) {
  double o = (double)orig, s = (double)ref;
  double abs_err = fabs(o - s);
  double rel_err = (fabs(s) > 1e-15) ? abs_err / fabs(s) : abs_err;
  bool   w       = nsan_check(orig, ref, label);

  printf("\n  %-30s\n", label.c_str());
  printf("    float32  = " BOLD "%+.10g" RESET "\n", o);
  printf("    shadow   = " BOLD "%+.10g" RESET "\n", s);
  printf("    rel err  = ");
  if (w) printf(RED "%+.2e" RESET "  [WARN]\n", rel_err);
  else   printf(GREEN "%+.2e" RESET "  [PASS]\n", rel_err);
}

// ═══════════════════════════════════════════════════════════════════════════
// SCREENS
// ═══════════════════════════════════════════════════════════════════════════

// ── About ─────────────────────────────────────────────────────────────────────
static void screen_about() {
  clear_screen();
  header("About NSan");
  printf("\n"
    "  " BOLD "What is NSan?" RESET "\n"
    "  NSan (Numerical Stability Sanitizer) is a compiler tool\n"
    "  that detects floating-point precision errors automatically.\n\n"

    "  " BOLD "The core idea:" RESET "\n"
    "  Every float operation is run TWICE:\n"
    "    1. Original: in float32 (as your code says)\n"
    "    2. Shadow  : in double/long-double (higher precision)\n"
    "  If the two answers diverge significantly -> WARNING.\n\n"

    "  " BOLD "How it works (full system):" RESET "\n"
    "  1. Your C++ source is compiled by Clang/LLVM\n"
    "  2. NSan's LLVM pass intercepts every `fadd`/`fmul`/etc.\n"
    "  3. The pass injects shadow computation code into your binary\n"
    "  4. At runtime, nsan_runtime.cpp checks each result\n"
    "  5. Warnings are printed to stderr as your program runs\n\n"

    "  " BOLD "This demo:" RESET "\n"
    "  Simulates step 4-5 manually so you can experiment\n"
    "  without needing LLVM installed.\n\n"

    "  " BOLD "Real-world impact:" RESET "\n"
    "  - Rockets/satellites lost due to FP errors\n"
    "  - Medical imaging algorithms silently wrong\n"
    "  - Financial calculations accumulating error\n"
    "  NSan finds these bugs during development.\n\n");
  status_bar();
  wait_enter();
}

// ── Preset tests ──────────────────────────────────────────────────────────────
static void run_cancellation() {
  printf("\n  " BOLD "Catastrophic Cancellation" RESET "\n");
  printf(DIM "  Computing (A + B) - A  where A >> B\n\n" RESET);

  float a = 1e7f, b = 1.234f;
  long double a_ld = 1e7L, b_ld = 1.234L;

  float       step1_f  = a + b;
  long double step1_ld = a_ld + b_ld;

  float       result   = step1_f - a;
  long double ref      = step1_ld - a_ld;

  printf("  A = 1e7,  B = 1.234\n");
  printf("  A + B  =>  float32: %.10g   ref: %.10g\n",
         (double)step1_f, (double)step1_ld);
  print_check_result("(A+B) - A", result, ref);

  printf("\n  Expected: 1.234  |  float says: %.6g\n", (double)result);
  if (result == 0.0f)
    printf(RED "  => float LOST B entirely!\n" RESET);
  else if (fabsf(result - b) > 0.01f)
    printf(YELLOW "  => float DEGRADED B from %.4g to %.4g\n" RESET,
           (double)b, (double)result);
}

static void run_summation(int N) {
  printf("\n  " BOLD "Naive vs Kahan Summation (N=%d)" RESET "\n", N);
  printf(DIM "  Values near 50000 -- float32 loses fractional parts\n\n" RESET);

  std::vector<float> v(N);
  srand(42);
  for (int i = 0; i < N; ++i)
    v[i] = 50000.0f + (float)(rand() % 1000) / 1000.0f;

  long double ref = 0.0L;
  for (int i = 0; i < N; ++i) ref += (long double)v[i];

  float naive = 0.0f;
  for (int i = 0; i < N; ++i) naive += v[i];

  float kahan = 0.0f, c = 0.0f;
  for (int i = 0; i < N; ++i) {
    float y = v[i] - c, t = kahan + y;
    c = (t - kahan) - y; kahan = t;
  }

  printf("  Reference : %.2f\n", (double)ref);
  printf("  Naive     : %.2f  (error: %.2e)\n",
         (double)naive,
         fabs((double)naive - (double)ref) / fabs((double)ref));
  printf("  Kahan     : %.2f  (error: %.2e)\n",
         (double)kahan,
         fabs((double)kahan - (double)ref) / fabs((double)ref));

  printf("\n  NSan checks on final results:\n");
  print_check_result("naive sum", naive, ref);
  print_check_result("kahan sum", kahan, ref);
}

static void run_all_preset() {
  run_cancellation();
  printf("\n"); hline();
  run_summation(5000);
}

// ── Custom computation ────────────────────────────────────────────────────────
static void screen_custom() {
  clear_screen();
  header("Custom Computation");
  printf(DIM
    "  Enter two values A and B, pick an operation.\n"
    "  NSan will compute in float32 AND long double and compare.\n\n"
    RESET);

  double A = get_double("Enter A");
  double B = get_double("Enter B");

  printf("\n  Operations:\n");
  printf("  [1] A + B\n");
  printf("  [2] A - B\n");
  printf("  [3] A * B\n");
  printf("  [4] A / B\n");
  printf("  [5] (A + B) - A   (cancellation demo)\n");
  printf("  [6] A^2 - B^2  vs  (A+B)*(A-B)  (factoring accuracy)\n");
  char op = get_choice();

  float       fa = (float)A, fb = (float)B;
  long double la = (long double)A, lb = (long double)B;
  float       result_f  = 0;
  long double result_ld = 0;
  const char *label = "";

  printf("\n");
  switch (op) {
    case '1':
      result_f  = fa + fb;
      result_ld = la + lb;
      label = "A + B";
      break;
    case '2':
      result_f  = fa - fb;
      result_ld = la - lb;
      label = "A - B";
      break;
    case '3':
      result_f  = fa * fb;
      result_ld = la * lb;
      label = "A * B";
      break;
    case '4':
      if (fabsf(fb) < 1e-30f) { printf(RED "  Division by zero!\n" RESET); break; }
      result_f  = fa / fb;
      result_ld = la / lb;
      label = "A / B";
      break;
    case '5':
      result_f  = (fa + fb) - fa;
      result_ld = (la + lb) - la;
      label = "(A+B)-A";
      printf("  A + B in float32 = %.10g\n", (double)(fa + fb));
      printf("  A + B in ref     = %.10g\n", (double)(la + lb));
      break;
    case '6': {
      float       diff_sq_f  = fa*fa - fb*fb;
      long double diff_sq_ld = la*la - lb*lb;
      float       factor_f   = (fa+fb) * (fa-fb);
      long double factor_ld  = (la+lb) * (la-lb);
      printf("  A^2 - B^2 (direct):\n");
      print_check_result("direct", diff_sq_f, diff_sq_ld);
      printf("  (A+B)*(A-B) (factored):\n");
      print_check_result("factored", factor_f, factor_ld);
      printf("\n  " BOLD "Note:" RESET " factored form is more numerically stable.\n");
      goto done;
    }
    default:
      printf(YELLOW "  Invalid choice.\n" RESET);
      goto done;
  }

  printf("  A = %.10g  B = %.10g\n", A, B);
  print_check_result(label, result_f, result_ld);

done:
  printf("\n");
  status_bar();
  wait_enter();
}

// ── Summation explorer ────────────────────────────────────────────────────────
static void screen_summation() {
  clear_screen();
  header("Summation Accuracy Explorer");
  printf(DIM
    "  Sum N values in a given range.\n"
    "  Choose N and value range; see naive vs Kahan accuracy.\n\n"
    RESET);

  int    N    = get_int("Number of values N (e.g. 10000)");
  double base = get_double("Base value (e.g. 50000)");
  double frac = get_double("Random range 0..X added to base (e.g. 1)");

  if (N <= 0 || N > 10000000) { printf(RED "  N out of range.\n" RESET); wait_enter(); return; }

  std::vector<float> v(N);
  srand((unsigned)time(nullptr));
  for (int i = 0; i < N; ++i)
    v[i] = (float)(base + (rand() % 10000) / 10000.0 * frac);

  long double ref = 0.0L;
  for (int i = 0; i < N; ++i) ref += (long double)v[i];

  float naive = 0.0f;
  for (int i = 0; i < N; ++i) naive += v[i];

  float kahan = 0.0f, c = 0.0f;
  for (int i = 0; i < N; ++i) {
    float y = v[i] - c, t = kahan + y;
    c = (t - kahan) - y; kahan = t;
  }

  double ne = fabs((double)naive - (double)ref) / fabs((double)ref);
  double ke = fabs((double)kahan - (double)ref) / fabs((double)ref);

  printf("\n  N = %d,  each value in [%.4g, %.4g]\n\n", N, base, base+frac);
  printf("  Reference : %.4f\n", (double)ref);
  printf("  Naive     : %.4f\n", (double)naive);
  printf("  Kahan     : %.4f\n\n", (double)kahan);
  printf("  Naive error : ");
  if (ne > g_threshold) printf(RED   "%.2e  [WARN]\n" RESET, ne);
  else                   printf(GREEN "%.2e  [PASS]\n" RESET, ne);
  printf("  Kahan error : ");
  if (ke > g_threshold) printf(RED   "%.2e  [WARN]\n" RESET, ke);
  else                   printf(GREEN "%.2e  [PASS]\n" RESET, ke);

  printf("\n  NSan checks:\n");
  print_check_result("naive sum", naive, ref);
  print_check_result("kahan sum", kahan, ref);

  printf("\n");
  status_bar();
  wait_enter();
}

// ── Log viewer ────────────────────────────────────────────────────────────────
static void screen_log() {
  clear_screen();
  header("Computation Log");

  if (g_log.empty()) {
    printf("\n  No computations recorded yet.\n"
           "  Run some tests from the main menu first.\n\n");
    status_bar();
    wait_enter();
    return;
  }

  // Show last 20 entries
  int start = (int)g_log.size() > 20 ? (int)g_log.size() - 20 : 0;
  if (start > 0)
    printf(DIM "  (Showing last 20 of %d entries)\n\n" RESET, (int)g_log.size());

  printf("  %-28s  %-14s  %-14s  %s\n",
         "Label", "float32", "shadow", "rel_err");
  hline();

  for (int i = start; i < (int)g_log.size(); ++i) {
    const LogEntry &e = g_log[i];
    printf("  %-28s  %-14.6g  %-14.6g  ",
           e.label.substr(0, 27).c_str(), e.float_val, e.shadow_val);
    if (e.warned) printf(RED   "%.2e [W]" RESET "\n", e.rel_err);
    else          printf(GREEN "%.2e [P]" RESET "\n", e.rel_err);
  }

  printf("\n");
  status_bar();
  wait_enter();
}

// ── Settings ──────────────────────────────────────────────────────────────────
static void screen_settings() {
  clear_screen();
  header("Settings");
  printf("\n"
    "  Current threshold : " YELLOW "%.2e" RESET "\n\n"
    "  Presets:\n"
    "  [1] 1e-3   (loose   -- only big errors)\n"
    "  [2] 1e-4   (default -- moderate sensitivity)\n"
    "  [3] 1e-5   (strict)\n"
    "  [4] 1e-6   (very strict -- catches tiny drifts)\n"
    "  [5] Custom value\n"
    "  [b] Back\n\n",
    g_threshold);
  status_bar();

  char c = get_choice();
  switch (c) {
    case '1': g_threshold = 1e-3; break;
    case '2': g_threshold = 1e-4; break;
    case '3': g_threshold = 1e-5; break;
    case '4': g_threshold = 1e-6; break;
    case '5': g_threshold = get_double("Enter threshold (e.g. 0.001)"); break;
  }
  printf(GREEN "\n  Threshold set to %.2e\n" RESET, g_threshold);
  wait_enter();
}

// ── What NSan injects ─────────────────────────────────────────────────────────
static void screen_llvm_view() {
  clear_screen();
  header("What NSan Injects into Your LLVM IR");
  printf("\n"
    "  Your source code:\n"
    CYAN "  ┌─────────────────────────────────────────────────┐\n"
    "  │  float a = 1.5f, b = 2.5f;                      │\n"
    "  │  float sum = a + b;    // <-- fadd float         │\n"
    "  └─────────────────────────────────────────────────┘\n"
    RESET "\n"
    "  LLVM IR before NSan:\n"
    DIM "    %%sum = fadd float %%a, %%b\n\n" RESET

    "  LLVM IR after NSan pass transforms it:\n"
    DIM "    ; Original op (unchanged)\n"
    "    %%sum = fadd float %%a, %%b\n\n"
    "    ; Shadow op injected by NSanPass::instrumentBinaryOp()\n"
    "    %%a_shadow = fpext float %%a to double\n"
    "    %%b_shadow = fpext float %%b to double\n"
    "    %%sum_shadow = fadd double %%a_shadow, %%b_shadow\n\n"
    "    ; Consistency check injected\n"
    "    call void @__nsan_check_consistency_float(\n"
    "           float %%sum, double %%sum_shadow)\n\n" RESET

    "  nsan_runtime.cpp then runs:\n"
    DIM "    double orig = (double)original;\n"
    "    double err  = fabs(orig - shadow) / fabs(shadow);\n"
    "    if (err > threshold)\n"
    "      fprintf(stderr, \"NSan Warning: ...\");\n\n" RESET

    "  This is done for EVERY float operation automatically.\n"
    "  You never write the shadow code -- the pass does it.\n\n"

    "  " BOLD "Files involved:\n" RESET
    "    src/nsan/NSanPass.cpp      <- injects the shadow IR\n"
    "    src/runtime/nsan_runtime.cpp <- checks at runtime\n\n");
  status_bar();
  wait_enter();
}

// ── Preset test menu ──────────────────────────────────────────────────────────
static void screen_presets() {
  clear_screen();
  header("Pre-built Tests");
  printf("\n"
    "  [1] All tests\n"
    "  [2] Catastrophic Cancellation\n"
    "  [3] Summation Accuracy (N=5000)\n"
    "  [b] Back\n\n");
  status_bar();

  char c = get_choice();
  clear_screen();
  header("Test Results");

  switch (c) {
    case '1': run_all_preset(); break;
    case '2': run_cancellation(); break;
    case '3': run_summation(5000); break;
    default: return;
  }

  printf("\n");
  status_bar();
  wait_enter();
}

// ── Main menu ────────────────────────────────────────────────────────────────
static void main_menu() {
  while (true) {
    clear_screen();
    printf(BOLD CYAN
      "\n+----------------------------------------------------------+\n"
      "|        NSan -- Numerical Stability Sanitizer             |\n"
      "|              Interactive Demo                            |\n"
      "+----------------------------------------------------------+\n"
      RESET);
    printf("\n"
      "  " BOLD "[1]" RESET "  Pre-built Tests (Cancellation, Summation)\n"
      "  " BOLD "[2]" RESET "  Custom Computation  (enter your own values)\n"
      "  " BOLD "[3]" RESET "  Summation Explorer  (choose N and range)\n"
      "  " BOLD "[4]" RESET "  View Computation Log\n"
      "  " BOLD "[5]" RESET "  Settings            (change warning threshold)\n"
      "  " BOLD "[6]" RESET "  How NSan Works       (LLVM IR view)\n"
      "  " BOLD "[7]" RESET "  About NSan\n"
      "  " BOLD "[q]" RESET "  Quit\n\n");
    status_bar();

    char c = get_choice();
    switch (c) {
      case '1': screen_presets();    break;
      case '2': screen_custom();     break;
      case '3': screen_summation();  break;
      case '4': screen_log();        break;
      case '5': screen_settings();   break;
      case '6': screen_llvm_view();  break;
      case '7': screen_about();      break;
      case 'q': case 'Q':
        clear_screen();
        printf(CYAN "\n  Thanks for using NSan Interactive Demo.\n"
               "  Total checks: %d  |  Warnings: %d\n\n" RESET,
               g_checks, g_warnings);
        return;
      default:
        printf(YELLOW "\n  Unknown option '%c' -- try again.\n" RESET, c);
        wait_enter("  Press Enter...");
    }
  }
}

// ── Entry point ───────────────────────────────────────────────────────────────
int main() {
  enable_ansi();
  srand((unsigned)time(nullptr));
  main_menu();
  return 0;
}
