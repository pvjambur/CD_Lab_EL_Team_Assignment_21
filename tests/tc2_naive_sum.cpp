#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float naive_sum(const std::vector<float>& v) {
    float sum = 0.0f;
    for (float x : v) sum += x;
    return sum;
}

int main() {
    int N = 1000000;
    std::vector<float> data(N, 0.1f);
    float  result = naive_sum(data);
    double ref    = (double)N * 0.1;
    printf("TC2 [Naive Float Sum]     float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
