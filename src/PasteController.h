#pragma once

#include "PlatformConfig.h"

#include <cstdint>

class PasteController {
public:
    PasteController() = default;

    PasteController(const PasteController &) = delete;
    PasteController &operator=(const PasteController &) = delete;

    void SetOwner(HWND owner) noexcept { m_owner = owner; }

    void CaptureTargetWindow(HWND candidate = nullptr);
    void RestoreTargetFocusAndPaste(HWND target, HWND target_focus, bool paste);

    void StartPasteTimer(HWND target, HWND focus);
    void OnPasteTimer();
    void StopPasteTimer();

    HWND GetTargetWindow() const noexcept { return m_targetWindow; }
    HWND GetTargetFocusWindow() const noexcept { return m_targetFocusWindow; }

private:
    static bool PasteModifiersDown();

    HWND m_owner = nullptr;
    HWND m_targetWindow = nullptr;
    HWND m_targetFocusWindow = nullptr;
    HWND m_pendingPasteTarget = nullptr;
    HWND m_pendingPasteFocus = nullptr;
    ULONGLONG m_pendingPasteDeadline = 0;
};
