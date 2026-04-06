// nsan_runtime.cpp — NSan Runtime Library
//
// Called by every instrumented binary after each float operation.
// Performance of __nsan_check_consistency_float() directly determines
// NSan's overhead. Target: 2–20x (Google NSan achieves 2.3x float→double).
//
// Optimisation levers (set in CMake or environment):
//   -ffast-math on this TU (shadow math only, never user-visible)
//   -finline-functions (check function inlined at call sites via LTO)
//   NSAN_CHECK_FREQ=100  (sample every 100th op instead of every op)

#include "nsan_runtime.h"
#include "configuration.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ── Global config (single instance, loaded once at startup) ──────────────────
static NSanConfig g_config = {
    DEFAULT_NSAN_EPSILON,
    DEFAULT_NSAN_REL_EPSILON,
    DEFAULT_NSAN_VERBOSITY,
    DEFAULT_NSAN_CHECK_FREQ,
    DEFAULT_NSAN_AGGRESSIVE_OPT,
    DEFAULT_NSAN_SHADOW_THRESHOLD
};

const NSanConfig &nsan_config() { return g_config; }

// ── Configuration loader ──────────────────────────────────────────────────────
// __attribute__((constructor)) ensures this runs before main(), so the first
// instrumented operation already has the correct thresholds.
__attribute__((constructor))
void init_nsan_configuration() {
    auto read_d = [](const char *var, double def) -> double {
        const char *v = getenv(var);
        return v ? atof(v) : def;
    };
    auto read_i = [](const char *var, int def) -> int {
        const char *v = getenv(var);
        return v ? atoi(v) : def;
    };

    g_config.epsilon          = read_d("NSAN_EPSILON",          DEFAULT_NSAN_EPSILON);
    g_config.rel_epsilon      = read_d("NSAN_REL_EPSILON",      DEFAULT_NSAN_REL_EPSILON);
    g_config.verbosity        = read_i("NSAN_VERBOSITY",        DEFAULT_NSAN_VERBOSITY);
    g_config.check_freq       = read_i("NSAN_CHECK_FREQ",       DEFAULT_NSAN_CHECK_FREQ);
    g_config.aggressive_opt   = read_i("NSAN_AGGRESSIVE_OPT",   DEFAULT_NSAN_AGGRESSIVE_OPT);
    g_config.shadow_threshold = read_d("NSAN_SHADOW_THRESHOLD", DEFAULT_NSAN_SHADOW_THRESHOLD);

    if (g_config.check_freq < 1) g_config.check_freq = 1;

    if (g_config.verbosity >= 2) {
        fprintf(stderr,
            "[NSan] config loaded:\n"
            "  NSAN_EPSILON=%.2e  NSAN_REL_EPSILON=%.2e\n"
            "  NSAN_VERBOSITY=%d  NSAN_CHECK_FREQ=%d\n"
            "  NSAN_AGGRESSIVE_OPT=%d  NSAN_SHADOW_THRESHOLD=%.2e\n",
            g_config.epsilon, g_config.rel_epsilon,
            g_config.verbosity, g_config.check_freq,
            g_config.aggressive_opt, g_config.shadow_threshold);
    }
}

// ── Core consistency check ────────────────────────────────────────────────────
// This function is injected after every `fadd float` / `fsub float` / etc.
// by NSanPass.cpp. Keep it as small and branch-free as possible.
//
// Optimisation notes:
//   - check_freq: when set to N>1, only 1/N calls actually compare
//     (reduces overhead from O(ops) to O(ops/N), trades coverage for speed)
//   - LTO allows the compiler to inline this at each call site
//   - fabs() compiles to a single instruction on x86 (no branch)

extern "C" {

void __nsan_check_consistency_float(float original, double shadow) {
    // Fast-path: NSAN_CHECK_FREQ sampling
    if (g_config.check_freq > 1) {
        static int _counter = 0;
        if (++_counter % g_config.check_freq != 0) return;
    }

    double orig_d   = static_cast<double>(original);
    double abs_err  = fabs(orig_d - shadow);
    // Relative error: |a-b| / max(|a|,|b|)  — avoids division-by-near-zero
    double denom    = fabs(shadow) > fabs(orig_d) ? fabs(shadow) : fabs(orig_d);
    double rel_err  = (denom > 1e-300) ? abs_err / denom : abs_err;

    bool triggered = (abs_err > g_config.epsilon ||
                      rel_err > g_config.rel_epsilon);

    if (__builtin_expect(triggered, 0)) {  // errors are rare: hint branch predictor
        fprintf(stderr,
            "NSan Warning: Numerical inconsistency detected\n"
            "  Original (float32) : %.15g\n"
            "  Shadow   (double)  : %.15g\n"
            "  Absolute error     : %.4e\n"
            "  Relative error     : %.4e  (threshold %.2e)\n",
            orig_d, shadow, abs_err, rel_err, g_config.rel_epsilon);
    } else if (g_config.verbosity >= 2) {
        fprintf(stderr,
            "[NSan trace] float=%.10g  shadow=%.10g  rel=%.2e  OK\n",
            orig_d, shadow, rel_err);
    }
}

void __nsan_check_consistency_double(double original, long double shadow) {
    double orig_d   = original;
    double shad_d   = static_cast<double>(shadow);
    double abs_err  = fabs(orig_d - shad_d);
    double denom    = fabs(shad_d) > fabs(orig_d) ? fabs(shad_d) : fabs(orig_d);
    double rel_err  = (denom > 1e-300) ? abs_err / denom : abs_err;

    if (abs_err > g_config.epsilon || rel_err > g_config.rel_epsilon) {
        fprintf(stderr,
            "NSan Warning: double precision inconsistency\n"
            "  Original (double)    : %.15g\n"
            "  Shadow   (long dbl)  : %.15Lg\n"
            "  Relative error       : %.4e\n",
            orig_d, shadow, rel_err);
    }
}

// ── Memory operations (Phase 3) ───────────────────────────────────────────────
// Called by the pass when it encounters load/store of FP values in memory.
// Currently stubs — shadow_memory.cpp provides the address mapping.
void __nsan_on_load(void * /*addr*/, size_t /*size*/) {}
void __nsan_on_store(void * /*addr*/, void * /*value*/, size_t /*size*/) {}

// ── Shadow stack (Phase 3) ───────────────────────────────────────────────────
// Thread-local stack used to pass shadow values across function call
// boundaries. thread_local is essential — avoids data races in multithreaded
// programs (common pitfall: using a global shadow stack).
static thread_local double tl_shadow_return = 0.0;

void __nsan_push_shadow_return(double shadow) {
    tl_shadow_return = shadow;
}

double __nsan_pop_shadow_return() {
    return tl_shadow_return;
}

void __nsan_push_shadow_parameters(uintptr_t /*func_addr*/, ...) {
    // Phase 3: implement variadic shadow parameter passing here
}

// ── User-callable API ─────────────────────────────────────────────────────────
void __nsan_check_float(float f) {
    // When user explicitly requests a check: compare f against its shadow.
    // Phase 3: retrieve the shadow from the shadow stack or memory.
    if (g_config.verbosity >= 1)
        fprintf(stderr, "[NSan] explicit check: float=%.10g\n", (double)f);
}

void __nsan_dump_shadow_mem(void *addr, size_t size) {
    fprintf(stderr, "[NSan] shadow memory dump: addr=%p size=%zu\n", addr, size);
    // Phase 3: walk shadow_memory_map and print entries in range
}

void __nsan_resume_float(float f) {
    // Drop the shadow for this value — resume from original.
    // Useful when user code calls an external library that NSan can't shadow.
    (void)f;
}

} // extern "C"
