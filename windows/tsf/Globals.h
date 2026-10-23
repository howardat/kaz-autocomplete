// Process-wide state shared by every text service instance (one per GUI thread).
#pragma once

#include <windows.h>

#include <memory>
#include <mutex>
#include <string>

#include "../core/Converter.h"

namespace kaz {

extern HINSTANCE g_module;
/// Live COM objects plus server locks; the DLL may unload at zero.
extern LONG g_objects;

extern const CLSID CLSID_KazType;
extern const GUID GUID_KazTypeProfile;
extern const GUID GUID_KazTypeDisplayAttribute;

/// Kazakh (Kazakhstan). The profile lives under this language.
constexpr LANGID kLangId = 0x043F;
/// Russian layout, used for keys the input method does not handle.
const HKL kRussianLayout = reinterpret_cast<HKL>(static_cast<ULONG_PTR>(0x04190419));
constexpr int kMaxCandidates = 9;

/// Directory holding KazType.dll and kaztype.bin, with a trailing backslash.
std::wstring moduleDir();

/// The converter, loaded on first use. Null if the model failed to load, in
/// which case letters pass through unconverted. Lock the returned guard's
/// mutex for every call: Converter is not thread-safe.
struct ConverterRef {
    std::unique_lock<std::mutex> lock;
    Converter* converter;
};
ConverterRef sharedConverter();

}  // namespace kaz
