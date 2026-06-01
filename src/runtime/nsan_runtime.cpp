#include "shadow_memory.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "../../include/internal/configuration.h"

extern "C" {

void __nsan_check_consistency(float orig, double shadow) {
    init_nsan_configuration();
    
    if (orig == 0.f && shadow == 0.0) return;
    double denom = std::fabs(shadow) > 1e-300 ? std::fabs(shadow) : 1e-300;
    double rel_err = std::fabs((double)orig - shadow) / denom;
    
    double eps = get_nsan_config().epsilon;
    
    if (rel_err > eps) {
        std::fprintf(stderr,
            "\033[31m[WARN]\033[0m NSan: numerical inconsistency\n"
            "  float32 = %+.10g\n  shadow  = %+.10g\n"
            "  rel_err = %.3e  (threshold %.1e)\n",
            (double)orig, shadow, rel_err, eps);
    } else if (get_nsan_config().verbosity >= 2) {
        std::fprintf(stderr, "\033[32m[PASS]\033[0m rel_err=%.3e\n", rel_err);
    }
}

// Stubs for API compliance
void __nsan_check_float(float f) { __nsan_check_consistency(f, (double)f); }
void __nsan_resume_float(float f) {}
void __nsan_dump_shadow_mem() {}

// Heap Shadow Memory Accessors
void* __nsan_shadow_ptr_load(void* ptr) {
    return __nsan_get_shadow_address(ptr, sizeof(float));
}

void* __nsan_shadow_ptr_store(void* ptr) {
    return __nsan_get_shadow_address(ptr, sizeof(float));
}

void __nsan_set_shadow_stack_tag(int64_t fn_addr) {}
void __nsan_push_shadow_arg(double v, int32_t i) {}
double __nsan_load_shadow_arg(float orig, int64_t fn_addr, int32_t i) { return (double)orig; }

void __nsan_set_shadow_return(double v, int64_t fn_addr) {}
double __nsan_get_shadow_return(int64_t fn_addr) { return 0.0; }

} // extern "C"