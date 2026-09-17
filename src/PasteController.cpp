#include "PasteController.h"

#include "Constants.h"

namespace {

bool IsKeyDown(int virtual_key) {
    return (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

bool IsControlOnlyDown() {
    return IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_MENU) && !IsKeyDown(VK_SHIFT);
}

void SendControlVPaste() {
    INPUT inputs[4]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = L'V';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = L'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

void SendPasteWithHeldControl() {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = L'V';
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = L'V';
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

} // namespace

void PasteController::CaptureTargetWindow(HWND candidate) {
    HWND foreground = candidate != nullptr ? candidate : GetForegroundWindow();
    if (foreground == nullptr || m_owner == nullptr) {
        return;
    }
    const HWND root = GetAncestor(foreground, GA_ROOT);
    if (root != nullptr) {
        foreground = root;
    }

    m_targetWindow = foreground;
    m_targetFocusWindow = foreground;
    const DWORD target_thread = GetWindowThreadProcessId(foreground, nullptr);
    GUITHREADINFO gui_info{sizeof(gui_info)};
    if (target_thread != 0 && GetGUIThreadInfo(target_thread, &gui_info) &&
        gui_info.hwndFocus != nullptr) {
        m_targetFocusWindow = gui_info.hwndFocus;
    }
}

void PasteController::RestoreTargetFocusAndPaste(HWND target, HWND target_focus, bool paste) {
    if (!::IsWindow(target)) {
        return;
    }
    if (::IsIconic(target)) {
        ::ShowWindow(target, SW_RESTORE);
    }
    const DWORD current_thread = GetCurrentThreadId();
    const DWORD target_thread = GetWindowThreadProcessId(target, nullptr);
    const bool attached = target_thread != 0 && target_thread != current_thread &&
        AttachThreadInput(current_thread, target_thread, TRUE) != FALSE;
    SetForegroundWindow(target);
    if (attached && ::IsWindow(target_focus) && GetAncestor(target_focus, GA_ROOT) == target) {
        ::SetFocus(target_focus);
    }
    if (attached) {
        AttachThreadInput(current_thread, target_thread, FALSE);
    }
    SetForegroundWindow(target);
    if (!paste) {
        return;
    }
    const HWND active_window = GetForegroundWindow();
    if (active_window != target && GetAncestor(active_window, GA_ROOT) != target) {
        return;
    }
    if (PasteModifiersDown()) {
        if (IsControlOnlyDown()) {
            SendPasteWithHeldControl();
        } else {
            StartPasteTimer(target, target_focus);
        }
        return;
    }
    SendControlVPaste();
}

bool PasteController::PasteModifiersDown() {
    return IsKeyDown(VK_CONTROL) || IsKeyDown(VK_MENU) || IsKeyDown(VK_SHIFT);
}

void PasteController::StartPasteTimer(HWND target, HWND focus) {
    m_pendingPasteTarget = target;
    m_pendingPasteFocus = focus;
    m_pendingPasteDeadline = GetTickCount64() + 3000;
    ::SetTimer(m_owner, AppConstants::Timer::kPaste, 15, nullptr);
}

void PasteController::OnPasteTimer() {
    const HWND target = m_pendingPasteTarget;
    const HWND focus = m_pendingPasteFocus;
    if (GetForegroundWindow() != target || GetTickCount64() > m_pendingPasteDeadline) {
        StopPasteTimer();
    } else if (!PasteModifiersDown()) {
        StopPasteTimer();
        RestoreTargetFocusAndPaste(target, focus, true);
    }
}

void PasteController::StopPasteTimer() {
    if (m_owner != nullptr) {
        KillTimer(m_owner, AppConstants::Timer::kPaste);
    }
    m_pendingPasteTarget = nullptr;
    m_pendingPasteFocus = nullptr;
    m_pendingPasteDeadline = 0;
}
