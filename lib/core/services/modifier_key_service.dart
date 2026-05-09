import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:win32/win32.dart';

/// 修饰键枚举。
enum ModifierKey {
  ctrl,
  shift,
  alt,
}

/// 修饰键监听服务。
///
/// 使用 Windows GetAsyncKeyState API 轮询检测修饰键（Ctrl, Shift, Alt）的按下和释放状态。
/// 这是实现 Maccy 循环选择模式的关键组件。
class ModifierKeyService {
  Timer? _pollTimer;
  bool _isMonitoring = false;

  // 当前修饰键状态
  bool _ctrlPressed = false;
  bool _shiftPressed = false;
  bool _altPressed = false;

  // 回调函数
  void Function()? _onAllModifiersReleased;
  void Function(Set<ModifierKey>)? _onModifierStateChanged;

  /// 开始监听修饰键状态。
  ///
  /// [pollInterval] 轮询间隔，默认 50ms。
  /// [onAllModifiersReleased] 所有修饰键释放时的回调。
  /// [onModifierStateChanged] 修饰键状态变化时的回调。
  void startMonitoring({
    Duration pollInterval = const Duration(milliseconds: 50),
    void Function()? onAllModifiersReleased,
    void Function(Set<ModifierKey>)? onModifierStateChanged,
  }) {
    if (!Platform.isWindows) {
      return;
    }

    if (_isMonitoring) {
      return;
    }

    _onAllModifiersReleased = onAllModifiersReleased;
    _onModifierStateChanged = onModifierStateChanged;
    _isMonitoring = true;

    _pollTimer = Timer.periodic(pollInterval, (_) {
      _checkModifierKeys();
    });
  }

  /// 停止监听修饰键状态。
  void stopMonitoring() {
    _pollTimer?.cancel();
    _pollTimer = null;
    _isMonitoring = false;
    _ctrlPressed = false;
    _shiftPressed = false;
    _altPressed = false;
  }

  /// 检查修饰键状态。
  void _checkModifierKeys() {
    if (!Platform.isWindows) {
      return;
    }

    // GetAsyncKeyState 返回值：高位为 1 表示当前按下
    final ctrlState = GetAsyncKeyState(VK_CONTROL);
    final shiftState = GetAsyncKeyState(VK_SHIFT);
    final altState = GetAsyncKeyState(VK_MENU); // VK_MENU = Alt key

    final ctrlPressed = (ctrlState & 0x8000) != 0;
    final shiftPressed = (shiftState & 0x8000) != 0;
    final altPressed = (altState & 0x8000) != 0;

    // 检测状态变化
    final stateChanged = ctrlPressed != _ctrlPressed ||
        shiftPressed != _shiftPressed ||
        altPressed != _altPressed;

    if (stateChanged) {
      _ctrlPressed = ctrlPressed;
      _shiftPressed = shiftPressed;
      _altPressed = altPressed;

      // 触发状态变化回调
      final pressedKeys = <ModifierKey>{};
      if (_ctrlPressed) pressedKeys.add(ModifierKey.ctrl);
      if (_shiftPressed) pressedKeys.add(ModifierKey.shift);
      if (_altPressed) pressedKeys.add(ModifierKey.alt);

      _onModifierStateChanged?.call(pressedKeys);

      // 检测是否所有修饰键都已释放
      if (!_ctrlPressed && !_shiftPressed && !_altPressed) {
        _onAllModifiersReleased?.call();
      }
    }
  }

  /// 获取当前修饰键状态。
  Set<ModifierKey> getCurrentModifiers() {
    final modifiers = <ModifierKey>{};
    if (_ctrlPressed) modifiers.add(ModifierKey.ctrl);
    if (_shiftPressed) modifiers.add(ModifierKey.shift);
    if (_altPressed) modifiers.add(ModifierKey.alt);
    return modifiers;
  }

  /// 检查是否有任何修饰键被按下。
  bool get hasAnyModifierPressed {
    return _ctrlPressed || _shiftPressed || _altPressed;
  }

  /// 检查是否所有修饰键都已释放。
  bool get allModifiersReleased {
    return !_ctrlPressed && !_shiftPressed && !_altPressed;
  }

  /// 检查特定修饰键是否被按下。
  bool isModifierPressed(ModifierKey key) {
    switch (key) {
      case ModifierKey.ctrl:
        return _ctrlPressed;
      case ModifierKey.shift:
        return _shiftPressed;
      case ModifierKey.alt:
        return _altPressed;
    }
  }

  /// 释放资源。
  void dispose() {
    stopMonitoring();
  }
}
