import 'dart:ffi';
import 'dart:ui';
import 'package:ffi/ffi.dart';
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
  /// 返回 [Offset] 光标坐标，失败时返回 (0, 0)。
  static Offset getCursorPosition() {
    final point = calloc<POINT>();
    try {
      final result = GetCursorPos(point);
      if (result == 0) return Offset.zero;

      return Offset(point.ref.x.toDouble(), point.ref.y.toDouble());
    } finally {
      free(point);
    }
  }

  /// 获取系统托盘图标的屏幕矩形区域。
  ///
  /// 用于实现 PopupPosition.statusItem 模式。
  ///
  /// 返回托盘图标的矩形区域，失败时返回 null。
  static Rect? getTrayIconRect() {
    // 查找托盘窗口
    final trayWnd = FindWindow('Shell_TrayWnd'.toNativeUtf16(), nullptr);
    if (trayWnd == 0) return null;

    final notifyWnd = FindWindowEx(
      trayWnd,
      0,
      'TrayNotifyWnd'.toNativeUtf16(),
      nullptr,
    );
    if (notifyWnd == 0) return null;

    final rect = calloc<RECT>();
    try {
      final result = GetWindowRect(notifyWnd, rect);
      if (result == 0) return null;

      return Rect.fromLTRB(
        rect.ref.left.toDouble(),
        rect.ref.top.toDouble(),
        rect.ref.right.toDouble(),
        rect.ref.bottom.toDouble(),
      );
    } finally {
      free(rect);
    }
  }

  /// 获取任务栏位置信息。
  ///
  /// 返回任务栏所在屏幕边缘：'bottom', 'top', 'left', 'right'。
  static String getTaskbarPosition() {
    final appBarData = calloc<APPBARDATA>();
    try {
      appBarData.ref.cbSize = sizeOf<APPBARDATA>();
      final result = SHAppBarMessage(ABM_GETTASKBARPOS, appBarData);

      if (result == 0) return 'bottom'; // 默认底部

      final edge = appBarData.ref.uEdge;
      switch (edge) {
        case ABE_LEFT:
          return 'left';
        case ABE_TOP:
          return 'top';
        case ABE_RIGHT:
          return 'right';
        case ABE_BOTTOM:
        default:
          return 'bottom';
      }
    } finally {
      free(appBarData);
    }
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
      return await screenRetriever.getPrimaryDisplay();
    }

    // 具体屏幕索引（1-based）
    if (screenIndex > 0 && screenIndex <= displays.length) {
      return displays[screenIndex - 1];
    }

    // 超出范围则返回主屏幕
    return await screenRetriever.getPrimaryDisplay();
  }

  /// 计算窗口弹出位置。
  ///
  /// [position] 弹出位置模式
  /// [screenIndex] 目标屏幕索引
  /// [windowSize] 窗口尺寸
  /// [lastPosition] 上次记录的位置（用于 lastPosition 模式）
  ///
  /// 返回 [Offset] 窗口左上角坐标。
  static Future<Offset> calculateWindowPosition({
    required PopupPosition position,
    required int screenIndex,
    required Size windowSize,
    Offset? lastPosition,
  }) async {
    final screen = await getTargetScreen(screenIndex);
    final screenRect = Rect.fromLTWH(
      screen.visiblePosition!.dx,
      screen.visiblePosition!.dy,
      screen.size.width,
      screen.size.height,
    );

    switch (position) {
      case PopupPosition.cursor:
        final cursorPos = getCursorPosition();

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

        return Offset(x, y);

      case PopupPosition.center:
        // 屏幕中心
        return Offset(
          screenRect.left + (screenRect.width - windowSize.width) / 2,
          screenRect.top + (screenRect.height - windowSize.height) / 2,
        );

      case PopupPosition.statusItem:
        final trayRect = getTrayIconRect();
        if (trayRect == null) {
          // 托盘区域获取失败，回退到屏幕右下角
          return Offset(
            screenRect.right - windowSize.width - 10,
            screenRect.bottom - windowSize.height - 50,
          );
        }

        final taskbarPos = getTaskbarPosition();

        switch (taskbarPos) {
          case 'bottom':
            // 托盘上方居中
            return Offset(
              trayRect.center.dx - windowSize.width / 2,
              trayRect.top - windowSize.height - 5,
            );
          case 'top':
            // 托盘下方居中
            return Offset(
              trayRect.center.dx - windowSize.width / 2,
              trayRect.bottom + 5,
            );
          case 'left':
            // 托盘右侧居中
            return Offset(
              trayRect.right + 5,
              trayRect.center.dy - windowSize.height / 2,
            );
          case 'right':
            // 托盘左侧居中
            return Offset(
              trayRect.left - windowSize.width - 5,
              trayRect.center.dy - windowSize.height / 2,
            );
          default:
            return Offset(
              screenRect.right - windowSize.width - 10,
              screenRect.bottom - windowSize.height - 50,
            );
        }

      case PopupPosition.lastPosition:
        if (lastPosition != null) {
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

          return Offset(x, y);
        }

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
        'bounds': Rect.fromLTWH(
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
