import 'dart:ffi';
import 'dart:ui' as ui;
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:screen_retriever/screen_retriever.dart';
import 'package:win32/win32.dart';

/// 窗口弹出位置枚举。
///
/// 对应 Maccy 的 PopupPosition 配置。
enum PopupPosition {
  cursor,       // 跟随光标位置
  center,       // 屏幕中心
  statusItem,   // 系统托盘图标下方
  lastPosition, // 记住上次位置
}

/// 多屏幕支持服务。
///
/// 提供屏幕检测、光标位置获取、窗口定位计算等功能。
/// 对应 Maccy 的 NSScreen+ForPopup.swift 实现。
class ScreenService {
  /// 获取光标当前位置（全局坐标）。
  ///
  /// 使用 Win32 GetCursorPos API。
  ///
  /// 返回 [ui.Offset] 光标坐标（逻辑坐标），失败时返回 (0, 0)。
  static ui.Offset getCursorPosition() {
    final point = calloc<POINT>();
    try {
      final result = GetCursorPos(point);
      if (result == 0) return ui.Offset.zero;

      // GetCursorPos 返回的是物理像素坐标，需要转换为逻辑坐标
      // 获取光标所在位置的 DPI
      final hMonitor = MonitorFromPoint(point.ref, MONITOR_DEFAULTTONEAREST);
      var dpiX = calloc<UINT>();
      var dpiY = calloc<UINT>();

      try {
        // 尝试获取监视器的 DPI
        final hr = GetDpiForMonitor(hMonitor, 0, dpiX, dpiY); // 0 = MDT_EFFECTIVE_DPI
        final scaleFactor = hr == 0 ? dpiX.value / 96.0 : 1.0;

        final logicalPos = ui.Offset(
          point.ref.x.toDouble() / scaleFactor,
          point.ref.y.toDouble() / scaleFactor,
        );

        debugPrint('[ScreenService] 光标物理坐标: (${point.ref.x}, ${point.ref.y}), DPI: ${dpiX.value}, 缩放: $scaleFactor, 逻辑坐标: $logicalPos');
        return logicalPos;
      } finally {
        free(dpiX);
        free(dpiY);
      }
    } finally {
      free(point);
    }
  }

  /// 获取系统托盘图标的屏幕矩形区域。
  ///
  /// 用于实现 PopupPosition.statusItem 模式。
  ///
  /// 返回托盘图标的矩形区域（已转换为逻辑坐标），失败时返回 null。
  static ui.Rect? getTrayIconRect() {
    debugPrint('[ScreenService] ========== 获取托盘图标位置 ==========');

    // 查找托盘窗口
    final trayWnd = FindWindow('Shell_TrayWnd'.toNativeUtf16(), nullptr);
    debugPrint('[ScreenService] Shell_TrayWnd 句柄: $trayWnd');
    if (trayWnd == 0) {
      debugPrint('[ScreenService] ❌ 未找到 Shell_TrayWnd');
      return null;
    }

    final notifyWnd = FindWindowEx(
      trayWnd,
      0,
      'TrayNotifyWnd'.toNativeUtf16(),
      nullptr,
    );
    debugPrint('[ScreenService] TrayNotifyWnd 句柄: $notifyWnd');
    if (notifyWnd == 0) {
      debugPrint('[ScreenService] ❌ 未找到 TrayNotifyWnd');
      return null;
    }

    final rect = calloc<RECT>();
    try {
      final result = GetWindowRect(notifyWnd, rect);
      debugPrint('[ScreenService] GetWindowRect 返回值: $result');
      if (result == 0) {
        debugPrint('[ScreenService] ❌ GetWindowRect 失败');
        return null;
      }

      // Win32 API 返回的是物理像素坐标，需要转换为逻辑坐标
      // 获取 DPI 缩放比例
      final dpi = GetDpiForWindow(notifyWnd);
      final scaleFactor = dpi / 96.0; // 96 DPI 是 100% 缩放
      debugPrint('[ScreenService] 窗口 DPI: $dpi, 缩放比例: $scaleFactor');

      final trayRect = ui.Rect.fromLTRB(
        rect.ref.left.toDouble() / scaleFactor,
        rect.ref.top.toDouble() / scaleFactor,
        rect.ref.right.toDouble() / scaleFactor,
        rect.ref.bottom.toDouble() / scaleFactor,
      );
      debugPrint('[ScreenService] 物理坐标: (${rect.ref.left}, ${rect.ref.top}, ${rect.ref.right}, ${rect.ref.bottom})');
      debugPrint('[ScreenService] ✓ 逻辑坐标托盘区域: $trayRect');
      return trayRect;
    } finally {
      free(rect);
    }
  }

  /// 获取任务栏位置信息。
  ///
  /// 返回任务栏所在屏幕边缘：'bottom', 'top', 'left', 'right'。
  static String getTaskbarPosition() {
    return 'bottom'; // 默认底部
  }

  /// 根据屏幕索引获取目标显示器。
  ///
  /// [screenIndex] 屏幕索引：
  /// - 0: 活动屏幕（光标所在屏幕）
  /// - 1+: 具体屏幕编号
  ///
  /// 返回 [Display] 目标显示器对象。
  static Future<Display> getTargetScreen(int screenIndex) async {
    final displays = await screenRetriever.getAllDisplays();

    if (displays.isEmpty) {
      throw Exception('未检测到任何显示器');
    }

    // 0 = 活动屏幕（光标所在）
    if (screenIndex == 0) {
      final cursorPos = getCursorPosition();

      // 查找包含光标的屏幕
      for (final display in displays) {
        final visiblePos = display.visiblePosition!;
        final size = display.size;

        if (cursorPos.dx >= visiblePos.dx &&
            cursorPos.dx < visiblePos.dx + size.width &&
            cursorPos.dy >= visiblePos.dy &&
            cursorPos.dy < visiblePos.dy + size.height) {
          return display;
        }
      }

      // 未找到则返回主屏幕
      return screenRetriever.getPrimaryDisplay();
    }

    // 具体屏幕索引（1-based）
    if (screenIndex > 0 && screenIndex <= displays.length) {
      return displays[screenIndex - 1];
    }

    // 超出范围则返回主屏幕
    return screenRetriever.getPrimaryDisplay();
  }

  /// 计算窗口弹出位置。
  ///
  /// [position] 弹出位置模式
  /// [screenIndex] 目标屏幕索引
  /// [windowSize] 窗口尺寸
  /// [lastPosition] 上次记录的位置（用于 lastPosition 模式）
  ///
  /// 返回 [ui.Offset] 窗口左上角坐标。
  static Future<ui.Offset> calculateWindowPosition({
    required PopupPosition position,
    required int screenIndex,
    required ui.Size windowSize,
    ui.Offset? lastPosition,
  }) async {
    debugPrint('[ScreenService] ========== 计算窗口位置 ==========');
    debugPrint('[ScreenService] 位置模式: $position');
    debugPrint('[ScreenService] 屏幕索引: $screenIndex');
    debugPrint('[ScreenService] 窗口尺寸: $windowSize');
    debugPrint('[ScreenService] 上次位置: $lastPosition');

    final screen = await getTargetScreen(screenIndex);
    debugPrint('[ScreenService] 目标屏幕: ${screen.size}');

    final screenRect = ui.Rect.fromLTWH(
      screen.visiblePosition!.dx,
      screen.visiblePosition!.dy,
      screen.size.width,
      screen.size.height,
    );
    debugPrint('[ScreenService] 屏幕可见区域: $screenRect');

    switch (position) {
      case PopupPosition.cursor:
        debugPrint('[ScreenService] 使用光标位置模式');
        final cursorPos = getCursorPosition();
        debugPrint('[ScreenService] 光标位置: $cursorPos');

        // 光标下方 10px，确保不超出屏幕边界
        var x = cursorPos.dx;
        var y = cursorPos.dy + 10;

        // 右边界检查
        if (x + windowSize.width > screenRect.right) {
          x = screenRect.right - windowSize.width;
        }

        // 底边界检查
        if (y + windowSize.height > screenRect.bottom) {
          y = cursorPos.dy - windowSize.height - 10; // 光标上方
        }

        // 左边界检查
        if (x < screenRect.left) {
          x = screenRect.left;
        }

        // 顶边界检查
        if (y < screenRect.top) {
          y = screenRect.top;
        }

        final result = ui.Offset(x, y);
        debugPrint('[ScreenService] ✓ 计算结果: $result');
        return result;

      case PopupPosition.center:
        debugPrint('[ScreenService] 使用屏幕中心模式');
        // 屏幕中心
        final result = ui.Offset(
          screenRect.left + (screenRect.width - windowSize.width) / 2,
          screenRect.top + (screenRect.height - windowSize.height) / 2,
        );
        debugPrint('[ScreenService] ✓ 计算结果: $result');
        return result;

      case PopupPosition.statusItem:
        debugPrint('[ScreenService] 使用托盘图标模式');
        final trayRect = getTrayIconRect();
        if (trayRect == null) {
          debugPrint('[ScreenService] ⚠️ 托盘区域获取失败，回退到屏幕右下角');
          // 托盘区域获取失败，回退到屏幕右下角
          final fallback = ui.Offset(
            screenRect.right - windowSize.width - 10,
            screenRect.bottom - windowSize.height - 50,
          );
          debugPrint('[ScreenService] 回退位置: $fallback');
          return fallback;
        }

        final taskbarPos = getTaskbarPosition();
        debugPrint('[ScreenService] 任务栏位置: $taskbarPos');

        ui.Offset result;
        switch (taskbarPos) {
          case 'bottom':
            // 托盘上方居中
            result = ui.Offset(
              trayRect.center.dx - windowSize.width / 2,
              trayRect.top - windowSize.height - 5,
            );
            debugPrint('[ScreenService] 托盘上方居中: $result');
            break;
          case 'top':
            // 托盘下方居中
            result = ui.Offset(
              trayRect.center.dx - windowSize.width / 2,
              trayRect.bottom + 5,
            );
            debugPrint('[ScreenService] 托盘下方居中: $result');
            break;
          case 'left':
            // 托盘右侧居中
            result = ui.Offset(
              trayRect.right + 5,
              trayRect.center.dy - windowSize.height / 2,
            );
            debugPrint('[ScreenService] 托盘右侧居中: $result');
            break;
          case 'right':
            // 托盘左侧居中
            result = ui.Offset(
              trayRect.left - windowSize.width - 5,
              trayRect.center.dy - windowSize.height / 2,
            );
            debugPrint('[ScreenService] 托盘左侧居中: $result');
            break;
          default:
            result = ui.Offset(
              screenRect.right - windowSize.width - 10,
              screenRect.bottom - windowSize.height - 50,
            );
            debugPrint('[ScreenService] 默认位置: $result');
            break;
        }
        debugPrint('[ScreenService] ✓ 计算结果: $result');
        return result;

      case PopupPosition.lastPosition:
        debugPrint('[ScreenService] 使用上次位置模式');
        if (lastPosition != null) {
          debugPrint('[ScreenService] 上次位置存在: $lastPosition');
          // 确保上次位置仍在屏幕范围内
          var x = lastPosition.dx;
          var y = lastPosition.dy;

          if (x + windowSize.width > screenRect.right) {
            x = screenRect.right - windowSize.width;
          }
          if (y + windowSize.height > screenRect.bottom) {
            y = screenRect.bottom - windowSize.height;
          }
          if (x < screenRect.left) {
            x = screenRect.left;
          }
          if (y < screenRect.top) {
            y = screenRect.top;
          }

          final result = ui.Offset(x, y);
          debugPrint('[ScreenService] ✓ 计算结果: $result');
          return result;
        }

        debugPrint('[ScreenService] 无上次位置记录，回退到屏幕中心');
        // 无上次位置记录，回退到屏幕中心
        return calculateWindowPosition(
          position: PopupPosition.center,
          screenIndex: screenIndex,
          windowSize: windowSize,
        );
    }
  }

  /// 获取所有显示器信息（用于设置界面）。
  ///
  /// 返回显示器列表，每个元素包含：
  /// - name: 显示器名称
  /// - isPrimary: 是否为主显示器
  /// - bounds: 屏幕边界
  static Future<List<Map<String, dynamic>>> getAllScreensInfo() async {
    final displays = await screenRetriever.getAllDisplays();
    final primary = await screenRetriever.getPrimaryDisplay();

    return displays.asMap().entries.map((entry) {
      final index = entry.key;
      final display = entry.value;

      return {
        'index': index + 1,
        'name': 'Display ${index + 1}',
        'isPrimary': display.id == primary.id,
        'bounds': ui.Rect.fromLTWH(
          display.visiblePosition!.dx,
          display.visiblePosition!.dy,
          display.size.width,
          display.size.height,
        ),
        'scaleFactor': display.scaleFactor,
      };
    }).toList();
  }
}
