// Mirror of ml/rules.py, loaded from kaztype.bin written by ml/export_win.py.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace kaz {

struct Vocab {
    int maxlen = 0;
    int numTags = 0;
    std::vector<std::u32string> vocab;
    int32_t pad = 0, unk = 0, digit = 0, latin = 0;
    /// Russian-keyboard letter -> Kazakh variants; index is the tag.
    std::map<char32_t, std::vector<char32_t>> rules;
    /// Kazakh-only letter -> Russian-keyboard letter (қ -> к, ...).
    std::map<char32_t, char32_t> toRussian;

    /// Call after filling vocab and rules.
    void index();

    /// Lowercase and map Kazakh letters back to what a Russian keyboard types.
    std::u32string russianize(const std::u32string& text) const;
    /// Ids for already-russianized text. Same rules as ml.rules.char_id.
    std::vector<int32_t> encode(const std::u32string& text) const;
    /// Variants for a russianized letter; just the letter if it never changes.
    std::vector<char32_t> variants(char32_t c) const;
    /// Number of valid tags per id.
    const std::vector<int>& tagCounts() const { return tagCount_; }

private:
    std::unordered_map<char32_t, int32_t> ids_;
    std::vector<int> tagCount_;
};

}  // namespace kaz
