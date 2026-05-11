import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:win32/win32.dart';

/// 应用标识服务
///
/// 用于获取当前前台应用的信息，实现 Maccy 的应用过滤功能。
/// 在 Windows 上通过 win32 API 获取进程信息。
class AppIdentifierService {
  /// 获取当前前台应用的标识符
  ///
  /// 返回应用的可执行文件名（不含 .exe 扩展名）
  /// 例如: "chrome", "notepad", "code"
  String? getForegroundAppIdentifier() {
    try {
      // 获取前台窗口句柄
      final hwnd = GetForegroundWindow();
      if (hwnd == 0) return null;

      // 获取窗口所属进程 ID
      final processId = calloc<DWORD>();
      GetWindowThreadProcessId(hwnd, processId);

      // 打开进程句柄
      final hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId.value,
      );

      if (hProcess == 0) {
        calloc.free(processId);
        return null;
      }

      // 获取进程可执行文件路径
      final exePath = calloc<Uint16>(MAX_PATH);
      final length = GetModuleFileNameEx(hProcess, 0, exePath.cast(), MAX_PATH);

      String? appName;
      if (length > 0) {
        final path = exePath.cast<Utf16>().toDartString();
        // 提取文件名并移除 .exe 扩展名
        appName = path.split(r'\').last.replaceAll('.exe', '').toLowerCase();
      }

      // 清理资源
      CloseHandle(hProcess);
      calloc.free(processId);
      calloc.free(exePath);

      return appName;
    } catch (e) {
      debugPrint(e.toString());
      return null;
    }
  }

  /// 获取前台应用的完整路径
  String? getForegroundAppPath() {
    try {
      final hwnd = GetForegroundWindow();
      if (hwnd == 0) return null;

      final processId = calloc<DWORD>();
      GetWindowThreadProcessId(hwnd, processId);

      final hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId.value,
      );

      if (hProcess == 0) {
        calloc.free(processId);
        return null;
      }

      final exePath = calloc<Uint16>(MAX_PATH);
      final length = GetModuleFileNameEx(hProcess, 0, exePath.cast(), MAX_PATH);

      String? path;
      if (length > 0) {
        path = exePath.cast<Utf16>().toDartString();
      }

      CloseHandle(hProcess);
      calloc.free(processId);
      calloc.free(exePath);

      return path;
    } catch (e) {
      return null;
    }
  }

  /// 获取前台窗口标题
  String? getForegroundWindowTitle() {
    try {
      final hwnd = GetForegroundWindow();
      if (hwnd == 0) return null;

      final titleBuffer = calloc<Uint16>(256);
      final length = GetWindowText(hwnd, titleBuffer.cast(), 256);

      String? title;
      if (length > 0) {
        title = titleBuffer.cast<Utf16>().toDartString();
      }

      calloc.free(titleBuffer);
      return title;
    } catch (e) {
      return null;
    }
  }

  /// 检查应用是否为系统应用
  ///
  /// 系统应用通常不应该被过滤
  bool isSystemApp(String? appIdentifier) {
    if (appIdentifier == null) return false;

    const systemApps = [
      'explorer',
      'dwm',
      'csrss',
      'winlogon',
      'services',
      'lsass',
      'svchost',
      'taskmgr',
      'systemsettings',
    ];

    return systemApps.contains(appIdentifier.toLowerCase());
  }

  /// 获取应用的友好名称
  ///
  /// 将可执行文件名转换为更友好的显示名称
  String getFriendlyName(String appIdentifier) {
    final friendlyNames = {
      'chrome': 'Google Chrome',
      'firefox': 'Mozilla Firefox',
      'msedge': 'Microsoft Edge',
      'code': 'Visual Studio Code',
      'notepad': 'Notepad',
      'notepad++': 'Notepad++',
      'explorer': 'File Explorer',
      'cmd': 'Command Prompt',
      'powershell': 'PowerShell',
      'wt': 'Windows Terminal',
      'slack': 'Slack',
      'discord': 'Discord',
      'teams': 'Microsoft Teams',
      'outlook': 'Microsoft Outlook',
      'excel': 'Microsoft Excel',
      'word': 'Microsoft Word',
      'powerpoint': 'Microsoft PowerPoint',
    };

    return friendlyNames[appIdentifier.toLowerCase()] ?? appIdentifier;
  }
}
