#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float poly() {
    float x = 1.0001f;
    return x * x * x - 3.0f * x + 2.0f;
}

int main() {
    float result = poly();
    double ref = 31.046553f;
    printf("TC5 [Polynomial x=1.0001] float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
