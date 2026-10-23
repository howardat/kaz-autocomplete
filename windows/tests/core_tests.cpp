// Unit tests for the portable core. Mirrors macos/Tests/KazCoreTests.
//   core_tests [path/to/kaztype.bin]
// Real-model tests run only when kaztype.bin exists (uv run python -m ml.export_win).

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>

#include "../core/Converter.h"
#include "../core/Text.h"

using namespace kaz;
namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                \
        }                                                              \
    } while (0)

static std::u32string u(const char* s) { return fromUtf8(s); }
static bool contains(const std::vector<std::u32string>& v, const std::u32string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

static Vocab fakeVocab() {
    Vocab v;
    v.maxlen = 8;
    v.numTags = 3;
    for (const char* s : {"<pad>", "<unk>", "<digit>", "<latin>", "а", "к", "з", "с", "т", "н", " "})
        v.vocab.push_back(u(s));
    v.pad = 0, v.unk = 1, v.digit = 2, v.latin = 3;
    v.rules = {{U'к', {U'к', U'қ'}}, {U'а', {U'а', U'ә'}}, {U'н', {U'н', U'ң'}}};
    v.index();
    return v;
}

/// Always prefers the second variant for к, the first for everything else.
struct FakeModel : TagModel {
    bool probabilities(const std::vector<int32_t>& ids, int last, std::vector<std::vector<float>>& out) override {
        out.clear();
        for (size_t i = ids.size() - last; i < ids.size(); i++)
            out.push_back(ids[i] == 5 ? std::vector<float>{0.2f, 0.8f, 0} : std::vector<float>{0.9f, 0.1f, 0});
        return true;
    }
};

static Converter fakeConverter() {
    return Converter(fakeVocab(), std::make_unique<FakeModel>(), std::make_unique<UserDict>());
}

static void fakeTests() {
    auto c = fakeConverter();
    CHECK(c.vocab().russianize(u("ҚАЗАҚ ӘН")) == u("казак ан"));
    CHECK((c.vocab().encode(u("к7z!")) == std::vector<int32_t>{5, 2, 3, 1}));

    auto r = c.convert(u("казак"));
    CHECK(r.candidates.front() == u("қазақ"));
    CHECK(r.modelTop == u("қазақ"));
    CHECK(contains(r.candidates, u("казак")));  // typed word always offered

    CHECK(c.convert(u("Казак")).candidates.front() == u("Қазақ"));

    // User choice wins and is forgotten.
    c.learn(u("кан"), u("кан"), u("қан"));
    CHECK(c.convert(u("кан")).candidates.front() == u("кан"));
    CHECK(c.convert(u("Кан")).candidates.front() == u("Кан"));
    c.learn(u("кан"), u("қан"), u("қан"));
    CHECK(!c.userDict().lookup(u("кан")));

    // User dictionary round trip.
    fs::path dir = fs::temp_directory_path() / ("kaztype-test-" + std::to_string(std::rand()));
    UserDict(dir / "d.tsv").set(u("кол"), u("қол"));
    CHECK(UserDict(dir / "d.tsv").lookup(u("кол")) == u("қол"));
    // Another process writing the file is picked up.
    UserDict a(dir / "d.tsv"), b(dir / "d.tsv");
    a.set(u("кун"), u("күн"));
    CHECK(b.lookup(u("кун")) == u("күн"));
    fs::remove_all(dir);

    // Text helpers.
    CHECK(toLower(U'Ә') == U'ә' && toUpper(U'ү') == U'Ү' && toLower(U'І') == U'і' && toUpper(U'һ') == U'Һ');
    CHECK(isUpper(U'Қ') && !isUpper(U'қ') && !isUpper(U'7'));
    CHECK(toUtf16(u("қ😀")) == std::u16string(u"қ😀") && fromUtf16(u"қ😀") == u("қ😀"));
}

static void realModelTests(const fs::path& bin) {
    auto c = Converter::load(bin, std::make_unique<UserDict>());
    const char* cases[][2] = {
        {"казакстан", "қазақстан"}, {"кулки", "күлкі"}, {"рахмет", "рахмет"},
        {"бугин", "бүгін"}, {"менин", "менің"},
    };
    for (auto& [typed, want] : cases) {
        auto got = c->convert(u(typed)).candidates.front();
        if (got != u(want)) std::printf("  %s -> %s, want %s\n", typed, toUtf8(got).c_str(), want);
        CHECK(got == u(want));
    }
    // "қол" (hand) vs "көл" (lake): both should at least be offered.
    auto r = c->convert(u("кол"), u("сол "));
    CHECK(contains(r.candidates, u("қол")) && contains(r.candidates, u("көл")));
    // Long words and context longer than the window still work.
    std::u32string longWord(80, U'к');
    CHECK(c->convert(longWord).candidates.front().size() == 80);
    CHECK(!c->convert(u("кол"), std::u32string(200, U'а')).candidates.empty());

    c->convert(u("казакстан"));
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 50; i++) c->convert(u("казакстан"), u("біз тұратын ел "));
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 50;
    std::printf("  latency %.2f ms per conversion\n", ms);
    // Shared CI runners are too slow and noisy for the target; they only report it.
    if (!std::getenv("CI")) CHECK(ms < 10);
}

int main(int argc, char** argv) {
    fakeTests();
    fs::path bin = argc > 1 ? fs::path(argv[1]) : fs::path(__FILE__).parent_path().parent_path() / "Resources" / "kaztype.bin";
    if (fs::exists(bin)) {
        realModelTests(bin);
    } else {
        std::printf("skipping real-model tests: run `uv run python -m ml.export_win` first\n");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
