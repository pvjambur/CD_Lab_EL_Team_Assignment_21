# NSan Project: Repository Structure & Git Configuration

## .gitignore Template

```
# Build artifacts
build/
dist/
*.o
*.a
*.so
*.dylib
*.dll

# CMake
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile

# LLVM/Clang generated files
*.ll
*.bc
*.s
*.ast

# IDE files
.vscode/
.idea/
*.swp
*.swo
*~
.DS_Store

# Testing
test_result*
*.gcda
*.gcno
*.coverage

# Documentation generated
docs/_build/
doxygen/

# Temporary files
*.tmp
*.log
core
core.*
```

## Directory Structure Template

```bash
# Create initial project structure

mkdir -p nsan-project/{src/{nsan,runtime},include/{nsan,internal},tests/{unit,functional,benchmarks},examples,cmake,docs}

# Create initial files
touch README.md
touch PROBLEM_STATEMENT.md
touch SKILLS.md
touch INSTRUCTIONS.md
touch QUICK_REFERENCE.md
touch CMakeLists.txt
touch .gitignore

# Create source files
touch src/nsan/NSanPass.cpp
touch src/nsan/NSanPass.h
touch src/nsan/ShadowValueMap.cpp
touch src/nsan/ShadowValueMap.h
touch src/runtime/nsan_runtime.cpp
touch src/runtime/nsan_runtime.h
touch src/runtime/shadow_memory.cpp
touch src/runtime/shadow_memory.h

# Create headers
touch include/nsan.h
touch include/nsan_interface.h

# Create tests
touch tests/unit/test_instrumentation.cpp
touch tests/functional/test_naive_summation.cpp
touch tests/benchmarks/benchmark.cpp

# Create examples
touch examples/simple_computation.cpp
```

## Git Initialization

```bash
cd nsan-project
git init
git add -A
git commit -m "Initial commit: NSan project structure and documentation"

# Set up remote (replace with your repo URL)
git remote add origin https://github.com/YOUR_GITHUB/nsan-project.git
git branch -M main
git push -u origin main
```

## Commit Message Convention

```
[COMPONENT] Brief description (50 char max)

Detailed explanation if needed.
Explain what changed and why.
Link to related issues/PRs.

Component tags:
- [PASS]: LLVM transformation pass
- [RUNTIME]: Runtime library code
- [TEST]: Test suite additions
- [DOC]: Documentation updates
- [BUILD]: Build system changes
- [INFRA]: Project infrastructure

Example:
[PASS] Instrument floating-point binary operations

- Detect fadd, fsub, fmul, fdiv instructions
- Create parallel shadow computations in double precision
- Maintain shadow value map for tracking

Related: PR #5, Issue #2
Tested: test_binary_ops.cpp (passed)
Performance: ~2.3x overhead on compensated_sum benchmark
```

## GitHub Integration

### GitHub Actions CI/CD Example

```yaml
# .github/workflows/test.yml
name: NSan Tests

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y llvm-dev clang cmake
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make
    
    - name: Run tests
      run: |
        cd build
        ctest --verbose
    
    - name: Upload coverage
      uses: codecov/codecov-action@v1
```

## Branch Strategy

```
main (stable, all tests passing)
  ├── develop (integration branch)
  │   ├── feature/ir-instrumentation
  │   ├── feature/runtime-checks
  │   ├── feature/diagnostics
  │   └── feature/performance-optimization
  │
  └── hotfix/critical-bug-fix
```

## Pull Request Template

```markdown
## Description
Brief summary of changes

## Type of Change
- [ ] New feature
- [ ] Bug fix
- [ ] Performance improvement
- [ ] Documentation update
- [ ] Refactoring

## Changes Made
- Detailed list of changes
- What was implemented
- How it was tested

## Testing
- [ ] Unit tests added/updated
- [ ] All tests passing: `ctest --verbose`
- [ ] No compiler warnings
- [ ] Performance impact measured

## Performance Impact
- Runtime overhead: X.Xx
- Memory overhead: Xx
- Benchmark results: ...

## Screenshots/Logs
(if applicable)

## Checklist
- [ ] Code follows project style
- [ ] Self-review completed
- [ ] Tests passing locally
- [ ] Documentation updated
- [ ] No breaking changes
```

## Release Checklist

```
Before v1.0 release:

- [ ] All issues in milestone closed
- [ ] Code coverage >80%
- [ ] Documentation complete
- [ ] Examples working
- [ ] Performance benchmarks acceptable
- [ ] All tests passing
- [ ] No known bugs remaining
- [ ] Changelog updated
- [ ] Version bumped in CMakeLists.txt
- [ ] Release notes written
- [ ] Git tag created: v1.0
- [ ] GitHub release published

Example version: 1.0.0
- Implements core NSan functionality
- LLVM IR instrumentation
- Runtime consistency checking
- ~2-20x performance overhead
- <5% false positive rate
```

## Documentation Standards

### Code Comments
```cpp
/// Brief description of what function does
/// 
/// Detailed explanation of behavior, edge cases, etc.
/// 
/// @param param1 What this parameter is
/// @param param2 What this parameter is
/// @return What is returned
/// @note Any important notes
/// @see Related functions
void myFunction(int param1, const string& param2);
```

### Commit Documentation
```
Every commit should explain:
1. WHAT was changed
2. WHY it was changed
3. HOW the change works
4. TESTING done to verify
```

### Issue Reporting Template

```markdown
## Bug Report

### Description
Clear description of the bug

### Steps to Reproduce
1. Step 1
2. Step 2
3. Step 3

### Expected Behavior
What should happen

### Actual Behavior
What actually happened

### Environment
- OS: (Ubuntu 20.04, macOS, etc.)
- LLVM Version: (llvm-12, llvm-13, etc.)
- Clang Version: ...

### Reproducible Code
```cpp
// Minimal code to reproduce the issue
float x = compute();
```

### Error Output
```
Include any error messages or logs
```

### Potential Fix
(if you have an idea)
```

## Contributor Guide

```
1. Fork the repository
2. Create a feature branch: git checkout -b feature/my-feature
3. Commit changes: git commit -m "[COMPONENT] Description"
4. Push to branch: git push origin feature/my-feature
5. Create Pull Request
6. Address review feedback
7. Merge once approved
```

## Testing Before Push

```bash
# Run locally before pushing
cd build
cmake ..
make

# Run tests
ctest --verbose

# Check for compiler warnings
cmake --build . -- -Werror

# Run benchmarks
./tests/benchmarks/benchmark

# Check code style (if linter configured)
clang-format -i src/**/*.cpp
```

## Useful Git Commands

```bash
# View detailed history
git log --oneline --graph --all

# Check what will be committed
git diff --cached

# Stash work temporarily
git stash
git stash pop

# Rebase for clean history
git rebase -i HEAD~3

# View file history
git log -p -- filename.cpp

# Find where bug was introduced
git bisect start
git bisect bad  # current version is broken
git bisect good v0.9  # last known good version
```

## CI/CD Integration

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Run tests before allowing commit
cd build
ctest --verbose
if [ $? -ne 0 ]; then
    echo "Tests failed. Commit aborted."
    exit 1
fi

exit 0
```

Install with:
```bash
chmod +x .git/hooks/pre-commit
```

### Setting Up for Claude Code

Create `.claudeconfig` in root:
```json
{
  "projectName": "NSan: Numerical Stability Sanitizer",
  "documentationFiles": [
    "README.md",
    "PROBLEM_STATEMENT.md",
    "INSTRUCTIONS.md",
    "SKILLS.md",
    "QUICK_REFERENCE.md"
  ],
  "buildCommand": "mkdir -p build && cd build && cmake .. && make",
  "testCommand": "cd build && ctest --verbose",
  "primarySourceDir": "src/",
  "primaryTestDir": "tests/",
  "language": "C++",
  "llvmVersion": "10+",
  "keyFiles": [
    "src/nsan/NSanPass.cpp",
    "src/runtime/nsan_runtime.cpp",
    "include/nsan.h"
  ]
}
```

---

## Getting Started Checklist

After cloning the repository:

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure project
cmake ..

# 3. Build everything
make

# 4. Run tests
ctest --verbose

# 5. Check documentation
less ../README.md
less ../INSTRUCTIONS.md

# 6. Start developing
# Read SKILLS.md for patterns
# Follow INSTRUCTIONS.md for implementation
# Reference QUICK_REFERENCE.md while coding
```

---

**This template is ready to be placed in your GitHub repository root for proper project structure and collaboration.**
