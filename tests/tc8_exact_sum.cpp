#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float exact_sum() {
    float s = 0;
    for(int i = 0; i < 1000; ++i) s += 1.0f;
    return s;
}

int main() {
    float result = exact_sum();
    double ref = 1000.0f;
    printf("TC8 [Exact Int Sum]       float=%.2f  ref=%.2f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
