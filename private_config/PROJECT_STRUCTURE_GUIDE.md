# NSan Project: File Storage & Additional Setup Guide

Complete guide on where to store files and what additional files to create.

---

## Part 1: File Storage Structure

### Recommended Repository Structure

```
nsan-project/                          ← Your GitHub repository root
│
├── 📚 DOCUMENTATION FILES (Root Level)
│   ├── START_HERE.txt                 ← Read this first!
│   ├── FINAL_SUMMARY.txt              ← Complete overview
│   ├── README.md                      ← Project context
│   ├── DOCUMENTATION_SUMMARY.md       ← Guide to all files
│   ├── INDEX.md                       ← Master navigation
│   ├── PROBLEM_STATEMENT.md           ← Why NSan needed
│   ├── INSTRUCTIONS.md                ← Main implementation guide
│   ├── SKILLS.md                      ← Code patterns
│   ├── QUICK_REFERENCE.md             ← Fast lookup
│   ├── REPO_SETUP_GUIDE.md            ← Git workflow
│   ├── CLAUDE_CODE_INTEGRATION.md     ← Claude Code setup
│   └── LANGUAGE_JUSTIFICATION.md      ← Why C++
│
├── 🔧 CONFIG FILES (Root Level)
│   ├── .claudeconfig                  ← Claude Code configuration
│   ├── .gitignore                     ← Git ignore patterns
│   ├── CMakeLists.txt                 ← Build configuration
│   ├── .clang-format                  ← Code style (LLVM)
│   └── .editorconfig                  ← Editor configuration
│
├── 🏗️ SOURCE CODE DIRECTORY
│   ├── src/
│   │   ├── nsan/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── NSanPass.cpp
│   │   │   ├── NSanPass.h
│   │   │   ├── ShadowValueMap.cpp
│   │   │   └── ShadowValueMap.h
│   │   │
│   │   └── runtime/
│   │       ├── CMakeLists.txt
│   │       ├── nsan_runtime.cpp
│   │       ├── nsan_runtime.h
│   │       ├── shadow_memory.cpp
│   │       └── shadow_memory.h
│   │
│   ├── include/
│   │   ├── nsan.h
│   │   ├── nsan_interface.h
│   │   └── internal/
│   │       ├── shadow_tracking.h
│   │       └── configuration.h
│   │
│   ├── tests/
│   │   ├── CMakeLists.txt
│   │   ├── unit/
│   │   │   ├── test_instrumentation.cpp
│   │   │   ├── test_shadow_values.cpp
│   │   │   └── test_memory.cpp
│   │   │
│   │   ├── functional/
│   │   │   ├── test_naive_summation.cpp
│   │   │   ├── test_kahan_summation.cpp
│   │   │   ├── test_matrix_ops.cpp
│   │   │   └── test_cancelation.cpp
│   │   │
│   │   └── benchmarks/
│   │       ├── benchmark_compensated_sum.cpp
│   │       └── benchmark_real_world.cpp
│   │
│   ├── examples/
│   │   ├── simple_computation.cpp
│   │   ├── matrix_determinant.cpp
│   │   └── iterative_solver.cpp
│   │
│   └── cmake/
│       ├── LLVMConfig.cmake
│       └── FindLLVM.cmake
│
├── 📖 DOCS DIRECTORY (Generated Documentation)
│   ├── architecture.md
│   ├── ir_transformations.md
│   ├── memory_model.md
│   ├── api_reference.md
│   └── images/
│       ├── architecture_diagram.png
│       └── data_flow.png
│
├── 🔄 CI/CD CONFIGURATION
│   └── .github/
│       ├── workflows/
│       │   ├── build.yml
│       │   ├── test.yml
│       │   └── lint.yml
│       │
│       └── ISSUE_TEMPLATE/
│           ├── bug_report.md
│           └── feature_request.md
│
├── 📋 PROJECT FILES
│   ├── CONTRIBUTING.md                ← Contributor guidelines
│   ├── CODE_OF_CONDUCT.md            ← Code of conduct
│   ├── LICENSE                        ← License file
│   ├── CHANGELOG.md                   ← Version history
│   └── ROADMAP.md                     ← Development roadmap
│
└── 📦 BUILD OUTPUT (Generated, not committed)
    ├── build/
    ├── dist/
    └── .cmake/
```

---

## Part 2: Where Exactly to Store the 12 Documentation Files

### 🎯 **Storage Location: Repository Root**

All 12 documentation files should go in the **root directory** of your GitHub repository:

```bash
nsan-project/                 ← Repository root
├── START_HERE.txt            ← HERE
├── FINAL_SUMMARY.txt         ← HERE
├── README.md                 ← HERE
├── DOCUMENTATION_SUMMARY.md  ← HERE
├── INDEX.md                  ← HERE
├── PROBLEM_STATEMENT.md      ← HERE
├── INSTRUCTIONS.md           ← HERE
├── SKILLS.md                 ← HERE
├── QUICK_REFERENCE.md        ← HERE
├── REPO_SETUP_GUIDE.md       ← HERE
├── CLAUDE_CODE_INTEGRATION.md ← HERE
├── LANGUAGE_JUSTIFICATION.md ← HERE
│
└── Other files below...
```

### Why Root Level?

✅ **Visibility**: GitHub displays README.md prominently
✅ **Quick Access**: Everyone sees documentation immediately
✅ **Discoverability**: GitHub renders .md files automatically
✅ **Convention**: Standard practice for open-source projects
✅ **Claude Code**: Easier for Claude to find documentation

### GitHub Display

When someone visits your repository, they see:
```
📚 README.md (automatically shown)
📚 START_HERE.txt (visible in file list)
📚 FINAL_SUMMARY.txt (visible in file list)
📚 All other .md files (visible and clickable)
```

---

## Part 3: Additional Files to Create (Beyond .claudeconfig)

Beyond `.claudeconfig`, you should also create these files:

### 1. `.gitignore` (Essential for Git)

```gitignore
# Build artifacts
build/
dist/
*.o
*.a
*.so
*.dylib
*.dll
*.exe

# CMake
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile

# LLVM/Clang generated
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

# Temporary
*.tmp
*.log
core
core.*

# Build directories
/build/
/install/
```

Save as: **`nsan-project/.gitignore`**

### 2. `CMakeLists.txt` (Root-level Build Configuration)

```cmake
# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(NSan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Add LLVM modules
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Include directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Compiler flags (LLVM style)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

# Add subdirectories
add_subdirectory(src/nsan)
add_subdirectory(src/runtime)
add_subdirectory(tests)

# Enable testing
enable_testing()

# Print configuration info
message(STATUS "LLVM libs: ${LLVM_LIBRARIES}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
```

Save as: **`nsan-project/CMakeLists.txt`**

### 3. `.clang-format` (Code Style - LLVM Standard)

```yaml
# LLVM style
BasedOnStyle: LLVM
IndentWidth: 2
UseTab: Never
ColumnLimit: 80
BreakBeforeBraces: Linux
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
```

Save as: **`nsan-project/.clang-format`**

### 4. `.editorconfig` (Cross-Editor Settings)

```ini
# EditorConfig helps maintain consistent coding styles for multiple developers
# working on the same project across various editors and IDEs.

root = true

# All files
[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

# C++ files
[*.{cpp,h,hpp,cc,cxx,hxx}]
indent_style = space
indent_size = 2

# Markdown files
[*.md]
indent_style = space
indent_size = 2
trim_trailing_whitespace = false

# CMake files
[CMakeLists.txt]
indent_style = space
indent_size = 2

# YAML/Config files
[*.{yml,yaml,json}]
indent_style = space
indent_size = 2
```

Save as: **`nsan-project/.editorconfig`**

### 5. `.github/workflows/build.yml` (CI/CD - Automated Testing)

```yaml
name: Build NSan

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y llvm-dev clang cmake build-essential
    
    - name: Create build directory
      run: mkdir -p build
    
    - name: Configure with CMake
      working-directory: build
      run: cmake ..
    
    - name: Build
      working-directory: build
      run: make -j$(nproc)
    
    - name: Run tests
      working-directory: build
      run: ctest --verbose
    
    - name: Check compilation without warnings
      working-directory: build
      run: make clean && cmake -DCMAKE_CXX_FLAGS="-Werror" .. && make
```

Save as: **`nsan-project/.github/workflows/build.yml`**

### 6. `CONTRIBUTING.md` (Contributor Guidelines)

```markdown
# Contributing to NSan

Thank you for contributing! Please follow these guidelines.

## Before You Start

1. Read README.md (project overview)
2. Read PROBLEM_STATEMENT.md (why this matters)
3. Read INSTRUCTIONS.md (how to implement)

## Development Setup

```bash
git clone https://github.com/YOUR_ORG/nsan-project.git
cd nsan-project
mkdir build && cd build
cmake ..
make
ctest --verbose
```

## Code Style

Follow LLVM coding conventions:
- 2-space indentation
- No tabs
- 80-character line limit
- See .clang-format for details

Format your code:
```bash
clang-format -i src/**/*.cpp
```

## Commit Messages

Use format: `[COMPONENT] Description`

Components: PASS, RUNTIME, TEST, DOC, BUILD, INFRA

Example:
```
[PASS] Instrument floating-point binary operations

- Detect fadd, fsub, fmul, fdiv instructions
- Create parallel shadow computations
- Maintain shadow value map

Related: PR #5, Issue #2
```

## Pull Request Process

1. Create feature branch: `git checkout -b feature/my-feature`
2. Make changes and commit
3. Push: `git push origin feature/my-feature`
4. Create Pull Request
5. Wait for code review
6. Address feedback
7. Merge when approved

## Testing

Write tests for your changes:
- Unit tests: `tests/unit/`
- Functional tests: `tests/functional/`
- Run: `cd build && ctest --verbose`

## Getting Help

- Check QUICK_REFERENCE.md for common questions
- See INSTRUCTIONS.md for detailed guidance
- Ask in Issues with detailed context

Thank you for contributing! 🙏
```

Save as: **`nsan-project/CONTRIBUTING.md`**

### 7. `LICENSE` (Choose one)

For open-source projects, use a standard license. For NSan (based on Google's work):

```text
Copyright 2026 NSan Contributors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

Save as: **`nsan-project/LICENSE`**

Or use:
- `MIT License` - Simple and permissive
- `Apache 2.0` - With patent protection
- `GPL v3` - Copyleft

### 8. `CHANGELOG.md` (Version History)

```markdown
# Changelog

All notable changes to this project are documented in this file.

## [Unreleased]

### Added
- Initial NSan implementation
- LLVM IR instrumentation pass
- Shadow memory management
- Consistency checking system
- Diagnostics and error reporting

### Changed
- TBD

### Fixed
- TBD

## [0.1.0] - 2026-04-06

### Added
- Project setup and documentation
- Claude Code integration
- Build system (CMake)
- Basic pass framework

[Unreleased]: https://github.com/YOUR_ORG/nsan-project/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/YOUR_ORG/nsan-project/releases/tag/v0.1.0
```

Save as: **`nsan-project/CHANGELOG.md`**

### 9. `ROADMAP.md` (Development Plan)

```markdown
# NSan Development Roadmap

## Phase 1: Foundation (Weeks 1-2) ✅ In Progress

- [x] Project setup
- [x] Documentation creation
- [ ] LLVM pass framework
- [ ] Floating-point detection

**Target**: Basic pass compiles and detects FP operations

## Phase 2: Core Instrumentation (Weeks 3-4)

- [ ] Binary operation instrumentation
- [ ] Function parameter tracking
- [ ] Return value handling
- [ ] Memory operations

**Target**: All FP operations instrumented

## Phase 3: Runtime System (Weeks 5-6)

- [ ] Shadow memory management
- [ ] Consistency checking
- [ ] Thread-local shadow stack
- [ ] Error reporting

**Target**: Runtime checks working correctly

## Phase 4: Advanced Features (Weeks 7-8)

- [ ] Diagnostics system
- [ ] Configuration system
- [ ] Performance profiling
- [ ] Edge case handling

**Target**: Production-ready diagnostics

## Phase 5: Testing & Validation (Weeks 9-10)

- [ ] Unit test suite
- [ ] Functional tests
- [ ] Performance benchmarks
- [ ] Documentation review

**Target**: All tests passing, performance acceptable

## Post-v1.0: Future Enhancements

- GPU support (CUDA)
- IDE integration
- Machine learning-based error filtering
- Distributed computing support
```

Save as: **`nsan-project/ROADMAP.md`**

### 10. `.github/ISSUE_TEMPLATE/bug_report.md`

```markdown
---
name: Bug Report
about: Report a bug in NSan
title: '[BUG] '
labels: bug
assignees: ''

---

## Describe the bug
A clear and concise description of what the bug is.

## To Reproduce
Steps to reproduce the behavior:
1. ...
2. ...
3. ...

## Expected behavior
What you expected to happen.

## Actual behavior
What actually happened.

## Environment
- OS: [e.g. Ubuntu 20.04]
- LLVM version: [e.g. LLVM 12]
- NSan phase: [e.g. Phase 2]

## Code example
```cpp
// Code that triggers the bug
```

## Error message
```
Error message and stack trace
```

## Additional context
Any other context about the problem.
```

Save as: **`nsan-project/.github/ISSUE_TEMPLATE/bug_report.md`**

### 11. `docs/architecture.md` (Technical Architecture)

```markdown
# NSan Architecture

## Overview

NSan is a compiler pass that instruments floating-point operations
with shadow computations in higher precision.

## Components

### 1. LLVM Pass (src/nsan/NSanPass.cpp)

- Entry point: `NSanPass::runOnFunction()`
- Detects floating-point operations
- Creates shadow IR
- Inserts consistency checks

### 2. Shadow Value Tracking (src/nsan/ShadowValueMap.cpp)

- Maps original values to shadow values
- Maintains correspondence throughout function
- Tracks temporary, parameter, and return values

### 3. Runtime Library (src/runtime/)

- `nsan_runtime.cpp`: Consistency checking
- `shadow_memory.cpp`: Memory management
- `diagnostics.cpp`: Error reporting

### 4. Public API (include/nsan.h)

- `__nsan_check_float()`
- `__nsan_dump_shadow_mem()`
- `__nsan_resume_float()`

## Data Flow

1. **Input**: C/C++ source with floating-point operations
2. **Compilation**: Clang frontend → LLVM IR
3. **Instrumentation**: NSan pass adds shadow operations
4. **Lowering**: LLVM backend → machine code
5. **Runtime**: Linking with nsan_runtime library
6. **Execution**: Program runs with shadow checking

## Shadow Value Propagation

```
float x = a + b;

Becomes:

float x = a + b;                    // Original
double x_shadow = (double)a +       // Shadow computation
                  (double)b;
__nsan_check_consistency_float(x, x_shadow);  // Check
```

See INSTRUCTIONS.md for detailed architectural sections.
```

Save as: **`nsan-project/docs/architecture.md`**

### 12. `docs/api_reference.md` (API Documentation)

```markdown
# NSan API Reference

## User-Callable Functions

### __nsan_check_float(float f)

Explicitly check a float value for consistency.

```cpp
float x = compute();
__nsan_check_float(x);  // Check if instrumented
```

### __nsan_dump_shadow_mem(void* addr, size_t size)

Dump shadow memory contents for debugging.

```cpp
float array[10];
__nsan_dump_shadow_mem(array, sizeof(array));
```

### __nsan_resume_float(float f)

Resume computation from original value (drop shadow).

```cpp
float x = untrustworthy_computation();
__nsan_resume_float(x);  // Start fresh
return x;
```

## Internal Functions

See include/nsan.h for complete function signatures.

## Environment Variables

- `NSAN_EPSILON=1e-5` - Absolute error threshold
- `NSAN_REL_EPSILON=1e-5` - Relative error threshold
- `NSAN_VERBOSITY=0|1|2` - 0=quiet, 1=warnings, 2=verbose

## Compilation

```bash
clang++ -fsanitize=numerical myprogram.cpp -o myprogram
```

## Running

```bash
./myprogram                              # Normal run
NSAN_EPSILON=1e-6 ./myprogram           # Custom threshold
NSAN_VERBOSITY=2 ./myprogram            # Verbose output
```
```

Save as: **`nsan-project/docs/api_reference.md`**

---

## Part 4: Complete File Checklist

### Files to Create (in order)

```
✅ Repository Root Documentation (12 files)
  [ ] START_HERE.txt
  [ ] FINAL_SUMMARY.txt
  [ ] README.md
  [ ] DOCUMENTATION_SUMMARY.md
  [ ] INDEX.md
  [ ] PROBLEM_STATEMENT.md
  [ ] INSTRUCTIONS.md
  [ ] SKILLS.md
  [ ] QUICK_REFERENCE.md
  [ ] REPO_SETUP_GUIDE.md
  [ ] CLAUDE_CODE_INTEGRATION.md
  [ ] LANGUAGE_JUSTIFICATION.md

✅ Configuration Files (Root Level)
  [ ] .claudeconfig
  [ ] .gitignore
  [ ] CMakeLists.txt
  [ ] .clang-format
  [ ] .editorconfig

✅ Project Files (Root Level)
  [ ] LICENSE
  [ ] CONTRIBUTING.md
  [ ] CHANGELOG.md
  [ ] ROADMAP.md

✅ GitHub Workflows
  [ ] .github/workflows/build.yml
  [ ] .github/ISSUE_TEMPLATE/bug_report.md

✅ Documentation
  [ ] docs/architecture.md
  [ ] docs/api_reference.md

✅ Source Code Directories (Create empty)
  [ ] src/nsan/
  [ ] src/runtime/
  [ ] include/
  [ ] tests/unit/
  [ ] tests/functional/
  [ ] tests/benchmarks/
  [ ] examples/
  [ ] cmake/
```

---

## Part 5: Setup Commands

### Clone and Setup Repository

```bash
# 1. Clone or create repository
git clone https://github.com/YOUR_USERNAME/nsan-project.git
cd nsan-project

# 2. Copy all 12 documentation files to root
cp /path/to/documentation/*.md .
cp /path/to/documentation/*.txt .

# 3. Create source directories
mkdir -p src/nsan src/runtime include tests/{unit,functional,benchmarks} examples cmake docs

# 4. Create initial configuration files

# Create .gitignore
cat > .gitignore << 'EOF'
build/
dist/
*.o
*.a
*.so
*.ll
*.bc
.vscode/
.idea/
*.swp
*~
.DS_Store
EOF

# Create CMakeLists.txt (use template from Part 3)

# Create .clang-format (use template from Part 3)

# Create .editorconfig (use template from Part 3)

# Create .claudeconfig (use template from CLAUDE_CODE_INTEGRATION.md)

# Create LICENSE
cat > LICENSE << 'EOF'
Copyright 2026 NSan Contributors
Licensed under Apache 2.0...
EOF

# 5. Create GitHub workflows directory
mkdir -p .github/{workflows,ISSUE_TEMPLATE}

# 6. Create other files
touch CONTRIBUTING.md CHANGELOG.md ROADMAP.md
mkdir -p docs

# 7. Initial commit
git add -A
git commit -m "Initial commit: NSan project structure and documentation"

# 8. Push to GitHub
git branch -M main
git push -u origin main
```

---

## Part 6: Files Summary Table

| File Type | File Name | Location | Purpose | Created By |
|-----------|-----------|----------|---------|-----------|
| **Documentation** | START_HERE.txt | Root | Quick orientation | Claude |
| **Documentation** | FINAL_SUMMARY.txt | Root | Complete overview | Claude |
| **Documentation** | README.md | Root | Project context | Claude |
| **Documentation** | DOCUMENTATION_SUMMARY.md | Root | Guide to docs | Claude |
| **Documentation** | INDEX.md | Root | Master navigation | Claude |
| **Documentation** | PROBLEM_STATEMENT.md | Root | Why NSan needed | Claude |
| **Documentation** | INSTRUCTIONS.md | Root | Implementation guide | Claude |
| **Documentation** | SKILLS.md | Root | Code patterns | Claude |
| **Documentation** | QUICK_REFERENCE.md | Root | Fast lookup | Claude |
| **Documentation** | REPO_SETUP_GUIDE.md | Root | Git workflow | Claude |
| **Documentation** | CLAUDE_CODE_INTEGRATION.md | Root | Claude Code setup | Claude |
| **Documentation** | LANGUAGE_JUSTIFICATION.md | Root | Why C++ | Claude |
| **Config** | .claudeconfig | Root | Claude Code config | You |
| **Config** | .gitignore | Root | Git ignore rules | You |
| **Config** | CMakeLists.txt | Root | Build system | You |
| **Config** | .clang-format | Root | Code style | You |
| **Config** | .editorconfig | Root | Editor settings | You |
| **Project** | LICENSE | Root | License | You |
| **Project** | CONTRIBUTING.md | Root | Contributor guide | You |
| **Project** | CHANGELOG.md | Root | Version history | You |
| **Project** | ROADMAP.md | Root | Development plan | You |
| **CI/CD** | .github/workflows/build.yml | .github/workflows/ | Build automation | You |
| **CI/CD** | .github/ISSUE_TEMPLATE/bug_report.md | .github/ISSUE_TEMPLATE/ | Issue template | You |
| **Docs** | docs/architecture.md | docs/ | Technical architecture | You |
| **Docs** | docs/api_reference.md | docs/ | API documentation | You |
| **Source** | src/nsan/ | src/ | LLVM pass (empty initially) | You |
| **Source** | src/runtime/ | src/ | Runtime lib (empty initially) | You |
| **Source** | include/ | include/ | Headers (empty initially) | You |
| **Tests** | tests/ | tests/ | Test suite (empty initially) | You |
| **Examples** | examples/ | examples/ | Example programs (empty initially) | You |

---

## Part 7: Quick Setup (Copy-Paste Ready)

```bash
#!/bin/bash
# setup_nsan_project.sh - Automated setup script

# Create repository
mkdir nsan-project && cd nsan-project
git init
git config user.name "Your Name"
git config user.email "your.email@example.com"

# Copy documentation files
# (Assuming they're in /tmp/nsan-docs/)
cp /tmp/nsan-docs/*.md .
cp /tmp/nsan-docs/*.txt .

# Create directories
mkdir -p src/{nsan,runtime} include tests/{unit,functional,benchmarks} examples cmake .github/{workflows,ISSUE_TEMPLATE} docs

# Create .gitignore
cat > .gitignore << 'EOF'
build/
dist/
*.o
*.a
*.so
*.dylib
*.ll
*.bc
*.s
.vscode/
.idea/
*.swp
*~
.DS_Store
EOF

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.12)
project(NSan)
set(CMAKE_CXX_STANDARD 17)
find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_subdirectory(src/nsan)
add_subdirectory(src/runtime)
add_subdirectory(tests)
enable_testing()
EOF

# Create .clang-format
cat > .clang-format << 'EOF'
BasedOnStyle: LLVM
IndentWidth: 2
UseTab: Never
ColumnLimit: 80
EOF

# Create .claudeconfig
# (See CLAUDE_CODE_INTEGRATION.md for full content)

# Create LICENSE
cat > LICENSE << 'EOF'
Copyright 2026 NSan Contributors
Licensed under Apache 2.0
EOF

# Create CONTRIBUTING.md
echo "# Contributing to NSan\n..." > CONTRIBUTING.md

# Create CHANGELOG.md
echo "# Changelog\n\n## [Unreleased]\n..." > CHANGELOG.md

# Create ROADMAP.md
echo "# Roadmap\n\n## Phase 1...\n..." > ROADMAP.md

# Create GitHub Actions workflow
mkdir -p .github/workflows
cat > .github/workflows/build.yml << 'EOF'
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: sudo apt-get install -y llvm-dev clang cmake
      - run: mkdir build && cd build && cmake .. && make
      - run: cd build && ctest --verbose
EOF

# Initial commit
git add -A
git commit -m "Initial commit: NSan project setup"

echo "✅ NSan project setup complete!"
echo "📍 Location: $(pwd)"
echo "📚 Documentation files: 12 in root"
echo "🔧 Configuration files: Ready"
echo "🚀 Ready for GitHub push"
```

---

## Summary

### Where to Store 12 Documentation Files
→ **Repository Root** (same level as CMakeLists.txt and .gitignore)

### Other Files to Create
1. **.claudeconfig** - Claude Code integration
2. **.gitignore** - Git configuration
3. **CMakeLists.txt** - Build system
4. **.clang-format** - Code style
5. **.editorconfig** - Editor settings
6. **LICENSE** - License file
7. **CONTRIBUTING.md** - Contributor guide
8. **CHANGELOG.md** - Version history
9. **ROADMAP.md** - Development plan
10. **.github/workflows/build.yml** - CI/CD
11. **docs/architecture.md** - Architecture docs
12. **docs/api_reference.md** - API docs

### Total Files to Create
- **12 Documentation files** (provided)
- **12 Additional files** (templates above)
- **Total: 24 files** for complete setup

All templates are provided above - just copy and modify as needed!

