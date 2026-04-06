# tests/

Test suite — not yet written. See `docs/PROJECT_STATUS.md` for what's needed.

| Directory | Purpose |
|-----------|---------|
| `unit/` | Unit tests for individual pass and runtime functions |
| `functional/` | End-to-end tests — compile a program, run it, check warnings |
| `benchmarks/` | Performance measurements (target: < 20x overhead) |

`CMakeLists.txt` has the test targets commented out — uncomment and add test
files when Phase 3 begins.
