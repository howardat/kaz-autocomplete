#include "Tagger.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "Text.h"

namespace kaz {
namespace {

class Reader {
public:
    explicit Reader(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("cannot open " + path.string());
        data_.assign(std::istreambuf_iterator<char>(f), {});
    }
    void bytes(void* dst, size_t n) {
        if (pos_ + n > data_.size()) throw std::runtime_error("kaztype.bin is truncated");
        std::memcpy(dst, data_.data() + pos_, n);
        pos_ += n;
    }
    uint32_t u32() {
        unsigned char b[4];
        bytes(b, 4);
        return b[0] | (b[1] << 8) | (b[2] << 16) | (uint32_t(b[3]) << 24);
    }
    std::u32string string() {
        std::string s(u32(), '\0');
        bytes(s.data(), s.size());
        return fromUtf8(s);
    }
    char32_t letter() {
        auto s = string();
        if (s.size() != 1) throw std::runtime_error("rule entries must be single letters");
        return s[0];
    }
    // Assumes a little-endian host, like every Windows target.
    std::vector<float> f32(size_t rows, size_t cols) {
        expectShape(rows, cols);
        std::vector<float> v(rows * cols);
        bytes(v.data(), v.size() * 4);
        return v;
    }
    LstmTagger::Q8 q8(size_t rows, size_t cols) {
        expectShape(rows, cols);
        LstmTagger::Q8 w;
        w.rows = int(rows);
        w.cols = int(cols);
        w.scale.resize(rows);
        w.q.resize(rows * cols);
        bytes(w.scale.data(), rows * 4);
        bytes(w.q.data(), rows * cols);
        return w;
    }
    bool atEnd() const { return pos_ == data_.size(); }

private:
    void expectShape(size_t rows, size_t cols) {
        if (u32() != rows || u32() != cols) throw std::runtime_error("kaztype.bin has an unexpected tensor shape");
    }
    std::vector<char> data_;
    size_t pos_ = 0;
};

inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

// Dot product with independent accumulators, so the compiler can vectorize it.
inline float dot(const float* a, const float* b, int n) {
    float acc[8] = {};
    int i = 0;
    for (; i + 8 <= n; i += 8)
        for (int k = 0; k < 8; k++) acc[k] += a[i + k] * b[i + k];
    float s = 0;
    for (; i < n; i++) s += a[i] * b[i];
    for (float v : acc) s += v;
    return s;
}

// Integer dot product: exact, and compilers vectorize it without fast-math.
inline int32_t dot8(const int8_t* a, const int8_t* b, int n) {
    int32_t s = 0;
    for (int i = 0; i < n; i++) s += int16_t(a[i]) * int16_t(b[i]);
    return s;
}

// Activations are in (-1, 1); stored as int8 in steps of 1/127.
inline int8_t quantAct(float x) { return int8_t(std::lround(std::clamp(x, -1.0f, 1.0f) * 127.0f)); }

}  // namespace

std::unique_ptr<LstmTagger> LstmTagger::load(const std::filesystem::path& path, Vocab& vocab) {
    Reader r(path);
    char magic[4];
    r.bytes(magic, 4);
    if (std::memcmp(magic, "KZT1", 4) != 0) throw std::runtime_error("not a kaztype.bin file");

    auto t = std::make_unique<LstmTagger>();
    t->maxlen_ = vocab.maxlen = int(r.u32());
    t->numTags_ = vocab.numTags = int(r.u32());
    int H = t->hidden_ = int(r.u32());
    int layers = int(r.u32());
    if (t->numTags_ < 1 || t->numTags_ > 16 || H < 1 || layers < 1) throw std::runtime_error("kaztype.bin has a bad header");

    vocab.vocab.resize(r.u32());
    for (auto& s : vocab.vocab) s = r.string();
    vocab.pad = int32_t(r.u32());
    vocab.unk = int32_t(r.u32());
    vocab.digit = int32_t(r.u32());
    vocab.latin = int32_t(r.u32());
    vocab.rules.clear();
    for (uint32_t n = r.u32(); n > 0; n--) {
        char32_t src = r.letter();
        auto& vs = vocab.rules[src];
        for (uint32_t k = r.u32(); k > 0; k--) vs.push_back(r.letter());
    }
    vocab.index();
    t->tagCount_ = vocab.tagCounts();

    size_t V = vocab.vocab.size(), G = 4 * size_t(H);
    t->layers_.resize(layers);
    for (int l = 0; l < layers; l++) {
        t->layers_[l].resize(2);
        for (auto& d : t->layers_[l]) {
            if (l == 0) {
                d.table = r.f32(V, G);
            } else {
                d.wih = r.q8(G, 2 * H);
                d.bias = r.f32(1, G);
            }
            d.whh = r.q8(G, H);
        }
    }
    t->outW_ = r.f32(t->numTags_, 2 * H);
    t->outB_ = r.f32(1, t->numTags_);
    if (!r.atEnd()) throw std::runtime_error("kaztype.bin has trailing data");
    return t;
}

bool LstmTagger::probabilities(const std::vector<int32_t>& ids, int last, std::vector<std::vector<float>>& out) {
    const int T = maxlen_, H = hidden_, G = 4 * H, L = int(layers_.size());
    if (int(ids.size()) != T || last < 0 || last > T) return false;
    for (int32_t id : ids)
        if (id < 0 || size_t(id) >= tagCount_.size()) return false;

    pre_.resize(size_t(T) * G);
    inQ_.resize(size_t(T) * 2 * H);  // previous layer output: [T, fwd H | bwd H]
    outQ_.resize(size_t(T) * 2 * H);
    out_.resize(size_t(T) * 2 * H);
    gates_.resize(G);
    hQ_.resize(H);
    c_.resize(H);
    constexpr float act = 1.0f / 127;

    for (int l = 0; l < L; l++) {
        if (l > 0) std::swap(inQ_, outQ_);
        for (int dir = 0; dir < 2; dir++) {
            const Dir& d = layers_[l][dir];
            // The last layer's backward pass only feeds the positions we output.
            int lo = (l == L - 1 && dir == 1) ? T - last : 0;

            // Input projections for every needed step at once.
            if (l == 0) {
                for (int t = lo; t < T; t++)
                    std::memcpy(&pre_[size_t(t) * G], &d.table[size_t(ids[t]) * G], G * sizeof(float));
            } else {
                for (int g = 0; g < G; g++) {
                    const int8_t* w = d.wih.q.data() + size_t(g) * 2 * H;
                    float scale = d.wih.scale[g] * act;
                    for (int t = lo; t < T; t++)
                        pre_[size_t(t) * G + g] = d.bias[g] + scale * dot8(w, &inQ_[size_t(t) * 2 * H], 2 * H);
                }
            }

            std::fill(hQ_.begin(), hQ_.end(), int8_t(0));
            std::fill(c_.begin(), c_.end(), 0.0f);
            for (int step = 0; step < T - lo; step++) {
                int t = dir == 0 ? step : T - 1 - step;
                const float* p = &pre_[size_t(t) * G];
                for (int g = 0; g < G; g++)
                    gates_[g] = p[g] + d.whh.scale[g] * act * dot8(d.whh.q.data() + size_t(g) * H, hQ_.data(), H);
                size_t o = size_t(t) * 2 * H + size_t(dir) * H;
                for (int k = 0; k < H; k++) {
                    float i = sigmoid(gates_[k]), f = sigmoid(gates_[H + k]);
                    float gg = std::tanh(gates_[2 * H + k]), og = sigmoid(gates_[3 * H + k]);
                    c_[k] = f * c_[k] + i * gg;
                    out_[o + k] = og * std::tanh(c_[k]);
                    hQ_[k] = outQ_[o + k] = quantAct(out_[o + k]);
                }
            }
        }
    }

    out.assign(last, std::vector<float>(numTags_, 0.0f));
    for (int r = 0; r < last; r++) {
        int t = T - last + r;
        int n = std::min(tagCount_[ids[t]], numTags_);
        float logits[16], mx = -1e30f, sum = 0;
        for (int k = 0; k < n; k++) {
            logits[k] = outB_[k] + dot(&outW_[size_t(k) * 2 * H], &out_[size_t(t) * 2 * H], 2 * H);
            mx = std::max(mx, logits[k]);
        }
        for (int k = 0; k < n; k++) sum += out[r][k] = std::exp(logits[k] - mx);
        for (int k = 0; k < n; k++) out[r][k] /= sum;
    }
    return true;
}

}  // namespace kaz
