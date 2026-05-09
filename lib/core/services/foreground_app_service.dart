import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'package:win32/win32.dart';

/// Windows 前台应用识别服务。
///
/// 使用 Win32 API 获取当前活动窗口的进程信息，用于记录剪贴板内容的来源应用。
/// 这对应 Maccy 的 `NSWorkspace.shared.frontmostApplication` 功能。
class ForegroundAppService {
  /// 获取当前前台应用的可执行文件名（不含路径和扩展名）。
  ///
  /// 实现逻辑：
  /// 1. GetForegroundWindow() - 获取前台窗口句柄
  /// 2. GetWindowThreadProcessId() - 获取窗口所属进程 ID
  /// 3. OpenProcess() - 打开进程句柄
  /// 4. QueryFullProcessImageName() - 查询进程完整路径
  /// 5. 提取文件名并移除 .exe 扩展名
  ///
  /// 返回应用名称，失败时返回 null。
  ///
  /// 示例返回值：
  /// - "chrome" (Google Chrome)
  /// - "Code" (VS Code)
  /// - "explorer" (Windows Explorer)
  static String? getForegroundAppName() {
    // 获取前台窗口句柄
    final hwnd = GetForegroundWindow();
    if (hwnd == 0) return null;

    final processId = calloc<DWORD>();
    try {
      // 获取窗口所属进程 ID
      GetWindowThreadProcessId(hwnd, processId);

      // 打开进程句柄（需要查询权限）
      final hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId.value,
      );

      if (hProcess == 0) return null;

      try {
        final exePath = calloc<WCHAR>(MAX_PATH);
        try {
          final size = calloc<DWORD>();
          try {
            size.value = MAX_PATH;

            // 查询进程完整路径
            final result = QueryFullProcessImageName(hProcess, 0, exePath, size);
            if (result == 0) return null;

            final path = exePath.toDartString();

            // 提取文件名：C:\Program Files\App\app.exe -> app
            final fileName = path.split('\\').last;
            return fileName.toLowerCase().replaceAll('.exe', '');
          } finally {
            free(size);
          }
        } finally {
          free(exePath);
        }
      } finally {
        CloseHandle(hProcess);
      }
    } finally {
      free(processId);
    }
  }

  /// 获取前台应用的窗口标题。
  ///
  /// 用于更详细的应用识别（例如区分不同的浏览器标签页）。
  ///
  /// 返回窗口标题，失败时返回 null。
  static String? getForegroundWindowTitle() {
    final hwnd = GetForegroundWindow();
    if (hwnd == 0) return null;

    final titleBuffer = calloc<WCHAR>(256);
    try {
      final length = GetWindowText(hwnd, titleBuffer, 256);
      if (length == 0) return null;

      return titleBuffer.toDartString();
    } finally {
      free(titleBuffer);
    }
  }

  /// 获取前台应用的完整信息（应用名 + 窗口标题）。
  ///
  /// 返回格式：{appName: "chrome", windowTitle: "Google - Chrome"}
  static Map<String, String>? getForegroundAppInfo() {
    final appName = getForegroundAppName();
    final windowTitle = getForegroundWindowTitle();

    if (appName == null) return null;

    return {
      'appName': appName,
      'windowTitle': windowTitle ?? '',
    };
  }
}
