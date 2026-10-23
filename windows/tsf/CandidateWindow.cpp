#include "CandidateWindow.h"

#include <dwmapi.h>
#include <windowsx.h>

#include <algorithm>

#include "Globals.h"

namespace kaz {
namespace {

const wchar_t kClassName[] = L"KazTypeCandidateWindow";

/// shadcn/ui "zinc" theme tokens, as used by its Command menu.
struct Theme {
    COLORREF background, border, foreground, muted, accent, kbdBg, kbdBorder;
};
const Theme kLight = {RGB(255, 255, 255), RGB(228, 228, 231), RGB(9, 9, 11), RGB(113, 113, 122),
                      RGB(244, 244, 245), RGB(250, 250, 250), RGB(228, 228, 231)};
const Theme kDark = {RGB(9, 9, 11), RGB(39, 39, 42), RGB(250, 250, 250), RGB(161, 161, 170),
                     RGB(39, 39, 42), RGB(24, 24, 27), RGB(63, 63, 70)};

bool darkMode() {
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light == 0;
}

UINT dpiFor(HWND hwnd) {
    // GetDpiForWindow needs Windows 10 1607; fall back to the system DPI.
    using Fn = UINT(WINAPI*)(HWND);
    static auto fn = reinterpret_cast<Fn>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")));
    if (fn && hwnd) return fn(hwnd);
    HDC dc = GetDC(nullptr);
    UINT dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(nullptr, dc);
    return dpi;
}

HFONT makeFont(UINT dpi, int pt, int weight) {
    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(pt, dpi, 72);
    lf.lfWeight = weight;
    lf.lfQuality = CLEARTYPE_QUALITY;
    // Segoe UI Variable on Windows 11; GDI falls back to Segoe UI.
    wcscpy_s(lf.lfFaceName, L"Segoe UI Variable Text");
    HFONT f = CreateFontIndirectW(&lf);
    return f;
}

void fillRound(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF border) {
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(b);
    DeleteObject(p);
}

}  // namespace

CandidateWindow::~CandidateWindow() {
    if (hwnd_) DestroyWindow(hwnd_);
    if (font_) DeleteObject(font_);
    if (small_) DeleteObject(small_);
}

bool CandidateWindow::create() {
    if (hwnd_) return true;
    WNDCLASSEXW wc = {sizeof(wc)};
    if (!GetClassInfoExW(g_module, kClassName, &wc)) {
        wc.style = CS_DROPSHADOW | CS_IME;
        wc.lpfnWndProc = proc;
        wc.hInstance = g_module;
        wc.hCursor = LoadCursor(nullptr, IDC_HAND);
        wc.lpszClassName = kClassName;
        if (!RegisterClassExW(&wc)) return false;
    }
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kClassName, L"", WS_POPUP, 0, 0, 1,
                            1, nullptr, nullptr, g_module, this);
    if (!hwnd_) return false;
    // Windows 11: rounded corners with the system shadow. Ignored elsewhere.
    DWORD round = 3;  // DWMWCP_ROUNDSMALL
    DwmSetWindowAttribute(hwnd_, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &round, sizeof(round));
    return true;
}

void CandidateWindow::show(const std::vector<std::wstring>& items, int selected, const RECT& anchor) {
    if (!create()) return;
    items_ = items;
    selected_ = selected;
    layout(anchor);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::select(int selected) {
    selected_ = selected;
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

void CandidateWindow::layout(const RECT& anchor) {
    UINT dpi = dpiFor(hwnd_);
    if (!font_ || fontDpi_ != dpi) {
        if (font_) DeleteObject(font_);
        if (small_) DeleteObject(small_);
        font_ = makeFont(dpi, 11, FW_NORMAL);
        small_ = makeFont(dpi, 8, FW_MEDIUM);
        fontDpi_ = dpi;
    }
    auto px = [dpi](int v) { return MulDiv(v, dpi, 96); };
    padding_ = px(4);     // container p-1
    rowPadX_ = px(8);     // item px-2
    rowHeight_ = px(32);  // item py-1.5 + text
    kbd_ = px(18);
    radius_ = px(6);      // rounded-sm on items, rounded-md on the panel

    HDC dc = GetDC(hwnd_);
    HGDIOBJ old = SelectObject(dc, font_);
    int text = 0;
    for (const auto& s : items_) {
        SIZE sz;
        GetTextExtentPoint32W(dc, s.c_str(), int(s.size()), &sz);
        text = std::max(text, int(sz.cx));
    }
    SelectObject(dc, old);
    ReleaseDC(hwnd_, dc);

    int w = std::max(px(160), 2 * padding_ + 2 * rowPadX_ + text + px(16) + kbd_);
    int h = 2 * padding_ + rowHeight_ * int(items_.size());

    // Below the word, or above it when that runs off the screen.
    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& wa = mi.rcWork;
    int x = std::max(LONG(wa.left), std::min(LONG(anchor.left - padding_ - rowPadX_), LONG(wa.right - w)));
    int y = anchor.bottom + px(4);
    if (y + h > wa.bottom) y = std::max(LONG(wa.top), LONG(anchor.top - px(4) - h));
    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CandidateWindow::paint(HDC target) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    const Theme& t = darkMode() ? kDark : kLight;
    COLORREF borderColor = t.border;
    DwmSetWindowAttribute(hwnd_, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));

    // Double-buffered to avoid flicker while typing.
    HDC dc = CreateCompatibleDC(target);
    HBITMAP bmp = CreateCompatibleBitmap(target, rc.right, rc.bottom);
    HGDIOBJ oldBmp = SelectObject(dc, bmp);

    fillRound(dc, {0, 0, rc.right, rc.bottom}, radius_ * 2, t.background, t.border);
    SetBkMode(dc, TRANSPARENT);
    for (size_t i = 0; i < items_.size(); i++) {
        RECT row = {padding_, padding_ + int(i) * rowHeight_, rc.right - padding_, padding_ + int(i + 1) * rowHeight_};
        bool sel = int(i) == selected_;
        if (sel) fillRound(dc, row, radius_, t.accent, t.accent);

        RECT text = row;
        text.left += rowPadX_;
        SelectObject(dc, font_);
        SetTextColor(dc, t.foreground);
        DrawTextW(dc, items_[i].c_str(), int(items_[i].size()), &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        // Shortcut hint, like shadcn's <CommandShortcut>/<kbd>.
        int top = row.top + (rowHeight_ - kbd_) / 2;
        RECT kbd = {row.right - rowPadX_ - kbd_, top, row.right - rowPadX_, top + kbd_};
        fillRound(dc, kbd, radius_ / 2 * 2, sel ? t.background : t.kbdBg, t.kbdBorder);
        SelectObject(dc, small_);
        SetTextColor(dc, t.muted);
        auto n = std::to_wstring(i + 1);
        DrawTextW(dc, n.c_str(), int(n.size()), &kbd, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    }

    BitBlt(target, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBmp);
    DeleteObject(bmp);
    DeleteDC(dc);
}

int CandidateWindow::rowAt(int y) const {
    if (rowHeight_ <= 0 || y < padding_) return -1;
    int i = (y - padding_) / rowHeight_;
    return i < int(items_.size()) ? i : -1;
}

LRESULT CALLBACK CandidateWindow::proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams));
    }
    auto* self = reinterpret_cast<CandidateWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (self) self->paint(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        // Hover highlights, as in shadcn's Command list.
        if (self) {
            int i = self->rowAt(GET_Y_LPARAM(lp));
            if (i >= 0 && i != self->hover_) {
                self->hover_ = i;
                if (self->onHover) self->onHover(i);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (self && self->onPick) {
            int i = self->rowAt(GET_Y_LPARAM(lp));
            if (i >= 0) self->onPick(i);
        }
        return 0;
    case WM_NCDESTROY:
        if (self) self->hwnd_ = nullptr;
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace kaz
