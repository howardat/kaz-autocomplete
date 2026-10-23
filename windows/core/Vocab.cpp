#include "Vocab.h"

#include "Text.h"

namespace kaz {

void Vocab::index() {
    ids_.clear();
    tagCount_.assign(vocab.size(), 1);
    for (size_t i = 0; i < vocab.size(); i++) {
        if (vocab[i].size() == 1) ids_[vocab[i][0]] = int32_t(i);
    }
    toRussian.clear();
    for (const auto& [src, vs] : rules) {
        auto it = ids_.find(src);
        if (it != ids_.end()) tagCount_[it->second] = int(vs.size());
        for (char32_t v : vs) {
            if (v != src) toRussian[v] = src;
        }
    }
}

std::u32string Vocab::russianize(const std::u32string& text) const {
    std::u32string out;
    out.reserve(text.size());
    for (char32_t c : text) {
        c = toLower(c);
        auto it = toRussian.find(c);
        out.push_back(it == toRussian.end() ? c : it->second);
    }
    return out;
}

std::vector<int32_t> Vocab::encode(const std::u32string& text) const {
    std::vector<int32_t> out;
    out.reserve(text.size());
    for (char32_t c : text) {
        auto it = ids_.find(c);
        if (it != ids_.end()) {
            out.push_back(it->second);
        } else if (isDigit(c)) {
            out.push_back(digit);
        } else if (isAsciiLetter(c)) {
            out.push_back(latin);
        } else if (isSpace(c)) {
            auto sp = ids_.find(U' ');
            out.push_back(sp == ids_.end() ? unk : sp->second);
        } else {
            out.push_back(unk);
        }
    }
    return out;
}

std::vector<char32_t> Vocab::variants(char32_t c) const {
    auto it = rules.find(c);
    return it == rules.end() ? std::vector<char32_t>{c} : it->second;
}

}  // namespace kaz
