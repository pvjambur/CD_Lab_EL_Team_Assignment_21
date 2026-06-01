// nsan_runtime.h - NSan Runtime Library Header
//
// External C interface for LLVM-instrumented code to call.

#ifndef NSAN_RUNTIME_H
#define NSAN_RUNTIME_H

#include <cstddef>
#include <cstdint>

extern "C" {

// Consistency checking
void __nsan_check_consistency_float(float original, double shadow);
void __nsan_check_consistency_double(double original, long double shadow);

// Memory operations
void __nsan_on_load(void *addr, size_t size);
void __nsan_on_store(void *addr, void *value, size_t size);

// Function boundaries
// func_addr identifies which function's shadow stack we're pushing to
void __nsan_push_shadow_parameters(uintptr_t func_addr, ...);
double __nsan_pop_shadow_return();
void __nsan_push_shadow_return(double shadow);

// User-facing interface
void __nsan_check_float(float f);
void __nsan_dump_shadow_mem(void *addr, size_t size);
void __nsan_resume_float(float f);

} // extern "C"

#endif // NSAN_RUNTIME_H
