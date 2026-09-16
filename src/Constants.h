#pragma once

#include "PlatformConfig.h"
#include <cstdint>

// ============================================================================
// 应用程序常量定义
// 集中管理所有常量，避免分散在各个文件中
// ============================================================================

namespace AppConstants {
    // 应用程序信息
    constexpr const wchar_t* kAppVersion = L"1.0.0";

    // ========================================================================
    // Windows 自定义消息常量
    // ========================================================================
    constexpr UINT kTrayIconMessage = WM_APP + 1;
    constexpr UINT kUiUpdateMessage = WM_APP + 2;

    // 预留消息ID空间，便于未来扩展
    constexpr UINT kCustomMessageBase = WM_APP + 100;

    // ========================================================================
    // 定时器ID
    // ========================================================================
    namespace Timer {
        constexpr UINT_PTR kSearch = 1;
        constexpr UINT_PTR kPreview = 2;
        constexpr UINT_PTR kPaste = 3;
    }

    // Bitmask carried by kUiUpdateMessage. Producers only describe what
    // changed; MainWindow decides the order and granularity of the work.
    namespace UiUpdate {
        constexpr std::uint32_t kSettings = 1u << 0;
        constexpr std::uint32_t kIgnoreRules = 1u << 1;
        constexpr std::uint32_t kHistory = 1u << 2;
        constexpr std::uint32_t kLayout = 1u << 3;
        constexpr std::uint32_t kTray = 1u << 4;
        constexpr std::uint32_t kFooter = 1u << 5;
    }

    // ========================================================================
    // 热键ID
    // ========================================================================
    namespace HotKey {
        constexpr int kOpenPopup = 1006;
        constexpr int kPinItem = 2;
        constexpr int kDeleteItem = 3;
        constexpr int kPreview = 4;
    }

    // ========================================================================
    // UI 尺寸常量
    // ========================================================================
    namespace UI {
        constexpr int kMinWindowWidth = 300;
        constexpr int kMinWindowHeight = 400;
        constexpr int kDefaultWindowWidth = 450;
        constexpr int kDefaultWindowHeight = 500;
        constexpr int kDefaultPreviewWidth = 450;
        constexpr int kDefaultImageMaxHeight = 40;
    }

    // ========================================================================
    // 设置窗口常量
    // ========================================================================
    namespace SettingsUI {
        constexpr int kPageCount = 6;
        constexpr int kIgnorePageCount = 3;
    }

    // ========================================================================
    // 数据库常量
    // ========================================================================
    namespace DB {
        constexpr int kDefaultHistorySize = 200;
        constexpr const wchar_t* kDatabaseFileName = L"maccy.db";
    }
}
