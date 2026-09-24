#include "ApplicationController.h"

#include "Constants.h"

#include <exception>
#include <string>
#include <utility>

ApplicationController::ApplicationController(
    StorageWorker &storage,
    PreviewWorker &preview,
    AppSettings &settings
)
    : m_storage(storage),
      m_previewWorker(preview),
      m_settings(settings),
      m_clipboard(settings, {}) {
    m_clipboard.SetSaveCallback(
        [this](ClipboardSnapshot capture) {
            SaveCapturedClipboard(std::move(capture));
        }
    );
}

ApplicationController::~ApplicationController() {
    Shutdown();
}

void ApplicationController::SetUiCallbacks(
    UiUpdateCallback request_update,
    PopupVisibleCallback is_popup_visible
) {
    m_requestUiUpdate = std::move(request_update);
    m_isPopupVisible = std::move(is_popup_visible);
}

void ApplicationController::AttachWindow(HWND window) noexcept {
    m_updateChecker.SetWindow(window);
    m_storage.SetUiWindow(window);
}

bool ApplicationController::InitializeClipboard(
    HWND owner,
    StorageWorker::IgnoreLists ignored_lists
) {
    if (!m_clipboard.Initialize(owner)) {
        return false;
    }
    m_clipboard.ReloadIgnoreLists(std::move(ignored_lists));
    return true;
}

void ApplicationController::StopUpdateChecks() noexcept {
    m_updateChecker.Stop();
}

void ApplicationController::Shutdown() noexcept {
    if (m_shutdown) {
        return;
    }
    m_shutdown = true;

    StopUpdateChecks();
    m_previewWorker.SetUiWindow(nullptr);
    m_clipboard.Shutdown();
    m_clipboard.SetSaveCallback({});
    m_storage.SetUiWindow(nullptr);
    m_updateChecker.SetWindow(nullptr);
    m_requestUiUpdate = {};
    m_isPopupVisible = {};
}

void ApplicationController::SaveCapturedClipboard(ClipboardSnapshot capture) {
    m_storage.SaveClipboardAsync(
        std::move(capture),
        m_settings.history_size,
        [this](bool success, std::string error) {
            if (!success) {
                ::OutputDebugStringA(error.c_str());
                ::OutputDebugStringA("\n");
                return;
            }
            if (m_requestUiUpdate) {
                const bool popup_visible = m_isPopupVisible && m_isPopupVisible();
                m_requestUiUpdate(
                    AppConstants::UiUpdate::kTray |
                    (popup_visible ? AppConstants::UiUpdate::kHistory : 0)
                );
            }
        }
    );
}

void ApplicationController::HandleClipboardUpdate() {
    const bool previous_ignore_events = m_settings.ignore_events;
    const bool previous_ignore_only = m_settings.ignore_only_next_event;
    try {
        m_clipboard.OnClipboardUpdate();
    } catch (const std::exception &error) {
        ::OutputDebugStringA(error.what());
        ::OutputDebugStringA("\n");
    } catch (...) {
        ::OutputDebugStringA("Unhandled clipboard update exception\n");
    }

    if (previous_ignore_events != m_settings.ignore_events ||
        previous_ignore_only != m_settings.ignore_only_next_event) {
        m_storage.SaveSettings(m_settings);
        if (m_requestUiUpdate) {
            m_requestUiUpdate(AppConstants::UiUpdate::kFooter | AppConstants::UiUpdate::kTray);
        }
    }
}

bool ApplicationController::WriteClipboardItem(
    const ClipboardItem &item,
    bool remove_formatting
) {
    return m_clipboard.WriteClipboardItem(item, remove_formatting);
}

bool ApplicationController::ClearClipboard() {
    return m_clipboard.ClearClipboard();
}

void ApplicationController::ReloadIgnoreLists(StorageWorker::IgnoreLists ignored_lists) {
    m_clipboard.ReloadIgnoreLists(std::move(ignored_lists));
}

bool ApplicationController::StartUpdateCheck(UpdateCheckMode mode) {
    return m_updateChecker.Start(mode);
}

bool ApplicationController::IsUpdateChecking() const noexcept {
    return m_updateChecker.IsChecking();
}

std::vector<UpdateCheckResult> ApplicationController::TakeUpdateResults() {
    return m_updateChecker.TakeResults();
}

void ApplicationController::DrainPreviewCallbacks() {
    m_previewWorker.DrainUiCallbacks();
}

void ApplicationController::DrainStorageCallbacks() {
    m_storage.DrainUiCallbacks();
}
