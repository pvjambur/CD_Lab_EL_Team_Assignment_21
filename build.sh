#!/usr/bin/env bash
# build.sh — Portable Build Script

REPO="$(cd "$(dirname "$0")" && pwd)"


if [ -z "$LLVM_BUILD" ]; then
  if [ -d "$HOME/python/llvm-workspace/build" ]; then
    LLVM_BUILD="$HOME/python/llvm-workspace/build"
  elif command -v llvm-config &>/dev/null; then
    LLVM_BUILD="$(dirname $(dirname $(which llvm-config)))"
  else
    echo "[ERROR] LLVM build path not found. Set the \$LLVM_BUILD variable."
    exit 1
  fi
fi

echo ""
echo "=================================================="
echo "  NSan Build Script"
echo "  Repo: $REPO"
echo "  LLVM: $LLVM_BUILD"
echo "=================================================="
echo ""


echo "[1/4] Cleaning project-local build artifacts..."
rm -rf "$REPO/build"
echo "      Done."


echo "[2/4] Resetting build directory..."
mkdir -p "$REPO/build"
cd "$REPO/build"
echo "      Done."


echo "[3/4] Running CMake..."
cmake "$REPO" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLLVM_DIR="$LLVM_BUILD/lib/cmake/llvm" \
  -DCMAKE_C_COMPILER="$LLVM_BUILD/bin/clang" \
  -DCMAKE_CXX_COMPILER="$LLVM_BUILD/bin/clang++" \
  -DENABLE_LTO=OFF \
  -DENABLE_SHADOW_OPTIMIZATION=ON

if [ $? -ne 0 ]; then
  echo "[ERROR] CMake configuration failed."
  exit 1
fi
echo "      CMake configure succeeded."


ninja
if [ $? -ne 0 ]; then
  echo "[ERROR] Build failed."
  exit 1
fi

echo "=================================================="
echo "  Build succeeded!"
echo "=================================================="