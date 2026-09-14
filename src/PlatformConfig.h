#pragma once

// ============================================================================
// Windows 平台配置
// 这个文件应该被所有需要 Windows API 的头文件包含
// 确保平台配置的一致性
// ============================================================================

// 禁用 Windows.h 中的 min/max 宏，避免与 std::min/max 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 减少 Windows.h 包含的内容，加快编译速度
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Windows 头文件
#include <windows.h>

// 编译器特定配置
#ifdef _MSC_VER
    // 禁用特定警告（根据项目需要调整）
    #pragma warning(push)
    #pragma warning(disable: 4505)  // 未引用的局部函数已移除
#endif

// 调试宏
#ifdef _DEBUG
    #define DEBUG_LOG(msg) OutputDebugStringW(msg)
    #define DEBUG_LOGF(fmt, ...) { \
        wchar_t buf[512]; \
        swprintf_s(buf, fmt, __VA_ARGS__); \
        OutputDebugStringW(buf); \
    }
#else
    #define DEBUG_LOG(msg) ((void)0)
    #define DEBUG_LOGF(fmt, ...) ((void)0)
#endif
