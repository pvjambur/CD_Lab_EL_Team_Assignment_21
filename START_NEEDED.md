# NSan — Start Here

Everything you need to run, understand, and develop the project.

---

## 1. Run the Demo Right Now (No LLVM needed)

```bash
cd examples/standalone_demo
g++ -std=c++14 -O1 nsan_demo.cpp -o nsan_demo.exe
./nsan_demo.exe
```

**That's it.** You will see live output like this:

```
+======================================================+
|       NSan -- Numerical Stability Sanitizer          |
|            Standalone Runtime Demo                   |
+======================================================+

-- Test 1: Catastrophic Cancellation ---------------
  a = 1e7,  b = 1.234
  float32 result   : 1        <-- WRONG
  reference result : 1.234    <-- correct

  [WARN]  (a+b)-a
          float32  = +1
          shadow   = +1.234
          rel_err  = 1.90e-001  (threshold 1.00e-004)

-- Test 2: Naive vs Kahan Summation ----------------
  Naive rel. error : 1.38e-004  <-- SIGNIFICANT
  Kahan rel. error : 0.00e+000  OK

  [WARN]  naive sum (final) ...
  [PASS]  kahan sum

+======================================================+
|                     Summary                          |
+======================================================+
  Checks performed : 10004
  Warnings fired   : 2
```

**Colours** (if your terminal supports ANSI):
- RED `[WARN]` = NSan detected numerical instability
- GREEN `[PASS]` = result matched the shadow within threshold
- YELLOW = the actual error value

---

## 2. What You Are Seeing

NSan works by running every float operation TWICE:
1. Once in `float` (as written in your code)
2. Once in higher precision (`double` or `long double`) — the "shadow"

If the two answers diverge by more than a threshold, it's a warning.

```
Your code:      float sum += value;

What NSan does: float  sum        += value;             // original
                double sum_shadow += (double)value;     // shadow
                if (|sum - sum_shadow| / |sum_shadow| > epsilon)
                    print WARNING;
```

The demo shows this manually. The real LLVM project does it **invisibly at compile time** — you never write the shadow code yourself.

---

## 3. Control the Demo

```bash
# Change the warning threshold
set NSAN_REL_EPSILON=1e-6 && ./nsan_demo.exe   # stricter -- more warnings
set NSAN_REL_EPSILON=1e-2 && ./nsan_demo.exe   # looser   -- fewer warnings
```

On Linux/Mac use `export NSAN_REL_EPSILON=1e-6` instead of `set`.

---

## 4. Run the Simple Standalone Example Too

```bash
cd examples
g++ -std=c++14 -O1 simple_computation.cpp -o simple.exe
./simple.exe
```

This shows naive vs Kahan sum results side by side. It does NOT do NSan checks —
it just shows the raw numbers so you can see the difference manually.

---

## 5. What the Full LLVM Project Does (Future)

When `runOnFunction()` is implemented in `src/nsan/NSanPass.cpp`:

```bash
# Linux only — requires LLVM dev libraries
mkdir build && cd build
cmake ..
make

# Then compile ANY program through NSan:
clang++ -Xclang -load -Xclang ./src/nsan/libnsan_pass.so \
  my_program.cpp -L./src/runtime -lnsan_runtime \
  -o my_program_nsan

./my_program_nsan
# NSan warnings print to stderr automatically
```

You would never modify your source code — NSan instruments it at compile time.

---

## 6. Do You Need a Frontend / GUI?

**No.** NSan is a command-line tool. Output goes to:
- **stdout/stderr** — warnings printed as they happen during the run
- Optional: `NSAN_LOG_FILE=/tmp/nsan.log ./my_program` to redirect to a file

If you want a visual dashboard, you could pipe warnings into a log file and
view with a text editor, or grep for `[WARN]` lines:

```bash
./nsan_demo.exe 2>&1 | grep WARN
./nsan_demo.exe 2>&1 | grep -c WARN    # count warnings
```

---

## 7. Project Status at a Glance

| Component | Status |
|-----------|--------|
| `examples/standalone_demo/` | **WORKS — run this first** |
| `src/nsan/NSanPass.cpp` | Stub — `runOnFunction()` not written |
| `src/runtime/nsan_runtime.cpp` | Partial — float check works |
| `tests/` | Empty — no tests written yet |
| Full LLVM build | Requires Linux + `llvm-dev` package |

Full audit: `docs/PROJECT_STATUS.md`

---

## 8. File Map

```
CD_Lab_EL_Team_Assignment_21/
|
+-- examples/
|   +-- standalone_demo/        <-- START HERE (works now)
|   |   +-- nsan_demo.cpp
|   |   +-- Makefile
|   |   +-- README.md
|   +-- simple_computation.cpp  <-- secondary demo
|
+-- src/
|   +-- nsan/                   <-- LLVM pass (Phase 1-2 work)
|   +-- runtime/                <-- runtime library (Phase 3 work)
|
+-- include/                    <-- public API headers
+-- tests/                      <-- test suite (Phase 5 work)
+-- docs/
|   +-- PROJECT_STATUS.md       <-- full bug list and implementation plan
|
+-- private_config/             <-- reference docs for implementation
|   +-- INSTRUCTIONS.md         <-- step-by-step implementation guide
|   +-- SKILLS.md               <-- copy-paste LLVM code patterns
|   +-- QUICK_REFERENCE.md      <-- cheatsheet
|
+-- CMakeLists.txt              <-- build system (Linux only)
+-- START_NEEDED.md             <-- this file
```
