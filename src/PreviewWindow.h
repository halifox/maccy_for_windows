#pragma once

#define NOMINMAX
#include <windows.h>

#include <atlbase.h>
#include <atlwin.h>

#include <string>

#include "Database.h"
#include "resource.h"

class PreviewWindow : public CDialogImpl<PreviewWindow> {
public:
    enum { IDD = IDD_PREVIEW };

    BEGIN_MSG_MAP(PreviewWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnControlColor)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnControlColor)
    END_MSG_MAP()

    bool Initialize(HWND owner, int width = 450);
    void SetWidth(int width);
    void SetItem(const ClipboardItem &item);
    void Hide();
    bool IsVisible() const noexcept;
    int MinimumHeight() const noexcept;
    bool ContainsWindow(HWND window) const noexcept;
    HWND Window() const noexcept { return m_hWnd; }

private:
    void ClearBitmap();
    void LayoutControls();
    bool LoadBitmapForItem(const ClipboardItem &item);
    void UpdateStatus(const ClipboardItem &item);

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnControlColor(UINT, WPARAM, LPARAM, BOOL &handled);

    HWND m_image = nullptr;
    HWND m_text = nullptr;
    HWND m_status = nullptr;
    HBITMAP m_bitmap = nullptr;
    HFONT m_font = nullptr;
    int m_bitmapWidth = 0;
    int m_bitmapHeight = 0;
    int m_width = 450;
};
