#include "PlatformConfig.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "MainWindow.h"
#include "Constants.h"
#include "PreviewWorker.h"
#include "StorageWorker.h"

CAppModule _Module;

namespace {

constexpr wchar_t kInstanceMutexName[] = L"Local\\org.maccy.windows.ClipboardManager";
constexpr wchar_t kActivationWindowClassName[] = L"Maccy.SingleInstance.ActivationWindow";
constexpr UINT kActivateExistingInstanceMessage = WM_APP + 7;
constexpr int kActivationWindowWaitAttempts = 200;
constexpr DWORD kActivationWindowWaitIntervalMs = 25;

MainWindow *g_mainWindow = nullptr;
bool g_activationRequested = false;
bool g_installerShutdownRequested = false;

class SingleInstanceMutex {
public:
    ~SingleInstanceMutex() {
        Close();
    }

    bool Create(bool &alreadyRunning, DWORD &error) noexcept {
        alreadyRunning = false;
        error = ERROR_SUCCESS;
        m_handle = ::CreateMutexW(nullptr, FALSE, kInstanceMutexName);
        if (m_handle == nullptr) {
            error = ::GetLastError();
            return false;
        }

        switch (TryAcquire(error)) {
        case AcquireResult::Acquired:
            return true;
        case AcquireResult::Busy:
            alreadyRunning = true;
            return true;
        case AcquireResult::Failed:
            return false;
        }
        return false;
    }

    enum class AcquireResult {
        Acquired,
        Busy,
        Failed
    };

    AcquireResult TryAcquire(DWORD &error) noexcept {
        const DWORD result = ::WaitForSingleObject(m_handle, 0);
        if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED) {
            m_owned = true;
            error = ERROR_SUCCESS;
            return AcquireResult::Acquired;
        }
        if (result == WAIT_TIMEOUT) {
            error = ERROR_SUCCESS;
            return AcquireResult::Busy;
        }
        error = ::GetLastError();
        return AcquireResult::Failed;
    }

    void Release() noexcept {
        Close();
    }

private:
    void Close() noexcept {
        if (m_owned && m_handle != nullptr) {
            ::ReleaseMutex(m_handle);
            m_owned = false;
        }
        if (m_handle != nullptr) {
            ::CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }

    HANDLE m_handle = nullptr;
    bool m_owned = false;
};

class ActivationWindow : public CWindowImpl<ActivationWindow> {
public:
    DECLARE_WND_CLASS_EX(_T("Maccy.SingleInstance.ActivationWindow"), 0, COLOR_WINDOW)

    BEGIN_MSG_MAP(ActivationWindow)
        MESSAGE_HANDLER(kActivateExistingInstanceMessage, OnActivateExistingInstance)
        MESSAGE_HANDLER(AppConstants::kInstallerShutdownMessage, OnInstallerShutdown)
    END_MSG_MAP()

    ~ActivationWindow() {
        Destroy();
    }

    bool Create(HINSTANCE instance, DWORD &error) noexcept {
        (void)instance;
        if (CWindowImpl<ActivationWindow>::Create(HWND_MESSAGE) == nullptr) {
            error = ::GetLastError();
            return false;
        }
        error = ERROR_SUCCESS;
        return true;
    }

    void Destroy() noexcept {
        if (IsWindow()) {
            DestroyWindow();
        }
    }

private:
    LRESULT OnActivateExistingInstance(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        if (g_mainWindow != nullptr && ::IsWindow(g_mainWindow->Window())) {
            g_mainWindow->ShowMainWindow();
        } else {
            g_activationRequested = true;
        }
        return 0;
    }

    LRESULT OnInstallerShutdown(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        if (g_mainWindow != nullptr && ::IsWindow(g_mainWindow->Window())) {
            g_mainWindow->ExitForInstaller();
        } else {
            g_installerShutdownRequested = true;
        }
        return 1;
    }
};

enum class ExistingInstanceResult {
    Forwarded,
    AlreadyRunning,
    BecamePrimary,
    Failed
};

ExistingInstanceResult ForwardActivationOrTakeOwnership(
    SingleInstanceMutex &instance_mutex,
    DWORD &error
) noexcept {
    for (int attempt = 0; attempt < kActivationWindowWaitAttempts; ++attempt) {
        const HWND activation_window = ::FindWindowExW(
            HWND_MESSAGE,
            nullptr,
            kActivationWindowClassName,
            nullptr
        );
        if (activation_window != nullptr) {
            DWORD primary_process_id = 0;
            if (::GetWindowThreadProcessId(activation_window, &primary_process_id) != 0) {
                ::AllowSetForegroundWindow(primary_process_id);
            }
            if (::PostMessageW(activation_window, kActivateExistingInstanceMessage, 0, 0)) {
                return ExistingInstanceResult::Forwarded;
            }

            const auto ownership = instance_mutex.TryAcquire(error);
            if (ownership == SingleInstanceMutex::AcquireResult::Acquired) {
                return ExistingInstanceResult::BecamePrimary;
            }
            if (ownership == SingleInstanceMutex::AcquireResult::Failed) {
                return ExistingInstanceResult::Failed;
            }
            return ExistingInstanceResult::AlreadyRunning;
        }

        const auto ownership = instance_mutex.TryAcquire(error);
        if (ownership == SingleInstanceMutex::AcquireResult::Acquired) {
            return ExistingInstanceResult::BecamePrimary;
        }
        if (ownership == SingleInstanceMutex::AcquireResult::Failed) {
            return ExistingInstanceResult::Failed;
        }
        ::Sleep(kActivationWindowWaitIntervalMs);
    }
    return ExistingInstanceResult::AlreadyRunning;
}

std::filesystem::path GetDatabasePath() {
    PWSTR local_app_data = nullptr;
    const HRESULT result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,
        KF_FLAG_DEFAULT,
        nullptr,
        &local_app_data
    );
    if (FAILED(result) || local_app_data == nullptr) {
        throw std::runtime_error("Unable to locate LocalAppData");
    }
    std::filesystem::path path(local_app_data);
    CoTaskMemFree(local_app_data);
    path /= L"maccy";
    std::filesystem::create_directories(path);
    path /= AppConstants::DB::kDatabaseFileName;
    return path;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SingleInstanceMutex instance_mutex;
    bool already_running = false;
    DWORD instance_error = ERROR_SUCCESS;
    if (!instance_mutex.Create(already_running, instance_error)) {
        const std::wstring message = L"Unable to establish application instance: " +
            std::to_wstring(instance_error);
        MessageBoxW(nullptr, message.c_str(), L"maccy error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (already_running) {
        const auto result = ForwardActivationOrTakeOwnership(instance_mutex, instance_error);
        if (result == ExistingInstanceResult::Forwarded ||
            result == ExistingInstanceResult::AlreadyRunning) {
            return 0;
        }
        if (result == ExistingInstanceResult::Failed) {
            const std::wstring message = L"Unable to contact the running application: " +
                std::to_wstring(instance_error);
            MessageBoxW(nullptr, message.c_str(), L"maccy error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        const std::wstring message = L"COM 初始化失败。HRESULT：" +
            std::to_wstring(static_cast<unsigned long>(com_result));
        MessageBoxW(nullptr, message.c_str(), L"maccy 启动失败", MB_OK | MB_ICONERROR);
        return 1;
    }
    const HRESULT module_result = _Module.Init(nullptr, instance);
    if (FAILED(module_result)) {
        CoUninitialize();
        const std::wstring message = L"WTL 模块初始化失败。HRESULT：" +
            std::to_wstring(static_cast<unsigned long>(module_result));
        MessageBoxW(nullptr, message.c_str(), L"maccy 启动失败", MB_OK | MB_ICONERROR);
        return 1;
    }

    ActivationWindow activation_window;
    if (!activation_window.Create(instance, instance_error)) {
        const std::wstring message = L"Unable to create application activation window: " +
            std::to_wstring(instance_error);
        _Module.Term();
        CoUninitialize();
        MessageBoxW(nullptr, message.c_str(), L"maccy error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // The application uses only the Tab and HotKey common controls.  The
    // standard controls (buttons, edits, list boxes, etc.) are provided by
    // USER32 and do not need to be included here.  In particular, asking
    // older comctl32 builds to initialize ICC_STANDARD_CLASSES can make
    // InitCommonControlsEx return FALSE and trip WTL's debug assertion.
    INITCOMMONCONTROLSEX common_controls{
        sizeof(INITCOMMONCONTROLSEX),
        ICC_TAB_CLASSES | ICC_HOTKEY_CLASS
    };
    if (!::InitCommonControlsEx(&common_controls)) {
        const DWORD error = GetLastError();
        activation_window.Destroy();
        _Module.Term();
        CoUninitialize();
        const std::wstring message = L"InitCommonControlsEx failed: " + std::to_wstring(error);
        MessageBoxW(nullptr, message.c_str(), L"maccy error", MB_OK | MB_ICONERROR);
        return 1;
    }

    try {
        StorageWorker storage(GetDatabasePath());
        storage.Start();
        AppSettings settings = storage.LoadSettings();
        if (!SetLaunchAtLogin(settings.launch_at_login)) {
            ::OutputDebugStringW(L"Unable to synchronize the Maccy logon startup entry.\n");
        }
        StorageWorker::IgnoreLists ignored_lists = storage.LoadIgnoreLists();
        PreviewWorker preview(storage.Path());
        MainWindow window(storage, preview, std::move(settings), std::move(ignored_lists));
        if (!window.Create(nullptr)) {
            const DWORD error = GetLastError();
            storage.Stop();
            activation_window.Destroy();
            _Module.Term();
            CoUninitialize();
            const std::wstring message = L"无法创建主窗口。Windows 错误代码：" +
                std::to_wstring(error);
            MessageBoxW(nullptr, message.c_str(), L"maccy 启动失败", MB_OK | MB_ICONERROR);
            return 1;
        }
        if (!window.IsInitialized()) {
            const std::wstring message = window.InitializationError().empty()
                ? L"主窗口初始化未完成。"
                : window.InitializationError();
            window.DestroyWindow();
            storage.Stop();
            activation_window.Destroy();
            _Module.Term();
            CoUninitialize();
            MessageBoxW(nullptr, message.c_str(), L"maccy 启动失败", MB_OK | MB_ICONERROR);
            return 1;
        }
        g_mainWindow = &window;
        preview.Start(window.Window());

        if (!window.AddTrayIcon()) {
            window.ShowMainWindow();
        }

        if (g_activationRequested) {
            g_activationRequested = false;
            window.ShowMainWindow();
        }
        if (g_installerShutdownRequested) {
            g_installerShutdownRequested = false;
            window.ExitForInstaller();
        }

        CMessageLoop messageLoop;
        _Module.AddMessageLoop(&messageLoop);
        const int exitCode = messageLoop.Run();
        _Module.RemoveMessageLoop();
        g_mainWindow = nullptr;
        preview.Stop();
        storage.Stop();
        activation_window.Destroy();
        instance_mutex.Release();
        _Module.Term();
        CoUninitialize();
        return exitCode;
    } catch (const std::exception &error) {
        g_mainWindow = nullptr;
        MessageBoxA(nullptr, error.what(), "maccy error", MB_OK | MB_ICONERROR);
        activation_window.Destroy();
        _Module.Term();
        CoUninitialize();
        return 1;
    }
}
