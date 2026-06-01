#!/usr/bin/env bash
# functional/test_pipeline.sh — E2E pass integration validation

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$REPO/tests/bins/tc1_cancellation"

echo "[*] Checking if binary exists: $BIN"
if [ ! -f "$BIN" ]; then
  echo "[!] Binary not found. Running build and run suite..."
  "$REPO/run.sh"
fi

echo "[*] Running TC1 through pipeline..."
OUTPUT="$("$BIN" 2>&1)"

if echo "$OUTPUT" | grep -q "NSan: numerical inconsistency"; then
  echo "[PASS] Functional test: Output contains expected precision warning."
  exit 0
else
  echo "[FAIL] Functional test: Warning not found on catastrophic cancellation test."
  exit 1
fi
