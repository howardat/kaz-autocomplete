// Words the user picked over the model's top guess. Keyed by the russianized,
// lowercased word; the value is the lowercased Kazakh word to put first.
//
// Stored as UTF-8 "key<TAB>word" lines. The input method runs inside every app,
// so each process reloads the file when another one has changed it.
#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace kaz {

class UserDict {
public:
    /// Empty path keeps the dictionary in memory only (tests).
    explicit UserDict(std::filesystem::path path = {});

    std::optional<std::u32string> lookup(const std::u32string& key);
    /// nullopt removes the entry.
    void set(const std::u32string& key, const std::optional<std::u32string>& word);
    const std::map<std::u32string, std::u32string>& entries() const { return entries_; }

private:
    void reloadIfChanged();
    void save();

    std::filesystem::path path_;
    std::filesystem::file_time_type loaded_{};
    std::map<std::u32string, std::u32string> entries_;
};

}  // namespace kaz
