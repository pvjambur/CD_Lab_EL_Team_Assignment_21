#!/usr/bin/env bash
# benchmark.sh — Performance comparison: uninstrumented vs. instrumented

REPO="$(cd "$(dirname "$0")" && pwd)"
PLUGIN="$REPO/build/src/nsan/libnsan_pass.so"
RUNTIME="$REPO/build/src/runtime/libnsan_runtime.a"
COMPILER=${CXX:-clang++}

if [ ! -f "$PLUGIN" ] || [ ! -f "$RUNTIME" ]; then
  echo "[ERROR] Build the project first using ./build.sh"
  exit 1
fi

SYSROOT_FLAG=""
if [ "$(uname)" = "Darwin" ]; then
  SDK="$(xcrun --show-sdk-path 2>/dev/null || echo '')"
  if [ -n "$SDK" ]; then
    SYSROOT_FLAG="-isysroot $SDK"
  fi
fi

echo "=================================================="
echo "  NSan Performance Comparison Benchmark"
echo "=================================================="
echo ""

mkdir -p "$REPO/build/benchmarks"

# 5 test programs for baseline evaluation
PROGRAMS=("tc1_cancellation" "tc2_naive_sum" "tc3_kahan" "tc4_alternating" "tc7_variance")

for prog in "${PROGRAMS[@]}"; do
  src_file="$REPO/tests/${prog}.cpp"
  
  # 1. Compile uninstrumented
  $COMPILER $SYSROOT_FLAG -O1 -std=c++17 \
    -nostdinc++ -isystem "$SDK/usr/include/c++/v1" -isystem "$SDK/usr/include" \
    "$src_file" "$RUNTIME" -o "$REPO/build/benchmarks/${prog}_plain" 2>/dev/null
  
  # 2. Compile instrumented
  ./nsan-clang++ $SYSROOT_FLAG -O1 -std=c++17 \
    -nostdinc++ -isystem "$SDK/usr/include/c++/v1" -isystem "$SDK/usr/include" \
    -fsanitize=numerical "$src_file" -o "$REPO/build/benchmarks/${prog}_nsan" 2>/dev/null

  echo "Benchmarking: $prog"
  
  echo "  Uninstrumented:"
  time "$REPO/build/benchmarks/${prog}_plain" > /dev/null 2>&1
  
  echo "  Instrumented (NSan):"
  time "$REPO/build/benchmarks/${prog}_nsan" > /dev/null 2>&1
  
  echo "--------------------------------------------------"
done

echo "Benchmark complete."
