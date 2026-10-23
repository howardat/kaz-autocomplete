#include "TextService.h"

#include <new>

#include "../core/Text.h"
#include "Globals.h"
#include "Keys.h"

namespace kaz {
namespace {

template <class T>
void release(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

/// Runs a callback with an edit cookie. Holds references to whatever the
/// callback uses, because asynchronous sessions run later.
class EditSession final : public ITfEditSession {
public:
    EditSession(std::function<HRESULT(TfEditCookie)> fn, IUnknown* a, IUnknown* b) : fn_(std::move(fn)), a_(a), b_(b) {
        if (a_) a_->AddRef();
        if (b_) b_->AddRef();
    }
    ~EditSession() {
        release(a_);
        release(b_);
    }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
            *ppv = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = InterlockedDecrement(&ref_);
        if (n == 0) delete this;
        return n;
    }
    STDMETHODIMP DoEditSession(TfEditCookie ec) override { return fn_(ec); }

private:
    LONG ref_ = 1;
    std::function<HRESULT(TfEditCookie)> fn_;
    IUnknown *a_, *b_;
};

/// Composition text is underlined, like marked text on macOS.
class DisplayAttributeInfo final : public ITfDisplayAttributeInfo {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ITfDisplayAttributeInfo) {
            *ppv = static_cast<ITfDisplayAttributeInfo*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = InterlockedDecrement(&ref_);
        if (n == 0) delete this;
        return n;
    }
    STDMETHODIMP GetGUID(GUID* guid) override {
        *guid = GUID_KazTypeDisplayAttribute;
        return S_OK;
    }
    STDMETHODIMP GetDescription(BSTR* desc) override {
        *desc = SysAllocString(L"KazType word being typed");
        return *desc ? S_OK : E_OUTOFMEMORY;
    }
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* da) override {
        *da = {};
        da->crText.type = TF_CT_NONE;
        da->crBk.type = TF_CT_NONE;
        da->lsStyle = TF_LS_SOLID;
        da->crLine.type = TF_CT_NONE;
        da->bAttr = TF_ATTR_INPUT;
        return S_OK;
    }
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE*) override { return E_NOTIMPL; }
    STDMETHODIMP Reset() override { return S_OK; }

private:
    LONG ref_ = 1;
};

class DisplayAttributeEnum final : public IEnumTfDisplayAttributeInfo {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IEnumTfDisplayAttributeInfo) {
            *ppv = static_cast<IEnumTfDisplayAttributeInfo*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = InterlockedDecrement(&ref_);
        if (n == 0) delete this;
        return n;
    }
    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** out) override {
        auto* e = new (std::nothrow) DisplayAttributeEnum();
        if (!e) return E_OUTOFMEMORY;
        e->index_ = index_;
        *out = e;
        return S_OK;
    }
    STDMETHODIMP Next(ULONG count, ITfDisplayAttributeInfo** items, ULONG* fetched) override {
        ULONG n = 0;
        if (count > 0 && index_ == 0) {
            items[0] = new (std::nothrow) DisplayAttributeInfo();
            if (!items[0]) return E_OUTOFMEMORY;
            index_ = n = 1;
        }
        if (fetched) *fetched = n;
        return n == count ? S_OK : S_FALSE;
    }
    STDMETHODIMP Reset() override {
        index_ = 0;
        return S_OK;
    }
    STDMETHODIMP Skip(ULONG count) override {
        if (count > 0 && index_ == 0) index_ = 1;
        return count == 1 ? S_OK : S_FALSE;
    }

private:
    LONG ref_ = 1;
    ULONG index_ = 0;
};

std::wstring wide(const std::u32string& s) {
    auto u = toUtf16(s);
    return std::wstring(u.begin(), u.end());
}

}  // namespace

TextService::TextService() { InterlockedIncrement(&g_objects); }
TextService::~TextService() { InterlockedDecrement(&g_objects); }

STDMETHODIMP TextService::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor || riid == IID_ITfTextInputProcessorEx) {
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (riid == IID_ITfThreadMgrEventSink) {
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (riid == IID_ITfKeyEventSink) {
        *ppv = static_cast<ITfKeyEventSink*>(this);
    } else if (riid == IID_ITfCompositionSink) {
        *ppv = static_cast<ITfCompositionSink*>(this);
    } else if (riid == IID_ITfDisplayAttributeProvider) {
        *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) TextService::AddRef() { return InterlockedIncrement(&ref_); }

STDMETHODIMP_(ULONG) TextService::Release() {
    ULONG n = InterlockedDecrement(&ref_);
    if (n == 0) delete this;
    return n;
}

// MARK: Activation

STDMETHODIMP TextService::ActivateEx(ITfThreadMgr* tm, TfClientId id, DWORD) {
    threadMgr_ = tm;
    threadMgr_->AddRef();
    clientId_ = id;

    ITfSource* source = nullptr;
    ITfKeystrokeMgr* keys = nullptr;
    ITfCategoryMgr* categories = nullptr;
    bool ok = SUCCEEDED(tm->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))) &&
              SUCCEEDED(source->AdviseSink(IID_ITfThreadMgrEventSink, static_cast<ITfThreadMgrEventSink*>(this),
                                           &threadMgrCookie_)) &&
              SUCCEEDED(tm->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keys))) &&
              SUCCEEDED(keys->AdviseKeyEventSink(id, static_cast<ITfKeyEventSink*>(this), TRUE));
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                   reinterpret_cast<void**>(&categories)))) {
        categories->RegisterGUID(GUID_KazTypeDisplayAttribute, &attrAtom_);
    }
    release(source);
    release(keys);
    release(categories);
    if (!ok) {
        Deactivate();
        return E_FAIL;
    }

    candidates_.onPick = [this](int i) { selectAsync(i, true); };
    candidates_.onHover = [this](int i) { selectAsync(i, false); };
    sharedConverter();  // load the model now, not on the first key
    return S_OK;
}

STDMETHODIMP TextService::Deactivate() {
    if (composition_) {
        if (ITfContext* ctx = compositionContext()) {
            edit(ctx, TF_ES_SYNC | TF_ES_READWRITE, [&](TfEditCookie ec) {
                commit(ec, ctx);
                return S_OK;
            });
            ctx->Release();
        }
        release(composition_);
    }
    reset();
    candidates_.onPick = nullptr;
    candidates_.onHover = nullptr;

    if (threadMgr_) {
        ITfSource* source = nullptr;
        if (threadMgrCookie_ != TF_INVALID_COOKIE &&
            SUCCEEDED(threadMgr_->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source)))) {
            source->UnadviseSink(threadMgrCookie_);
        }
        threadMgrCookie_ = TF_INVALID_COOKIE;
        release(source);
        ITfKeystrokeMgr* keys = nullptr;
        if (SUCCEEDED(threadMgr_->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keys)))) {
            keys->UnadviseKeyEventSink(clientId_);
        }
        release(keys);
    }
    release(threadMgr_);
    clientId_ = TF_CLIENTID_NULL;
    return S_OK;
}

STDMETHODIMP TextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) {
    // Focus moved to another field: keep what was typed, like macOS does.
    if (ITfContext* ctx = composition_ ? compositionContext() : nullptr) {
        edit(ctx, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this, ctx](TfEditCookie ec) {
            if (composition_) commit(ec, ctx);
            return S_OK;
        });
        ctx->Release();
    }
    candidates_.hide();
    return S_OK;
}

// MARK: Keys

TextService::Action TextService::classify(WPARAM vk, LPARAM lp, char32_t& letter, int& arg) const {
    if (isModifierKey(vk)) return Action::None;
    bool composing = composition_ != nullptr;
    if (commandHeld()) return composing ? Action::CommitAndPass : Action::None;

    letter = russianLetter(vk, lp);
    if (letter && sharedConverter().converter) return Action::Letter;
    // Digits and punctuation: we type them, or the Kazakh layout types letters.
    char32_t symbol = russianSymbol(vk, lp);
    if (!composing) {
        letter = symbol;
        return symbol ? Action::Symbol : Action::None;
    }

    switch (vk) {
    case VK_BACK: return Action::Backspace;
    case VK_ESCAPE: return Action::Escape;
    case VK_RETURN: return Action::Commit;
    case VK_UP: arg = -1; return Action::Move;
    case VK_DOWN: arg = 1; return Action::Move;
    }
    int d = 0;
    if (vk >= '1' && vk <= '9' && !(GetKeyState(VK_SHIFT) & 0x8000)) d = int(vk - '0');
    if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) d = int(vk - VK_NUMPAD0);
    if (d > 0 && result_.candidates.size() > 1 && d <= int(result_.candidates.size())) {
        arg = d - 1;
        return Action::Pick;
    }
    if (symbol) {
        letter = symbol;
        return Action::Symbol;
    }
    // Space, Tab, anything else: commit, then let the app handle the key.
    return Action::CommitAndPass;
}

STDMETHODIMP TextService::OnTestKeyDown(ITfContext*, WPARAM vk, LPARAM lp, BOOL* eaten) {
    char32_t letter = 0;
    int arg = 0;
    *eaten = classify(vk, lp, letter, arg) != Action::None;
    return S_OK;
}

STDMETHODIMP TextService::OnKeyDown(ITfContext* ctx, WPARAM vk, LPARAM lp, BOOL* eaten) {
    char32_t letter = 0;
    int arg = 0;
    Action action = classify(vk, lp, letter, arg);
    *eaten = action != Action::None && action != Action::CommitAndPass;

    HRESULT hr = edit(ctx, TF_ES_SYNC | TF_ES_READWRITE, [&](TfEditCookie ec) -> HRESULT {
        switch (action) {
        case Action::None:
            break;
        case Action::Letter:
            if (!composition_) {
                context_ = readContext(ec, ctx);
                startComposition(ec, ctx);
                if (!composition_) return E_FAIL;
            }
            typed_.push_back(letter);
            update(ec, ctx);
            break;
        case Action::Symbol:
            if (composition_) commit(ec, ctx);
            // A one-character composition, ended at once.
            startComposition(ec, ctx);
            if (!composition_) return E_FAIL;
            finish(ec, ctx, std::u32string(1, letter));
            break;
        case Action::Backspace:
            typed_.pop_back();
            if (typed_.empty()) {
                finish(ec, ctx, U"");
            } else {
                update(ec, ctx);
            }
            break;
        case Action::Escape:
            finish(ec, ctx, typed_);
            break;
        case Action::Move: {
            int n = int(result_.candidates.size());
            if (n < 2) break;
            selected_ = (selected_ + arg + n) % n;
            setText(ec, ctx, result_.candidates[selected_], true);
            candidates_.select(selected_);
            break;
        }
        case Action::Pick:
            selected_ = arg;
            commit(ec, ctx);
            break;
        case Action::Commit:
        case Action::CommitAndPass:
            commit(ec, ctx);
            break;
        }
        return S_OK;
    });
    // A field we cannot compose in: let the app type the key itself.
    if (FAILED(hr) && (action == Action::Letter || action == Action::Symbol)) *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten) {
    *eaten = FALSE;
    return S_OK;
}

// MARK: Composition

STDMETHODIMP TextService::OnCompositionTerminated(TfEditCookie, ITfComposition* composition) {
    // The app ended it (click elsewhere, field cleared): drop our state.
    if (composition == composition_) release(composition_);
    reset();
    return S_OK;
}

HRESULT TextService::edit(ITfContext* ctx, DWORD flags, std::function<HRESULT(TfEditCookie)> fn) {
    if (!ctx) return E_INVALIDARG;
    auto* session = new (std::nothrow) EditSession(std::move(fn), static_cast<ITfTextInputProcessorEx*>(this), ctx);
    if (!session) return E_OUTOFMEMORY;
    HRESULT sessionHr = E_FAIL;
    HRESULT hr = ctx->RequestEditSession(clientId_, session, flags, &sessionHr);
    session->Release();
    if (FAILED(hr)) return hr;
    return (flags & TF_ES_SYNC) ? sessionHr : S_OK;
}

ITfContext* TextService::compositionContext() const {
    ITfRange* range = nullptr;
    ITfContext* ctx = nullptr;
    if (composition_ && SUCCEEDED(composition_->GetRange(&range))) range->GetContext(&ctx);
    release(range);
    return ctx;
}

void TextService::startComposition(TfEditCookie ec, ITfContext* ctx) {
    ITfInsertAtSelection* insert = nullptr;
    ITfContextComposition* compose = nullptr;
    ITfRange* range = nullptr;
    if (SUCCEEDED(ctx->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&insert))) &&
        SUCCEEDED(insert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &range)) &&
        SUCCEEDED(ctx->QueryInterface(IID_ITfContextComposition, reinterpret_cast<void**>(&compose)))) {
        ITfComposition* composition = nullptr;
        if (SUCCEEDED(compose->StartComposition(ec, range, this, &composition))) composition_ = composition;
    }
    release(insert);
    release(compose);
    release(range);
}

void TextService::update(TfEditCookie ec, ITfContext* ctx) {
    {
        auto ref = sharedConverter();
        if (!ref.converter) return;
        result_ = ref.converter->convert(typed_, context_, kMaxCandidates);
    }
    selected_ = 0;
    setText(ec, ctx, result_.candidates[0], true);
    showCandidates(ec, ctx);
}

void TextService::setText(TfEditCookie ec, ITfContext* ctx, const std::u32string& text, bool marked) {
    ITfRange* range = nullptr;
    if (!composition_ || FAILED(composition_->GetRange(&range))) return;
    auto w = wide(text);
    range->SetText(ec, 0, w.c_str(), LONG(w.size()));

    ITfProperty* attr = nullptr;
    if (SUCCEEDED(ctx->GetProperty(GUID_PROP_ATTRIBUTE, &attr))) {
        if (marked && attrAtom_ != TF_INVALID_GUIDATOM) {
            VARIANT v;
            VariantInit(&v);
            v.vt = VT_I4;
            v.lVal = LONG(attrAtom_);
            attr->SetValue(ec, range, &v);
        } else {
            attr->Clear(ec, range);
        }
        attr->Release();
    }

    // Caret after the word.
    ITfRange* caret = nullptr;
    if (SUCCEEDED(range->Clone(&caret))) {
        caret->Collapse(ec, TF_ANCHOR_END);
        TF_SELECTION sel = {};
        sel.range = caret;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        ctx->SetSelection(ec, 1, &sel);
        caret->Release();
    }
    range->Release();
}

void TextService::finish(TfEditCookie ec, ITfContext* ctx, const std::u32string& text) {
    if (composition_) {
        setText(ec, ctx, text, false);
        composition_->EndComposition(ec);
        release(composition_);
    }
    reset();
}

void TextService::commit(TfEditCookie ec, ITfContext* ctx) {
    if (!composition_ || typed_.empty() || result_.candidates.empty()) return finish(ec, ctx, typed_);
    std::u32string chosen = result_.candidates[selected_];
    {
        auto ref = sharedConverter();
        if (ref.converter) ref.converter->learn(typed_, chosen, result_.modelTop);
    }
    finish(ec, ctx, chosen);
}

void TextService::showCandidates(TfEditCookie ec, ITfContext* ctx) {
    if (result_.candidates.size() < 2) return candidates_.hide();

    RECT rc = {};
    bool found = false;
    ITfContextView* view = nullptr;
    ITfRange* range = nullptr;
    if (SUCCEEDED(ctx->GetActiveView(&view)) && composition_ && SUCCEEDED(composition_->GetRange(&range))) {
        BOOL clipped = FALSE;
        found = SUCCEEDED(view->GetTextExt(ec, range, &rc, &clipped)) && (rc.bottom > rc.top || rc.right > rc.left);
    }
    release(view);
    release(range);
    if (!found) {
        // Apps without layout info: fall back to the system caret.
        GUITHREADINFO gti = {};
        gti.cbSize = sizeof(gti);
        if (GetGUIThreadInfo(0, &gti) && gti.hwndCaret) {
            rc = gti.rcCaret;
            MapWindowPoints(gti.hwndCaret, nullptr, reinterpret_cast<POINT*>(&rc), 2);
            found = true;
        }
    }
    if (!found) return candidates_.hide();

    std::vector<std::wstring> items;
    for (const auto& c : result_.candidates) items.push_back(wide(c));
    candidates_.show(items, selected_, rc);
}

std::u32string TextService::readContext(TfEditCookie ec, ITfContext* ctx) {
    TF_SELECTION sel = {};
    ULONG n = 0;
    if (FAILED(ctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &n)) || n == 0) return {};
    ITfRange* range = sel.range;
    range->Collapse(ec, TF_ANCHOR_START);
    LONG moved = 0;
    range->ShiftStart(ec, -64, &moved, nullptr);
    WCHAR buf[64];
    ULONG got = 0;
    if (FAILED(range->GetText(ec, 0, buf, 64, &got))) got = 0;
    range->Release();
    return fromUtf16(std::u16string(buf, buf + got));
}

void TextService::reset() {
    typed_.clear();
    context_.clear();
    result_ = {};
    selected_ = 0;
    candidates_.hide();
}

void TextService::selectAsync(int index, bool commitNow) {
    ITfContext* ctx = composition_ ? compositionContext() : nullptr;
    if (!ctx) return;
    edit(ctx, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this, ctx, index, commitNow](TfEditCookie ec) {
        if (!composition_ || index >= int(result_.candidates.size())) return S_OK;
        selected_ = index;
        if (commitNow) {
            commit(ec, ctx);
        } else {
            setText(ec, ctx, result_.candidates[index], true);
            candidates_.select(index);
        }
        return S_OK;
    });
    ctx->Release();
}

// MARK: Display attributes

STDMETHODIMP TextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** out) {
    if (!out) return E_INVALIDARG;
    *out = new (std::nothrow) DisplayAttributeEnum();
    return *out ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP TextService::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** out) {
    if (!out) return E_INVALIDARG;
    *out = nullptr;
    if (guid != GUID_KazTypeDisplayAttribute) return E_INVALIDARG;
    *out = new (std::nothrow) DisplayAttributeInfo();
    return *out ? S_OK : E_OUTOFMEMORY;
}

}  // namespace kaz
