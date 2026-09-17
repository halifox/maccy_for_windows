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
    bool WriteClipboardItem(const ClipboardItem &item, bool remove_formatting);

    // Configuration
    void ReloadIgnoreLists();

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

    Database& m_database;
    AppSettings& m_settings;
    HWND m_owner = nullptr;

    bool m_clipboardListenerAdded = false;
    bool m_skipNextClipboardEvent = false;

    std::vector<std::wstring> m_ignoredApps;
    std::vector<std::wstring> m_ignoredFormats;
    std::vector<std::wregex> m_ignoredRegexpPatterns;
};
