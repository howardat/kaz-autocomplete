// Numbered candidate list shown under the word being typed, styled after
// the shadcn/ui Command menu (zinc theme, light and dark).
#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

namespace kaz {

class CandidateWindow {
public:
    ~CandidateWindow();

    /// anchor: screen rect of the composition text.
    void show(const std::vector<std::wstring>& items, int selected, const RECT& anchor);
    void select(int selected);
    void hide();
    bool visible() const { return hwnd_ && IsWindowVisible(hwnd_); }

    /// Called with the row index when the user clicks a candidate.
    std::function<void(int)> onPick;
    /// Called with the row index when the mouse moves onto a candidate.
    std::function<void(int)> onHover;

private:
    static LRESULT CALLBACK proc(HWND, UINT, WPARAM, LPARAM);
    bool create();
    void layout(const RECT& anchor);
    void paint(HDC dc);
    int rowAt(int y) const;

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr, small_ = nullptr;
    UINT fontDpi_ = 0;
    std::vector<std::wstring> items_;
    int selected_ = 0, hover_ = -1;
    int rowHeight_ = 0, padding_ = 0, rowPadX_ = 0, kbd_ = 0, radius_ = 0;
};

}  // namespace kaz
