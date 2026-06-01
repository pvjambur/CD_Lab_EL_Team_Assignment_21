#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>

extern "C" void __nsan_check_consistency(float orig, double shadow);

float kahan_sum(const std::vector<float>& v) {
    float sum = 0.0f, c = 0.0f;
    for (float x : v) {
        float y = x - c;
        float t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}

int main() {
    int N = 1000000;
    std::vector<float> data(N, 0.1f);
    float  result = kahan_sum(data);
    double ref    = (double)N * 0.1;
    printf("TC3 [Kahan Sum — SILENT]  float=%.6f  ref=%.6f  rel_err=%.2e\n", result, ref, fabs((double)result - ref) / fabs(ref));
    __nsan_check_consistency(result, ref);
    return 0;
}
