// nsan_gui.cpp  --  NSan Windows GUI Application
//
// Opens a real Win32 window with clickable buttons, coloured log output,
// and input dialogs. No extra libraries — only the Win32 API (ships with
// every Windows installation).
//
// Build:
//   g++ -std=c++14 -O1 nsan_gui.cpp -o nsan_gui.exe -lcomctl32 -mwindows
//
// -mwindows  : GUI app (no console window pops up)
// -lcomctl32 : status bar control

#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

// ── Button IDs ────────────────────────────────────────────────────────────────
#define ID_BTN_RUN_ALL   101
#define ID_BTN_CANCEL    102
#define ID_BTN_SUM5000   103
#define ID_BTN_CUSTOM    104
#define ID_BTN_SUM_EXP   105
#define ID_BTN_LLVM      106
#define ID_BTN_SETTINGS  107
#define ID_BTN_ABOUT     108
#define ID_BTN_CLEAR     109
#define ID_STATUSBAR     201
#define ID_LOG           202

// ── Layout constants ──────────────────────────────────────────────────────────
#define SIDEBAR_W   190
#define BTN_H        34
#define BTN_W       170
#define BTN_X         10
#define STATUS_H      22
#define WINDOW_W     900
#define WINDOW_H     620
#define FONT_SIZE      9

// ── Globals ───────────────────────────────────────────────────────────────────
static HINSTANCE g_hInst   = NULL;
static HWND      g_hWnd    = NULL;
static HWND      g_hLog    = NULL;
static HWND      g_hStatus = NULL;
static HFONT     g_hFont   = NULL;
static HFONT     g_hMono   = NULL;
static HMODULE   g_hRichEdit = NULL;

static int    g_checks   = 0;
static int    g_warnings = 0;
static double g_threshold = 1e-4;

// ── Colour palette ────────────────────────────────────────────────────────────
#define COL_NORMAL  RGB(30,  30,  30)
#define COL_WARN    RGB(200, 50,  50)
#define COL_PASS    RGB(30,  140, 30)
#define COL_HEADER  RGB(0,   90,  200)
#define COL_DIM     RGB(110, 110, 110)
#define COL_YELLOW  RGB(190, 120, 0)
#define COL_SIDEBAR RGB(45,  45,  55)

// ═══════════════════════════════════════════════════════════════════════════════
//  LOG HELPERS
// ═══════════════════════════════════════════════════════════════════════════════

static void log_append(const char *text, COLORREF col, bool bold = false) {
    if (!g_hLog) return;

    // Move caret to end (GetWindowTextLength works for both EDIT and RichEdit)
    LRESULT len = GetWindowTextLengthA(g_hLog);
    SendMessage(g_hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);

    // Set character format (CHARFORMAT works with all RichEdit versions)
    if (g_hRichEdit) {
        CHARFORMATA cf = {};
        cf.cbSize      = sizeof(cf);
        cf.dwMask      = CFM_COLOR | CFM_BOLD | CFM_FACE;
        cf.crTextColor = col;
        cf.dwEffects   = bold ? CFE_BOLD : 0;
        strcpy(cf.szFaceName, "Consolas");
        SendMessage(g_hLog, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    // Insert text
    SendMessage(g_hLog, EM_REPLACESEL, FALSE, (LPARAM)text);

    // Scroll to bottom
    SendMessage(g_hLog, WM_VSCROLL, SB_BOTTOM, 0);
}

static void log_line(const char *text, COLORREF col = COL_NORMAL, bool bold = false) {
    log_append(text, col, bold);
    log_append("\r\n", COL_NORMAL, false);
}

static void log_separator() {
    log_line("----------------------------------------------------------", COL_DIM);
}

static void log_header(const char *title) {
    log_separator();
    std::string t = "  ";
    t += title;
    log_line(t.c_str(), COL_HEADER, true);
    log_separator();
}

static void update_status() {
    char buf[128];
    sprintf(buf, "  Checks: %d  |  Warnings: %d  |  Threshold: %.0e",
            g_checks, g_warnings, g_threshold);
    SendMessageA(g_hStatus, SB_SETTEXTA, 0, (LPARAM)buf);
}

// ── Format a double to a short string ─────────────────────────────────────────
static std::string fmt(double v, int prec = 10) {
    std::ostringstream ss;
    ss << std::setprecision(prec) << v;
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  NSAN CORE  (same logic as the terminal demo)
// ═══════════════════════════════════════════════════════════════════════════════

static bool nsan_check_log(float original, long double shadow,
                            const std::string &label) {
    ++g_checks;
    double o   = (double)original;
    double s   = (double)shadow;
    double ae  = fabs(o - s);
    double re  = (fabs(s) > 1e-15) ? ae / fabs(s) : ae;
    bool   w   = (re > g_threshold);
    if (w) ++g_warnings;

    // Label
    std::string ln = "  " + label;
    while ((int)ln.size() < 36) ln += ' ';
    log_append(ln.c_str(), COL_DIM);

    // float32 value
    std::string fv = "f32=" + fmt(o, 8);
    while ((int)fv.size() < 22) fv += ' ';
    log_append(fv.c_str(), COL_NORMAL);

    // shadow value
    std::string sv = "ref=" + fmt(s, 8);
    while ((int)sv.size() < 22) sv += ' ';
    log_append(sv.c_str(), COL_DIM);

    // result
    char relbuf[32];
    sprintf(relbuf, "err=%.2e", re);
    if (w) {
        log_append(relbuf, COL_WARN, true);
        log_line("  [WARN]", COL_WARN, true);
    } else {
        log_append(relbuf, COL_PASS);
        log_line("  [PASS]", COL_PASS);
    }

    update_status();
    return w;
}

// ── Test 1: Catastrophic cancellation ─────────────────────────────────────────
static void run_cancellation() {
    log_header("Test: Catastrophic Cancellation");
    log_line("  (A + B) - A should equal B.", COL_DIM);
    log_line("  When A >> B, float loses B during A+B.\r\n", COL_DIM);

    float       a = 1e7f, b = 1.234f;
    long double a_ld = 1e7L, b_ld = 1.234L;
    float       result   = (a + b) - a;
    long double ref      = (a_ld + b_ld) - a_ld;

    char buf[128];
    sprintf(buf, "  A=1e7,  B=1.234,  Expected=1.234,  float says=%.6g", (double)result);
    log_line(buf, COL_NORMAL);
    log_line("");

    nsan_check_log(result, ref, "(A+B)-A");

    if (result == 0.0f)
        log_line("  => float LOST B entirely!", COL_WARN, true);
    else if (fabsf(result - b) > 0.01f) {
        sprintf(buf, "  => float degraded B from 1.234 to %.6g", (double)result);
        log_line(buf, COL_YELLOW, true);
    } else {
        log_line("  => float preserved B.", COL_PASS);
    }
    log_line("");
}

// ── Test 2: Naive vs Kahan ─────────────────────────────────────────────────────
static void run_summation(int N) {
    char buf[128];
    sprintf(buf, "Test: Naive vs Kahan Summation (N=%d)", N);
    log_header(buf);
    log_line("  Values near 50000. Naive drifts; Kahan stays accurate.\r\n", COL_DIM);

    std::vector<float> v(N);
    srand(42);
    for (int i = 0; i < N; ++i)
        v[i] = 50000.0f + (float)(rand() % 1000) / 1000.0f;

    long double ref = 0.0L;
    for (int i = 0; i < N; ++i) ref += (long double)v[i];

    float naive = 0.0f;
    for (int i = 0; i < N; ++i) naive += v[i];

    float kahan = 0.0f, c = 0.0f;
    for (int i = 0; i < N; ++i) {
        float y = v[i] - c, t = kahan + y;
        c = (t - kahan) - y; kahan = t;
    }

    double ne = fabs((double)naive - (double)ref) / fabs((double)ref);
    double ke = fabs((double)kahan - (double)ref) / fabs((double)ref);

    sprintf(buf, "  Reference : %.2f", (double)ref);   log_line(buf, COL_DIM);
    sprintf(buf, "  Naive     : %.2f  (rel err %.2e)", (double)naive, ne);
    log_line(buf, ne > g_threshold ? COL_WARN : COL_NORMAL);
    sprintf(buf, "  Kahan     : %.2f  (rel err %.2e)", (double)kahan, ke);
    log_line(buf, ke > g_threshold ? COL_WARN : COL_PASS);
    log_line("");

    nsan_check_log(naive, ref,  "naive sum (final)");
    nsan_check_log(kahan, ref,  "kahan sum (final)");
    log_line("");
}

// ── Run all presets ────────────────────────────────────────────────────────────
static void run_all() {
    run_cancellation();
    run_summation(5000);
}

// ── Custom: user-supplied A, B, operation ─────────────────────────────────────
static void run_custom(double A, double B, int op) {
    float       fa = (float)A, fb = (float)B;
    long double la = (long double)A, lb = (long double)B;
    float       rf = 0; long double rl = 0;
    const char *lbl = "";
    char buf[128];

    switch (op) {
        case 0: rf = fa + fb; rl = la + lb; lbl = "A + B";      break;
        case 1: rf = fa - fb; rl = la - lb; lbl = "A - B";      break;
        case 2: rf = fa * fb; rl = la * lb; lbl = "A * B";      break;
        case 3:
            if (fabsf(fb) < 1e-30f) {
                log_line("  Error: division by zero.", COL_WARN, true); return;
            }
            rf = fa / fb; rl = la / lb; lbl = "A / B"; break;
        case 4: rf = (fa+fb)-fa; rl = (la+lb)-la; lbl = "(A+B)-A"; break;
        case 5: {
            float       direct_f  = fa*fa - fb*fb;
            long double direct_ld = la*la - lb*lb;
            float       factor_f  = (fa+fb)*(fa-fb);
            long double factor_ld = (la+lb)*(la-lb);
            log_header("Custom: A^2 - B^2  vs  (A+B)(A-B)");
            sprintf(buf, "  A=%.6g  B=%.6g", A, B); log_line(buf, COL_DIM);
            log_line("");
            nsan_check_log(direct_f, direct_ld, "A^2-B^2  (direct)");
            nsan_check_log(factor_f, factor_ld, "(A+B)(A-B) (stable)");
            log_line("  Factored form is more numerically stable.", COL_DIM);
            log_line(""); return;
        }
    }

    log_header("Custom Computation");
    sprintf(buf, "  A=%.10g   B=%.10g   Op: %s", A, B, lbl);
    log_line(buf, COL_DIM);
    log_line("");
    nsan_check_log(rf, rl, lbl);
    log_line("");
}

// ── Summation explorer ─────────────────────────────────────────────────────────
static void run_sum_explorer(int N, double base, double range) {
    std::vector<float> v(N);
    srand(42);
    for (int i = 0; i < N; ++i)
        v[i] = (float)(base + (rand() % 10000) / 10000.0 * range);

    long double ref = 0.0L;
    for (int i = 0; i < N; ++i) ref += (long double)v[i];

    float naive = 0.0f;
    for (int i = 0; i < N; ++i) naive += v[i];

    float kahan = 0.0f, c = 0.0f;
    for (int i = 0; i < N; ++i) {
        float y = v[i]-c, t = kahan+y; c = (t-kahan)-y; kahan = t;
    }

    char buf[128], tbuf[64];
    sprintf(tbuf, "Summation Explorer (N=%d)", N);
    log_header(tbuf);
    sprintf(buf, "  N=%d  base=%.4g  range=[%.4g, %.4g]", N, base, base, base+range);
    log_line(buf, COL_DIM);
    log_line("");

    sprintf(buf, "  Reference : %.4f", (double)ref); log_line(buf, COL_DIM);

    double ne = fabs((double)naive-(double)ref)/fabs((double)ref);
    double ke = fabs((double)kahan-(double)ref)/fabs((double)ref);
    sprintf(buf, "  Naive     : %.4f  err=%.2e", (double)naive, ne);
    log_line(buf, ne > g_threshold ? COL_WARN : COL_NORMAL);
    sprintf(buf, "  Kahan     : %.4f  err=%.2e", (double)kahan, ke);
    log_line(buf, ke > g_threshold ? COL_WARN : COL_PASS);
    log_line("");

    nsan_check_log(naive, ref, "naive sum");
    nsan_check_log(kahan, ref, "kahan sum");
    log_line("");
}

// ═══════════════════════════════════════════════════════════════════════════════
//  INPUT DIALOG  (lightweight modal input box — no .rc file needed)
// ═══════════════════════════════════════════════════════════════════════════════

struct InputDlgData {
    const char *prompt;
    char        result[256];
    bool        ok;
    HWND        hEdit;
    bool        done;
};

static LRESULT CALLBACK InputDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    InputDlgData *d = (InputDlgData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
            d = (InputDlgData *)cs->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)d);

            HFONT hf = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");

            HWND hLbl = CreateWindowA("STATIC", d->prompt,
                WS_CHILD|WS_VISIBLE, 12, 12, 280, 18, hwnd, NULL, g_hInst, NULL);
            SendMessage(hLbl, WM_SETFONT, (WPARAM)hf, TRUE);

            d->hEdit = CreateWindowA("EDIT", "",
                WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,
                12, 36, 280, 24, hwnd, NULL, g_hInst, NULL);
            SendMessage(d->hEdit, WM_SETFONT, (WPARAM)hf, TRUE);
            SetFocus(d->hEdit);

            HWND hOK = CreateWindowA("BUTTON", "OK",
                WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                12, 70, 80, 28, hwnd, (HMENU)IDOK, g_hInst, NULL);
            SendMessage(hOK, WM_SETFONT, (WPARAM)hf, TRUE);

            HWND hCancel = CreateWindowA("BUTTON", "Cancel",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                100, 70, 80, 28, hwnd, (HMENU)IDCANCEL, g_hInst, NULL);
            SendMessage(hCancel, WM_SETFONT, (WPARAM)hf, TRUE);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wp) == IDOK) {
                GetWindowTextA(d->hEdit, d->result, 255);
                d->ok = true; d->done = true;
                EnableWindow(g_hWnd, TRUE);
                DestroyWindow(hwnd);
            } else if (LOWORD(wp) == IDCANCEL) {
                d->ok = false; d->done = true;
                EnableWindow(g_hWnd, TRUE);
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_RETURN && d) {
                GetWindowTextA(d->hEdit, d->result, 255);
                d->ok = true; d->done = true;
                EnableWindow(g_hWnd, TRUE);
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_CLOSE:
            d->ok = false; d->done = true;
            EnableWindow(g_hWnd, TRUE);
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// Returns false if user cancelled
static bool ShowInputBox(HWND parent, const char *title,
                          const char *prompt, char *out, int outLen) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSA wc = {};
        wc.lpfnWndProc   = InputDlgProc;
        wc.hInstance     = g_hInst;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "NSanInputDlg";
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wc);
        registered = true;
    }

    InputDlgData d = {};
    d.prompt = prompt;
    d.ok = false; d.done = false;

    RECT pr; GetWindowRect(parent, &pr);
    int x = pr.left + (pr.right - pr.left - 310) / 2;
    int y = pr.top  + (pr.bottom - pr.top - 115) / 2;

    EnableWindow(parent, FALSE);
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "NSanInputDlg", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, 310, 115, parent, NULL, g_hInst, &d);

    MSG msg;
    while (!d.done && GetMessageA(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageA(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (d.ok) strncpy(out, d.result, outLen - 1);
    return d.ok;
}

// ── Simple numeric input helper ────────────────────────────────────────────────
static bool ask_double(const char *title, const char *prompt, double *out) {
    char buf[64] = "";
    if (!ShowInputBox(g_hWnd, title, prompt, buf, sizeof(buf))) return false;
    *out = atof(buf);
    return true;
}
static bool ask_int(const char *title, const char *prompt, int *out) {
    double d = 0;
    if (!ask_double(title, prompt, &d)) return false;
    *out = (int)d;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  BUTTON HANDLERS
// ═══════════════════════════════════════════════════════════════════════════════

static void on_custom() {
    double A = 0, B = 0;
    if (!ask_double("Custom Computation", "Enter value A:", &A)) return;
    if (!ask_double("Custom Computation", "Enter value B:", &B)) return;

    // Operation choice dialog
    int choice = MessageBoxA(g_hWnd,
        "Choose operation:\n\n"
        "Yes  =  (A + B)\n"
        "No   =  (A - B)\n"
        "(Close the box to see more options)",
        "Operation", MB_YESNOCANCEL | MB_ICONQUESTION);

    int op = -1;
    if      (choice == IDYES)    op = 0;  // A+B
    else if (choice == IDNO)     op = 1;  // A-B
    else {
        // second box
        int ch2 = MessageBoxA(g_hWnd,
            "Choose operation:\n\n"
            "Yes  =  (A * B)\n"
            "No   =  (A / B)",
            "Operation", MB_YESNOCANCEL | MB_ICONQUESTION);
        if      (ch2 == IDYES) op = 2;
        else if (ch2 == IDNO)  op = 3;
        else {
            // third box
            int ch3 = MessageBoxA(g_hWnd,
                "Choose operation:\n\n"
                "Yes  =  (A + B) - A   (cancellation)\n"
                "No   =  A^2 - B^2  vs  (A+B)(A-B)",
                "Operation", MB_YESNO | MB_ICONQUESTION);
            op = (ch3 == IDYES) ? 4 : 5;
        }
    }
    if (op >= 0) run_custom(A, B, op);
}

static void on_sum_explorer() {
    int N = 0; double base = 0, range = 0;
    if (!ask_int   ("Summation Explorer", "Number of values N (e.g. 10000):", &N)) return;
    if (!ask_double("Summation Explorer", "Base value (e.g. 50000):", &base))      return;
    if (!ask_double("Summation Explorer", "Random range 0..X (e.g. 1):", &range))  return;
    if (N <= 0 || N > 5000000) {
        MessageBoxA(g_hWnd, "N must be between 1 and 5,000,000.", "Error", MB_OK|MB_ICONERROR);
        return;
    }
    run_sum_explorer(N, base, range);
}

static void on_settings() {
    char buf[64];
    sprintf(buf, "%.2e", g_threshold);
    char out[64] = "";
    strncpy(out, buf, sizeof(out));
    if (!ShowInputBox(g_hWnd, "Settings",
                      "Warning threshold (e.g. 1e-4, 1e-6):", out, sizeof(out)))
        return;
    double t = atof(out);
    if (t <= 0) { MessageBoxA(g_hWnd, "Invalid threshold.", "Error", MB_OK|MB_ICONERROR); return; }
    g_threshold = t;
    sprintf(buf, "Threshold set to %.2e", g_threshold);
    log_line(buf, COL_DIM);
    update_status();
}

static void on_llvm_view() {
    MessageBoxA(g_hWnd,
        "What NSan's LLVM pass injects into your code:\r\n\r\n"

        "Your source:\r\n"
        "   float sum = a + b;\r\n\r\n"

        "LLVM IR BEFORE NSan:\r\n"
        "   %sum = fadd float %a, %b\r\n\r\n"

        "LLVM IR AFTER NSan pass:\r\n"
        "   %sum         = fadd float %a, %b\r\n"
        "   %a_shadow    = fpext float %a to double\r\n"
        "   %b_shadow    = fpext float %b to double\r\n"
        "   %sum_shadow  = fadd double %a_shadow, %b_shadow\r\n"
        "   call void @__nsan_check_consistency_float(\r\n"
        "        float %sum, double %sum_shadow)\r\n\r\n"

        "nsan_runtime.cpp checks:\r\n"
        "   double err = |original - shadow| / |shadow|\r\n"
        "   if (err > threshold) print WARNING;\r\n\r\n"

        "This happens for EVERY float op automatically.\r\n"
        "You never write the shadow code yourself.",

        "NSan: LLVM IR Transformation", MB_OK | MB_ICONINFORMATION);
}

static void on_about() {
    MessageBoxA(g_hWnd,
        "NSan  --  Numerical Stability Sanitizer\r\n"
        "Compiler Design Lab Project\r\n\r\n"

        "WHAT IT IS:\r\n"
        "A compiler plugin that detects floating-point\r\n"
        "precision errors automatically.\r\n\r\n"

        "HOW IT WORKS:\r\n"
        "1. NSan's LLVM pass intercepts every float op\r\n"
        "2. Injects a parallel shadow computation in double\r\n"
        "3. At runtime, checks if float == double (approx)\r\n"
        "4. Prints a WARNING if they diverge too much\r\n\r\n"

        "THIS DEMO:\r\n"
        "Simulates the runtime checking layer so you can\r\n"
        "experiment without needing LLVM installed.\r\n\r\n"

        "REAL USAGE (Linux + LLVM):\r\n"
        "clang++ -Xclang -load -Xclang libnsan_pass.so \\\r\n"
        "  my_prog.cpp -lnsan_runtime -o my_prog\r\n"
        "./my_prog   # NSan warnings printed automatically",

        "About NSan", MB_OK | MB_ICONINFORMATION);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  MAIN WINDOW PROCEDURE
// ═══════════════════════════════════════════════════════════════════════════════

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_BTN_RUN_ALL:  run_all();          break;
                case ID_BTN_CANCEL:   run_cancellation(); break;
                case ID_BTN_SUM5000:  run_summation(5000);break;
                case ID_BTN_CUSTOM:   on_custom();        break;
                case ID_BTN_SUM_EXP:  on_sum_explorer();  break;
                case ID_BTN_LLVM:     on_llvm_view();     break;
                case ID_BTN_SETTINGS: on_settings();      break;
                case ID_BTN_ABOUT:    on_about();         break;
                case ID_BTN_CLEAR:
                    SetWindowTextA(g_hLog, "");
                    g_checks = 0; g_warnings = 0;
                    update_status();
                    break;
            }
            return 0;

        case WM_SIZE: {
            int W = LOWORD(lp), H = HIWORD(lp);
            if (g_hLog)
                SetWindowPos(g_hLog, NULL, SIDEBAR_W + 4, 0,
                             W - SIDEBAR_W - 4, H - STATUS_H,
                             SWP_NOZORDER);
            if (g_hStatus)
                SendMessage(g_hStatus, WM_SIZE, 0, 0);
            return 0;
        }

        // Paint the sidebar background
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc; GetClientRect(hwnd, &rc);
            rc.right = SIDEBAR_W;
            HBRUSH hbr = CreateSolidBrush(COL_SIDEBAR);
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);
            // Right panel background (white)
            RECT rc2; GetClientRect(hwnd, &rc2);
            rc2.left = SIDEBAR_W;
            HBRUSH hbr2 = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rc2, hbr2);
            DeleteObject(hbr2);
            return 1;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// ── Create a sidebar button ────────────────────────────────────────────────────
static HWND make_button(HWND parent, const char *label, int id, int y) {
    HWND h = CreateWindowA("BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BTN_X, y, BTN_W, BTN_H,
        parent, (HMENU)(UINT_PTR)id, g_hInst, NULL);
    SendMessage(h, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    return h;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  WINMAIN  --  entry point
// ═══════════════════════════════════════════════════════════════════════════════

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;

    // Load RichEdit for coloured text.
    // riched20.dll registers the ANSI class "RichEdit20A" which we use below.
    g_hRichEdit = LoadLibraryA("riched20.dll");

    // Initialise common controls (status bar)
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    // Create fonts
    g_hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    g_hMono = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");

    // Register main window class
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "NSanGUI";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    // Create main window
    g_hWnd = CreateWindowExA(0, "NSanGUI",
        "NSan  --  Numerical Stability Sanitizer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_W, WINDOW_H,
        NULL, NULL, hInstance, NULL);

    // ── Sidebar buttons ──────────────────────────────────────────────────────
    int y = 12;
    make_button(g_hWnd, "Run All Tests",       ID_BTN_RUN_ALL,  y); y += BTN_H + 6;
    make_button(g_hWnd, "Cancellation Demo",   ID_BTN_CANCEL,   y); y += BTN_H + 6;
    make_button(g_hWnd, "Summation (N=5000)",  ID_BTN_SUM5000,  y); y += BTN_H + 14;
    make_button(g_hWnd, "Custom Computation",  ID_BTN_CUSTOM,   y); y += BTN_H + 6;
    make_button(g_hWnd, "Summation Explorer",  ID_BTN_SUM_EXP,  y); y += BTN_H + 14;
    make_button(g_hWnd, "View LLVM IR",        ID_BTN_LLVM,     y); y += BTN_H + 6;
    make_button(g_hWnd, "Settings",            ID_BTN_SETTINGS, y); y += BTN_H + 6;
    make_button(g_hWnd, "About NSan",          ID_BTN_ABOUT,    y); y += BTN_H + 14;
    make_button(g_hWnd, "Clear Log",           ID_BTN_CLEAR,    y);

    // ── RichEdit log panel ───────────────────────────────────────────────────
    // "RichEdit20A" is the ANSI class registered by riched20.dll.
    // Fall back to plain "EDIT" if the dll didn't load.
    const char *richClass = g_hRichEdit ? "RichEdit20A" : "EDIT";

    g_hLog = CreateWindowExA(WS_EX_CLIENTEDGE, richClass, "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        SIDEBAR_W + 4, 0,
        WINDOW_W - SIDEBAR_W - 4 - 16, WINDOW_H - STATUS_H - 39,
        g_hWnd, (HMENU)ID_LOG, hInstance, NULL);
    SendMessage(g_hLog, WM_SETFONT, (WPARAM)g_hMono, TRUE);
    SendMessage(g_hLog, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 28));

    // ── Status bar ───────────────────────────────────────────────────────────
    g_hStatus = CreateWindowExA(0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        g_hWnd, (HMENU)ID_STATUSBAR, hInstance, NULL);
    SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    update_status();

    // ── Welcome message ──────────────────────────────────────────────────────
    log_line("  NSan -- Numerical Stability Sanitizer", COL_HEADER, true);
    log_line("  Click a button on the left to begin.", COL_DIM);
    log_line("", COL_NORMAL);
    log_line("  - 'Run All Tests'       : pre-built demonstrations", COL_DIM);
    log_line("  - 'Custom Computation'  : enter your own values", COL_DIM);
    log_line("  - 'Summation Explorer'  : choose N and value range", COL_DIM);
    log_line("  - 'View LLVM IR'        : see what NSan injects", COL_DIM);
    log_separator();

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_hRichEdit) FreeLibrary(g_hRichEdit);
    DeleteObject(g_hFont);
    DeleteObject(g_hMono);
    return (int)msg.wParam;
}
