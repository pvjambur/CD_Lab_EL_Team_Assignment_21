#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float alternating() {
    int N = 50000;
    float s = 0.0f;
    int sign = 1;
    for (int i = 1; i <= N; ++i) {
        s += sign * (1.0f / (float)i);
        sign = -sign;
    }
    return s;
}

int main() {
    float result = alternating();
    double ref = 0.693122f;
    printf("TC4 [Smearing 0.1f*1000]  float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
