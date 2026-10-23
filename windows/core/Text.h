// Small Unicode helpers. The app only needs case and class rules for ASCII and
// Cyrillic (U+0400..U+052F), which covers every Kazakh and Russian letter.
#pragma once

#include <string>

namespace kaz {

char32_t toLower(char32_t c);
char32_t toUpper(char32_t c);
inline bool isUpper(char32_t c) { return toLower(c) != c; }
bool isCyrillicLetter(char32_t c);
bool isDigit(char32_t c);
bool isSpace(char32_t c);
inline bool isAsciiLetter(char32_t c) { return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z'); }

std::u32string lower(const std::u32string& s);

std::u32string fromUtf8(const std::string& s);
std::string toUtf8(const std::u32string& s);
std::u32string fromUtf16(const std::u16string& s);
std::u16string toUtf16(const std::u32string& s);

}  // namespace kaz
