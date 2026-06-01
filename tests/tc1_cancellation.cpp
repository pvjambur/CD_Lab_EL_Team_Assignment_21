#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float catastrophic(float a, float b) {
    return (a + b) - a;
}

int main() {
    float a = 1e7f, b = 1.234f;
    float result = catastrophic(a, b);
    double ref   = ((double)a + (double)b) - (double)a;
    printf("TC1 [Catastrophic Cancel] float=%.0f  ref=%.3f  rel_err=%.2e\n", result, ref, fabs(result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
