#include <cstdio>
#include <cmath>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

int main() {
    float result = fmaf(1e-7f, 1e7f, -1.0f);
    double ref = 1.4210855e-14f;
    printf("TC9 [FMA Cancel]          float=%.2e  ref=%.2e  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
