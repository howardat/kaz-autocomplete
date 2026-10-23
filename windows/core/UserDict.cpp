#include "UserDict.h"

#include <fstream>
#include <system_error>

#include "Text.h"

namespace fs = std::filesystem;

namespace kaz {

UserDict::UserDict(fs::path path) : path_(std::move(path)) { reloadIfChanged(); }

void UserDict::reloadIfChanged() {
    if (path_.empty()) return;
    std::error_code ec;
    auto mtime = fs::last_write_time(path_, ec);
    if (ec || mtime == loaded_) return;
    std::ifstream f(path_, std::ios::binary);
    if (!f) return;
    entries_.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto tab = line.find('\t');
        if (tab == std::string::npos || tab == 0 || tab + 1 == line.size()) continue;
        entries_[fromUtf8(line.substr(0, tab))] = fromUtf8(line.substr(tab + 1));
    }
    loaded_ = mtime;
}

std::optional<std::u32string> UserDict::lookup(const std::u32string& key) {
    reloadIfChanged();
    auto it = entries_.find(key);
    if (it == entries_.end()) return std::nullopt;
    return it->second;
}

void UserDict::set(const std::u32string& key, const std::optional<std::u32string>& word) {
    reloadIfChanged();
    auto it = entries_.find(key);
    if (!word) {
        if (it == entries_.end()) return;
        entries_.erase(it);
    } else {
        if (it != entries_.end() && it->second == *word) return;
        entries_[key] = *word;
    }
    save();
}

void UserDict::save() {
    if (path_.empty()) return;
    std::error_code ec;
    fs::create_directories(path_.parent_path(), ec);
    fs::path tmp = path_;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return;
        for (const auto& [k, v] : entries_) f << toUtf8(k) << '\t' << toUtf8(v) << '\n';
        if (!f) return;
    }
    fs::rename(tmp, path_, ec);  // replaces atomically on NTFS and POSIX
    if (!ec) loaded_ = fs::last_write_time(path_, ec);
}

}  // namespace kaz
