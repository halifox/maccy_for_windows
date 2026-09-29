#pragma once

#include "PlatformConfig.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <atlwin.h>

#include <string>

#include "PreviewDecoder.h"
#include "resource.h"

class PreviewWindow : public CDialogImpl<PreviewWindow> {
public:
    enum { IDD = IDD_PREVIEW };

    BEGIN_MSG_MAP(PreviewWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_PREVIEW_PIN, BN_CLICKED, OnCommand)
        COMMAND_HANDLER(IDC_PREVIEW_DELETE, BN_CLICKED, OnCommand)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnControlColor)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnControlColor)
    END_MSG_MAP()

    bool Initialize(HWND owner);
    void SetItem(const ClipboardItem &item, std::wstring text, PreviewBitmap bitmap = {});
    void GetImageSize(UINT &width, UINT &height) const noexcept;
    void Hide();
    bool IsVisible() const noexcept;
    HWND Window() const noexcept { return m_hWnd; }

private:
    void ClearBitmap();
    bool UpdateFont(UINT dpi);
    void UpdateStatus(const ClipboardItem &item);

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(WORD, WORD id, HWND, BOOL &handled);
    LRESULT OnControlColor(UINT, WPARAM, LPARAM, BOOL &handled);

    CStatic m_image;
    CEdit m_text;
    CStatic m_status;
    CButton m_pinButton;
    CButton m_deleteButton;
    CBitmap m_bitmap;
    CFont m_font;
    int m_bitmapWidth = 0;
    int m_bitmapHeight = 0;
    sqlite3_int64 m_itemId = 0;
};
