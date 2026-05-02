#!/usr/bin/env bash
# run.sh — Run and evaluate all test cases in the tests/ directory

REPO="$(cd "$(dirname "$0")" && pwd)"
PLUGIN="$REPO/build/src/nsan/libnsan_pass.so"
RUNTIME="$REPO/build/src/runtime/libnsan_runtime.a"
TESTS_DIR="$REPO/tests"
BINS_DIR="$REPO/tests/bins"

echo ""
echo "=================================================="
echo "  NSan Modular Test Runner"
echo "=================================================="
echo ""

# ── Check build outputs exist ──────────────────────────────────────────────
if [ ! -f "$PLUGIN" ]; then
  echo "[ERROR] Plugin not found: $PLUGIN"
  echo "        Run ./build.sh first."
  exit 1
fi

if [ ! -f "$RUNTIME" ]; then
  echo "[ERROR] Runtime not found: $RUNTIME"
  echo "        Run ./build.sh first."
  exit 1
fi

# ── Platform Detection and Flags ───────────────────────────────────────────
SYSROOT_FLAG=""
if [ "$(uname)" = "Darwin" ]; then
  SDK="$(xcrun --show-sdk-path 2>/dev/null || echo '')"
  if [ -n "$SDK" ]; then
    SYSROOT_FLAG="-isysroot $SDK"
  fi
fi

mkdir -p "$BINS_DIR"

# ── Compile test files ─────────────────────────────────────────────────────
echo "[1/2] Compiling test cases from tests/..."
for test_file in "$TESTS_DIR"/*.cpp; do
  test_name=$(basename "$test_file" .cpp)
  bin_path="$BINS_DIR/$test_name"
  
  COMPILER=${CXX:-clang++}

  if [ "$(uname)" = "Darwin" ]; then
    $COMPILER $SYSROOT_FLAG -O1 -std=c++17 \
      -nostdinc++ \
      -isystem "$SDK/usr/include/c++/v1" \
      -isystem "$SDK/usr/include" \
      -fpass-plugin="$PLUGIN" \
      "$test_file" "$RUNTIME" -o "$bin_path" 2>&1
  else
    $COMPILER -O1 -std=c++17 -fpass-plugin="$PLUGIN" \
      -Wno-pass-failed -fno-crash-diagnostics \
      "$test_file" "$RUNTIME" -o "$bin_path" 2>&1
  fi
  
  if [ $? -ne 0 ]; then
    echo "  [FAIL] Compilation of $test_name failed."
    exit 1
  fi
done
echo "      All test files compiled."
echo ""

# ── Run test files ─────────────────────────────────────────────────────────
echo "[2/2] Running individual test binaries..."
echo "--------------------------------------------------"
for bin_path in "$BINS_DIR"/*; do
  test_name=$(basename "$bin_path")
  echo "Executing $test_name..."
  "$bin_path" 2>&1
  echo "──────────────────────────────────────────────────"
done
echo ""
echo "[OK] All tests executed successfully."