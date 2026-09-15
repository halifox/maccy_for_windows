#pragma once

#include <string>
#include <vector>
#include <windows.h>

class Database;

// 忽略子页面基类
class IgnorePageBase {
public:
    virtual ~IgnorePageBase() = default;

    virtual void Initialize(HWND page_window, Database &database) = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Refresh() = 0;
    virtual bool AddValue() = 0;
    virtual bool EditValue() = 0;
    virtual bool RemoveValue() = 0;
    virtual bool ResetToDefaults() = 0;
    virtual bool SaveList() = 0;

    HWND GetPageWindow() const { return m_pageWindow; }

protected:
    HWND m_pageWindow = nullptr;
    HWND m_list = nullptr;
    HWND m_description = nullptr;
    Database *m_database = nullptr;
    std::vector<std::wstring> m_values;
};

// 应用程序忽略页面
class IgnoreApplicationsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void LoadList();
    void UpdateDescription();
};

// Pasteboard 类型忽略页面
class IgnoreFormatsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void LoadList();
    void UpdateDescription();
    HWND m_whitelist = nullptr;
};

// 正则表达式忽略页面
class IgnoreRegexpsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void LoadList();
    void UpdateDescription();
};
