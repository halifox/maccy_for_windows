/// 文本格式化工具。
///
/// 提供特殊字符可视化功能，对应 Maccy 的 showSpecialSymbols 配置。
/// 将不可见字符转换为可见符号，便于用户识别剪贴板内容的格式。
class TextFormatter {
  /// 格式化文本以显示特殊字符。
  ///
  /// 当 [showSpecialChars] 为 true 时：
  /// - 前导空格 → · (中点符号)
  /// - 尾随空格 → · (中点符号)
  /// - 换行符 → ⏎ (回车符号)
  /// - 制表符 → ⇥ (Tab 符号)
  ///
  /// 当 [showSpecialChars] 为 false 时：
  /// - 仅去除首尾空白字符
  ///
  /// [text] 原始文本内容
  /// [showSpecialChars] 是否显示特殊字符
  ///
  /// 返回格式化后的文本。
  ///
  /// 示例：
  /// ```dart
  /// // showSpecialChars = true
  /// formatForDisplay('  hello\nworld\t', showSpecialChars: true)
  /// // 返回: '··hello⏎world⇥'
  ///
  /// // showSpecialChars = false
  /// formatForDisplay('  hello\nworld\t', showSpecialChars: false)
  /// // 返回: 'hello\nworld'
  /// ```
  static String formatForDisplay(String text, {required bool showSpecialChars}) {
    if (!showSpecialChars) {
      return text.trim();
    }

    String formatted = text;

    // 前导空格 → ·
    formatted = formatted.replaceAllMapped(
      RegExp(r'^( +)'),
      (match) => '·' * match.group(1)!.length,
    );

    // 尾随空格 → ·
    formatted = formatted.replaceAllMapped(
      RegExp(r'( +)$'),
      (match) => '·' * match.group(1)!.length,
    );

    // 换行 → ⏎
    formatted = formatted.replaceAll('\n', '⏎');

    // 制表符 → ⇥
    formatted = formatted.replaceAll('\t', '⇥');

    return formatted;
  }

  /// 生成内容预览文本（用于列表显示）。
  ///
  /// 限制文本长度并应用特殊字符格式化。
  ///
  /// [text] 原始文本
  /// [maxLength] 最大字符数（默认 100）
  /// [showSpecialChars] 是否显示特殊字符
  ///
  /// 返回预览文本。
  static String generatePreview(
    String text, {
    int maxLength = 100,
    required bool showSpecialChars,
  }) {
    // 先格式化特殊字符
    String preview = formatForDisplay(text, showSpecialChars: showSpecialChars);

    // 限制长度
    if (preview.length > maxLength) {
      preview = '${preview.substring(0, maxLength)}...';
    }

    return preview;
  }

  /// 还原特殊字符（用于复制到剪贴板时）。
  ///
  /// 将可视化符号转换回原始字符。
  ///
  /// [formattedText] 格式化后的文本
  ///
  /// 返回原始文本。
  static String restoreSpecialChars(String formattedText) {
    String restored = formattedText;

    // ⏎ → 换行
    restored = restored.replaceAll('⏎', '\n');

    // ⇥ → 制表符
    restored = restored.replaceAll('⇥', '\t');

    // · → 空格（这个转换可能不精确，因为无法区分前导/尾随）
    // 通常不需要还原，因为复制时使用原始内容
    // restored = restored.replaceAll('·', ' ');

    return restored;
  }

  /// 检测文本是否包含特殊字符。
  ///
  /// [text] 待检测的文本
  ///
  /// 返回 true 表示包含空格、换行或制表符。
  static bool hasSpecialChars(String text) {
    return text.contains(RegExp(r'^\s+|\s+$|\n|\t'));
  }
}
