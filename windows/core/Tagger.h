// The char BiLSTM from ml/model.py, run on the CPU with int8 weights.
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "Vocab.h"

namespace kaz {

/// Per-char tag probabilities for a right-aligned window of ids.
class TagModel {
public:
    virtual ~TagModel() = default;
    /// ids.size() == maxlen. Fills `last` rows of numTags probabilities for the
    /// last `last` positions. Returns false on failure.
    virtual bool probabilities(const std::vector<int32_t>& ids, int last, std::vector<std::vector<float>>& out) = 0;
};

class LstmTagger : public TagModel {
public:
    /// Loads kaztype.bin into the tagger and fills `vocab`. Throws std::runtime_error.
    static std::unique_ptr<LstmTagger> load(const std::filesystem::path& path, Vocab& vocab);

    bool probabilities(const std::vector<int32_t>& ids, int last, std::vector<std::vector<float>>& out) override;

    struct Q8 {
        int rows = 0, cols = 0;
        std::vector<float> scale;
        std::vector<int8_t> q;
    };
    struct Dir {
        std::vector<float> table;  // layer 0: [vocab, 4H] input projection incl. bias
        Q8 wih;                    // layer > 0: [4H, 2H]
        std::vector<float> bias;   // layer > 0: [4H]
        Q8 whh;                    // [4H, H]
    };

private:
    int maxlen_ = 0, numTags_ = 0, hidden_ = 0;
    std::vector<int> tagCount_;
    std::vector<std::vector<Dir>> layers_;  // [layer][fwd, bwd]
    std::vector<float> outW_, outB_;
    // Scratch, reused across calls.
    std::vector<float> pre_, out_, gates_, c_;
    std::vector<int8_t> inQ_, outQ_, hQ_;
};

}  // namespace kaz
