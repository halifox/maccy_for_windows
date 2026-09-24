#pragma once

#include "PlatformConfig.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include <functional>
#include <string>
#include <vector>

#include "Constants.h"
#include "ClipboardData.h"
#include "HistoryRenderer.h"
#include "KeyboardHandler.h"
#include "resource.h"

class HistoryListControls : public CMessageMap {
public:
    using FooterUpdateCallback = std::function<void(UINT, WPARAM)>;
    using PasteCallback = std::function<void(int)>;
    using SelectionCallback = std::function<void(sqlite3_int64)>;

    HistoryListControls();

    HistoryListControls(const HistoryListControls &) = delete;
    HistoryListControls &operator=(const HistoryListControls &) = delete;

    CContainedWindowT<CListBox> &HistoryListWindow() noexcept { return m_historyList; }
    CContainedWindowT<CListBox> &PinsListWindow() noexcept { return m_pinsList; }

    void Configure(KeyboardHandler &keyboard_handler,
                   HistoryRenderer &renderer,
                   HWND search_window,
                   std::vector<ClipboardItem> &items,
                   std::wstring &search_query,
                   bool &loading_list,
                   bool &popup_visible,
                   FooterUpdateCallback request_footer_update,
                   PasteCallback paste_item,
                   SelectionCallback selection_changed);
    void Shutdown() noexcept;

    BEGIN_MSG_MAP(HistoryListControls)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnHistoryListMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnHistoryListMouseLeave)
        MESSAGE_HANDLER(WM_KEYDOWN, OnHistoryListKeyDown)
        MESSAGE_HANDLER(WM_SYSKEYDOWN, OnHistoryListKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnHistoryListKeyDown)
        MESSAGE_HANDLER(WM_SYSKEYUP, OnHistoryListKeyDown)
        MESSAGE_HANDLER(WM_CHAR, OnHistoryListChar)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnHistoryListButtonUp)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnHistoryListScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnHistoryListScroll)
        REFLECTED_COMMAND_HANDLER(IDC_HISTORY_LIST, LBN_SELCHANGE, OnHistoryListSelectionChanged)
        REFLECTED_COMMAND_HANDLER(IDC_HISTORY_PINS, LBN_SELCHANGE, OnHistoryListSelectionChanged)
        MESSAGE_HANDLER(OCM_MEASUREITEM, OnMeasureItem)
        MESSAGE_HANDLER(OCM_DRAWITEM, OnDrawItem)
    END_MSG_MAP()

protected:
    CContainedWindowT<CListBox> m_historyList;
    CContainedWindowT<CListBox> m_pinsList;

private:
    CListBox &CurrentHistoryList() noexcept;
    bool IsConfigured() const noexcept;

    LRESULT OnHistoryListMouseMove(UINT, WPARAM, LPARAM lParam, BOOL &handled);
    LRESULT OnHistoryListMouseLeave(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnHistoryListKeyDown(UINT message, WPARAM key, LPARAM, BOOL &handled);
    LRESULT OnHistoryListChar(UINT, WPARAM character, LPARAM, BOOL &handled);
    LRESULT OnHistoryListButtonUp(UINT, WPARAM, LPARAM lParam, BOOL &handled);
    LRESULT OnHistoryListScroll(UINT message, WPARAM wParam, LPARAM lParam, BOOL &handled);
    LRESULT OnHistoryListSelectionChanged(WORD, WORD id, HWND, BOOL &handled);
    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL &handled);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled);

    KeyboardHandler *m_keyboardHandler = nullptr;
    HistoryRenderer *m_renderer = nullptr;
    CEdit m_search;
    std::vector<ClipboardItem> *m_items = nullptr;
    std::wstring *m_searchQuery = nullptr;
    bool *m_loadingList = nullptr;
    bool *m_popupVisible = nullptr;
    FooterUpdateCallback m_requestFooterUpdate;
    PasteCallback m_pasteItem;
    SelectionCallback m_selectionChanged;
};
