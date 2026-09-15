#include "IgnorePages.h"
#include "Database.h"
#include "resource.h"
#include "SettingsWindow.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

namespace {

int SelectedListViewItem(HWND list) {
    return list == nullptr ? -1 : ListView_GetNextItem(list, -1, LVNI_SELECTED);
}

void SetListViewColumnWidth(HWND list, int width) {
    if (list != nullptr) {
        ListView_SetColumnWidth(list, 0, width);
    }
}

} // namespace

// EditIgnoreDialog 的前向声明（已在 SettingsWindow.cpp 中定义）
class EditIgnoreDialog;

// ===== IgnoreApplicationsPage =====

void IgnoreApplicationsPage::Initialize(HWND page_window, Database &database) {
    m_pageWindow = page_window;
    m_database = &database;
    m_list = ::GetDlgItem(page_window, IDC_I_LIST);
    m_description = ::GetDlgItem(page_window, IDC_I_DESCRIPTION);

    if (m_list != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        ListView_InsertColumn(m_list, 0, &column);
    }

    LoadList();
    UpdateDescription();
}

void IgnoreApplicationsPage::Show() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_SHOW);
        Refresh();
    }
}

void IgnoreApplicationsPage::Hide() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_HIDE);
    }
}

void IgnoreApplicationsPage::Refresh() {
    LoadList();

    if (m_list == nullptr) {
        return;
    }

    ListView_DeleteAllItems(m_list);
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        ListView_InsertItem(m_list, &row);
    }

    RECT list_rect{};
    ::GetClientRect(m_list, &list_rect);
    SetListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreApplicationsPage::AddValue() {
    std::array<wchar_t, MAX_PATH> path{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = m_pageWindow;
    dialog.lpstrFilter = L"Windows application (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) {
        return false;
    }

    std::wstring value = path.data();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    // 应用程序页面可以使用文件选择对话框来编辑路径
    std::array<wchar_t, MAX_PATH> path{};
    wcscpy_s(path.data(), path.size(), m_values[selected].c_str());

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = m_pageWindow;
    dialog.lpstrFilter = L"Windows application (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) {
        return false;
    }

    std::wstring value = path.data();
    if (value.empty()) {
        return false;
    }

    // 检查重复
    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow, L"该应用程序已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::ResetToDefaults() {
    // 应用程序页面没有默认值
    return false;
}

bool IgnoreApplicationsPage::SaveList() {
    if (m_database == nullptr) {
        return false;
    }

    try {
        m_database->ReplaceList(DatabaseList::IgnoredApplications, m_values);
        return true;
    } catch (...) {
        return false;
    }
}

void IgnoreApplicationsPage::LoadList() {
    if (m_database != nullptr) {
        m_values = m_database->GetList(DatabaseList::IgnoredApplications);
    }
}

void IgnoreApplicationsPage::UpdateDescription() {
    if (m_description != nullptr) {
        ::SetWindowTextW(
            m_description,
            L"忽略来自特定应用的内容。\r\n请注意此选项并非总是有效，最好使用忽略剪贴板类型设置。"
        );
    }
}

// ===== IgnoreFormatsPage =====

void IgnoreFormatsPage::Initialize(HWND page_window, Database &database) {
    m_pageWindow = page_window;
    m_database = &database;
    m_list = ::GetDlgItem(page_window, IDC_I_LIST);
    m_description = ::GetDlgItem(page_window, IDC_I_DESCRIPTION);
    m_whitelist = ::GetDlgItem(page_window, IDC_I_WHITELIST);

    if (m_list != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        ListView_InsertColumn(m_list, 0, &column);
    }

    LoadList();
    UpdateDescription();
}

void IgnoreFormatsPage::Show() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_SHOW);
        Refresh();
    }
}

void IgnoreFormatsPage::Hide() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_HIDE);
    }
}

void IgnoreFormatsPage::Refresh() {
    LoadList();

    if (m_list == nullptr) {
        return;
    }

    ListView_DeleteAllItems(m_list);
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        ListView_InsertItem(m_list, &row);
    }

    RECT list_rect{};
    ::GetClientRect(m_list, &list_rect);
    SetListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreFormatsPage::AddValue() {
    EditIgnoreDialog dialog(
        L"",
        L"输入要忽略的 pasteboard 类型（例如：com.example.custom）。",
        1
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();

    // 选中新添加的项
    const int inserted = static_cast<int>(m_values.size() - 1);
    if (m_list != nullptr && inserted >= 0) {
        ListView_SetItemState(m_list, inserted, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    return saved;
}

bool IgnoreFormatsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    EditIgnoreDialog dialog(
        m_values[selected],
        L"编辑要忽略的 pasteboard 类型（例如：com.example.custom）。",
        1
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty()) {
        return false;
    }

    // 检查重复
    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow, L"该值已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreFormatsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreFormatsPage::ResetToDefaults() {
    if (m_database != nullptr) {
        try {
            m_database->ResetIgnoredFormats();
            Refresh();
            return true;
        } catch (...) {
        }
    }
    return false;
}

bool IgnoreFormatsPage::SaveList() {
    if (m_database == nullptr) {
        return false;
    }

    try {
        m_database->ReplaceList(DatabaseList::IgnoredFormats, m_values);
        return true;
    } catch (...) {
        return false;
    }
}

void IgnoreFormatsPage::LoadList() {
    if (m_database != nullptr) {
        m_values = m_database->GetList(DatabaseList::IgnoredFormats);
    }
}

void IgnoreFormatsPage::UpdateDescription() {
    if (m_description != nullptr) {
        ::SetWindowTextW(
            m_description,
            L"忽略特定剪贴板内容类型。\r\n默认提供了一些已知的适用于特定应用的类型。您可以删除预置类型，或根据需要添加自定义类型。"
        );
    }
}

// ===== IgnoreRegexpsPage =====

void IgnoreRegexpsPage::Initialize(HWND page_window, Database &database) {
    m_pageWindow = page_window;
    m_database = &database;
    m_list = ::GetDlgItem(page_window, IDC_I_LIST);
    m_description = ::GetDlgItem(page_window, IDC_I_DESCRIPTION);

    if (m_list != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        ListView_InsertColumn(m_list, 0, &column);
    }

    LoadList();
    UpdateDescription();
}

void IgnoreRegexpsPage::Show() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_SHOW);
        Refresh();
    }
}

void IgnoreRegexpsPage::Hide() {
    if (m_pageWindow != nullptr) {
        ::ShowWindow(m_pageWindow, SW_HIDE);
    }
}

void IgnoreRegexpsPage::Refresh() {
    LoadList();

    if (m_list == nullptr) {
        return;
    }

    ListView_DeleteAllItems(m_list);
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        ListView_InsertItem(m_list, &row);
    }

    RECT list_rect{};
    ::GetClientRect(m_list, &list_rect);
    SetListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreRegexpsPage::AddValue() {
    EditIgnoreDialog dialog(
        L"",
        L"输入正则表达式以忽略匹配的内容（例如：^[a-zA-Z0-9]{50}$）。",
        2
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();

    // 选中新添加的项
    const int inserted = static_cast<int>(m_values.size() - 1);
    if (m_list != nullptr && inserted >= 0) {
        ListView_SetItemState(m_list, inserted, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    return saved;
}

bool IgnoreRegexpsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    EditIgnoreDialog dialog(
        m_values[selected],
        L"编辑正则表达式以忽略匹配的内容（例如：^[a-zA-Z0-9]{50}$）。",
        2
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty()) {
        return false;
    }

    // 检查重复
    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow, L"该值已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreRegexpsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreRegexpsPage::ResetToDefaults() {
    // 正则表达式页面没有默认值
    return false;
}

bool IgnoreRegexpsPage::SaveList() {
    if (m_database == nullptr) {
        return false;
    }

    try {
        m_database->ReplaceList(DatabaseList::IgnoredRegexps, m_values);
        return true;
    } catch (...) {
        return false;
    }
}

void IgnoreRegexpsPage::LoadList() {
    if (m_database != nullptr) {
        m_values = m_database->GetList(DatabaseList::IgnoredRegexps);
    }
}

void IgnoreRegexpsPage::UpdateDescription() {
    if (m_description != nullptr) {
        ::SetWindowTextW(
            m_description,
            L"可以根据定义的正则表达式忽略某些副本。"
        );
    }
}
