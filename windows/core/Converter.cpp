#include "Converter.h"

#include <algorithm>
#include <cmath>

#include "Text.h"

namespace kaz {

Converter::Converter(Vocab vocab, std::unique_ptr<TagModel> model, std::unique_ptr<UserDict> userDict)
    : vocab_(std::move(vocab)), model_(std::move(model)), userDict_(std::move(userDict)) {}

std::unique_ptr<Converter> Converter::load(const std::filesystem::path& modelFile, std::unique_ptr<UserDict> userDict) {
    Vocab vocab;
    auto tagger = LstmTagger::load(modelFile, vocab);
    return std::make_unique<Converter>(std::move(vocab), std::move(tagger), std::move(userDict));
}

Converter::Result Converter::convert(const std::u32string& typed, const std::u32string& context, int limit) {
    std::u32string word = vocab_.russianize(typed);
    if (word.empty()) return {{typed}, typed};

    auto ids = vocab_.encode(vocab_.russianize(context));
    auto wordIds = vocab_.encode(word);
    ids.insert(ids.end(), wordIds.begin(), wordIds.end());
    if (int(ids.size()) > vocab_.maxlen) ids.erase(ids.begin(), ids.end() - vocab_.maxlen);
    int rows = std::min(int(word.size()), vocab_.maxlen);
    std::vector<int32_t> padded(vocab_.maxlen - ids.size(), vocab_.pad);
    padded.insert(padded.end(), ids.begin(), ids.end());

    std::vector<std::u32string> ranked;
    std::vector<std::vector<float>> probs;
    if (model_ && model_->probabilities(padded, rows, probs)) {
        size_t offset = word.size() - rows;  // only if the word is longer than maxlen
        for (auto& s : beam(word.substr(offset), probs, limit)) ranked.push_back(word.substr(0, offset) + s);
    } else {
        ranked = {word};
    }
    std::u32string modelTop = ranked[0];

    if (auto mine = userDict_->lookup(word)) {
        ranked.erase(std::remove(ranked.begin(), ranked.end(), *mine), ranked.end());
        ranked.insert(ranked.begin(), *mine);
    }
    if (int(ranked.size()) > limit) ranked.resize(limit);
    if (std::find(ranked.begin(), ranked.end(), word) == ranked.end()) {
        // Keep the raw word reachable, in place of the weakest guess.
        if (int(ranked.size()) >= limit) ranked.pop_back();
        ranked.push_back(word);
    }
    for (auto& s : ranked) s = matchCase(s, typed);
    return {ranked, matchCase(modelTop, typed)};
}

void Converter::learn(const std::u32string& typed, const std::u32string& chosen, const std::u32string& modelTop) {
    std::u32string c = lower(chosen);
    if (c == lower(modelTop)) {
        userDict_->set(vocab_.russianize(typed), std::nullopt);
    } else {
        userDict_->set(vocab_.russianize(typed), c);
    }
}

std::vector<std::u32string> Converter::beam(const std::u32string& word, const std::vector<std::vector<float>>& probs,
                                            int width) const {
    std::vector<std::pair<std::u32string, float>> beams{{U"", 0.0f}};
    for (size_t i = 0; i < word.size() && i < probs.size(); i++) {
        auto vs = vocab_.variants(word[i]);
        if (vs.size() == 1) {
            for (auto& b : beams) b.first += vs[0];
            continue;
        }
        std::vector<std::pair<std::u32string, float>> next;
        for (const auto& b : beams)
            for (size_t j = 0; j < vs.size(); j++)
                next.push_back({b.first + vs[j], b.second + std::log(std::max(probs[i][j], 1e-9f))});
        std::stable_sort(next.begin(), next.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        if (int(next.size()) > width) next.resize(width);
        beams = std::move(next);
    }
    std::vector<std::u32string> out;
    for (auto& b : beams) out.push_back(std::move(b.first));
    return out;
}

std::u32string matchCase(const std::u32string& word, const std::u32string& typed) {
    std::u32string out = word;
    for (size_t i = 0; i < out.size() && i < typed.size(); i++)
        if (isUpper(typed[i])) out[i] = toUpper(out[i]);
    return out;
}

}  // namespace kaz
