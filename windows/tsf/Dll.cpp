// COM server entry points and registration (regsvr32 KazType.dll).

#include <windows.h>
#include "TsfCompat.h"

#include <new>
#include <string>

#include "Globals.h"
#include "TextService.h"

using namespace kaz;

namespace {

const wchar_t kDescription[] = L"KazType";

// Categories: keyboard TIP, Store/immersive apps, the systray language
// switcher, and our composition underline.
const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
    &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
};

class ClassFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    // Static object: lifetime is the DLL's.
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;
        auto* service = new (std::nothrow) TextService();
        if (!service) return E_OUTOFMEMORY;
        HRESULT hr = service->QueryInterface(riid, ppv);
        service->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override {
        lock ? InterlockedIncrement(&g_objects) : InterlockedDecrement(&g_objects);
        return S_OK;
    }
};

ClassFactory g_factory;

std::wstring guidString(REFGUID guid) {
    wchar_t buf[64];
    StringFromGUID2(guid, buf, 64);
    return buf;
}

std::wstring modulePath() {
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(g_module, path, MAX_PATH);
    return std::wstring(path, n);
}

bool setString(HKEY key, const wchar_t* name, const std::wstring& value) {
    return RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
                          DWORD((value.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

std::wstring clsidKey() { return L"CLSID\\" + guidString(CLSID_KazType); }

bool registerServer() {
    HKEY key = nullptr, inproc = nullptr;
    bool ok = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsidKey().c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                              nullptr) == ERROR_SUCCESS &&
              setString(key, nullptr, kDescription) &&
              RegCreateKeyExW(key, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr, &inproc, nullptr) ==
                  ERROR_SUCCESS &&
              setString(inproc, nullptr, modulePath()) && setString(inproc, L"ThreadingModel", L"Apartment");
    if (inproc) RegCloseKey(inproc);
    if (key) RegCloseKey(key);
    return ok;
}

void unregisterServer() { RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey().c_str()); }

bool registerProfile() {
    ITfInputProcessorProfileMgr* mgr = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&mgr))))
        return false;
    std::wstring icon = modulePath();
    // The Russian substitute layout makes keys we pass through (digits,
    // punctuation, space) type what they would on a Russian keyboard.
    HRESULT hr = mgr->RegisterProfile(CLSID_KazType, kLangId, GUID_KazTypeProfile, kDescription,
                                      ULONG(wcslen(kDescription)), icon.c_str(), ULONG(icon.size()), 0,
                                      kRussianLayout, 0, TRUE, 0);
    mgr->Release();
    return SUCCEEDED(hr);
}

void unregisterProfile() {
    ITfInputProcessorProfileMgr* mgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&mgr)))) {
        mgr->UnregisterProfile(CLSID_KazType, kLangId, GUID_KazTypeProfile, 0);
        mgr->Release();
    }
}

bool registerCategories(bool add) {
    ITfCategoryMgr* mgr = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                reinterpret_cast<void**>(&mgr))))
        return false;
    bool ok = true;
    for (const GUID* cat : kCategories) {
        HRESULT hr = add ? mgr->RegisterCategory(CLSID_KazType, *cat, CLSID_KazType)
                         : mgr->UnregisterCategory(CLSID_KazType, *cat, CLSID_KazType);
        ok = ok && SUCCEEDED(hr);
    }
    mgr->Release();
    return ok;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (clsid != CLSID_KazType) return CLASS_E_CLASSNOTAVAILABLE;
    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return g_objects == 0 ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
    if (!registerServer() || !registerProfile() || !registerCategories(true)) {
        registerCategories(false);
        unregisterProfile();
        unregisterServer();
        return E_FAIL;
    }
    return S_OK;
}

STDAPI DllUnregisterServer() {
    registerCategories(false);
    unregisterProfile();
    unregisterServer();
    return S_OK;
}
