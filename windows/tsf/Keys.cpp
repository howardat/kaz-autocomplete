#include "Keys.h"

#include <vector>

#include "../core/Text.h"

namespace kaz {
namespace {

bool isRussian(HKL hkl) { return LOWORD(reinterpret_cast<ULONG_PTR>(hkl)) == 0x0419; }

/// The thread's layout if it is Russian (the profile asks for Russian as its
/// substitute layout), else the first installed Russian layout, else null.
HKL russianLayout() {
    HKL current = GetKeyboardLayout(0);
    if (isRussian(current)) return current;
    int n = GetKeyboardLayoutList(0, nullptr);
    std::vector<HKL> layouts(n > 0 ? n : 0);
    n = GetKeyboardLayoutList(n, layouts.data());
    for (int i = 0; i < n; i++)
        if (isRussian(layouts[i])) return layouts[i];
    return nullptr;
}

/// Russian - PC layout by scan code, lowercase.
char32_t tableLetter(UINT scan) {
    static const char32_t row1[] = U"йцукенгшщзхъ";  // 0x10..0x1B
    static const char32_t row2[] = U"фывапролджэ";   // 0x1E..0x28
    static const char32_t row3[] = U"ячсмитьбю";     // 0x2C..0x34
    if (scan >= 0x10 && scan <= 0x1B) return row1[scan - 0x10];
    if (scan >= 0x1E && scan <= 0x28) return row2[scan - 0x1E];
    if (scan >= 0x2C && scan <= 0x34) return row3[scan - 0x2C];
    if (scan == 0x29) return U'ё';
    return 0;
}

/// Number row, \ above Enter, / right of the period, and \ left of Z on ISO keyboards.
bool isSymbolKey(UINT scan) { return (scan >= 0x02 && scan <= 0x0D) || scan == 0x2B || scan == 0x35 || scan == 0x56; }

/// Russian - PC layout for symbol keys.
char32_t tableSymbol(UINT scan, bool shift) {
    static const char32_t row[] = U"1234567890-=";       // 0x02..0x0D
    static const char32_t shifted[] = U"!\"№;%:?*()_+";
    if (scan >= 0x02 && scan <= 0x0D) return (shift ? shifted : row)[scan - 0x02];
    if (scan == 0x35) return shift ? U',' : U'.';
    return shift ? U'/' : U'\\';
}

/// ToUnicodeEx on the Russian layout without Ctrl and Alt: 1 with the
/// character in `out`, -1 or 2+ for a dead key or several characters, 0 for
/// nothing or no Russian layout installed.
int toRussian(WPARAM vk, UINT scan, wchar_t& out) {
    HKL hkl = russianLayout();
    BYTE state[256];
    if (!hkl || !GetKeyboardState(state)) return 0;
    state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;
    state[VK_MENU] = state[VK_LMENU] = state[VK_RMENU] = 0;
    wchar_t buf[4];
    // Flag 4: leave the kernel's dead-key state alone.
    int n = ToUnicodeEx(UINT(vk), scan, state, buf, 4, 4, hkl);
    if (n == 1) out = buf[0];
    return n;
}

}  // namespace

char32_t russianLetter(WPARAM vk, LPARAM lParam) {
    UINT scan = (lParam >> 16) & 0xFF;
    bool extended = (lParam >> 24) & 1;
    if (extended) return 0;

    wchar_t ch = 0;
    int n = toRussian(vk, scan, ch);
    if (n == 1 && isCyrillicLetter(ch)) return ch;
    if (n != 0) return 0;  // some other character

    char32_t c = tableLetter(scan);
    if (!c) return 0;
    bool shift = GetKeyState(VK_SHIFT) & 0x8000;
    bool caps = GetKeyState(VK_CAPITAL) & 1;
    return shift != caps ? toUpper(c) : c;
}

char32_t russianSymbol(WPARAM vk, LPARAM lParam) {
    UINT scan = (lParam >> 16) & 0xFF;
    bool extended = (lParam >> 24) & 1;
    if (extended || !isSymbolKey(scan)) return 0;

    wchar_t ch = 0;
    int n = toRussian(vk, scan, ch);
    if (n == 1) return ch;
    if (n != 0) return 0;
    return tableSymbol(scan, GetKeyState(VK_SHIFT) & 0x8000);
}

bool isModifierKey(WPARAM vk) {
    switch (vk) {
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_MENU: case VK_LMENU: case VK_RMENU:
    case VK_LWIN: case VK_RWIN: case VK_APPS:
    case VK_CAPITAL: case VK_NUMLOCK: case VK_SCROLL:
        return true;
    default:
        return false;
    }
}

bool commandHeld() {
    return (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000) ||
           (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);
}

}  // namespace kaz
