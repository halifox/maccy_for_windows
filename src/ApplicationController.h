#pragma once

#include "ClipboardMonitor.h"
#include "PreviewWorker.h"
#include "StorageWorker.h"
#include "UpdateChecker.h"

#include <cstdint>
#include <functional>
#include <vector>

class ApplicationController {
public:
    using UiUpdateCallback = std::function<void(std::uint32_t)>;
    using PopupVisibleCallback = std::function<bool()>;

    ApplicationController(StorageWorker &storage, PreviewWorker &preview,
                          AppSettings &settings);
    ~ApplicationController();

    ApplicationController(const ApplicationController &) = delete;
    ApplicationController &operator=(const ApplicationController &) = delete;

    void SetUiCallbacks(UiUpdateCallback request_update,
                        PopupVisibleCallback is_popup_visible);
    void AttachWindow(HWND window) noexcept;
    bool InitializeClipboard(HWND owner, StorageWorker::IgnoreLists ignored_lists);
    void StopUpdateChecks() noexcept;
    void Shutdown() noexcept;

    void HandleClipboardUpdate();
    bool WriteClipboardItem(const ClipboardItem &item, bool remove_formatting);
    bool ClearClipboard();
    void ReloadIgnoreLists(StorageWorker::IgnoreLists ignored_lists);

    bool StartUpdateCheck(UpdateCheckMode mode);
    bool IsUpdateChecking() const noexcept;
    std::vector<UpdateCheckResult> TakeUpdateResults();

    void DrainPreviewCallbacks();
    void DrainStorageCallbacks();

private:
    void SaveCapturedClipboard(ClipboardSnapshot capture);

    StorageWorker &m_storage;
    PreviewWorker &m_previewWorker;
    AppSettings &m_settings;
    ClipboardMonitor m_clipboard;
    UpdateChecker m_updateChecker;
    UiUpdateCallback m_requestUiUpdate;
    PopupVisibleCallback m_isPopupVisible;
    bool m_shutdown = false;
};
