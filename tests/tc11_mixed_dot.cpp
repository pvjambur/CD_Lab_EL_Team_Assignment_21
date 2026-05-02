#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

int main() {
    float result = 100.0f * 1.0f + 0.001f * 10000.0f;
    double ref = 500.0f;
    printf("TC11 [Mixed Magnitude]    float=%.2f  ref=%.2f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
