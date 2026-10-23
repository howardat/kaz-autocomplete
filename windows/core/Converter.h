// Turns a word typed with Russian letters into ranked Kazakh candidates.
// Port of macos/Sources/KazCore/Converter.swift; keep the two in step.
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Tagger.h"
#include "UserDict.h"
#include "Vocab.h"

namespace kaz {

class Converter {
public:
    struct Result {
        /// Best first. Never empty; the typed word is the last resort.
        std::vector<std::u32string> candidates;
        /// What the model alone would pick, before the user dictionary.
        std::u32string modelTop;
    };

    Converter(Vocab vocab, std::unique_ptr<TagModel> model, std::unique_ptr<UserDict> userDict);

    /// Loads kaztype.bin. Throws std::runtime_error.
    static std::unique_ptr<Converter> load(const std::filesystem::path& modelFile, std::unique_ptr<UserDict> userDict);

    /// typed: the word as typed, any case. context: text before it, any script.
    Result convert(const std::u32string& typed, const std::u32string& context = {}, int limit = 5);

    /// Remember that `typed` should become `chosen` when it differs from the model.
    void learn(const std::u32string& typed, const std::u32string& chosen, const std::u32string& modelTop);

    const Vocab& vocab() const { return vocab_; }
    UserDict& userDict() { return *userDict_; }

private:
    std::vector<std::u32string> beam(const std::u32string& word, const std::vector<std::vector<float>>& probs, int width) const;

    Vocab vocab_;
    std::unique_ptr<TagModel> model_;
    std::unique_ptr<UserDict> userDict_;
};

/// Copy per-char case from what was typed.
std::u32string matchCase(const std::u32string& word, const std::u32string& typed);

}  // namespace kaz
