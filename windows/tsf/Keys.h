// Reading keys as if the Russian layout were active, like the macOS version.
#pragma once

#include <windows.h>

namespace kaz {

/// The Cyrillic letter this key types on a Russian layout (with Shift and
/// Caps Lock applied), or 0. Uses an installed Russian layout when there is
/// one, else the standard ЙЦУКЕН positions.
char32_t russianLetter(WPARAM vk, LPARAM lParam);

/// Shift, Ctrl, Alt, Win, Caps Lock and friends pressed on their own.
bool isModifierKey(WPARAM vk);

/// Ctrl, Alt or Win is held.
bool commandHeld();

}  // namespace kaz
