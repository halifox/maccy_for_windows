import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:screen_retriever/screen_retriever.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:tray_manager/tray_manager.dart';

/// 窗口定位模式。
///
/// 对应 Maccy 的 PopupPosition 枚举。
enum PopupPosition {
  /// 跟随光标位置。
  cursor,

  /// 屏幕中心。
  center,

  /// 状态栏图标下方。
  statusItem,

  /// 记住上次位置。
  lastPosition,
}

/// 窗口定位服务。
///
/// 负责根据不同的定位模式计算窗口应该出现的位置，支持多屏幕环境。
class WindowPositionService {
  final SharedPreferences _prefs;
  final Size _windowSize;

  WindowPositionService(this._prefs, this._windowSize);

  /// 根据定位模式和屏幕索引计算窗口位置。
  ///
  /// [position] 定位模式。
  /// [screenIndex] 屏幕索引，0 表示活动屏幕（光标所在），1+ 表示具体屏幕编号。
  /// 返回窗口左上角的坐标。
  Future<Offset> calculatePosition(
    PopupPosition position, {
    int screenIndex = 0,
  }) async {
    switch (position) {
      case PopupPosition.cursor:
        return _positionNearCursor(screenIndex);

      case PopupPosition.center:
        return _positionAtCenter(screenIndex);

      case PopupPosition.statusItem:
        return _positionNearTray(screenIndex);

      case PopupPosition.lastPosition:
        return _positionAtLastLocation(screenIndex);
    }
  }

  /// 光标跟随模式。
  ///
  /// 将窗口定位在光标附近，自动处理屏幕边界，确保窗口不会超出可见区域。
  Future<Offset> _positionNearCursor(int screenIndex) async {
    final cursorPoint = await screenRetriever.getCursorScreenPoint();
    final targetDisplay = await _getTargetDisplay(screenIndex, cursorPoint);

    final visiblePos = targetDisplay.visiblePosition ?? Offset.zero;
    final visibleSize = targetDisplay.visibleSize ?? targetDisplay.size;

    // 默认在光标右下方偏移 10px
    double x = cursorPoint.dx + 10;
    double y = cursorPoint.dy + 10;

    // 边界检测：如果右侧空间不足，则显示在光标左侧
    if (x + _windowSize.width > visiblePos.dx + visibleSize.width) {
      x = cursorPoint.dx - _windowSize.width - 10;
    }

    // 边界检测：如果下方空间不足，则显示在光标上方
    if (y + _windowSize.height > visiblePos.dy + visibleSize.height) {
      y = cursorPoint.dy - _windowSize.height - 10;
    }

    // 最终边界限制
    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    return Offset(x, y);
  }

  /// 屏幕中心模式。
  ///
  /// 将窗口居中显示在指定屏幕上。
  Future<Offset> _positionAtCenter(int screenIndex) async {
    final targetDisplay = await _getTargetDisplay(screenIndex);

    final visiblePos = targetDisplay.visiblePosition ?? Offset.zero;
    final visibleSize = targetDisplay.visibleSize ?? targetDisplay.size;

    final x = visiblePos.dx + (visibleSize.width - _windowSize.width) / 2;
    final y = visiblePos.dy + (visibleSize.height - _windowSize.height) / 2;

    return Offset(x, y);
  }

  /// 状态栏图标模式。
  ///
  /// 将窗口定位在系统托盘图标附近，根据任务栏位置自动调整对齐方式。
  Future<Offset> _positionNearTray(int screenIndex) async {
    final trayBounds = await trayManager.getBounds();

    // 如果无法获取托盘位置，回退到光标模式
    if (trayBounds == null) {
      return _positionNearCursor(screenIndex);
    }

    final targetDisplay = await _getTargetDisplay(
      screenIndex,
      trayBounds.center,
    );

    final visiblePos = targetDisplay.visiblePosition ?? Offset.zero;
    final visibleSize = targetDisplay.visibleSize ?? targetDisplay.size;
    final screenWidth = targetDisplay.size.width;
    final screenHeight = targetDisplay.size.height;
    final screenX = targetDisplay.visiblePosition?.dx ?? 0;
    final screenY = targetDisplay.visiblePosition?.dy ?? 0;

    double x = trayBounds.center.dx - _windowSize.width / 2;
    double y = trayBounds.center.dy - _windowSize.height / 2;

    // 检测任务栏位置并调整窗口位置
    // 底部任务栏（Windows 默认）
    if (trayBounds.top > screenY + screenHeight * 0.8) {
      y = trayBounds.top - _windowSize.height - 10;
    }
    // 顶部任务栏
    else if (trayBounds.bottom < screenY + screenHeight * 0.2) {
      y = trayBounds.bottom + 10;
    }
    // 右侧任务栏
    else if (trayBounds.left > screenX + screenWidth * 0.8) {
      x = trayBounds.left - _windowSize.width - 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    }
    // 左侧任务栏
    else if (trayBounds.right < screenX + screenWidth * 0.2) {
      x = trayBounds.right + 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    }

    // 边界限制
    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    return Offset(x, y);
  }

  /// 记住位置模式。
  ///
  /// 从 SharedPreferences 读取上次保存的窗口位置，如果不存在则回退到中心模式。
  Future<Offset> _positionAtLastLocation(int screenIndex) async {
    final savedX = _prefs.getDouble('lastWindowPositionX');
    final savedY = _prefs.getDouble('lastWindowPositionY');

    if (savedX != null && savedY != null) {
      // 验证保存的位置是否仍在有效屏幕范围内
      final displays = await screenRetriever.getAllDisplays();
      for (final display in displays) {
        final rect = Rect.fromLTWH(
          display.visiblePosition?.dx ?? 0,
          display.visiblePosition?.dy ?? 0,
          display.visibleSize?.width ?? display.size.width,
          display.visibleSize?.height ?? display.size.height,
        );

        if (rect.contains(Offset(savedX, savedY))) {
          return Offset(savedX, savedY);
        }
      }
    }

    // 如果没有保存的位置或位置无效，回退到中心模式
    return _positionAtCenter(screenIndex);
  }

  /// 保存当前窗口位置。
  ///
  /// 在 lastPosition 模式下，窗口移动后应调用此方法保存位置。
  Future<void> saveCurrentPosition(Offset position) async {
    await _prefs.setDouble('lastWindowPositionX', position.dx);
    await _prefs.setDouble('lastWindowPositionY', position.dy);
  }

  /// 获取目标显示器。
  ///
  /// [screenIndex] 0 = 活动屏幕（光标或参考点所在），1+ = 具体屏幕编号。
  /// [referencePoint] 参考点，用于确定活动屏幕。
  Future<Display> _getTargetDisplay(
    int screenIndex, [
    Offset? referencePoint,
  ]) async {
    final displays = await screenRetriever.getAllDisplays();

    if (displays.isEmpty) {
      throw Exception('No displays found');
    }

    // screenIndex = 0: 活动屏幕（光标所在或参考点所在）
    if (screenIndex == 0) {
      final point = referencePoint ??
          await screenRetriever.getCursorScreenPoint();

      for (final display in displays) {
        final rect = Rect.fromLTWH(
          display.visiblePosition?.dx ?? 0,
          display.visiblePosition?.dy ?? 0,
          display.visibleSize?.width ?? display.size.width,
          display.visibleSize?.height ?? display.size.height,
        );

        if (rect.contains(point)) {
          return display;
        }
      }

      // 如果没有找到包含参考点的屏幕，返回主屏幕
      return await screenRetriever.getPrimaryDisplay();
    }

    // screenIndex >= 1: 具体屏幕编号
    if (screenIndex <= displays.length) {
      return displays[screenIndex - 1];
    }

    // 超出范围，返回主屏幕
    return await screenRetriever.getPrimaryDisplay();
  }
}
