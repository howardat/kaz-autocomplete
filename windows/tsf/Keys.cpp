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

}  // namespace

char32_t russianLetter(WPARAM vk, LPARAM lParam) {
    UINT scan = (lParam >> 16) & 0xFF;
    bool extended = (lParam >> 24) & 1;
    if (extended) return 0;

    if (HKL hkl = russianLayout()) {
        BYTE state[256];
        if (GetKeyboardState(state)) {
            state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;
            state[VK_MENU] = state[VK_LMENU] = state[VK_RMENU] = 0;
            wchar_t buf[4];
            // Flag 4: leave the kernel's dead-key state alone.
            int n = ToUnicodeEx(UINT(vk), scan, state, buf, 4, 4, hkl);
            if (n == 1 && isCyrillicLetter(buf[0])) return buf[0];
            if (n != 0) return 0;  // some other character
        }
    }

    char32_t c = tableLetter(scan);
    if (!c) return 0;
    bool shift = GetKeyState(VK_SHIFT) & 0x8000;
    bool caps = GetKeyState(VK_CAPITAL) & 1;
    return shift != caps ? toUpper(c) : c;
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
