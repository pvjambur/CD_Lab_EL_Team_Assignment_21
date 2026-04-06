// configuration.h — NSan runtime configuration
//
// All tunable parameters. Values are loaded from environment variables
// at startup by init_nsan_configuration() in nsan_runtime.cpp.
//
// Environment variables:
//   NSAN_EPSILON          absolute error threshold       (default 1e-5)
//   NSAN_REL_EPSILON      relative error threshold       (default 1e-4)
//   NSAN_VERBOSITY        0=quiet 1=warn 2=verbose       (default 1)
//   NSAN_CHECK_FREQ       1=every op, N=every Nth op     (default 1)
//   NSAN_AGGRESSIVE_OPT   1=enable compiler hint paths   (default 0)
//   NSAN_SHADOW_THRESHOLD precision-upgrade threshold    (default 1e-10)

#ifndef NSAN_CONFIGURATION_H
#define NSAN_CONFIGURATION_H

// ── Default values ─────────────────────────────────────────────────────────
// Target: 2–20x overhead. Higher CHECK_FREQ reduces calls but misses errors.
constexpr double DEFAULT_NSAN_EPSILON          = 1e-5;
constexpr double DEFAULT_NSAN_REL_EPSILON      = 1e-4;
constexpr int    DEFAULT_NSAN_VERBOSITY        = 1;
constexpr int    DEFAULT_NSAN_CHECK_FREQ       = 1;     // every operation
constexpr int    DEFAULT_NSAN_AGGRESSIVE_OPT   = 0;
constexpr double DEFAULT_NSAN_SHADOW_THRESHOLD = 1e-10;

// ── Live config struct (populated by init_nsan_configuration) ──────────────
struct NSanConfig {
    double epsilon;
    double rel_epsilon;
    int    verbosity;
    int    check_freq;       // only check every Nth operation (performance knob)
    int    aggressive_opt;
    double shadow_threshold; // upgrade float→double shadow to double→long double
                             // when relative error is below this (Phase 4)
};

/// Called once at library load (via __attribute__((constructor))).
/// Reads environment variables and fills the global NSanConfig.
void init_nsan_configuration();

/// Returns a const reference to the global config. Inline so the compiler
/// can see through it during LTO and optimise check_freq comparisons away.
const NSanConfig &nsan_config();

#endif // NSAN_CONFIGURATION_H
