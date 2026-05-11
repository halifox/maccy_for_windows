import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:maccy/core/models/hotkey_config.dart';

/// 共享首选项 Provider。
///
/// 在应用启动时（main.dart）通过 overrides 进行预注入，确保设置项可以同步读取本地配置。
final sharedPrefsProvider = Provider<SharedPreferences>((ref) {
  throw UnimplementedError(
    'sharedPrefsProvider must be accessed after appStartup',
  );
});

/// 自动持久化的 Notifier 基类。
///
/// 封装了对 SharedPreferences 的读写操作，当 state 改变时自动同步至本地磁盘。
/// 支持 String, int, bool, double, List&lt;String&gt; 等基本数据类型。
///
/// 字段说明:
/// [key] 持久化存储对应的唯一键名。
/// [defaultValue] 当本地没有存储记录时使用的默认值。
class PersistentNotifier<T> extends Notifier<T> {

  PersistentNotifier(this.key, this.defaultValue);
  final String key;
  final T defaultValue;
  late final _prefs = ref.read(sharedPrefsProvider);

  @override
  T build() {
    return _get(key) ?? defaultValue;
  }

  /// 更新状态并持久化。
  ///
  /// [value] 新的配置值。
  void set(T value) {
    if (state == value) return;
    state = value;
    _set(key, value);
  }

  /// 内部读取方法。
  T? _get(String key) {
    if (T == String) return _prefs.getString(key) as T?;
    if (T == int) return _prefs.getInt(key) as T?;
    if (T == bool) return _prefs.getBool(key) as T?;
    if (T == double) return _prefs.getDouble(key) as T?;
    if (T == List<String>) return _prefs.getStringList(key) as T?;
    return null;
  }

  /// 内部保存方法。
  void _set(String key, T value) {
    if (value is String) {
      _prefs.setString(key, value);
    } else if (value is int) {
      _prefs.setInt(key, value);
    } else if (value is bool) {
      _prefs.setBool(key, value);
    } else if (value is double) {
      _prefs.setDouble(key, value);
    } else if (value is List<String>) {
      _prefs.setStringList(key, value);
    }
  }
}

/// JSON 持久化 Notifier。
///
/// 继承自 [PersistentNotifier]，但在存储时将对象序列化为 JSON 字符串，
/// 读取时再通过提供的 [fromJson] 函数进行反序列化。
class JsonPersistentNotifier<T> extends PersistentNotifier<T> {

  JsonPersistentNotifier(
    super.key,
    super.defaultValue, {
    required this.fromJson,
    required this.toJson,
  });
  final T Function(Map<String, dynamic> json) fromJson;
  final Map<String, dynamic> Function(T value) toJson;

  @override
  T? _get(String key) {
    final rawJson = _prefs.getString(key);
    if (rawJson == null || rawJson.isEmpty) return null;
    try {
      return fromJson(jsonDecode(rawJson) as Map<String, dynamic>);
    } catch (e) {
      debugPrint(e.toString());
      return null;
    }
  }

  @override
  void _set(String key, T value) {
    _prefs.setString(key, jsonEncode(toJson(value)));
  }
}

/// 快速创建持久化 Notifier Provider 的工具函数。
NotifierProvider<PersistentNotifier<T>, T> pref<T>(String key, T defaultValue) {
  return NotifierProvider<PersistentNotifier<T>, T>(
    () => PersistentNotifier<T>(key, defaultValue),
  );
}

/// 快速创建 JSON 持久化 Notifier Provider 的工具函数。
NotifierProvider<JsonPersistentNotifier<T>, T> prefJson<T>(
  String key,
  T defaultValue, {
  required T Function(Map<String, dynamic> json) fromJson,
  required Map<String, dynamic> Function(T value) toJson,
}) {
  return NotifierProvider<JsonPersistentNotifier<T>, T>(
    () => JsonPersistentNotifier<T>(
      key,
      defaultValue,
      fromJson: fromJson,
      toJson: toJson,
    ),
  );
}

// ============================================================================
// General Settings (启动与更新)
// ============================================================================

/// 是否开机自动启动应用
final launchAtStartupProvider = pref<bool>('launchAtStartup', false);

/// 是否自动检查版本更新
final autoCheckUpdatesProvider = pref<bool>('autoCheckUpdates', true);

// ============================================================================
// Keyboard Shortcuts (快捷键)
// ============================================================================

/// 唤起剪贴板历史面板的全局快捷键
final hotkeyOpenProvider = prefJson<AppHotKeyConfig>(
  'hotkeyOpen',
  const AppHotKeyConfig(modifiers: ['alt'], key: 'V'),
  fromJson: AppHotKeyConfig.fromJson,
  toJson: (v) => v.toJson(),
);

/// 快速置顶当前选中项的快捷键
final hotkeyPinProvider = prefJson<AppHotKeyConfig>(
  'hotkeyPin',
  const AppHotKeyConfig(modifiers: ['alt'], key: 'P'),
  fromJson: AppHotKeyConfig.fromJson,
  toJson: (v) => v.toJson(),
);

/// 快速删除当前选中项的快捷键
final hotkeyDeleteProvider = prefJson<AppHotKeyConfig>(
  'hotkeyDelete',
  const AppHotKeyConfig(modifiers: ['alt'], key: 'D'),
  fromJson: AppHotKeyConfig.fromJson,
  toJson: (v) => v.toJson(),
);

// ============================================================================
// Search Settings (搜索)
// ============================================================================

/// 搜索匹配模式（exact: 精确, fuzzy: 模糊, regex: 正则, mixed: 混合）
final searchModeProvider = pref<String>('searchMode', 'exact');

// ============================================================================
// Behavior Settings (行为逻辑)
// ============================================================================

/// 选择记录后是否自动执行粘贴操作
final autoPasteProvider = pref<bool>('autoPaste', false);

/// 是否始终以纯文本格式粘贴（去除格式）
final pastePlainProvider = pref<bool>('pastePlain', false);

// ============================================================================
// Storage Settings (存储与清理策略)
// ============================================================================

/// 历史记录保留的最大条数上限
final historyLimitProvider = pref<int>('historyLimit', 200);

/// 排序方式 (lastCopiedAt: 最近复制, firstCopiedAt: 首次复制, numberOfCopies: 复制次数)
final sortByProvider = pref<String>('sortBy', 'lastCopiedAt');

/// 是否记录并保存文本内容
final saveTextProvider = pref<bool>('saveText', true);

/// 是否记录并保存图片内容
final saveImagesProvider = pref<bool>('saveImages', true);

/// 是否记录并保存文件/文件夹路径
final saveFilesProvider = pref<bool>('saveFiles', true);

/// 应用退出时是否自动清空所有历史记录
final clearOnExitProvider = pref<bool>('clearOnExit', false);

/// 清空历史时是否同步清空系统当前的剪贴板
final clearSystemClipboardProvider = pref<bool>('clearSystemClipboard', false);

// ============================================================================
// Appearance Settings (UI 与外观配置)
// ============================================================================

/// 面板弹出位置（cursor: 鼠标光标处, center: 屏幕中央, statusItem: 状态栏, lastPosition: 记住位置）
final popupPositionProvider = pref<String>('popupPosition', 'cursor');

/// 多屏幕环境下的目标屏幕索引（0: 活动屏幕, 1+: 具体屏幕编号）
final popupScreenProvider = pref<int>('popupScreen', 0);

/// 置顶项目在列表中的显示位置（top: 顶部, bottom: 底部）
final pinPositionProvider = pref<String>('pinPosition', 'top');

/// 列表中图片预览区域的最大高度（像素）
final imageHeightProvider = pref<int>('imageHeight', 40);

/// 显示详细内容预览前的延迟毫秒数
final previewDelayProvider = pref<int>('previewDelay', 1500);

/// 搜索结果匹配文字的高亮样式（bold: 加粗, color: 颜色高亮）
final highlightMatchProvider = pref<String>('highlightMatch', 'bold');

/// 是否显示空格、换行等特殊不可见字符（空格→·, 换行→⏎, Tab→⇥）
final showSpecialCharsProvider = pref<bool>('showSpecialChars', false);

/// 是否在系统菜单栏/托盘显示应用图标
final showMenuBarIconProvider = pref<bool>('showMenuBarIcon', true);

/// 菜单栏图标的样式类型 (clipboard: 剪贴板图标, scissors: 剪刀图标, text: 文字图标)
final menuBarIconTypeProvider = pref<String>('menuBarIconType', 'clipboard');

/// 是否在菜单栏图标旁实时显示最近复制的内容预览
final showRecentCopyInMenuBarProvider = pref<bool>(
  'showRecentCopyInMenuBar',
  false,
);

/// 是否显示搜索框
final showSearchProvider = pref<String>('showSearch', 'always');

/// 搜索框的可见性策略（always: 总是显示, onType: 输入时显示, never: 从不显示）
final searchVisibilityProvider = pref<String>('searchVisibility', 'always');

/// 是否在搜索框上方显示标题栏
final showTitleProvider = pref<bool>('showTitle', true);

/// 是否显示来源应用程序的图标
final showAppIconProvider = pref<bool>('showAppIcon', false);

/// 是否显示列表底部的功能操作菜单（清空历史、设置、退出）
final showFooterMenuProvider = pref<bool>('showFooterMenu', true);

/// 剪贴板面板的默认窗口宽度（像素）
final windowWidthProvider = pref<double>('windowWidth', 450.0);

/// 剪贴板面板的默认窗口高度（像素）
final windowHeightProvider = pref<double>('windowHeight', 800.0);

/// 应用主题模式（system: 跟随系统, light: 浅色, dark: 深色）
final themeModeProvider = pref<String>('themeMode', 'system');

// ============================================================================
// Ignore Settings (忽略/过滤规则)
// ============================================================================

/// 忽略的应用程序列表（Bundle ID 或进程名）
final ignoredAppsProvider = pref<List<String>>('ignoredApps', []);

/// 反向模式：仅记录列表内的应用（true 时 ignoredApps 变为白名单）
final ignoreAllAppsExceptListedProvider = pref<bool>(
  'ignoreAllAppsExceptListed',
  false,
);

/// 忽略的剪贴板类型列表（如密码管理器的特殊类型）
final ignoredPasteboardTypesProvider = pref<List<String>>(
  'ignoredPasteboardTypes',
  [
    'com.agilebits.onepassword',
    'com.typeit4me.clipping',
    'de.petermaurer.TransientPasteboardType',
    'net.antelle.keeweb',
  ],
);

/// 正则表达式过滤规则列表（匹配的内容将被忽略）
final ignoreRegexpProvider = pref<List<String>>('ignoreRegexp', []);

// ============================================================================
// Advanced Settings (高级配置)
// ============================================================================

/// 剪贴板检查间隔（秒）
final clipboardCheckIntervalProvider = pref<double>(
  'clipboardCheckInterval',
  0.5,
);

/// 是否暂停捕获新的剪贴板内容（临时开关）
final ignoreEventsProvider = pref<bool>('ignoreEvents', false);

/// 仅忽略下一次剪贴板事件（配合 ignoreEvents 使用）
final ignoreOnlyNextEventProvider = pref<bool>('ignoreOnlyNextEvent', false);

// ============================================================================
// Computed/Derived Providers (派生状态)
// ============================================================================

/// 所有启用的剪贴板类型集合（根据 saveText/saveImages/saveFiles 计算）
final enabledPasteboardTypesProvider = Provider<Set<String>>((ref) {
  final types = <String>{};
  if (ref.watch(saveTextProvider)) {
    types.addAll(['text', 'html', 'rtf']);
  }
  if (ref.watch(saveImagesProvider)) {
    types.addAll(['image', 'png', 'tiff']);
  }
  if (ref.watch(saveFilesProvider)) {
    types.add('file');
  }
  return types;
});
