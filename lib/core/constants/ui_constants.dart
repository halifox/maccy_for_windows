/// Maccy UI 规格常量。
///
/// 完全基于 Maccy v2.6.1 源码中的 Popup.swift 定义。
class MaccyUIConstants {
  MaccyUIConstants._();

  // ============================================================================
  // Popup 尺寸常量（来自 Popup.swift）
  // ============================================================================

  /// 垂直内边距（上下）
  static const double verticalPadding = 5.0;

  /// 水平内边距（左右）
  static const double horizontalPadding = 5.0;

  /// 垂直分隔符内边距
  static const double verticalSeparatorPadding = 6.0;

  /// 水平分隔符内边距
  static const double horizontalSeparatorPadding = 6.0;

  /// 圆角半径（macOS 26+: 6, 旧版: 4）
  static const double cornerRadius = 6.0;

  /// 列表项高度（macOS 26+: 24, 旧版: 22）
  static const double itemHeight = 24.0;

  /// 默认窗口宽度
  static const double defaultWindowWidth = 450.0;

  /// 默认窗口高度
  static const double defaultWindowHeight = 800.0;

  // ============================================================================
  // ListItemView 尺寸常量（来自 ListItemView.swift）
  // ============================================================================

  /// 应用图标大小
  static const double appIconSize = 15.0;

  /// 应用图标左边距
  static const double appIconLeadingPadding = 4.0;

  /// 应用图标垂直边距
  static const double appIconVerticalPadding = 5.0;

  /// 内容左边距（无图标时）
  static const double contentLeadingPadding = 10.0;

  /// 内容左边距（有图标时）
  static const double contentLeadingPaddingWithIcon = 5.0;

  /// 图片右边距
  static const double imageTrailingPadding = 5.0;

  /// 图片垂直边距
  static const double imageVerticalPadding = 5.0;

  /// 标题右边距
  static const double titleTrailingPadding = 5.0;

  /// 快捷键右边距
  static const double shortcutTrailingPadding = 10.0;

  /// 快捷键占位宽度（无快捷键时）
  static const double shortcutPlaceholderWidth = 50.0;

  // ============================================================================
  // HistoryListView 尺寸常量（来自 HistoryListView.swift）
  // ============================================================================

  /// 分隔符水平边距
  static const double dividerHorizontalPadding = 10.0;

  /// 分隔符垂直边距
  static const double dividerVerticalPadding = 3.0;

  /// 滚动指示器左边距
  static const double scrollIndicatorLeadingMargin = 10.0;

  // ============================================================================
  // 颜色常量
  // ============================================================================

  /// 选中项背景色透明度
  static const double selectionBackgroundOpacity = 0.8;

  /// 悬停背景色透明度（用于修复 macOS 26 悬停问题）
  static const double hoverBackgroundOpacity = 0.001;

  // ============================================================================
  // 字体常量
  // ============================================================================

  /// 主要文本字体大小
  static const double primaryFontSize = 13.0;

  /// 快捷键文本字体大小
  static const double shortcutFontSize = 12.0;

  /// 搜索框文本字体大小
  static const double searchFieldFontSize = 12.0;

  /// 系统字体名称（macOS）
  static const String systemFontFamily = '.AppleSystemUIFont';

  /// 系统字体名称（Windows）
  static const String systemFontFamilyWindows = 'Segoe UI';

  // ============================================================================
  // 动画常量
  // ============================================================================

  /// 窗口调整大小动画时长（秒）
  static const double resizeAnimationDuration = 0.2;

  /// 列表项动画速度倍数
  static const double listAnimationSpeed = 3.0;

  /// 搜索可见性动画时长（秒）
  static const double searchVisibilityAnimationDuration = 0.2;

  // ============================================================================
  // 预览常量
  // ============================================================================

  /// 预览延迟（毫秒，默认值）
  static const int defaultPreviewDelay = 1500;

  /// 预览悬浮窗与主窗口的间距
  static const double previewOffset = 10.0;

  // ============================================================================
  // 搜索框常量
  // ============================================================================

  /// 搜索框内边距
  static const double searchFieldPadding = 5.0;

  /// 搜索框内部垂直边距
  static const double searchFieldInternalVerticalPadding = 4.0;

  /// 搜索框内部水平边距
  static const double searchFieldInternalHorizontalPadding = 5.0;

  /// 搜索框圆角半径
  static const double searchFieldCornerRadius = 4.0;

  /// 搜索框图标大小
  static const double searchFieldIconSize = 12.0;
}
