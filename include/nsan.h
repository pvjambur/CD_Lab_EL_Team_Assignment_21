// nsan.h - NSan Public API
//
// User-callable functions for interacting with the
// Numerical Stability Sanitizer at runtime.

#ifndef NSAN_H
#define NSAN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Explicitly check a float value for consistency.
void __nsan_check_float(float f);

/// Dump shadow memory contents for debugging.
void __nsan_dump_shadow_mem(void *addr, size_t size);

/// Resume computation from original value (drop shadow).
void __nsan_resume_float(float f);

#ifdef __cplusplus
}
#endif

#endif // NSAN_H
