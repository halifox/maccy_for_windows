#include "KeyboardHandler.h"
#include "Constants.h"
#include "resource.h"

#include <algorithm>
#include <array>
#include <imm.h>
#include <utility>
#include <windowsx.h>

KeyboardHandler* KeyboardHandler::s_hookOwner = nullptr;
std::atomic<bool> KeyboardHandler::s_hotkeyCaptureActive{false};

void KeyboardHandler::SetHotkeyCaptureActive(bool active) noexcept {
    s_hotkeyCaptureActive.store(active, std::memory_order_release);
}

bool KeyboardHandler::IsHotkeyCaptureActive() noexcept {
    return s_hotkeyCaptureActive.load(std::memory_order_acquire);
}

KeyboardHandler::KeyboardHandler(AppSettings& settings)
    : m_settings(settings) {}

KeyboardHandler::~KeyboardHandler() {
    Shutdown();
}

bool KeyboardHandler::Initialize(CWindow owner, CEdit search, CListBox historyList,
                                 CListBox pinsList,
                                 const std::array<CButton, AppConstants::UI::kFooterButtonCount>& footerButtons) {
    m_owner = owner;
    m_search = search;
    m_historyList = historyList;
    m_pinsList = pinsList;
    m_footerButtons = footerButtons;
    return true;
}

void KeyboardHandler::Shutdown() {
    if (m_keyboardHook != nullptr) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
    if (s_hookOwner == this) {
        s_hookOwner = nullptr;
    }
    m_suppressingOpenKey = false;
    m_suppressingCaptureKey = 0;
    m_captureTargetWindow = nullptr;
    m_capturedKey = 0;
    m_capturedModifiers = 0;
    m_capturedKeyReleased = false;
    m_pendingWinKey = 0;
    m_pendingWinScan = 0;
    m_pendingWinFlags = 0;
    m_winKeyPassedThrough = false;
    m_suppressingWinKey = false;
    m_hotkeyRegistered = false;
    SetHotkeyCaptureActive(false);
    m_callbacks = {};
}

bool KeyboardHandler::InstallOpenHotkeyHook(UINT) {
    if (s_hookOwner != nullptr && s_hookOwner != this) {
        return false;
    }
    s_hookOwner = this;
    m_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                       GetModuleHandleW(nullptr), 0);
    m_hotkeyRegistered = m_keyboardHook != nullptr;
    if (!m_hotkeyRegistered) {
        s_hookOwner = nullptr;
    }
    return m_hotkeyRegistered;
}

void KeyboardHandler::UninstallOpenHotkeyHook(UINT) {
    if (m_keyboardHook != nullptr) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
        if (s_hookOwner == this) {
            s_hookOwner = nullptr;
        }
        m_suppressingOpenKey = false;
        m_suppressingCaptureKey = 0;
        m_captureTargetWindow = nullptr;
        m_capturedKey = 0;
        m_capturedModifiers = 0;
        m_capturedKeyReleased = false;
        m_pendingWinKey = 0;
        m_winKeyPassedThrough = false;
        m_suppressingWinKey = false;
    }
    m_hotkeyRegistered = false;
}

LRESULT CALLBACK KeyboardHandler::LowLevelKeyboardProc(int code, WPARAM message, LPARAM data) {
    KeyboardHandler* const handler = s_hookOwner;
    const HHOOK hook = handler != nullptr ? handler->m_keyboardHook : nullptr;
    if (code < 0 || handler == nullptr) {
        return CallNextHookEx(hook, code, message, data);
    }
    const auto* const key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
    const bool keyDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool keyUp = message == WM_KEYUP || message == WM_SYSKEYUP;
    if ((key->flags & LLKHF_INJECTED) != 0) {
        return CallNextHookEx(hook, code, message, data);
    }

    const bool isWinKey = key->vkCode == VK_LWIN || key->vkCode == VK_RWIN;
    const bool isModifierKey = isWinKey || key->vkCode == VK_SHIFT || key->vkCode == VK_LSHIFT ||
        key->vkCode == VK_RSHIFT || key->vkCode == VK_CONTROL || key->vkCode == VK_LCONTROL ||
        key->vkCode == VK_RCONTROL || key->vkCode == VK_MENU || key->vkCode == VK_LMENU ||
        key->vkCode == VK_RMENU;
    const HWND focusedWindow = GetFocus();
    // Prefer an explicit capture flag from the settings hotkey edit (M-15).
    const bool capturingHotkey = IsHotkeyCaptureActive();

    if (handler->m_suppressingCaptureKey == key->vkCode && (keyDown || keyUp)) {
        if (keyUp) {
            handler->m_suppressingCaptureKey = 0;
            handler->m_capturedKeyReleased = true;
            if (!handler->m_suppressingWinKey) {
                if (IsWindow(handler->m_captureTargetWindow)) {
                    PostMessageW(handler->m_captureTargetWindow, AppConstants::kCaptureHotkeyMessage,
                                 static_cast<WPARAM>(handler->m_capturedKey),
                                 static_cast<LPARAM>(handler->m_capturedModifiers));
                }
                handler->m_captureTargetWindow = nullptr;
                handler->m_capturedKey = 0;
                handler->m_capturedKeyReleased = false;
            }
        }
        return 1;
    }

    if (capturingHotkey && keyDown && !isModifierKey) {
        UINT modifiers = 0;
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) modifiers |= MOD_ALT;
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) modifiers |= MOD_CONTROL;
        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) modifiers |= MOD_SHIFT;
        if (handler->m_pendingWinKey != 0 || (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0) modifiers |= MOD_WIN;
        if (modifiers == 0 && key->vkCode != VK_BACK) {
            return CallNextHookEx(hook, code, message, data);
        }
        handler->m_suppressingCaptureKey = key->vkCode;
        handler->m_captureTargetWindow =
            focusedWindow != nullptr ? focusedWindow : GetForegroundWindow();
        handler->m_capturedKey = key->vkCode;
        handler->m_capturedModifiers = modifiers;
        handler->m_capturedKeyReleased = false;
        if (handler->m_pendingWinKey != 0) {
            handler->m_suppressingWinKey = true;
        }
        return 1;
    }

    const UINT configuredModifiers = handler->m_settings.open_hotkey.modifiers &
        (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN);
    if (!capturingHotkey && !handler->m_suppressingOpenKey && keyDown && !isModifierKey &&
        (configuredModifiers & MOD_WIN) == 0 &&
        key->vkCode == handler->m_settings.open_hotkey.virtual_key) {
        UINT pressed = 0;
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) pressed |= MOD_ALT;
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) pressed |= MOD_CONTROL;
        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) pressed |= MOD_SHIFT;
        if (pressed == configuredModifiers) {
            handler->m_suppressingOpenKey = true;
            PostMessageW(handler->m_owner.m_hWnd, AppConstants::kOpenHotkeyMessage, 0,
                         reinterpret_cast<LPARAM>(GetForegroundWindow()));
            return 1;
        }
    }

    if (handler->m_suppressingOpenKey &&
        key->vkCode == handler->m_settings.open_hotkey.virtual_key && (keyDown || keyUp)) {
        if (keyUp) {
            handler->m_suppressingOpenKey = false;
        }
        return 1;
    }

    if (isWinKey) {
        if ((configuredModifiers & MOD_WIN) == 0 && !capturingHotkey) {
            return CallNextHookEx(hook, code, message, data);
        }
        if (keyDown) {
            if (handler->m_suppressingWinKey || handler->m_pendingWinKey == key->vkCode) {
                return 1;
            }
            if (handler->m_winKeyPassedThrough) {
                return CallNextHookEx(hook, code, message, data);
            }
            handler->m_pendingWinKey = key->vkCode;
            handler->m_pendingWinScan = key->scanCode;
            handler->m_pendingWinFlags = key->flags;
            return 1;
        }
        if (keyUp) {
            if (handler->m_suppressingWinKey) {
                handler->m_suppressingWinKey = false;
                handler->m_pendingWinKey = 0;
                handler->m_pendingWinScan = 0;
                handler->m_pendingWinFlags = 0;
                if (handler->m_capturedKeyReleased) {
                    if (IsWindow(handler->m_captureTargetWindow)) {
                        PostMessageW(handler->m_captureTargetWindow, AppConstants::kCaptureHotkeyMessage,
                                     static_cast<WPARAM>(handler->m_capturedKey),
                                     static_cast<LPARAM>(handler->m_capturedModifiers));
                    }
                    handler->m_captureTargetWindow = nullptr;
                    handler->m_capturedKey = 0;
                    handler->m_capturedKeyReleased = false;
                }
                return 1;
            }
            if (handler->m_pendingWinKey == key->vkCode) {
                // Replay the delayed Win-down with original scan/extended flags (H-07).
                INPUT replay{};
                replay.type = INPUT_KEYBOARD;
                replay.ki.wVk = static_cast<WORD>(key->vkCode);
                replay.ki.wScan = static_cast<WORD>(handler->m_pendingWinScan);
                if ((handler->m_pendingWinFlags & LLKHF_EXTENDED) != 0) {
                    replay.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                }
                SendInput(1, &replay, sizeof(replay));
                handler->m_pendingWinKey = 0;
                handler->m_pendingWinScan = 0;
                handler->m_pendingWinFlags = 0;
                return CallNextHookEx(hook, code, message, data);
            }
            if (handler->m_winKeyPassedThrough) {
                handler->m_winKeyPassedThrough = false;
            }
        }
        return CallNextHookEx(hook, code, message, data);
    }

    if (handler->m_winKeyPassedThrough) {
        return CallNextHookEx(hook, code, message, data);
    }

    if (handler->m_pendingWinKey != 0 && isModifierKey) {
        return CallNextHookEx(hook, code, message, data);
    }

    if (!capturingHotkey && keyDown && handler->m_pendingWinKey != 0 &&
        key->vkCode == handler->m_settings.open_hotkey.virtual_key) {
        UINT pressed = 0;
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) pressed |= MOD_ALT;
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) pressed |= MOD_CONTROL;
        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) pressed |= MOD_SHIFT;
        pressed |= MOD_WIN;
        if (pressed == configuredModifiers) {
            handler->m_suppressingOpenKey = true;
            handler->m_suppressingWinKey = true;
            PostMessageW(handler->m_owner.m_hWnd, AppConstants::kOpenHotkeyMessage, 0,
                         reinterpret_cast<LPARAM>(GetForegroundWindow()));
            return 1;
        }
    }

    if (keyDown && handler->m_pendingWinKey != 0) {
        // Let ordinary Win+key shortcuts pass through after replaying the delayed Win-down.
        INPUT replay{};
        replay.type = INPUT_KEYBOARD;
        replay.ki.wVk = static_cast<WORD>(handler->m_pendingWinKey);
        replay.ki.wScan = static_cast<WORD>(handler->m_pendingWinScan);
        if ((handler->m_pendingWinFlags & LLKHF_EXTENDED) != 0) {
            replay.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        SendInput(1, &replay, sizeof(replay));
        handler->m_pendingWinKey = 0;
        handler->m_pendingWinScan = 0;
        handler->m_pendingWinFlags = 0;
        handler->m_winKeyPassedThrough = true;
    }

    return CallNextHookEx(hook, code, message, data);
}

bool KeyboardHandler::IsComposing(CWindow window) const {
    HIMC context = ImmGetContext(window.m_hWnd);
    const bool composing = context && ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0) > 0;
    if (context) ImmReleaseContext(window.m_hWnd, context);
    return m_imeComposing || composing;
}

bool KeyboardHandler::MouseCanSelect() {
    POINT position{};
    GetCursorPos(&position);
    if (m_keyboardNavigating && position.x == m_keyboardPointer.x && position.y == m_keyboardPointer.y) {
        return false;
    }
    m_keyboardNavigating = false;
    return true;
}

void KeyboardHandler::TypeToSearch(WPARAM character, CEdit& search) {
    if (!m_settings.show_search) return;
    search.SetFocus();
    const wchar_t text[] = {static_cast<wchar_t>(character), 0};
    search.ReplaceSel(text);
}

void KeyboardHandler::FocusSearchOrPopup(CEdit& search, CWindow mainWindow, bool searchVisible) {
    if (searchVisible) {
        search.SetFocus();
    } else {
        mainWindow.SetFocus();
    }
}

void KeyboardHandler::SetActiveFooter(int index, const std::vector<ClipboardItem>& items) {
    if (index < 0 || index >= static_cast<int>(m_footerButtons.size()) ||
        (m_activeFooter == index && m_activeItemIndex < 0)) {
        return;
    }

    const int previousItem = m_activeItemIndex;
    m_activeFooter = index;
    m_activeItemIndex = -1;
    m_activeItemId = 0;
    ClearHistoryHover();
    SetListSelection(m_historyList, -1);
    SetListSelection(m_pinsList, -1);
    InvalidateHistoryItem(previousItem, items, m_historyList, m_pinsList);
    InvalidateFooterButtons();
}

CListBox KeyboardHandler::ListForItem(int index, const std::vector<ClipboardItem>& items,
                                      CListBox historyList, CListBox pinsList) const {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return CListBox();
    return items[static_cast<size_t>(index)].pinned ? pinsList : historyList;
}

int KeyboardHandler::RowForItem(int index, const std::vector<ClipboardItem>& items,
                                CListBox historyList, CListBox pinsList) const {
    const CListBox list = ListForItem(index, items, historyList, pinsList);
    if (list.m_hWnd == nullptr) return -1;
    const int count = list.GetCount();
    for (int row = 0; row < count; ++row) {
        if (ItemIndexAtRow(list, row, items) == index) return row;
    }
    return -1;
}

int KeyboardHandler::ItemIndexAtRow(CListBox list, int row,
                                    const std::vector<ClipboardItem>& items) const {
    if (list.m_hWnd == nullptr || row < 0) return -1;
    const DWORD_PTR value = list.GetItemData(row);
    if (value == LB_ERR || static_cast<size_t>(value) >= items.size()) return -1;
    return static_cast<int>(value);
}

int KeyboardHandler::HistoryItemAtPoint(CListBox window, POINT point,
                                        CListBox historyList, CListBox pinsList,
                                        const std::vector<ClipboardItem>& items) const {
    if (window.m_hWnd != historyList.m_hWnd && window.m_hWnd != pinsList.m_hWnd) return -1;
    BOOL outside = FALSE;
    const UINT row = window.ItemFromPoint(point, outside);
    if (outside) return -1;
    return ItemIndexAtRow(window, static_cast<int>(row), items);
}

void KeyboardHandler::InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                            CListBox historyList, CListBox pinsList) {
    const int row = RowForItem(index, items, historyList, pinsList);
    if (row < 0) return;
    RECT rect{};
    CListBox list = ListForItem(index, items, historyList, pinsList);
    if (list.m_hWnd != nullptr && list.GetItemRect(row, &rect) != LB_ERR) {
        list.InvalidateRect(&rect, FALSE);
    }
}

void KeyboardHandler::SetListSelection(CListBox list, int row) const {
    if (list.m_hWnd == nullptr) return;
    list.SetCurSel(row);
}

void KeyboardHandler::InvalidateFooterButtons() const {
    for (CButton button : m_footerButtons) {
        if (button.m_hWnd != nullptr) {
            button.InvalidateRect(nullptr, FALSE);
        }
    }
}

void KeyboardHandler::SetActiveHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                           CListBox historyList, CListBox pinsList,
                                           bool scrollIntoView) {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return;
    const int previous = m_activeItemIndex;
    const int previousFooter = m_activeFooter;
    m_activeItemIndex = index;
    m_activeItemId = items[index].id;
    m_activeFooter = -1;

    if (scrollIntoView) {
        SetListSelection(historyList, -1);
        SetListSelection(pinsList, -1);
        CListBox list = ListForItem(index, items, historyList, pinsList);
        if (list.m_hWnd != nullptr) {
            SetListSelection(list, RowForItem(index, items, historyList, pinsList));
        }
    }

    InvalidateHistoryItem(previous, items, historyList, pinsList);
    InvalidateHistoryItem(index, items, historyList, pinsList);
    if (previousFooter >= 0) {
        InvalidateFooterButtons();
    }
}

void KeyboardHandler::BeginHistoryMouseTracking() {
    TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, m_hoverList.m_hWnd, 0};
    ::TrackMouseEvent(&tracking);
}

void KeyboardHandler::OnHistoryMouseMove(CListBox window, POINT point,
                                        const std::vector<ClipboardItem>& items,
                                        CListBox historyList, CListBox pinsList,
                                        bool popupVisible) {
    if (!MouseCanSelect()) return;
    if (!popupVisible || (window.m_hWnd != historyList.m_hWnd && window.m_hWnd != pinsList.m_hWnd)) {
        return;
    }

    m_hoverList = window;
    BeginHistoryMouseTracking();

    const int index = HistoryItemAtPoint(window, point, historyList, pinsList, items);
    if (index < 0) {
        ClearHistoryHover();
        return;
    }

    const sqlite3_int64 item_id = items[static_cast<size_t>(index)].id;
    if (index == m_hoveredItemIndex && item_id == m_hoveredItemId) {
        if (m_activeFooter >= 0 || m_activeItemIndex != index) {
            SetActiveHistoryItem(index, items, historyList, pinsList, false);
            if (m_callbacks.preview) {
                m_callbacks.preview(item_id, false);
            }
        }
        return;
    }

    const int previous_index = m_hoveredItemIndex;
    m_hoveredItemIndex = index;
    m_hoveredItemId = item_id;
    SetActiveHistoryItem(index, items, historyList, pinsList, false);

    InvalidateHistoryItem(previous_index, items, historyList, pinsList);
    InvalidateHistoryItem(index, items, historyList, pinsList);

    if (m_callbacks.preview) {
                m_callbacks.preview(item_id, false);
            }
}

void KeyboardHandler::OnHistoryMouseLeave() {
    ClearHistoryHover();
}

void KeyboardHandler::UpdateHistoryHoverFromCursor(const std::vector<ClipboardItem>& items,
                                                   CListBox historyList, CListBox pinsList,
                                                   bool popupVisible) {
    if (!popupVisible) return;
    POINT cursor{};
    if (!GetCursorPos(&cursor)) return;
    for (const CListBox& list : {historyList, pinsList}) {
        if (!list.IsWindowVisible()) continue;
        POINT point = cursor;
        list.ScreenToClient(&point);
        RECT rect{};
        list.GetClientRect(&rect);
        if (::PtInRect(&rect, point)) {
            OnHistoryMouseMove(list, point, items, historyList, pinsList, popupVisible);
            return;
        }
    }
    ClearHistoryHover();
}

void KeyboardHandler::ClearHistoryHover() {
    m_hoveredItemIndex = -1;
    m_hoveredItemId = 0;
    if (m_owner.m_hWnd != nullptr) {
        KillTimer(m_owner.m_hWnd, AppConstants::Timer::kPreview);
    }
}

void KeyboardHandler::NavigateHistoryFromSearch(bool forward,
                                               const std::vector<ClipboardItem>& items,
                                               CListBox historyList, CListBox pinsList,
                                               bool showFooter) {
    m_keyboardNavigating = true;
    GetCursorPos(&m_keyboardPointer);
    ClearHistoryHover();

    const int count = static_cast<int>(items.size());
    const int total = count + (showFooter ? AppConstants::UI::kFooterButtonCount : 0);
    if (!total) return;

    int current = m_activeFooter >= 0 ? count + m_activeFooter : m_activeItemIndex;
    const int target = std::clamp(current + (forward ? 1 : -1), 0, total - 1);

    if (target >= count) {
        SetActiveFooter(target - count, items);
    } else {
        SetActiveHistoryItem(target, items, historyList, pinsList);
        if (m_callbacks.preview) {
                m_callbacks.preview(m_activeItemId, true);
            }
    }
}

bool KeyboardHandler::HandlePopupKey(WPARAM key, const CEdit& search, const std::vector<ClipboardItem>& items) {
    if (m_callbacks.search && search.m_hWnd != nullptr &&
        (key == VK_RETURN || key == VK_UP || key == VK_DOWN || key == VK_PRIOR || key == VK_NEXT ||
         (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000))) {
        m_callbacks.search();
    }

    if (HandlePopupShortcut(key, items)) return true;

    const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (key == VK_ESCAPE) {
        if (m_callbacks.hide_window) {
            m_callbacks.hide_window();
        }
        return true;
    }
    if (key == VK_RETURN) {
        if (m_activeFooter >= 0 &&
            m_activeFooter < static_cast<int>(m_footerButtons.size())) {
            CButton &button = m_footerButtons[static_cast<size_t>(m_activeFooter)];
            if (button.m_hWnd != nullptr && m_owner.m_hWnd != nullptr) {
                const int control_id = button.GetDlgCtrlID();
                ::SendMessageW(
                    m_owner.m_hWnd,
                    WM_COMMAND,
                    MAKEWPARAM(static_cast<WORD>(control_id), BN_CLICKED),
                    reinterpret_cast<LPARAM>(button.m_hWnd)
                );
            }
        } else if (m_callbacks.paste) {
            m_callbacks.paste(m_activeItemIndex);
        }
        return true;
    }
    if (key == VK_UP || key == VK_DOWN || key == VK_TAB) {
        const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool forward = key == VK_DOWN || (key == VK_TAB && !shift);
        NavigateHistoryFromSearch(forward, items, m_historyList, m_pinsList, m_settings.show_footer);
        return true;
    }
    if ((ctrl && (key == VK_HOME || key == VK_END)) || key == VK_PRIOR || key == VK_NEXT) {
        m_keyboardNavigating = true;
        GetCursorPos(&m_keyboardPointer);
        ClearHistoryHover();
        if (!items.empty()) {
            const int target = (key == VK_HOME || key == VK_PRIOR) ? 0 : static_cast<int>(items.size()) - 1;
            SetActiveHistoryItem(target, items, m_historyList, m_pinsList);
            if (m_callbacks.preview) {
                m_callbacks.preview(m_activeItemId, true);
            }
        }
        return true;
    }
    if (ctrl && key == 'F' && m_settings.show_search) {
        return true;
    }
    if (ctrl && key == 'U') {
        return true;
    }
    return false;
}

bool KeyboardHandler::HandlePopupShortcut(WPARAM key, const std::vector<ClipboardItem>& items) {
    if (IsHotKeyPressed(m_settings.pin_hotkey, key)) {
        if (m_callbacks.toggle_pin) {
            m_callbacks.toggle_pin();
        }
        return true;
    }
    if (IsHotKeyPressed(m_settings.delete_hotkey, key)) {
        if (m_callbacks.delete_item) {
            m_callbacks.delete_item();
        }
        return true;
    }
    if (IsHotKeyPressed(m_settings.preview_hotkey, key)) {
        if (m_callbacks.toggle_preview) {
            m_callbacks.toggle_preview();
        }
        return true;
    }

    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (ctrl && alt && key == VK_BACK) {
        if (m_callbacks.clear_history) {
            m_callbacks.clear_history(shift);
        }
        return true;
    }
    if (ctrl && !alt && key == VK_OEM_COMMA) {
        if (m_callbacks.open_settings) {
            m_callbacks.open_settings();
        }
        return true;
    }
    if (ctrl && !alt && key == 'Q') {
        if (m_callbacks.exit) {
            m_callbacks.exit();
        }
        return true;
    }

    // Keep native Edit shortcuts and global popup navigation available.
    if (ctrl && !alt && std::wstring_view(L"ACFUVXYZ").find(static_cast<wchar_t>(key)) != std::wstring_view::npos) {
        return false;
    }

    if ((ctrl != alt) && !(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000)) {
        int number = 0;
        WPARAM digit_key = key;
        if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
            digit_key = static_cast<WPARAM>('0' + (key - VK_NUMPAD0));
        }
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            bool matches = false;
            if (item.pinned) {
                if (item.pin.size() == 1) {
                    const wchar_t pin = static_cast<wchar_t>(towupper(item.pin[0]));
                    // Digit pins must accept both main-row and numpad keys (L-16 + L-21).
                    matches = pin == static_cast<wchar_t>(key) ||
                        (pin >= L'0' && pin <= L'9' && pin == static_cast<wchar_t>(digit_key));
                }
            } else {
                matches = (++number <= 9 && digit_key == static_cast<WPARAM>('0' + number));
            }
            if (matches) {
                SetActiveHistoryItem(static_cast<int>(i), items, m_historyList, m_pinsList);
                if (m_callbacks.paste) {
                    m_callbacks.paste(static_cast<int>(i));
                }
                return true;
            }
        }
    }
    return false;
}

bool KeyboardHandler::OnChar(WPARAM character) {
    return m_settings.show_search && character >= 0x20 && character != 0x7f;
}

void KeyboardHandler::SetCallbacks(Callbacks callbacks) {
    m_callbacks = std::move(callbacks);
}
