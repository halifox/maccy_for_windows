import 'package:flutter/material.dart';
import 'package:screen_retriever/screen_retriever.dart';

/// 弹窗位置枚举（对应 Maccy 的 PopupPosition）。
enum PopupPosition {
  cursor,        // 光标位置
  statusItem,    // 状态栏图标下方
  window,        // 活动窗口中心
  center,        // 屏幕中心
  lastPosition,  // 记住上次位置
}

/// 弹窗位置计算服务。
///
/// 完全基于 Maccy 的 PopupPosition.swift 实现。
class PopupPositionService {
  PopupPositionService._();

  /// 计算弹窗位置。
  ///
  /// [position] 位置模式
  /// [windowSize] 弹窗大小
  /// [lastPosition] 上次保存的位置（相对坐标，0.0-1.0）
  /// [screenIndex] 目标屏幕索引（0 = 活动屏幕，1+ = 具体屏幕）
  ///
  /// 返回弹窗的左上角坐标。
  static Future<Offset> calculatePosition({
    required PopupPosition position,
    required Size windowSize,
    Offset? lastPosition,
    int screenIndex = 0,
  }) async {
    switch (position) {
      case PopupPosition.center:
        return _calculateCenterPosition(windowSize, screenIndex);

      case PopupPosition.window:
        return _calculateWindowCenterPosition(windowSize);

      case PopupPosition.statusItem:
        return _calculateStatusItemPosition(windowSize);

      case PopupPosition.lastPosition:
        return _calculateLastPosition(windowSize, lastPosition, screenIndex);

      case PopupPosition.cursor:
        return _calculateCursorPosition(windowSize);
    }
  }

  /// 计算屏幕中心位置。
  static Future<Offset> _calculateCenterPosition(Size windowSize, int screenIndex) async {
    try {
      final displays = await screenRetriever.getAllDisplays();

      Display targetDisplay;
      if (screenIndex == 0) {
        // 活动屏幕（光标所在屏幕）
        targetDisplay = await screenRetriever.getPrimaryDisplay();
      } else if (screenIndex <= displays.length) {
        // 指定屏幕
        targetDisplay = displays[screenIndex - 1];
      } else {
        // 回退到主屏幕
        targetDisplay = await screenRetriever.getPrimaryDisplay();
      }

      final visibleFrame = targetDisplay.visibleSize ?? targetDisplay.size;
      final visiblePosition = targetDisplay.visiblePosition ?? Offset.zero;

      // 居中计算
      final x = visiblePosition.dx + (visibleFrame.width - windowSize.width) / 2;
      final y = visiblePosition.dy + (visibleFrame.height - windowSize.height) / 2;

      return Offset(x, y);
    } catch (e) {
      // 回退到光标位置
      return _calculateCursorPosition(windowSize);
    }
  }

  /// 计算活动窗口中心位置。
  static Future<Offset> _calculateWindowCenterPosition(Size windowSize) async {
    // Windows 平台暂不支持获取前台窗口位置
    // 回退到屏幕中心
    return _calculateCenterPosition(windowSize, 0);
  }

  /// 计算状态栏图标下方位置。
  static Future<Offset> _calculateStatusItemPosition(Size windowSize) async {
    try {
      final primaryDisplay = await screenRetriever.getPrimaryDisplay();
      final screenSize = primaryDisplay.size;

      // Windows 任务栏通常在底部，状态栏图标在右下角
      // 将弹窗放在屏幕右上角
      final x = screenSize.width - windowSize.width - 10;
      const y = 50.0; // 距离顶部 50px

      return Offset(x, y);
    } catch (e) {
      return _calculateCursorPosition(windowSize);
    }
  }

  /// 计算上次保存的位置。
  static Future<Offset> _calculateLastPosition(
    Size windowSize,
    Offset? lastPosition,
    int screenIndex,
  ) async {
    if (lastPosition == null) {
      return _calculateCursorPosition(windowSize);
    }

    try {
      final displays = await screenRetriever.getAllDisplays();

      Display targetDisplay;
      if (screenIndex == 0) {
        targetDisplay = await screenRetriever.getPrimaryDisplay();
      } else if (screenIndex <= displays.length) {
        targetDisplay = displays[screenIndex - 1];
      } else {
        targetDisplay = await screenRetriever.getPrimaryDisplay();
      }

      final visibleFrame = targetDisplay.visibleSize ?? targetDisplay.size;
      final visiblePosition = targetDisplay.visiblePosition ?? Offset.zero;

      // 将相对坐标（0.0-1.0）转换为绝对坐标
      // Maccy 的锚点是窗口顶部中心
      final anchorX = visiblePosition.dx + visibleFrame.width * lastPosition.dx;
      final anchorY = visiblePosition.dy + visibleFrame.height * lastPosition.dy;

      final x = anchorX - windowSize.width / 2;
      final y = anchorY - windowSize.height;

      return Offset(x, y);
    } catch (e) {
      return _calculateCursorPosition(windowSize);
    }
  }

  /// 计算光标位置。
  static Future<Offset> _calculateCursorPosition(Size windowSize) async {
    try {
      final cursorPosition = await screenRetriever.getCursorScreenPoint();

      // 弹窗显示在光标下方
      final x = cursorPosition.dx;
      final y = cursorPosition.dy;

      // 确保弹窗不超出屏幕边界
      final primaryDisplay = await screenRetriever.getPrimaryDisplay();
      final screenSize = primaryDisplay.size;

      final adjustedX = (x + windowSize.width > screenSize.width)
          ? screenSize.width - windowSize.width - 10
          : x;

      final adjustedY = (y + windowSize.height > screenSize.height)
          ? y - windowSize.height - 10
          : y;

      return Offset(adjustedX, adjustedY);
    } catch (e) {
      // 回退到屏幕中心
      return _calculateCenterPosition(windowSize, 0);
    }
  }

  /// 保存窗口位置（转换为相对坐标）。
  ///
  /// [absolutePosition] 窗口的绝对位置
  /// [windowSize] 窗口大小
  ///
  /// 返回相对坐标（0.0-1.0），用于下次恢复位置。
  static Future<Offset> savePosition(Offset absolutePosition, Size windowSize) async {
    try {
      final primaryDisplay = await screenRetriever.getPrimaryDisplay();
      final visibleFrame = primaryDisplay.visibleSize ?? primaryDisplay.size;
      final visiblePosition = primaryDisplay.visiblePosition ?? Offset.zero;

      // 计算锚点（窗口顶部中心）
      final anchorX = absolutePosition.dx + windowSize.width / 2;
      final anchorY = absolutePosition.dy + windowSize.height;

      // 转换为相对坐标
      final relativeX = (anchorX - visiblePosition.dx) / visibleFrame.width;
      final relativeY = (anchorY - visiblePosition.dy) / visibleFrame.height;

      return Offset(
        relativeX.clamp(0.0, 1.0),
        relativeY.clamp(0.0, 1.0),
      );
    } catch (e) {
      // 默认位置：屏幕中心偏上
      return const Offset(0.5, 0.8);
    }
  }

  /// 将 PopupPosition 枚举转换为字符串。
  static String positionToString(PopupPosition position) {
    switch (position) {
      case PopupPosition.cursor:
        return 'cursor';
      case PopupPosition.statusItem:
        return 'statusItem';
      case PopupPosition.window:
        return 'window';
      case PopupPosition.center:
        return 'center';
      case PopupPosition.lastPosition:
        return 'lastPosition';
    }
  }

  /// 将字符串转换为 PopupPosition 枚举。
  static PopupPosition stringToPosition(String position) {
    switch (position) {
      case 'cursor':
        return PopupPosition.cursor;
      case 'statusItem':
        return PopupPosition.statusItem;
      case 'window':
        return PopupPosition.window;
      case 'center':
        return PopupPosition.center;
      case 'lastPosition':
        return PopupPosition.lastPosition;
      default:
        return PopupPosition.cursor;
    }
  }
}
