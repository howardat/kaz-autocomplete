// The TSF text service: one instance per GUI thread that uses KazType.
// Buffers the word being typed, shows it converted as a composition, and
// commits on space, punctuation or a pick. Port of macos/.../InputController.swift.
#pragma once

#include <windows.h>
#include "TsfCompat.h"

#include <functional>
#include <string>

#include "../core/Converter.h"
#include "CandidateWindow.h"

namespace kaz {

class TextService final : public ITfTextInputProcessorEx,
                    public ITfThreadMgrEventSink,
                    public ITfKeyEventSink,
                    public ITfCompositionSink,
                    public ITfDisplayAttributeProvider {
public:
    TextService();
    ~TextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor(Ex)
    STDMETHODIMP Activate(ITfThreadMgr* tm, TfClientId id) override { return ActivateEx(tm, id, 0); }
    STDMETHODIMP ActivateEx(ITfThreadMgr* tm, TfClientId id, DWORD flags) override;
    STDMETHODIMP Deactivate() override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* previous) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL) override { return S_OK; }
    STDMETHODIMP OnTestKeyDown(ITfContext* ctx, WPARAM vk, LPARAM lp, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* ctx, WPARAM vk, LPARAM lp, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ec, ITfComposition* composition) override;

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** out) override;
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** out) override;

private:
    enum class Action { None, Letter, Backspace, Escape, Commit, Move, Pick, CommitAndPass };
    Action classify(WPARAM vk, LPARAM lp, char32_t& letter, int& arg) const;

    HRESULT edit(ITfContext* ctx, DWORD flags, std::function<HRESULT(TfEditCookie)> fn);
    ITfContext* compositionContext() const;

    void startComposition(TfEditCookie ec, ITfContext* ctx);
    void update(TfEditCookie ec, ITfContext* ctx);
    void setText(TfEditCookie ec, ITfContext* ctx, const std::u32string& text, bool marked);
    void finish(TfEditCookie ec, ITfContext* ctx, const std::u32string& text);
    void commit(TfEditCookie ec, ITfContext* ctx);
    void showCandidates(TfEditCookie ec, ITfContext* ctx);
    std::u32string readContext(TfEditCookie ec, ITfContext* ctx);
    void reset();
    /// Candidate window clicks and hovers arrive outside an edit session.
    void selectAsync(int index, bool commitNow);

    LONG ref_ = 1;
    ITfThreadMgr* threadMgr_ = nullptr;
    TfClientId clientId_ = TF_CLIENTID_NULL;
    DWORD threadMgrCookie_ = TF_INVALID_COOKIE;
    TfGuidAtom attrAtom_ = TF_INVALID_GUIDATOM;
    ITfComposition* composition_ = nullptr;
    CandidateWindow candidates_;

    std::u32string typed_, context_;
    Converter::Result result_;
    int selected_ = 0;
};

}  // namespace kaz
