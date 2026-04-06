// simple_computation.cpp - NSan Example
//
// Demonstrates naive vs compensated summation.
// Compile with: clang++ -fsanitize=numerical simple_computation.cpp -o simple
// Run:         ./simple

#include <cstdio>
#include <vector>

// Naive summation — numerically unstable
float NaiveSum(const std::vector<float> &values) {
  float sum = 0.0f;
  for (float v : values) {
    sum += v; // Precision lost here for large arrays
  }
  return sum;
}

// Kahan summation — numerically stable
float KahanSum(const std::vector<float> &values) {
  float sum = 0.0f;
  float c = 0.0f; // Correction term
  for (float v : values) {
    float y = v - c;
    float t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }
  return sum;
}

int main() {
  std::vector<float> values(10000000);
  for (int i = 0; i < 10000000; i++) {
    values[i] = static_cast<float>(rand()) / RAND_MAX;
  }

  float naive = NaiveSum(values);
  float kahan = KahanSum(values);

  printf("Naive Sum:  %.10f\n", naive);
  printf("Kahan Sum:  %.10f\n", kahan);
  printf("Difference: %.10e\n", naive - kahan);

  return 0;
}
