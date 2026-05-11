import 'package:maccy/core/models/hotkey_config.dart';

/// 快捷键工具类。
///
/// 用于在不同平台正确显示快捷键文本。
class HotkeyUtils {
  /// 获取修饰键的显示文本。
  ///
  /// [modifier] 修饰键标识符 ('meta', 'alt', 'control', 'shift')。
  static String getModifierDisplay(String modifier) {
    return switch (modifier) {
      'meta' => 'Win',
      'alt' => 'Alt',
      'control' => 'Ctrl',
      'shift' => 'Shift',
      _ => modifier,
    };
  }

  /// 获取快捷键配置的完整显示文本。
  ///
  /// 例如: Ctrl + Alt + V
  static String getHotkeyDisplay(AppHotKeyConfig config) {
    if (config.modifiers.isEmpty) return config.key;

    final sortedModifiers = _sortModifiers(config.modifiers);
    final buffer = StringBuffer();

    for (final modifier in sortedModifiers) {
      buffer.write(getModifierDisplay(modifier));
      buffer.write(' + ');
    }

    buffer.write(config.key);
    return buffer.toString();
  }

  /// 对修饰键进行排序。
  static List<String> _sortModifiers(List<String> modifiers) {
    final sorted = List<String>.from(modifiers);

    // 排序规则: Ctrl -> Alt -> Shift -> Win
    final weights = {
      'control': 1,
      'alt': 2,
      'shift': 3,
      'meta': 4,
    };

    sorted.sort((a, b) => (weights[a] ?? 0).compareTo(weights[b] ?? 0));
    return sorted;
  }
}
