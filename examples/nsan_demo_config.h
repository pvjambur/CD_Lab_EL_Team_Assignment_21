// nsan_demo_config.h
// Shared runtime configuration for all three demo executables.
// Mirrors the real nsan_runtime.cpp config — same env vars, same defaults.
//
// Include this once at the top of each demo .cpp file.

#ifndef NSAN_DEMO_CONFIG_H
#define NSAN_DEMO_CONFIG_H

#include <cstdio>
#include <cstdlib>

struct NSanDemoConfig {
    double epsilon;           // NSAN_EPSILON         default 1e-5
    double rel_epsilon;       // NSAN_REL_EPSILON      default 1e-4
    int    verbosity;         // NSAN_VERBOSITY        0=quiet 1=warn 2=verbose
    int    check_freq;        // NSAN_CHECK_FREQ       1=every op, N=every Nth
    double shadow_threshold;  // NSAN_SHADOW_THRESHOLD default 1e-10 (Phase 4)
};

static NSanDemoConfig g_nsan_cfg = {1e-5, 1e-4, 1, 1, 1e-10};
static int            g_nsan_checks   = 0;
static int            g_nsan_warnings = 0;
static int            g_nsan_freq_ctr = 0;  // internal counter for check_freq

static void nsan_load_config() {
    auto rd = [](const char *v, double d) { const char *s = getenv(v); return s ? atof(s) : d; };
    auto ri = [](const char *v, int    d) { const char *s = getenv(v); return s ? atoi(s) : d; };

    g_nsan_cfg.epsilon          = rd("NSAN_EPSILON",          1e-5);
    g_nsan_cfg.rel_epsilon      = rd("NSAN_REL_EPSILON",      1e-4);
    g_nsan_cfg.verbosity        = ri("NSAN_VERBOSITY",        1);
    g_nsan_cfg.check_freq       = ri("NSAN_CHECK_FREQ",       1);
    g_nsan_cfg.shadow_threshold = rd("NSAN_SHADOW_THRESHOLD", 1e-10);
    if (g_nsan_cfg.check_freq < 1) g_nsan_cfg.check_freq = 1;
}

// Core check — mirrors __nsan_check_consistency_float in nsan_runtime.cpp.
// Returns true if a warning was fired.
static bool nsan_check(float original, long double shadow, const char *label) {
    ++g_nsan_freq_ctr;
    // NSAN_CHECK_FREQ: only check every Nth operation (performance knob)
    if (g_nsan_cfg.check_freq > 1 && g_nsan_freq_ctr % g_nsan_cfg.check_freq != 0)
        return false;

    ++g_nsan_checks;
    double o = static_cast<double>(original);
    double s = static_cast<double>(shadow);
    double ae = o > s ? o - s : s - o;  // fabs without <cmath>
    if (ae < 0) ae = -ae;
    double denom = (s > o ? s : o);
    if (denom < 0) denom = -denom;
    double re = (denom > 1e-300) ? ae / denom : ae;

    bool warn = (ae > g_nsan_cfg.epsilon || re > g_nsan_cfg.rel_epsilon);

    if (warn) {
        ++g_nsan_warnings;
        // Verbose trace even for warnings
        (void)label;  // used by callers that print their own output
    } else if (g_nsan_cfg.verbosity >= 2) {
        fprintf(stderr, "[NSan trace] %-28s  f32=%.8g  ref=%.8g  rel=%.2e  OK\n",
                label, o, s, re);
    }
    return warn;
}

#endif // NSAN_DEMO_CONFIG_H
