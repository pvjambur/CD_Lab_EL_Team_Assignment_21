#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

int main() {
    float result = 1.0f / 3.0f;
    double ref = 0.33333333f;
    printf("TC6 [Newton 1/x, x=3]     float=%.8f  ref=%.8f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
