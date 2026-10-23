// WebAssembly entry points for the web demo. Runs windows/core unchanged, so
// the browser gives exactly what the Windows input method gives.
#include <emscripten/emscripten.h>

#include <memory>
#include <string>

#include "Converter.h"
#include "Text.h"

static std::unique_ptr<kaz::Converter> g_converter;
static std::string g_out;

extern "C" {

/// Loads the model from the in-memory file system. Returns 1 on success.
EMSCRIPTEN_KEEPALIVE int kaz_load(const char* path) {
    try {
        g_converter = kaz::Converter::load(path, std::make_unique<kaz::UserDict>());
        return 1;
    } catch (...) {
        return 0;
    }
}

/// Candidates for one word, best first, one per line; the last line is the
/// model's own top pick (before the user's choices).
EMSCRIPTEN_KEEPALIVE const char* kaz_convert(const char* typed, const char* context, int limit) {
    g_out.clear();
    if (!g_converter) return g_out.c_str();
    auto r = g_converter->convert(kaz::fromUtf8(typed), kaz::fromUtf8(context), limit);
    for (const auto& c : r.candidates) g_out += kaz::toUtf8(c) + "\n";
    g_out += kaz::toUtf8(r.modelTop);
    return g_out.c_str();
}

/// Remember a pick for this session, as the input methods do.
EMSCRIPTEN_KEEPALIVE void kaz_learn(const char* typed, const char* chosen, const char* modelTop) {
    if (g_converter) g_converter->learn(kaz::fromUtf8(typed), kaz::fromUtf8(chosen), kaz::fromUtf8(modelTop));
}
}
