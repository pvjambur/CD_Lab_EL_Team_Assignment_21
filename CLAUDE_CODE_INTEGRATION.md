# Claude Code Integration Guide for NSan Project

Complete guide on how to connect your NSan project with Claude Code for AI-assisted development.

---

## Part 1: How to Link Project to Claude Code

### Method 1: Using .claudeconfig (Recommended)

Claude Code uses a configuration file to understand your project structure and requirements.

#### Step 1: Create .claudeconfig in Repository Root

```json
{
  "projectName": "NSan: Numerical Stability Sanitizer",
  "projectDescription": "Floating-point numerical sanitizer for LLVM",
  
  "documentationFiles": [
    "README.md",
    "PROBLEM_STATEMENT.md",
    "INSTRUCTIONS.md",
    "SKILLS.md",
    "QUICK_REFERENCE.md",
    "REPO_SETUP_GUIDE.md",
    "INDEX.md",
    "DOCUMENTATION_SUMMARY.md"
  ],
  
  "sourceDirectories": [
    "src/nsan",
    "src/runtime",
    "include",
    "tests"
  ],
  
  "language": "C++",
  "languageVersion": "C++17",
  
  "buildSystem": "CMake",
  "buildCommand": "mkdir -p build && cd build && cmake .. && make",
  "cleanCommand": "rm -rf build",
  "testCommand": "cd build && ctest --verbose",
  
  "llvmVersion": "10+",
  "llvmDependencies": [
    "llvm-dev",
    "clang-dev",
    "cmake"
  ],
  
  "keyFiles": {
    "mainPass": "src/nsan/NSanPass.cpp",
    "passHeader": "src/nsan/NSanPass.h",
    "runtime": "src/runtime/nsan_runtime.cpp",
    "shadowMemory": "src/runtime/shadow_memory.cpp",
    "publicAPI": "include/nsan.h"
  },
  
  "implementationPhases": {
    "phase1": {
      "name": "Foundation",
      "duration": "weeks 1-2",
      "files": ["src/nsan/NSanPass.cpp", "CMakeLists.txt"],
      "deliverables": ["Pass compiles", "Floating-point detection working"]
    },
    "phase2": {
      "name": "Core Instrumentation",
      "duration": "weeks 3-4",
      "files": ["src/nsan/NSanPass.cpp", "src/nsan/ShadowValueMap.cpp"],
      "deliverables": ["Binary ops instrumented", "Function params tracked"]
    },
    "phase3": {
      "name": "Runtime System",
      "duration": "weeks 5-6",
      "files": ["src/runtime/nsan_runtime.cpp", "src/runtime/shadow_memory.cpp"],
      "deliverables": ["Consistency checking works", "Shadow memory functional"]
    },
    "phase4": {
      "name": "Advanced Features",
      "duration": "weeks 7-8",
      "files": ["src/runtime/diagnostics.cpp", "src/runtime/configuration.cpp"],
      "deliverables": ["Diagnostics complete", "Configuration system working"]
    },
    "phase5": {
      "name": "Testing & Validation",
      "duration": "weeks 9-10",
      "files": ["tests/", "examples/"],
      "deliverables": ["All tests passing", "Benchmarks acceptable"]
    }
  },
  
  "claudeContext": {
    "skillsFile": "SKILLS.md",
    "instructionsFile": "INSTRUCTIONS.md",
    "quickReferenceFile": "QUICK_REFERENCE.md",
    "errorMessagesFile": "QUICK_REFERENCE.md#error-messages-guide",
    "codePatterns": "SKILLS.md#development-patterns-to-follow"
  },
  
  "environmentSetup": {
    "requirements": [
      "LLVM 10+ development libraries",
      "Clang compiler",
      "CMake 3.12+"
    ],
    "setupInstructions": "README.md#getting-started",
    "verificationCommand": "llvm-config --version && clang++ --version && cmake --version"
  },
  
  "gitWorkflow": {
    "defaultBranch": "main",
    "developmentBranch": "develop",
    "featureBranchPrefix": "feature/",
    "bugfixBranchPrefix": "bugfix/",
    "commitMessageFormat": "[COMPONENT] Description",
    "components": ["PASS", "RUNTIME", "TEST", "DOC", "BUILD", "INFRA"]
  },
  
  "preferredCommands": {
    "build": "mkdir -p build && cd build && cmake .. && make",
    "test": "cd build && ctest --verbose",
    "clean": "rm -rf build",
    "format": "clang-format -i src/**/*.cpp",
    "lint": "cmake --build build -- -Werror"
  },
  
  "context": {
    "problemDomain": "floating-point numerical stability",
    "relatedTools": ["Verificarlo", "FpDebug", "VERROU"],
    "advantages": "1-4 orders of magnitude faster than existing tools",
    "challenges": [
      "LLVM IR complexity",
      "Thread-safety for shadow stack",
      "Type punning in memory",
      "Untyped memory operations"
    ]
  }
}
```

#### Step 2: Commit to Repository

```bash
cd your-nsan-project
git add .claudeconfig
git commit -m "[BUILD] Add Claude Code configuration"
git push origin main
```

#### Step 3: Claude Code Will Auto-Discover

When Claude Code accesses your repository:
1. It reads .claudeconfig automatically
2. Loads all documentation files listed
3. Understands your build system
4. Knows your source structure
5. References correct files for help

---

### Method 2: Using .claude/instructions.md (Alternative)

Create a `.claude/` directory with detailed instructions:

```
.claude/
├── instructions.md       (Main instructions)
├── context.md           (Project context)
├── patterns.md          (Code patterns)
└── commands.md          (Build/test commands)
```

```markdown
# .claude/instructions.md

## NSan Project Context

This is a numerical stability sanitizer built as an LLVM pass.

## Key Files
- **Main Pass**: `src/nsan/NSanPass.cpp`
- **Runtime**: `src/runtime/nsan_runtime.cpp`
- **Tests**: `tests/`
- **Documentation**: See root directory

## Build Commands
\`\`\`bash
mkdir -p build && cd build && cmake .. && make
\`\`\`

## Test Commands
\`\`\`bash
cd build && ctest --verbose
\`\`\`

## When implementing:
1. Follow INSTRUCTIONS.md for current phase
2. Reference SKILLS.md for code patterns
3. Check QUICK_REFERENCE.md for commands
4. Use error messages table when debugging
```

---

### Method 3: Manual Configuration

If Claude Code doesn't auto-discover, use this prompt:

```
I'm working on a project called NSan (Numerical Stability Sanitizer).

PROJECT STRUCTURE:
- src/nsan/        - LLVM pass implementation
- src/runtime/     - Runtime library
- include/         - Public headers
- tests/           - Test suite
- examples/        - Example programs

BUILD: mkdir -p build && cd build && cmake .. && make
TEST:  cd build && ctest --verbose
LANG:  C++17 with LLVM

KEY DOCS (in repository):
- README.md              - Overview
- PROBLEM_STATEMENT.md   - Problem explanation
- INSTRUCTIONS.md        - Main technical guide
- SKILLS.md              - Code patterns & best practices
- QUICK_REFERENCE.md     - Quick command reference

Current task: [DESCRIBE YOUR CURRENT TASK]

Please:
1. Reference INSTRUCTIONS.md for implementation details
2. Use patterns from SKILLS.md when coding
3. Check QUICK_REFERENCE.md for commands
4. Follow the [CURRENT PHASE] described in INSTRUCTIONS.md
```

---

## Part 2: Prompts for Claude Code Orchestration

### Prompt Template 1: Project Setup & Understanding

```
I'm starting a new NSan (Numerical Stability Sanitizer) compiler project in C++.

TASK: Set up the project structure and help me understand the architecture

CONTEXT:
- Project: LLVM-based floating-point numerical sanitizer
- Language: C++17 with LLVM 10+
- Build: CMake
- Documentation: [Upload or reference all documentation files]

REQUIREMENTS:
1. Create initial project structure (directories, CMakeLists.txt)
2. Explain the architecture from INSTRUCTIONS.md
3. Guide me through Phase 1 (Foundation)
4. Set up initial LLVM pass framework

REFERENCE:
- See INSTRUCTIONS.md (Project Setup, Architecture Overview, Phase 1)
- See README.md (Directory Structure)
- See REPO_SETUP_GUIDE.md (Git workflow)

Provide:
- Step-by-step setup instructions
- Code for initial CMakeLists.txt
- Skeleton for NSanPass.cpp
- Explanation of how LLVM passes work
```

### Prompt Template 2: Current Implementation Phase

```
I'm implementing Phase [N] of the NSan project.

CURRENT PHASE: [PHASE_NAME]
(e.g., Phase 2: Core Instrumentation - weeks 3-4)

TASK: Guide me through implementing [SPECIFIC_FEATURE]
(e.g., Binary floating-point operations instrumentation)

REQUIREMENTS:
- Implement [FEATURE] as described in INSTRUCTIONS.md Phase [N]
- Follow code patterns from SKILLS.md
- Include tests in tests/[test_type]/
- Maintain performance (target: 2-20x overhead)

REFERENCE FILES:
- INSTRUCTIONS.md Phase [N]: [Specific section]
- SKILLS.md Section: [Skill area]
- QUICK_REFERENCE.md: [Relevant section]

Please provide:
1. Architecture explanation for this feature
2. Complete code implementation
3. Code comments explaining the approach
4. Test cases to verify correctness
5. Integration steps with existing code
```

### Prompt Template 3: Building with Claude Code

```
ORCHESTRATE my NSan project implementation end-to-end.

PROJECT SCOPE: Complete NSan implementation (10 weeks, 5 phases)

PHASES:
1. Foundation (weeks 1-2) - Setup, basic pass framework
2. Core Instrumentation (weeks 3-4) - Instrument FP operations
3. Runtime System (weeks 5-6) - Shadow memory, checking
4. Advanced Features (weeks 7-8) - Diagnostics, config
5. Testing & Validation (weeks 9-10) - Tests, benchmarks

APPROACH:
For each phase:
1. Explain what needs to be built
2. Show relevant code patterns (from SKILLS.md)
3. Provide complete implementation
4. Add tests
5. Verify against deliverables

REFERENCE:
- Main: INSTRUCTIONS.md (5 phases with step-by-step)
- Patterns: SKILLS.md (code patterns, best practices)
- Quick lookup: QUICK_REFERENCE.md
- Problem context: PROBLEM_STATEMENT.md

Starting with: Phase 1 - Foundation

Please:
1. Explain Phase 1 objectives
2. List deliverables to achieve
3. Generate complete code for each component
4. Show build/test commands
5. Verify all works end-to-end
6. Then guide Phase 2, etc.
```

### Prompt Template 4: Debugging Issues

```
I'm stuck on a problem in NSan implementation.

ISSUE: [Describe the problem]
ERROR: [Show error message]
CONTEXT: [What were you trying to do?]

PHASE: [Current phase]
FILE: [File where problem occurs]

REFERENCE:
- Error message guide: QUICK_REFERENCE.md (Error Messages section)
- Troubleshooting: INSTRUCTIONS.md (Troubleshooting section)
- Related patterns: SKILLS.md (Common Pitfalls section)

Please:
1. Identify the root cause
2. Reference relevant documentation
3. Provide solution with code
4. Explain why this approach works
5. Suggest how to test the fix
```

### Prompt Template 5: Code Review & Quality

```
TASK: Review my NSan code implementation for Phase [N]

REQUIREMENTS:
- Check against SKILLS.md code style guidelines
- Verify follows patterns from QUICK_REFERENCE.md
- Ensure no common pitfalls (see SKILLS.md)
- Check thread-safety (critical for shadow stack)
- Verify performance impact

CODE TO REVIEW:
[Paste your code]

REFERENCE:
- Code style: SKILLS.md (Code Style Guidelines)
- Patterns: SKILLS.md (Development Patterns)
- Pitfalls: SKILLS.md (Common Pitfalls to Avoid)

Provide:
1. Style issues and fixes
2. Any bugs or logical errors
3. Performance suggestions
4. Thread-safety assessment
5. Missing error handling
6. Test coverage recommendations
```

### Prompt Template 6: Testing Strategy

```
TASK: Design and implement comprehensive tests for NSan

CURRENT STATUS: [Phase completed]
COMPONENTS TO TEST: [List components]

REQUIREMENTS:
- Unit tests for individual components
- Functional tests with real numerical code
- Performance benchmarks
- Edge case coverage

REFERENCE:
- Testing strategy: INSTRUCTIONS.md (Testing Strategy section)
- Example tests: INSTRUCTIONS.md (Phase 5 examples)
- Test templates: QUICK_REFERENCE.md (Testing Patterns)

Please:
1. Design test suite structure
2. Create unit test templates
3. Create functional test examples
4. Create benchmark template
5. Show how to run tests
6. Explain what good test coverage looks like
```

### Prompt Template 7: Integration Checkpoint

```
CHECKPOINT: End of Phase [N]

PHASE COMPLETED: [Phase description]

DELIVERABLES CHECKLIST:
[List items from INSTRUCTIONS.md]

REQUIREMENTS:
- Verify all deliverables met
- Check code quality
- Confirm tests passing
- Validate performance
- Update documentation

REFERENCE:
- Phase deliverables: INSTRUCTIONS.md Phase [N]
- Quality standards: SKILLS.md
- Testing: INSTRUCTIONS.md (Testing Strategy)

Please:
1. Review what was accomplished
2. Verify against checklist
3. Identify any gaps
4. Suggest improvements
5. Plan for next phase
```

---

## Part 3: Language Choice Analysis

### Language Evaluation for NSan

| Aspect | C++ | Rust | Go | Python |
|--------|-----|------|----|---------| 
| **LLVM Integration** | ⭐⭐⭐⭐⭐ Excellent | ⭐⭐⭐ Good | ⭐⭐ Fair | ⭐ Poor |
| **Performance** | ⭐⭐⭐⭐⭐ Native | ⭐⭐⭐⭐⭐ Native | ⭐⭐⭐ Good | ⭐⭐ Slow |
| **Low-Level Control** | ⭐⭐⭐⭐⭐ Full | ⭐⭐⭐⭐ Restricted | ⭐⭐⭐ Some | ⭐ Limited |
| **Memory Safety** | ⭐⭐ Manual | ⭐⭐⭐⭐⭐ Automatic | ⭐⭐⭐ Good | ⭐⭐⭐ Good |
| **IR Manipulation** | ⭐⭐⭐⭐⭐ Native | ⭐⭐⭐⭐ Via FFI | ⭐⭐⭐ Via FFI | ⭐⭐ Via bindings |
| **Existing Code** | ⭐⭐⭐⭐⭐ All LLVM | ⭐⭐ New code | ⭐⭐ Wrapper | ⭐⭐ Wrapper |
| **Community** | ⭐⭐⭐⭐⭐ Large | ⭐⭐⭐⭐ Growing | ⭐⭐⭐⭐ Growing | ⭐⭐⭐⭐ Huge |
| **Learning Curve** | ⭐⭐⭐ Moderate | ⭐⭐ Steep | ⭐⭐⭐⭐ Easy | ⭐⭐⭐⭐⭐ Easy |

---

## **✅ RECOMMENDATION: C++17**

### Why C++ is the Clear Choice for NSan

#### 1. **LLVM Integration** (Critical Factor)
```cpp
// C++ works DIRECTLY with LLVM IR - no translation needed
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

class NSanPass : public FunctionPass {
    bool runOnFunction(Function& F) override {
        // Direct access to LLVM data structures
        for (auto& BB : F) {
            for (auto& I : BB) {
                // Work with LLVM IR directly
            }
        }
    }
};
```

#### 2. **Native Performance**
- Shadow operations must be fast (2-20x overhead target)
- C++ compiles to native machine code
- No garbage collection pauses
- Direct memory control

#### 3. **Low-Level Memory Operations**
NSan requires:
- Custom memory allocators for shadow memory
- Thread-local storage
- Pointer arithmetic for address mapping
- Type punning detection in memory

**Only C/C++ makes this practical**

#### 4. **Existing LLVM Ecosystem**
- All LLVM code is in C++
- All existing sanitizers (ASan, TSan, UBSan) are C++
- LLVM bindings are available, but suboptimal
- No translation overhead

#### 5. **Production Ready**
- Google's NSan implementation is C++
- LLVM team uses C++
- Compiler-rt uses C++
- Proven in production systems

---

## **❌ Why Other Languages Don't Work Well**

### Rust
```rust
// Would need FFI for LLVM integration
extern "C" {
    fn create_instruction(...) -> *mut LLVMValue;
}
// This defeats purpose - all performance gain lost to FFI calls
```
**Downsides**:
- FFI overhead on every IR operation (thousands per function)
- Memory layout incompatibilities
- Borrow checker complexity with LLVM pointers
- Less mature LLVM bindings
- Custom runtime needed

### Go
```go
// Go isn't suitable for systems programming at this level
// Would require CGO for all LLVM operations
package main
import "C"

// Every LLVM call crosses language boundary
```
**Downsides**:
- CGO overhead (slow)
- Garbage collection interferes with real-time checks
- Designed for distributed systems, not compilers
- Manual memory management still needed
- No existing Go LLVM passes

### Python
```python
# Would be pure wrapper over C++ LLVM bindings
# Adds layers of indirection
from llvmlite import ir
# Still calls C++ underneath, but slower
```
**Downsides**:
- Interpreted language (too slow)
- GIL (Global Interpreter Lock) issues
- Wrong level of abstraction
- Not suitable for compiler passes
- Would need to rewrite in C++ for production

---

## **C++ Justification Summary**

### Technical Reasons
1. **Direct LLVM Integration**: Work with IR directly, no translation
2. **Performance**: Native compilation, shadow ops need speed
3. **Memory Control**: Required for shadow memory mapping
4. **Low-Level Access**: Type punning, thread-local storage
5. **Existing Ecosystem**: All LLVM code is C++

### Practical Reasons
1. **Reference Implementation**: Google's NSan is C++
2. **Community**: LLVM developers use C++
3. **Learning Resources**: Tons of LLVM C++ examples
4. **Production Proven**: Used in real-world systems
5. **No Technical Debt**: Don't need to rewrite later

### Code Quality Reasons
1. **Type Safety**: Template metaprogramming for instrumentation
2. **Performance**: Zero-cost abstractions
3. **Maintainability**: C++17 features make code cleaner
4. **Testing**: Easy to write unit tests
5. **Integration**: Links directly with LLVM libraries

---

## Part 4: Best Practices for Claude Code + C++

### Configure Claude Code for C++

In your prompts, specify:

```
LANGUAGE: C++17
BUILD_SYSTEM: CMake
COMPILER: Clang (with LLVM toolchain)

CODING_STANDARDS:
- Follow LLVM coding conventions
- Use C++17 features
- RAII for resource management
- No exceptions in hot paths
- Thread-safe shadow operations

LLVM_SPECIFICS:
- Use llvm::isa<>(), llvm::dyn_cast<>() for IR operations
- Follow pass architecture patterns
- Implement runOnFunction() for FunctionPass
- Use IRBuilder<> for instruction creation
- Maintain compatibility with LLVM 10+
```

### Example: Asking Claude Code to Generate C++

```
TASK: Implement binary floating-point operation instrumentation

LANGUAGE: C++17 with LLVM

REQUIREMENTS:
1. Detect fadd, fsub, fmul, fdiv instructions
2. Create shadow operations (float->double)
3. Maintain shadow value mapping
4. Insert consistency checks

REFERENCE:
- Pattern: SKILLS.md "Instrument Binary Operations"
- Code style: SKILLS.md "Code Style Guidelines"
- Example: INSTRUCTIONS.md Phase 2

LLVM_KNOWLEDGE:
- Use dyn_cast<BinaryOperator>() to detect ops
- Use IRBuilder to create shadow ops
- Store mappings in shadow_map_

Generate:
1. Function signature with comments
2. Implementation with explanations
3. Example usage
4. Potential bugs to watch for
```

Claude will generate production-quality C++.

---

## Part 5: Quick Reference for Integration

### File to Use for Each Task

| Task | Primary File | Secondary |
|------|--------------|-----------|
| Setup Claude Code | This file (Part 1) | .claudeconfig template |
| Architecture questions | INSTRUCTIONS.md | README.md |
| Implementation patterns | SKILLS.md | QUICK_REFERENCE.md |
| Quick commands | QUICK_REFERENCE.md | INSTRUCTIONS.md |
| Debugging | QUICK_REFERENCE.md error table | INSTRUCTIONS.md troubleshooting |
| Language justification | **This section** | Language evaluation table |
| Code review | SKILLS.md code style | SKILLS.md pitfalls |

---

## Conclusion

### Claude Code Integration
1. **Use .claudeconfig** - Auto-discovery is most reliable
2. **Use prompt templates** - Provides consistent context
3. **Reference documentation** - Claude will find answers faster
4. **Provide phase details** - Helps Claude give better guidance

### Language Choice
**C++ is the only reasonable choice for NSan because:**
- Direct LLVM integration (critical)
- Performance requirements (2-20x overhead target)
- Memory access patterns (shadow memory mapping)
- Production ecosystem (Google's NSan, all LLVM code)
- No viable alternatives (Rust/Go/Python all inferior for this task)

You're building a compiler pass, not a general application. C++ isn't optional—it's the right tool for the job.

---

**Ready to integrate with Claude Code? Start with creating .claudeconfig in your repository root!** ✨
