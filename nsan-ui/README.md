# NSan UI — Live Demo Frontend

A browser UI for the NSan (Numerical Stability Sanitizer) LLVM pass. It has two modes:

- **Simulated** — pure JS math (`Math.fround`), works with zero setup, good for quick browsing.
- **Live** — `server.py` shells out to your *real* `nsan-clang++`, your compiled
  `libnsan_pass.so` (NSanPass.cpp), and `libnsan_runtime.a`, and streams the genuine
  compiler/runtime output back into the page. This is what you want for a demo/evaluation —
  it proves the plugin is actually running, not just illustrated.

## 1. Where this folder goes

Drop this whole `nsan-ui/` folder as a **sibling** of your existing project folders, i.e.
directly inside your repo root, next to `nsan-clang++`, `tests/`, `build/`, etc.:

```
CD_Lab_EL_Team_Assignment_21/          <- repo root
├── CMakeLists.txt
├── nsan-clang++
├── build.sh / run.sh / benchmark.sh
├── build/                             <- created by build.sh, contains the .so/.a
│   └── src/... /libnsan_pass.so
│   └── src/... /libnsan_runtime.a
├── src/
├── include/
├── tests/
│   ├── tc1_cancellation.cpp
│   ├── ... tc2..tc12
├── examples/, examples_demo/, screenshots/, assets/
└── nsan-ui/                            <- THIS folder, new
    ├── index.html
    ├── css/style.css
    ├── js/main.js
    ├── server.py
    └── README.md
```

`server.py` auto-detects the repo root as **one directory above itself**
(`nsan-ui/../`), so as long as `nsan-ui/` sits at the repo root, no configuration is needed.

## 2. Build the real project first (required for Live mode)

```bash
cd CD_Lab_EL_Team_Assignment_21
./build.sh
```

This produces `libnsan_pass.so` and `libnsan_runtime.a` somewhere under `build/`.
`server.py` searches for them with `glob` (`build/**/libnsan_pass.so`, etc.), so any
reasonable CMake output layout is found automatically.

## 3. Run the UI

```bash
cd nsan-ui
python3 server.py
```

To automatically restart the server when you edit files, run:

```bash
cd nsan-ui
python3 server.py --reload
```

Then open **http://localhost:8765** in your browser (or use VS Code's "Live Preview" /
"Live Server" extension pointed at `index.html` — but note only `server.py` gives you Live
mode, since it's the one running the real subprocess calls).

The status strip at the top of the page tells you exactly what was detected:

- 🟢 green dot — `nsan-clang++` and `libnsan_pass.so` were both found; Live mode is fully usable.
- 🟠 orange dot — either the server isn't running, or the project hasn't been built yet
  (run `./build.sh`), or the paths need to be overridden (see below).

## 4. If auto-detection picks the wrong paths

Override with environment variables before starting the server:

```bash
NSAN_REPO_ROOT=/path/to/repo \
NSAN_CLANGXX=/path/to/repo/nsan-clang++ \
NSAN_PASS_PLUGIN=/path/to/repo/build/src/nsan/libnsan_pass.so \
NSAN_RUNTIME_LIB=/path/to/repo/build/src/runtime/libnsan_runtime.a \
python3 server.py
```

## 5. What "Live" mode actually does (so you can explain it in your demo)

| UI element | What happens on the backend |
|---|---|
| **Instrumentation viewer → Live tab** | `server.py` runs `clang++ -S -emit-llvm -O1 <tc>.cpp -o -` for the "original" pane, then `clang++ -S -emit-llvm -O1 -fpass-plugin=libnsan_pass.so <tc>.cpp -o -` for the "instrumented" pane, and diffs the two — this is your actual `NSanPass.cpp` transforming real IR. |
| **Shadow Lab → "Run on real compiler"** | Writes a tiny 2-line C++ file with your chosen operands/operator, compiles it with the real `nsan-clang++ -fsanitize=numerical`, runs the binary with `NSAN_REL_EPSILON` set to your slider value, and shows the actual stdout/stderr. |
| **Test Suite card → "Run this file through nsan-clang++"** | Compiles and runs the actual `tests/tcN_*.cpp` file through your real toolchain and prints the real `[WARN]` (or silence). |

If `server.py` isn't running, or the plugin isn't built, every one of these falls back
gracefully to the **simulated** JS computation so the page never breaks — it just tells you
in the status bar / terminal panel that it's not connected.

## 6. Opening this in VS Code

1. Open the `nsan-ui/` folder in VS Code (`code nsan-ui`), or open the whole repo and just
   navigate into `nsan-ui/`.
2. Open a terminal in VS Code (`` Ctrl+` ``) and run `python3 server.py` from inside `nsan-ui/`.
3. `Cmd/Ctrl+Click` the printed `http://localhost:8765` link, or open it manually in a browser.
4. No extensions or build step required — it's stdlib Python + vanilla HTML/CSS/JS.

## 7. Troubleshooting

- **"nsan-clang++ not found"** — you haven't run `./build.sh`, or `NSAN_REPO_ROOT` is wrong.
- **"libnsan_pass.so not found"** — same fix; check `build/` actually contains it after
  `./build.sh` completes with no errors.
- **CORS / fetch errors in the browser console** — make sure you're loading the page from
  `http://localhost:8765` (served by `server.py`), not by double-clicking `index.html`
  directly — Live mode needs the same-origin API endpoints `server.py` exposes.
- **Port already in use** — `PORT=8080 python3 server.py`.
