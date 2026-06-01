#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float unstable_variance() {
    std::vector<float> data = {1e6f+1, 1e6f+2, 1e6f+3, 1e6f+4, 1e6f+5};
    float sum = 0, sum2 = 0;
    for (float x : data) { sum += x; sum2 += x * x; }
    float mean = sum / data.size();
    return sum2 / data.size() - mean * mean;
}

int main() {
    float result = unstable_variance();
    double ref = 2.0;
    printf("TC7 [Unstable Variance]   float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
