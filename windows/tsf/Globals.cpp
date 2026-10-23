#include "Globals.h"

#include <shlobj.h>

#include <filesystem>

namespace kaz {

HINSTANCE g_module = nullptr;
LONG g_objects = 0;

// {D47F4271-B272-4D65-A2EB-E6D7D4CE8652}
const CLSID CLSID_KazType = {0xd47f4271, 0xb272, 0x4d65, {0xa2, 0xeb, 0xe6, 0xd7, 0xd4, 0xce, 0x86, 0x52}};
// {CE05D587-91CD-44C2-BE3A-BCCAD5A33BEE}
const GUID GUID_KazTypeProfile = {0xce05d587, 0x91cd, 0x44c2, {0xbe, 0x3a, 0xbc, 0xca, 0xd5, 0xa3, 0x3b, 0xee}};
// {BFF00223-9F48-46E1-89F1-C84A32320FD4}
const GUID GUID_KazTypeDisplayAttribute = {0xbff00223, 0x9f48, 0x46e1, {0x89, 0xf1, 0xc8, 0x4a, 0x32, 0x32, 0x0f, 0xd4}};

std::wstring moduleDir() {
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(g_module, path, MAX_PATH);
    std::wstring dir(path, n);
    return dir.substr(0, dir.find_last_of(L'\\') + 1);
}

static std::filesystem::path userDictPath() {
    PWSTR appData = nullptr;
    std::filesystem::path p;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData))) {
        p = std::filesystem::path(appData) / L"KazType" / L"user_dict.tsv";
    }
    CoTaskMemFree(appData);
    return p;
}

ConverterRef sharedConverter() {
    static std::mutex mutex;
    static std::unique_ptr<Converter> converter;
    static bool tried = false;
    std::unique_lock<std::mutex> lock(mutex);
    if (!tried) {
        tried = true;
        try {
            converter = Converter::load(std::filesystem::path(moduleDir()) / L"kaztype.bin",
                                        std::make_unique<UserDict>(userDictPath()));
        } catch (const std::exception& e) {
            OutputDebugStringA(("KazType: failed to load model: " + std::string(e.what()) + "\n").c_str());
        }
    }
    return {std::move(lock), converter.get()};
}

}  // namespace kaz
