#pragma once

#include "PlatformConfig.h"

#include <cstdint>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <shellapi.h>

#include "Database.h"
#include "Settings.h"

// Clipboard monitoring and capture component
class ClipboardMonitor {
public:
    ClipboardMonitor(Database& database, AppSettings& settings);
    ~ClipboardMonitor();

    ClipboardMonitor(const ClipboardMonitor&) = delete;
    ClipboardMonitor& operator=(const ClipboardMonitor&) = delete;

    // Lifecycle
    bool Initialize(HWND owner);
    void Shutdown();

    // Clipboard operations
    bool OnClipboardUpdate();
    bool ReadClipboardAndSave();
    bool SetClipboardItem(const ClipboardItem& item, bool remove_formatting);

    // Target window management
    void CaptureTargetWindow(HWND candidate = nullptr);
    void RestoreTargetFocusAndPaste(HWND target, HWND target_focus, bool paste);

    // Configuration
    void ReloadIgnoreLists();

    // State management
    void SetPasting(bool pasting) { m_pasting = pasting; }
    bool IsPasting() const { return m_pasting; }
    void SetSkipNextEvent(bool skip) { m_skipNextClipboardEvent = skip; }
    std::wstring GetLastCopyText() const { return m_lastCopyText; }
    void UpdateTrayTooltip(NOTIFYICONDATAW& notifyIcon, bool trayIconAdded) const;

    // Paste timer management
    void StartPasteTimer(HWND target, HWND focus);
    void OnPasteTimer(HWND mainWindow);
    void StopPasteTimer(HWND mainWindow);

    HWND GetTargetWindow() const { return m_targetWindow; }
    HWND GetTargetFocusWindow() const { return m_targetFocusWindow; }

private:
    // Clipboard capture
    std::optional<ClipboardCapture> CaptureClipboard() const;

    // Ignore rules
    bool ShouldIgnoreApplication(std::wstring_view application) const;
    bool ShouldIgnoreFormat(std::wstring_view format) const;
    bool ShouldIgnoreText(std::wstring_view text) const;

    // Clipboard data extraction
    static std::wstring ExtractClipboardText();
    static std::wstring ExtractClipboardAnsiText();
    static std::wstring FilesPreview(HGLOBAL data);
    static std::wstring HashCapture(const std::vector<ClipboardFormatData>& data);

    // Application identification
    static std::wstring GetSourceApplication();
    static bool MatchesApplication(std::wstring_view actual, std::wstring_view configured);

    // Paste helpers
    static bool PasteModifiersDown();

    Database& m_database;
    AppSettings& m_settings;
    HWND m_owner = nullptr;

    bool m_clipboardListenerAdded = false;
    bool m_pasting = false;
    bool m_skipNextClipboardEvent = false;

    std::vector<std::wstring> m_ignoredApps;
    std::vector<std::wstring> m_ignoredFormats;
    std::vector<std::wregex> m_ignoredRegexpPatterns;

    HWND m_targetWindow = nullptr;
    HWND m_targetFocusWindow = nullptr;
    RECT m_targetCaretRect{};
    bool m_hasTargetCaretRect = false;

    HWND m_pendingPasteTarget = nullptr;
    HWND m_pendingPasteFocus = nullptr;
    ULONGLONG m_pendingPasteDeadline = 0;

    std::wstring m_lastCopyText;
};
