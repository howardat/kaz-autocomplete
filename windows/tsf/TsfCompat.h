// TSF declarations that MinGW's msctf.h lacks. The Windows SDK (MSVC) has all
// of them, so this only fills gaps for the MinGW cross-build.
#pragma once

#include <windows.h>
#include <msctf.h>

#ifdef __MINGW32__

#ifndef TF_CLIENTID_NULL
#define TF_CLIENTID_NULL 0
#endif
#ifndef TF_INVALID_GUIDATOM
#define TF_INVALID_GUIDATOM 0
#endif

static const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT = {
    0x13a016df, 0x560b, 0x46cd, {0x94, 0x7a, 0x4c, 0x3a, 0xf1, 0xe0, 0xe3, 0x5d}};
static const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT = {
    0x25504fb4, 0x7bab, 0x4bc1, {0x9c, 0x69, 0xcf, 0x81, 0x89, 0x0f, 0x0e, 0xf5}};

static const IID IID_ITfTextInputProcessorEx = {
    0x6e4e2102, 0xf9cd, 0x433d, {0xb4, 0x96, 0x30, 0x3c, 0xe0, 0x3a, 0x65, 0x07}};
struct ITfTextInputProcessorEx : public ITfTextInputProcessor {
    virtual HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD dwFlags) = 0;
};

static const IID IID_ITfDisplayAttributeProvider = {
    0xfee47777, 0x163c, 0x4769, {0x99, 0x6a, 0x6e, 0x9c, 0x50, 0xad, 0x8f, 0x54}};
struct ITfDisplayAttributeProvider : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** ppInfo) = 0;
};

#endif  // __MINGW32__
