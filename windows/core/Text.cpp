#include "Text.h"

namespace kaz {

char32_t toLower(char32_t c) {
    if (c >= U'A' && c <= U'Z') return c + 0x20;
    if (c >= 0x0410 && c <= 0x042F) return c + 0x20;  // А..Я
    if (c >= 0x0400 && c <= 0x040F) return c + 0x50;  // Ѐ..Џ, incl. І
    if (c == 0x04C0) return 0x04CF;                   // Ӏ
    bool evenUpper = (c >= 0x0460 && c <= 0x0481) || (c >= 0x048A && c <= 0x04BF) ||
                     (c >= 0x04D0 && c <= 0x052F);    // Қ Ғ Ң Ұ Ү Һ Ә Ө ...
    if (evenUpper && c % 2 == 0) return c + 1;
    if (c >= 0x04C1 && c <= 0x04CE && c % 2 == 1) return c + 1;
    return c;
}

char32_t toUpper(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 0x20;
    if (c >= 0x0430 && c <= 0x044F) return c - 0x20;
    if (c >= 0x0450 && c <= 0x045F) return c - 0x50;
    if (c == 0x04CF) return 0x04C0;
    bool oddLower = (c >= 0x0461 && c <= 0x0481) || (c >= 0x048B && c <= 0x04BF) ||
                    (c >= 0x04D1 && c <= 0x052F);
    if (oddLower && c % 2 == 1) return c - 1;
    if (c >= 0x04C2 && c <= 0x04CE && c % 2 == 0) return c - 1;
    return c;
}

bool isCyrillicLetter(char32_t c) {
    // Letters only: skip the signs and combining marks at U+0482..U+0489.
    return c >= 0x0400 && c <= 0x04FF && !(c >= 0x0482 && c <= 0x0489);
}

bool isDigit(char32_t c) { return c >= U'0' && c <= U'9'; }

bool isSpace(char32_t c) {
    return (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0x85 || c == 0xA0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F ||
           c == 0x205F || c == 0x3000;
}

std::u32string lower(const std::u32string& s) {
    std::u32string out(s);
    for (auto& c : out) c = toLower(c);
    return out;
}

std::u32string fromUtf8(const std::string& s) {
    std::u32string out;
    for (size_t i = 0; i < s.size();) {
        unsigned char b = s[i];
        int n = b < 0x80 ? 0 : b < 0xE0 ? 1 : b < 0xF0 ? 2 : 3;
        char32_t c = n == 0 ? b : b & (0x3F >> n);
        if (i + n >= s.size()) break;  // truncated sequence
        for (int k = 1; k <= n; k++) c = (c << 6) | (s[i + k] & 0x3F);
        out.push_back(c);
        i += n + 1;
    }
    return out;
}

std::string toUtf8(const std::u32string& s) {
    std::string out;
    for (char32_t c : s) {
        if (c < 0x80) {
            out += char(c);
        } else if (c < 0x800) {
            out += char(0xC0 | (c >> 6));
            out += char(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            out += char(0xE0 | (c >> 12));
            out += char(0x80 | ((c >> 6) & 0x3F));
            out += char(0x80 | (c & 0x3F));
        } else {
            out += char(0xF0 | (c >> 18));
            out += char(0x80 | ((c >> 12) & 0x3F));
            out += char(0x80 | ((c >> 6) & 0x3F));
            out += char(0x80 | (c & 0x3F));
        }
    }
    return out;
}

std::u32string fromUtf16(const std::u16string& s) {
    std::u32string out;
    for (size_t i = 0; i < s.size(); i++) {
        char32_t c = s[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF) {
            c = 0x10000 + ((c - 0xD800) << 10) + (s[++i] - 0xDC00);
        }
        out.push_back(c);
    }
    return out;
}

std::u16string toUtf16(const std::u32string& s) {
    std::u16string out;
    for (char32_t c : s) {
        if (c < 0x10000) {
            out.push_back(char16_t(c));
        } else {
            c -= 0x10000;
            out.push_back(char16_t(0xD800 + (c >> 10)));
            out.push_back(char16_t(0xDC00 + (c & 0x3FF)));
        }
    }
    return out;
}

}  // namespace kaz
