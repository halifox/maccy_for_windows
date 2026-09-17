#include "PlatformConfig.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
#include <atlapp.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "MainWindow.h"
#include "ClipboardAgent.h"
#include "Constants.h"
#include "DatabaseActor.h"
#include "PreviewWorker.h"

CAppModule _Module;

namespace {

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
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        return 1;
    }
    if (FAILED(_Module.Init(nullptr, instance))) {
        CoUninitialize();
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
        _Module.Term();
        CoUninitialize();
        const std::wstring message = L"InitCommonControlsEx failed: " + std::to_wstring(error);
        MessageBoxW(nullptr, message.c_str(), L"maccy error", MB_OK | MB_ICONERROR);
        return 1;
    }

    try {
        DatabaseActor database(GetDatabasePath());
        DatabaseInitialState initial_state = database.Start();
        ClipboardAgent clipboard(
            [&database](ClipboardSnapshot snapshot) {
                return database.PostClipboardSnapshot(std::move(snapshot));
            },
            [&database](const ClipboardAgentSettings &settings) {
                database.Post([settings](DatabaseContext &context) {
                    context.settings.ignore_events = settings.ignore_events;
                    context.settings.ignore_only_next_event = settings.ignore_only_next_event;
                    context.settings.Save(context.database);
                });
            }
        );
        clipboard.Start(
            ClipboardAgentSettings::FromAppSettings(initial_state.settings),
            initial_state.ignored_lists
        );
        PreviewWorker preview(database.Path());
        preview.Start();
        MainWindow window(database, clipboard, preview, std::move(initial_state));
        if (!window.Create(nullptr)) {
            _Module.Term();
            CoUninitialize();
            return 1;
        }

        if (!window.AddTrayIcon()) {
            window.ShowMainWindow();
        }

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        _Module.Term();
        CoUninitialize();
        return static_cast<int>(message.wParam);
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), "maccy error", MB_OK | MB_ICONERROR);
        _Module.Term();
        CoUninitialize();
        return 1;
    }
}
