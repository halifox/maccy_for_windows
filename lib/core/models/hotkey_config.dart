import 'dart:convert';
import 'package:flutter/services.dart';
import 'package:hotkey_manager/hotkey_manager.dart';

/// 快捷键配置模型类。
///
/// 用于定义、解析和转换应用程序的全局热键配置（如 Alt+V）。
///
/// 字段说明:
/// [modifiers] 修饰键列表，可选值包括 'control', 'alt', 'shift', 'meta'。
/// [key] 主键位字符串，如 'V', 'A', '1' 等。
class AppHotKeyConfig {

  const AppHotKeyConfig({required this.modifiers, required this.key});

  /// 从 JSON 映射创建配置实例。
  ///
  /// [json] 包含快捷键信息的映射。
  factory AppHotKeyConfig.fromJson(Map<String, dynamic> json) {
    return AppHotKeyConfig(
      modifiers: List<String>.from(json['modifiers'] as Iterable? ?? []),
      key: json['key'] as String? ?? 'V',
    );
  }

  /// 从原始 JSON 字符串安全解析配置。
  ///
  /// 若解析失败或字符串为空，将返回默认配置（Alt+V）。
  ///
  /// [rawJson] 原始 JSON 字符串。
  factory AppHotKeyConfig.fromRawJson(String rawJson) {
    try {
      if (rawJson.isEmpty) {
        return const AppHotKeyConfig(modifiers: ['alt'], key: 'V');
      }
      return AppHotKeyConfig.fromJson(jsonDecode(rawJson) as Map<String, dynamic>);
    } catch (_) {
      return const AppHotKeyConfig(modifiers: ['alt'], key: 'V');
    }
  }
  final List<String> modifiers;
  final String key;

  /// 将配置转换为 JSON 映射。
  ///
  /// 返回 [Map<String, dynamic>]。
  Map<String, dynamic> toJson() => {'modifiers': modifiers, 'key': key};

  /// 将当前配置转换为 JSON 字符串。
  String toRawJson() => jsonEncode(toJson());

  /// 转换为 hotkey_manager 插件所需的 [HotKey] 对象。
  ///
  /// 映射内部的修饰键和键位至插件枚举，并设置作用域为全局系统级。
  ///
  /// 返回 [HotKey] 实例，若键位解析失败则返回 null。
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

  /// 将字符串键位解析为 [PhysicalKeyboardKey]。
  ///
  /// [key] 待解析的键位字符串（不区分大小写）。
  static PhysicalKeyboardKey? _parseKey(String key) {
    final keyUpper = key.toUpperCase();
    if (RegExp(r'^[A-Z]$').hasMatch(keyUpper)) {
      final map = {
        'A': PhysicalKeyboardKey.keyA,
        'B': PhysicalKeyboardKey.keyB,
        'C': PhysicalKeyboardKey.keyC,
        'D': PhysicalKeyboardKey.keyD,
        'E': PhysicalKeyboardKey.keyE,
        'F': PhysicalKeyboardKey.keyF,
        'G': PhysicalKeyboardKey.keyG,
        'H': PhysicalKeyboardKey.keyH,
        'I': PhysicalKeyboardKey.keyI,
        'J': PhysicalKeyboardKey.keyJ,
        'K': PhysicalKeyboardKey.keyK,
        'L': PhysicalKeyboardKey.keyL,
        'M': PhysicalKeyboardKey.keyM,
        'N': PhysicalKeyboardKey.keyN,
        'O': PhysicalKeyboardKey.keyO,
        'P': PhysicalKeyboardKey.keyP,
        'Q': PhysicalKeyboardKey.keyQ,
        'R': PhysicalKeyboardKey.keyR,
        'S': PhysicalKeyboardKey.keyS,
        'T': PhysicalKeyboardKey.keyT,
        'U': PhysicalKeyboardKey.keyU,
        'V': PhysicalKeyboardKey.keyV,
        'W': PhysicalKeyboardKey.keyW,
        'X': PhysicalKeyboardKey.keyX,
        'Y': PhysicalKeyboardKey.keyY,
        'Z': PhysicalKeyboardKey.keyZ,
      };
      return map[keyUpper];
    } else if (RegExp(r'^[0-9]$').hasMatch(keyUpper)) {
      final map = {
        '0': PhysicalKeyboardKey.digit0,
        '1': PhysicalKeyboardKey.digit1,
        '2': PhysicalKeyboardKey.digit2,
        '3': PhysicalKeyboardKey.digit3,
        '4': PhysicalKeyboardKey.digit4,
        '5': PhysicalKeyboardKey.digit5,
        '6': PhysicalKeyboardKey.digit6,
        '7': PhysicalKeyboardKey.digit7,
        '8': PhysicalKeyboardKey.digit8,
        '9': PhysicalKeyboardKey.digit9,
      };
      return map[keyUpper];
    }
    return null;
  }
}
