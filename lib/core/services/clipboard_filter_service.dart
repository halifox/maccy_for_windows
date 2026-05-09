/// 剪贴板内容过滤服务。
///
/// 提供应用过滤和正则表达式过滤功能，对应 Maccy 的过滤机制。
class ClipboardFilterService {
  /// 检查应用是否应该被忽略。
  ///
  /// 支持两种模式：
  /// - 黑名单模式：在列表中的应用被忽略
  /// - 白名单模式：不在列表中的应用被忽略
  ///
  /// [appName] 应用名称（如 "chrome", "notepad"）
  /// [ignoredApps] 忽略/允许的应用列表
  /// [isWhitelistMode] 是否为白名单模式
  ///
  /// 返回 true 表示应该忽略该应用的剪贴板内容。
  static bool shouldIgnoreApp(
    String? appName, {
    required List<String> ignoredApps,
    required bool isWhitelistMode,
  }) {
    // 如果没有应用名，不过滤
    if (appName == null || appName.isEmpty) return false;

    // 如果过滤列表为空
    if (ignoredApps.isEmpty) {
      // 白名单模式：列表为空则忽略所有
      // 黑名单模式：列表为空则不忽略任何
      return isWhitelistMode;
    }

    final normalizedAppName = appName.toLowerCase();

    // 检查应用名是否在列表中（支持部分匹配）
    final isInList = ignoredApps.any((app) {
      final normalizedApp = app.toLowerCase();
      return normalizedAppName.contains(normalizedApp) ||
          normalizedApp.contains(normalizedAppName);
    });

    // 白名单模式：不在列表中则忽略
    // 黑名单模式：在列表中则忽略
    return isWhitelistMode ? !isInList : isInList;
  }

  /// 检查内容是否匹配任何忽略的正则表达式规则。
  ///
  /// [content] 剪贴板文本内容
  /// [patterns] 正则表达式模式列表
  ///
  /// 返回 true 表示内容匹配某个忽略规则，应该被过滤。
  static bool shouldIgnoreContent(String content, List<String> patterns) {
    if (patterns.isEmpty) return false;

    for (final pattern in patterns) {
      if (pattern.isEmpty) continue;

      try {
        final regex = RegExp(pattern, caseSensitive: false);
        if (regex.hasMatch(content)) {
          return true;
        }
      } catch (e) {
        // 忽略无效的正则表达式
        continue;
      }
    }

    return false;
  }

  /// 检查剪贴板类型是否应该被忽略。
  ///
  /// [type] 内容类型（'text', 'image', 'file'）
  /// [ignoredTypes] 忽略的类型列表
  ///
  /// 返回 true 表示该类型应该被过滤。
  static bool shouldIgnoreType(String type, List<String> ignoredTypes) {
    if (ignoredTypes.isEmpty) return false;
    return ignoredTypes.contains(type);
  }
}
