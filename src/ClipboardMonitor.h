#pragma once

#include "PlatformConfig.h"

#include <cstdint>
#include <array>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <shellapi.h>

#include <utility>

#include "ClipboardData.h"
#include "Settings.h"

// Clipboard monitoring and capture component
class ClipboardMonitor {
public:
    using IgnoreLists = std::array<std::vector<std::wstring>, 3>;
    using SaveCallback = std::function<void(ClipboardSnapshot)>;

    ClipboardMonitor(AppSettings& settings, IgnoreLists ignored_lists);
    ~ClipboardMonitor();

    ClipboardMonitor(const ClipboardMonitor&) = delete;
    ClipboardMonitor& operator=(const ClipboardMonitor&) = delete;

    // Lifecycle
    bool Initialize(HWND owner);
    void Shutdown();

    // Clipboard operations
    void OnClipboardUpdate();
    void ReadClipboardAndSave();
    bool WriteClipboardItem(const ClipboardItem &item, bool remove_formatting);
    bool ClearClipboard();

    // Configuration
    void ReloadIgnoreLists(IgnoreLists ignored_lists);
    void SetSaveCallback(SaveCallback callback) { m_saveCallback = std::move(callback); }

private:
    // Clipboard capture
    std::optional<ClipboardSnapshot> CaptureClipboard() const;

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

    AppSettings& m_settings;
    SaveCallback m_saveCallback;
    HWND m_owner = nullptr;

    bool m_clipboardListenerAdded = false;
    std::optional<std::wstring> m_expectedClipboardFingerprint;

    std::vector<std::wstring> m_ignoredApps;
    std::vector<std::wstring> m_ignoredFormats;
    std::vector<std::wregex> m_ignoredRegexpPatterns;
};
