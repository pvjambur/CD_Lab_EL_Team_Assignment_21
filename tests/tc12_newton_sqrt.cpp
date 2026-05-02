#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

int main() {
    float x = 1.5f;
    for(int i = 0; i < 5; ++i) x = 0.5f * (x + 2.0f / x);
    float result = x;
    double ref = 1.4142136f;
    printf("TC12 [Newton sqrt(2)]     float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
