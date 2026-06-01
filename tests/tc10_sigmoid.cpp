#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

int main() {
    float result = 1.0f / (1.0f + expf(-(-15.0f)));
    double ref = 3.0590223e-07f;
    printf("TC10 [Sigmoid x=-15]      float=%.4e  ref=%.4e  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
