#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include <filesystem>
#include <stdexcept>

#include "Database.h"

CAppModule _Module;

namespace {

constexpr UINT kTrayIconMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCommandShow = 1001;
constexpr UINT kTrayCommandExit = 1002;

std::filesystem::path GetDatabasePath() {
    PWSTR local_app_data = nullptr;
    const HRESULT result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,
        KF_FLAG_DEFAULT,
        nullptr,
        &local_app_data
    );
    if (FAILED(result)) {
        throw std::runtime_error("Unable to locate the LocalAppData folder");
    }

    const std::filesystem::path path =
        std::filesystem::path(local_app_data) / L"LowMemApp" / L"lowmem.db";
    CoTaskMemFree(local_app_data);
    return path;
}

} // namespace

class MainWindow : public CWindowImpl<MainWindow> {
public:
    DECLARE_WND_CLASS_EX(L"LowMemAppWindow", CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(kTrayIconMessage, OnTrayIcon)
    END_MSG_MAP()

    bool AddTrayIcon() {
        m_notifyIcon = {};
        m_notifyIcon.cbSize = sizeof(m_notifyIcon);
        m_notifyIcon.hWnd = m_hWnd;
        m_notifyIcon.uID = kTrayIconId;
        m_notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_notifyIcon.uCallbackMessage = kTrayIconMessage;
        m_notifyIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        lstrcpynW(m_notifyIcon.szTip, L"LowMemApp", ARRAYSIZE(m_notifyIcon.szTip));

        m_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &m_notifyIcon) == TRUE;
        return m_trayIconAdded;
    }

    void ShowMainWindow() {
        if (::IsIconic(m_hWnd)) {
            ShowWindow(SW_RESTORE);
        } else {
            ShowWindow(SW_SHOW);
        }

        SetForegroundWindow(m_hWnd);
        UpdateWindow();
    }

private:
    void RemoveTrayIcon() {
        if (m_trayIconAdded) {
            Shell_NotifyIconW(NIM_DELETE, &m_notifyIcon);
            m_trayIconAdded = false;
        }
    }

    void ShowTrayMenu() {
        HMENU menu = CreatePopupMenu();
        if (menu == nullptr) {
            return;
        }

        m_trayMenuShowing = true;

        AppendMenuW(menu, MF_STRING, kTrayCommandShow, L"\u6253\u5f00\u754c\u9762");
        AppendMenuW(menu, MF_STRING, kTrayCommandExit, L"\u9000\u51fa");

        POINT cursor{};
        GetCursorPos(&cursor);
        SetForegroundWindow(m_hWnd);
        TrackPopupMenu(
            menu,
            TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            cursor.x,
            cursor.y,
            0,
            m_hWnd,
            nullptr
        );

        // Required so the menu is dismissed correctly when the user clicks away.
        ::PostMessageW(m_hWnd, WM_NULL, 0, 0);
        m_trayMenuShowing = false;
        DestroyMenu(menu);
    }

    void ExitApplication() {
        m_exiting = true;
        RemoveTrayIcon();
        DestroyWindow();
    }

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL &) {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(&ps);

        RECT rc{};
        GetClientRect(&rc);

        DrawTextW(
            dc,
            L"\u6258\u76d8\u5e94\u7528\n\n\u70b9\u51fb\u53f3\u4e0b\u89d2\u6258\u76d8\u56fe\u6807\u53ef\u91cd\u65b0\u6253\u5f00\u6b64\u754c\u9762\u3002\n\u70b9\u51fb\u7a97\u53e3\u5916\u90e8\u4f1a\u81ea\u52a8\u9690\u85cf\u3002\n\u5173\u95ed\u7a97\u53e3\u53ea\u4f1a\u9690\u85cf\u754c\u9762\uff0c\u53f3\u952e\u6258\u76d8\u56fe\u6807\u5e76\u9009\u62e9\u201c\u9000\u51fa\u201d\u624d\u80fd\u7ed3\u675f\u5e94\u7528\u3002",
            -1,
            &rc,
            DT_CENTER | DT_VCENTER | DT_WORDBREAK
        );

        EndPaint(&ps);
        return 0;
    }

    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM, BOOL &) {
        // Treat the main window like a transient popup: clicking another
        // window hides it while keeping the tray icon and process alive.
        if (LOWORD(wParam) == WA_INACTIVE && !m_exiting && !m_trayMenuShowing) {
            ShowWindow(SW_HIDE);
        }
        return 0;
    }

    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &) {
        if (m_exiting) {
            DestroyWindow();
        } else {
            ShowWindow(SW_HIDE);
        }
        return 0;
    }

    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        switch (LOWORD(wParam)) {
        case kTrayCommandShow:
            handled = TRUE;
            ShowMainWindow();
            break;
        case kTrayCommandExit:
            handled = TRUE;
            ExitApplication();
            break;
        default:
            handled = FALSE;
            break;
        }
        return 0;
    }

    LRESULT OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
        if (wParam != kTrayIconId) {
            return 0;
        }

        switch (static_cast<UINT>(lParam)) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            ShowMainWindow();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            break;
        default:
            break;
        }
        return 0;
    }

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &) {
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    NOTIFYICONDATAW m_notifyIcon{};
    bool m_trayIconAdded = false;
    bool m_exiting = false;
    bool m_trayMenuShowing = false;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    if (FAILED(_Module.Init(nullptr, instance))) {
        return 1;
    }

    try {
        Database database(GetDatabasePath());
        MainWindow window;

        if (!window.Create(nullptr, CWindow::rcDefault, L"LowMemApp", WS_OVERLAPPEDWINDOW)) {
            _Module.Term();
            return 1;
        }

        // A successful tray registration means the application starts resident in the tray.
        // If Explorer is unavailable, keep a visible window as a usable fallback.
        if (!window.AddTrayIcon()) {
            window.ShowWindow(show);
            window.UpdateWindow();
        }

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        _Module.Term();
        return static_cast<int>(msg.wParam);
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), "Database error", MB_OK | MB_ICONERROR);
        _Module.Term();
        return 1;
    }
}
