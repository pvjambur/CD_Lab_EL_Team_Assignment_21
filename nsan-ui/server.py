#!/usr/bin/env python3
"""
NSan UI backend — no third-party dependencies (stdlib only).

Sits in <repo_root>/nsan-ui/ alongside your real nsan-clang++, tests/, and build/.
Serves the static frontend AND real endpoints that shell out to your actual
NSanPass plugin + libnsan_runtime.a, so the demo shows genuine compiler output
instead of a JS simulation whenever this server is running.

Run:
    cd nsan-ui
    python3 server.py
Then open http://localhost:8765
"""
import argparse
import glob
import http.server
import json
import os
import re
import socketserver
import subprocess
import sys
import tempfile
import time
import urllib.parse
import urllib.request
import urllib.error
import io
import zipfile

# ---------------------------------------------------------------------------
# Configuration — auto-detected, override with env vars if detection is wrong.
# ---------------------------------------------------------------------------
HERE = os.path.dirname(os.path.abspath(__file__))


def _load_dotenv(path):
    """Load key=value pairs from a .env file into os.environ (stdlib only)."""
    if not os.path.isfile(path):
        return
    with open(path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, val = line.partition("=")
            key = key.strip()
            val = val.strip().strip('"').strip("'")
            if key and key not in os.environ:   # real env vars always win
                os.environ[key] = val


_load_dotenv(os.path.join(HERE, ".env"))


REPO_ROOT = os.environ.get("NSAN_REPO_ROOT", os.path.abspath(os.path.join(HERE, "..")))
TESTS_DIR = os.path.join(REPO_ROOT, "tests")
NSAN_CLANGXX = os.environ.get("NSAN_CLANGXX", os.path.join(REPO_ROOT, "nsan-clang++"))
def _detect_clangxx():
    if os.environ.get("NSAN_UI_CLANGXX"):
        return os.environ.get("NSAN_UI_CLANGXX")
    if os.environ.get("CXX"):
        return os.environ.get("CXX")
    local_llvm = os.path.abspath(os.path.join(REPO_ROOT, "../llvm-workspace/build/bin/clang++"))
    if os.path.isfile(local_llvm) and os.access(local_llvm, os.X_OK):
        return local_llvm
    return "clang++"

CLANGXX = _detect_clangxx()
PORT = int(os.environ.get("PORT", 8765))

SYSROOT_ARGS = []
if sys.platform == "darwin":
    try:
        sdk = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
        if sdk and os.path.exists(sdk):
            SYSROOT_ARGS = ["-isysroot", sdk, "-nostdinc++", "-isystem", f"{sdk}/usr/include/c++/v1", "-isystem", f"{sdk}/usr/include"]
    except Exception:
        pass


# The optimization level used ONLY for the side-by-side IR viewer. LLVM's New
# Pass Manager EP callback (registerOptimizerLastEPCallback) generally needs
# the optimizer pipeline to actually run, which Clang skips at -O0. If your
# pass is registered differently, change this.
IR_OPT_LEVEL = os.environ.get("NSAN_IR_OPT_LEVEL", "-O1")


def find_first(patterns):
    for p in patterns:
        matches = glob.glob(os.path.join(REPO_ROOT, p), recursive=True)
        if matches:
            return matches[0]
    return None


PASS_PLUGIN = os.environ.get("NSAN_PASS_PLUGIN") or find_first(
    ["build/**/libnsan_pass.so", "build/**/libnsan_pass.dylib", "**/libnsan_pass.so"]
)
RUNTIME_LIB = os.environ.get("NSAN_RUNTIME_LIB") or find_first(
    ["build/**/libnsan_runtime.a", "**/libnsan_runtime.a"]
)


# ---------------------------------------------------------------------------
# GitHub CI integration
# ---------------------------------------------------------------------------
GH_TOKEN = os.environ.get("GITHUB_TOKEN", "")


def detect_gh_repo():
    env_repo = os.environ.get("NSAN_GH_REPO", "")
    if env_repo:
        return env_repo
    try:
        result = subprocess.run(
            ["git", "remote", "get-url", "origin"],
            cwd=REPO_ROOT, capture_output=True, text=True, timeout=5,
        )
        url = result.stdout.strip()
        if "github.com" in url:
            m = re.search(r"github\.com[:/]([^/]+/[^/.]+?)(?:\.git)?$", url)
            if m:
                return m.group(1)
    except Exception:
        pass
    return ""


GH_REPO = detect_gh_repo()


def gh_request(method, path, body=None, token=None, raw=False):
    """Proxy a GitHub REST API call. Returns dict with ok/data/error keys."""
    tok = token or GH_TOKEN
    url = f"https://api.github.com{path}"
    headers = {
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "NSan-UI/1.0",
    }
    if tok:
        headers["Authorization"] = f"Bearer {tok}"
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            raw_data = resp.read()
            if raw:
                return {"ok": True, "status": resp.status, "raw": raw_data}
            return {"ok": True, "status": resp.status,
                    "data": json.loads(raw_data) if raw_data else {}}
    except urllib.error.HTTPError as e:
        err_body = ""
        try:
            err_body = e.read().decode("utf-8", errors="replace")
        except Exception:
            pass
        return {"ok": False, "status": e.code, "error": err_body or str(e)}
    except urllib.error.URLError as e:
        return {"ok": False, "error": f"Network error: {e.reason}"}
    except Exception as e:
        return {"ok": False, "error": str(e)}


def parse_tc_results_from_log(log_text):
    """
    Parse GitHub Actions CI log to extract per-TC WARN/SILENT outcomes.
    Handles GitHub's timestamp prefix and multiple run.sh output formats.
    """
    ts_prefix = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d+Z\s*")
    lines = [ts_prefix.sub("", line) for line in log_text.splitlines()]
    cleaned = "\n".join(lines)

    results = {}

    # Strategy 1 – explicit section markers: "=== TC1 ===", "Running TC1", "## TC1"
    section_re = re.compile(
        r"(?:={2,}\s*TC(\d+)|Running\s+TC(\d+)|##\s*TC(\d+)|TC(\d+)\s*:)",
        re.IGNORECASE,
    )
    sections = list(section_re.finditer(cleaned))
    if sections:
        for idx, match in enumerate(sections):
            tc_num = next(g for g in match.groups() if g is not None)
            start = match.end()
            end = sections[idx + 1].start() if idx + 1 < len(sections) else len(cleaned)
            chunk = cleaned[start:end]
            results[f"TC{tc_num}"] = "warn" if "[WARN]" in chunk else "silent"
        return results

    # Strategy 2 – TC-prefixed lines: "TC1 [Name - WARN]" or "TC3 [Name - SILENT]"
    for i in range(1, 13):
        m = re.search(rf"\bTC{i}\b[^\n]*(WARN|SILENT)", cleaned, re.IGNORECASE)
        if m:
            results[f"TC{i}"] = "warn" if "WARN" in m.group(1).upper() else "silent"

    # Strategy 3 – compile/binary names like "tc1" followed by [WARN]
    if not results:
        for i in range(1, 13):
            chunk = re.search(rf"tc{i}[^\n]*\n(?:[^\n]*\n){{0,5}}", cleaned, re.IGNORECASE)
            if chunk:
                results[f"TC{i}"] = "warn" if "[WARN]" in chunk.group(0) else "silent"

    return results


# ---------------------------------------------------------------------------
# Helpers that actually invoke the real toolchain
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(description="Serve the NSan UI")
    parser.add_argument("--reload", action="store_true",
                        help="Restart automatically when watched files change")
    parser.add_argument("--host", default="", help="Host interface to bind to")
    parser.add_argument("--port", type=int, default=PORT,
                        help="Port to bind to")
    parser.add_argument("--watch-dir", action="append", default=[],
                        help="Additional directory to watch for changes")
    return parser.parse_args()


def collect_watch_files(paths):
    snapshots = {}
    for path in paths:
        if not path or not os.path.exists(path):
            continue
        if os.path.isfile(path):
            try:
                stat_result = os.stat(path)
            except FileNotFoundError:
                continue
            snapshots[path] = (stat_result.st_mtime_ns, stat_result.st_size)
            continue
        for root, _, filenames in os.walk(path):
            for filename in filenames:
                full_path = os.path.join(root, filename)
                try:
                    stat_result = os.stat(full_path)
                except FileNotFoundError:
                    continue
                snapshots[full_path] = (stat_result.st_mtime_ns, stat_result.st_size)
    return snapshots


def has_changes(previous, current):
    if set(previous) != set(current):
        return True
    for path, previous_state in previous.items():
        if current.get(path) != previous_state:
            return True
    return False


def build_watch_paths(extra_dirs=None):
    watch_paths = [HERE]
    if REPO_ROOT and REPO_ROOT != HERE:
        watch_paths.append(REPO_ROOT)
    for path in extra_dirs or []:
        if path and os.path.exists(path):
            watch_paths.append(path)
    for candidate in ["tests", "src", "include", "examples", "examples_demo", "assets", "build"]:
        full_path = os.path.join(REPO_ROOT, candidate)
        if os.path.exists(full_path) and full_path not in watch_paths:
            watch_paths.append(full_path)
    return [path for path in watch_paths if path and os.path.exists(path)]


def run_server(host, port):
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer((host, port), Handler) as httpd:
        print(f"NSan UI serving on http://localhost:{port}")
        print(json.dumps(health(), indent=2))
        if not health()["nsanClangFound"]:
            print("\n⚠  nsan-clang++ not found — 'Live' mode will error until you build the project "
                  "or set NSAN_CLANGXX / NSAN_REPO_ROOT.")
        if not health()["passPluginFound"]:
            print("⚠  libnsan_pass.so not found — the IR viewer's 'Live' tab will error until you "
                  "run build.sh or set NSAN_PASS_PLUGIN.")
        httpd.serve_forever()


def run_with_reload(host, port, watch_dirs=None):
    watch_paths = build_watch_paths(watch_dirs)
    child = subprocess.Popen(
        [sys.executable, __file__, "--host", host, "--port", str(port)],
        cwd=HERE,
        env=os.environ.copy(),
    )
    previous_files = collect_watch_files(watch_paths)
    try:
        while True:
            time.sleep(1)
            current_files = collect_watch_files(watch_paths)
            if has_changes(previous_files, current_files):
                previous_files = current_files
                print("Detected changes, restarting server...")
                child.terminate()
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait(timeout=5)
                child = subprocess.Popen(
                    [sys.executable, __file__, "--host", host, "--port", str(port)],
                    cwd=HERE,
                    env=os.environ.copy(),
                )
    except KeyboardInterrupt:
        child.terminate()
        try:
            child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait(timeout=5)
        print("\nStopped auto-reload server")


def run_cmd(cmd, cwd=None, env=None, timeout=30, input_text=None):
    try:
        p = subprocess.run(
            cmd, cwd=cwd, env=env, capture_output=True, text=True,
            timeout=timeout, input=input_text,
        )
        return {"ok": True, "returncode": p.returncode, "stdout": p.stdout,
                "stderr": p.stderr, "cmd": " ".join(cmd)}
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "timeout", "cmd": " ".join(cmd)}
    except FileNotFoundError as e:
        return {"ok": False, "error": f"not found: {e}", "cmd": " ".join(cmd)}


def compile_and_run(src_path, eps=None, extra_args=None):
    if not os.path.isfile(NSAN_CLANGXX):
        return {"ok": False, "error": f"nsan-clang++ not found at {NSAN_CLANGXX}. "
                                       f"Run build.sh first, or set NSAN_CLANGXX."}
    with tempfile.TemporaryDirectory() as td:
        binpath = os.path.join(td, "a.out")
        cmd = [NSAN_CLANGXX, "-fsanitize=numerical", src_path, "-o", binpath] + (extra_args or [])
        compile_result = run_cmd(cmd, timeout=60)
        if not compile_result["ok"] or compile_result["returncode"] != 0:
            return {"ok": False, "stage": "compile", **compile_result}
        env = os.environ.copy()
        if eps is not None:
            env["NSAN_REL_EPSILON"] = str(eps)
        run_result = run_cmd([binpath], env=env, timeout=10)
        return {"ok": True, "compile": compile_result, "run": run_result}


def extract_function(ir_text, fn_name="all"):
    """Grab relevant function definitions out of a full .ll dump, excluding
    #include system headers and LLVM declarations."""
    lines = ir_text.splitlines()
    out = []
    capturing = False
    depth = 0
    for line in lines:
        if not capturing:
            if line.startswith("define ") and not ("@__cxa_" in line or "@_GLOBAL__" in line or "@__cxx_" in line):
                capturing = True
        if capturing:
            out.append(line)
            depth += line.count("{") - line.count("}")
            if depth <= 0 and len(out) > 1:
                out.append("")
                capturing = False
    return "\n".join(out).strip() if out else ir_text



def get_ir(src_path, fn_name="main"):
    source_code = ""
    try:
        with open(src_path, "r", encoding="utf-8") as f:
            source_code = f.read()
    except Exception as e:
        source_code = f"// Error reading source: {e}"

    cmd_orig = [CLANGXX] + SYSROOT_ARGS + ["-S", "-emit-llvm", IR_OPT_LEVEL, src_path, "-o", "-"]
    original = run_cmd(cmd_orig, timeout=30)
    if original.get("ok") and original.get("returncode") == 0:
        original["ir"] = extract_function(original["stdout"], fn_name)

    if not PASS_PLUGIN:
        instrumented = {"ok": False, "error": "libnsan_pass.so not found — build the "
                                               "project first, or set NSAN_PASS_PLUGIN."}
    else:
        cmd_inst = [CLANGXX] + SYSROOT_ARGS + ["-S", "-emit-llvm", IR_OPT_LEVEL, f"-fpass-plugin={PASS_PLUGIN}", src_path, "-o", "-"]
        instrumented = run_cmd(
            cmd_inst,
            timeout=30,
        )
        if instrumented.get("ok") and instrumented.get("returncode") == 0:
            instrumented["ir"] = extract_function(instrumented["stdout"], fn_name)

    return {"source": source_code, "original": original, "instrumented": instrumented}



def health():
    return {
        "repoRoot": REPO_ROOT,
        "nsanClangFound": os.path.isfile(NSAN_CLANGXX),
        "nsanClangPath": NSAN_CLANGXX,
        "passPluginFound": bool(PASS_PLUGIN),
        "passPluginPath": PASS_PLUGIN,
        "runtimeLibFound": bool(RUNTIME_LIB),
        "runtimeLibPath": RUNTIME_LIB,
        "clangxx": CLANGXX,
        "irOptLevel": IR_OPT_LEVEL,
    }


def list_testcases():
    if not os.path.isdir(TESTS_DIR):
        return []
    return sorted(f for f in os.listdir(TESTS_DIR) if re.match(r"tc\d+_.*\.cpp$", f))


# ---------------------------------------------------------------------------
# HTTP handler
# ---------------------------------------------------------------------------
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=HERE, **kwargs)

    def _json(self, obj, status=200):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_json_body(self):
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw or b"{}")
        except json.JSONDecodeError:
            return {}

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        qs = urllib.parse.parse_qs(parsed.query)

        if parsed.path == "/api/health":
            return self._json(health())

        if parsed.path == "/api/testcases":
            return self._json({"files": list_testcases()})

        if parsed.path == "/api/ir":
            tc = qs.get("tc", [None])[0]
            fn = qs.get("fn", ["main"])[0]
            if not tc:
                return self._json({"error": "missing ?tc="}, 400)
            src = os.path.join(TESTS_DIR, tc)
            if not os.path.isfile(src):
                return self._json({"error": f"no such test file: {tc}"}, 404)
            return self._json(get_ir(src, fn))

        # ---- GitHub CI proxy routes ----

        if parsed.path == "/api/ci/config":
            return self._json({
                "repo": GH_REPO,
                "hasToken": bool(GH_TOKEN),
                "workflowFile": "build.yml",
            })

        if parsed.path == "/api/ci/runs":
            if not GH_REPO:
                return self._json(
                    {"error": "GitHub repo not detected. Set NSAN_GH_REPO in nsan-ui/.env"}, 400)
            result = gh_request("GET", f"/repos/{GH_REPO}/actions/runs?per_page=10")
            if not result["ok"]:
                return self._json(
                    {"error": f"GitHub API {result.get('status','error')}: {result.get('error','unknown')}",
                     "status": result.get("status")}, 502)
            return self._json(result["data"])

        if parsed.path == "/api/ci/jobs":
            run_id = qs.get("run_id", [None])[0]
            if not run_id:
                return self._json({"error": "missing ?run_id="}, 400)
            if not GH_REPO:
                return self._json({"error": "GitHub repo not detected"}, 400)
            result = gh_request("GET", f"/repos/{GH_REPO}/actions/runs/{run_id}/jobs")
            if not result["ok"]:
                return self._json(
                    {"error": result.get("error"), "status": result.get("status")}, 502)
            return self._json(result["data"])

        if parsed.path == "/api/ci/logs":
            job_id = qs.get("job_id", [None])[0]
            if not job_id:
                return self._json({"error": "missing ?job_id="}, 400)
            if not GH_TOKEN:
                return self._json(
                    {"error": "GITHUB_TOKEN not set. Add it to nsan-ui/.env and restart the server.",
                     "needsToken": True}, 403)
            if not GH_REPO:
                return self._json({"error": "GitHub repo not detected"}, 400)
            result = gh_request(
                "GET", f"/repos/{GH_REPO}/actions/jobs/{job_id}/logs",
                raw=True)
            if not result["ok"]:
                return self._json(
                    {"error": result.get("error"), "status": result.get("status")}, 502)
            log_text = result["raw"].decode("utf-8", errors="replace")
            tc_results = parse_tc_results_from_log(log_text)
            MAX_LOG = 60_000
            if len(log_text) > MAX_LOG:
                log_text = "… (showing last 60 KB)\n" + log_text[-MAX_LOG:]
            return self._json({"log": log_text, "tc_results": tc_results})

        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)

        if parsed.path == "/api/run":
            body = self._read_json_body()
            tc = body.get("tc")
            if not tc:
                return self._json({"error": "missing 'tc' in body"}, 400)
            src = os.path.join(TESTS_DIR, tc)
            if not os.path.isfile(src):
                return self._json({"error": f"no such test file: {tc}"}, 404)
            return self._json(compile_and_run(src, eps=body.get("eps")))

        if parsed.path == "/api/lab":
            body = self._read_json_body()
            a, b, op, eps = body.get("a"), body.get("b"), body.get("op", "+"), body.get("eps")
            if a is None or b is None:
                return self._json({"error": "missing 'a'/'b' in body"}, 400)
            cpp_op = {"+": "+", "-": "-", "*": "*", "/": "/"}.get(op, "+")
            source = f"""// auto-generated by nsan-ui lab
#include <cstdio>
extern "C" void __nsan_check_consistency(float orig, double shadow);
int main() {{
    float a = {a}f;
    float b = {b}f;
    float z = a {cpp_op} b;
    printf("lab_result=%f\\n", z);
    __nsan_check_consistency(z, (double){a} {cpp_op} (double){b});
    return 0;
}}
"""
            with tempfile.TemporaryDirectory() as td:
                src_path = os.path.join(td, "lab.cpp")
                with open(src_path, "w") as f:
                    f.write(source)
                result = compile_and_run(src_path, eps=eps)
                result["source"] = source
                return self._json(result)

        if parsed.path == "/api/ci/trigger":
            body = self._read_json_body()
            ref = body.get("ref", "main")
            if not GH_TOKEN:
                return self._json(
                    {"error": "GITHUB_TOKEN not set. Add it to nsan-ui/.env and restart the server.",
                     "needsToken": True}, 403)
            if not GH_REPO:
                return self._json({"error": "GitHub repo not detected"}, 400)
            result = gh_request(
                "POST",
                f"/repos/{GH_REPO}/actions/workflows/build.yml/dispatches",
                body={"ref": ref},
            )
            if not result["ok"]:
                if result.get("status") == 422:
                    return self._json(
                        {"error": f"Cannot dispatch: workflow_dispatch trigger may be missing or "
                                  f"branch '{ref}' does not exist. Details: {result.get('error','')}"},
                        422)
                return self._json(
                    {"error": result.get("error", "Trigger failed"),
                     "status": result.get("status")}, 502)
            return self._json(
                {"ok": True,
                 "message": f"Workflow dispatch triggered on '{ref}'. Refresh in a few seconds."})

        self.send_error(404)


def main():
    args = parse_args()
    if args.reload:
        run_with_reload(args.host, args.port, args.watch_dir)
    else:
        run_server(args.host, args.port)


if __name__ == "__main__":
    main()
