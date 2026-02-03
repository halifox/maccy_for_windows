import 'dart:convert';
import 'package:flutter/services.dart';
import 'package:hotkey_manager/hotkey_manager.dart';

/// 快捷键配置模型类
class AppHotKeyConfig {
  /// 修饰键列表（如 control, alt, shift, meta）
  final List<String> modifiers;
  /// 主键位（如 A, B, C, V, 1, 2...）
  final String key;

  /// 构造函数
  const AppHotKeyConfig({
    required this.modifiers,
    required this.key,
  });

  /// 转换为 JSON 对象
  Map<String, dynamic> toJson() => {
        'modifiers': modifiers,
        'key': key,
      };

  /// 从 JSON 对象创建实例
  factory AppHotKeyConfig.fromJson(Map<String, dynamic> json) {
    return AppHotKeyConfig(
      modifiers: List<String>.from(json['modifiers'] ?? []),
      key: json['key'] ?? 'V',
    );
  }

  /// 从原始 JSON 字符串创建实例，包含默认值处理
  factory AppHotKeyConfig.fromRawJson(String rawJson) {
    try {
      if (rawJson.isEmpty) return const AppHotKeyConfig(modifiers: ['alt'], key: 'V');
      return AppHotKeyConfig.fromJson(jsonDecode(rawJson));
    } catch (_) {
      return const AppHotKeyConfig(modifiers: ['alt'], key: 'V');
    }
  }

  /// 转换为原始 JSON 字符串
  String toRawJson() => jsonEncode(toJson());

  /// 转换为 hotkey_manager 所需的 HotKey 对象
  HotKey? toHotKey() {
    final physicalKey = _parseKey(key);
    if (physicalKey == null) return null;

    final hotKeyModifiers = modifiers.map((m) {
      return switch (m) {
        'meta' => HotKeyModifier.meta,
        'alt' => HotKeyModifier.alt,
        'shift' => HotKeyModifier.shift,
        'control' => HotKeyModifier.control,
        _ => HotKeyModifier.meta,
      };
    }).toList();

    return HotKey(
      key: physicalKey,
      modifiers: hotKeyModifiers,
      scope: HotKeyScope.system,
    );
  }

  /// 解析字符串键位为 PhysicalKeyboardKey 对象
  static PhysicalKeyboardKey? _parseKey(String key) {
    final keyUpper = key.toUpperCase();
    if (RegExp(r'^[A-Z]$').hasMatch(keyUpper)) {
      final map = {
        'A': PhysicalKeyboardKey.keyA, 'B': PhysicalKeyboardKey.keyB, 'C': PhysicalKeyboardKey.keyC,
        'D': PhysicalKeyboardKey.keyD, 'E': PhysicalKeyboardKey.keyE, 'F': PhysicalKeyboardKey.keyF,
        'G': PhysicalKeyboardKey.keyG, 'H': PhysicalKeyboardKey.keyH, 'I': PhysicalKeyboardKey.keyI,
        'J': PhysicalKeyboardKey.keyJ, 'K': PhysicalKeyboardKey.keyK, 'L': PhysicalKeyboardKey.keyL,
        'M': PhysicalKeyboardKey.keyM, 'N': PhysicalKeyboardKey.keyN, 'O': PhysicalKeyboardKey.keyO,
        'P': PhysicalKeyboardKey.keyP, 'Q': PhysicalKeyboardKey.keyQ, 'R': PhysicalKeyboardKey.keyR,
        'S': PhysicalKeyboardKey.keyS, 'T': PhysicalKeyboardKey.keyT, 'U': PhysicalKeyboardKey.keyU,
        'V': PhysicalKeyboardKey.keyV, 'W': PhysicalKeyboardKey.keyW, 'X': PhysicalKeyboardKey.keyX,
        'Y': PhysicalKeyboardKey.keyY, 'Z': PhysicalKeyboardKey.keyZ,
      };
      return map[keyUpper];
    } else if (RegExp(r'^[0-9]$').hasMatch(keyUpper)) {
      final map = {
        '0': PhysicalKeyboardKey.digit0, '1': PhysicalKeyboardKey.digit1, '2': PhysicalKeyboardKey.digit2,
        '3': PhysicalKeyboardKey.digit3, '4': PhysicalKeyboardKey.digit4, '5': PhysicalKeyboardKey.digit5,
        '6': PhysicalKeyboardKey.digit6, '7': PhysicalKeyboardKey.digit7, '8': PhysicalKeyboardKey.digit8,
        '9': PhysicalKeyboardKey.digit9,
      };
      return map[keyUpper];
    }
    return null;
  }
}
