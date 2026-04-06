// nsan_runtime.cpp - NSan Runtime Library
//
// Provides runtime support for the NSan instrumentation pass.
// Includes consistency checking between original and shadow values.
//
// See: private_config/INSTRUCTIONS.md Phase 3 for details.

#include "nsan_runtime.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

extern "C" {

// Configuration (from environment variables)
static double NSAN_EPSILON = 1e-5;
static double NSAN_REL_EPSILON = 1e-5;
static int NSAN_VERBOSITY = 1;

void __nsan_check_consistency_float(float original, double shadow) {
  double original_d = static_cast<double>(original);
  double abs_error = fabs(original_d - shadow);
  double rel_error =
      (fabs(shadow) > 1e-15) ? abs_error / fabs(shadow) : abs_error;

  if (abs_error > NSAN_EPSILON || rel_error > NSAN_REL_EPSILON) {
    fprintf(stderr,
            "NSan Warning: Numerical inconsistency detected\n"
            "  Original (float): %.15g\n"
            "  Shadow (double):  %.15g\n"
            "  Relative error:   %.2e\n",
            original_d, shadow, rel_error);
  }
}

void __nsan_check_consistency_double(double original, long double shadow) {
  // TODO: Implement double/quad consistency checking
}

} // extern "C"
