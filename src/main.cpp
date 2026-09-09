#include <windows.h>
#include <atlbase.h>
#include <atlapp.h>

CAppModule _Module;

#include <atlwin.h>

class MainWindow : public CWindowImpl<MainWindow> {
public:
    DECLARE_WND_CLASS_EX(L"LowMemAppWindow", CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(&ps);

        RECT rc{};
        GetClientRect(&rc);

        DrawTextW(
            dc,
            L"Hello WTL",
            -1,
            &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );

        EndPaint(&ps);
        return 0;
    }

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
        PostQuitMessage(0);
        return 0;
    }
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    _Module.Init(nullptr, instance);

    MainWindow window;

    if (!window.Create(nullptr, CWindow::rcDefault, L"LowMemApp")) {
        return 1;
    }

    window.ShowWindow(show);
    window.UpdateWindow();

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    _Module.Term();
    return static_cast<int>(msg.wParam);
}