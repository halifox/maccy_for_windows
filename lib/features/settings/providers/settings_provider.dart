import 'dart:convert';

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
/// 封装了对 SharedPreferences 的读写操作，当 [state] 改变时自动同步至本地磁盘。
/// 支持 String, int, bool, double, List<String> 等基本数据类型。
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
      return fromJson(jsonDecode(rawJson));
    } catch (_) {
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

/// 启动与更新设置
/// 是否开机自动启动应用
final launchAtStartupProvider = pref<bool>('launchAtStartup', false);

/// 是否自动检查版本更新
final autoCheckUpdatesProvider = pref<bool>('autoCheckUpdates', true);

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

/// 行为逻辑设置
/// 选择记录后是否自动执行粘贴操作
final autoPasteProvider = pref<bool>('autoPaste', true);
/// 是否始终以纯文本格式粘贴（去除格式）
final pastePlainProvider = pref<bool>('pastePlain', false);
/// 搜索匹配模式（fuzzy: 模糊, exact: 精确, regex: 正则, mixed: 混合）
final searchModeProvider = pref<String>('searchMode', 'fuzzy');
/// 是否暂停捕获新的剪贴板内容
final isPausedProvider = pref<bool>('isPaused', false);

/// 存储与清理策略设置
/// 历史记录保留的最大条数上限
final historyLimitProvider = pref<int>('historyLimit', 200);
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

/// UI 与外观配置设置
/// 面板弹出位置（cursor: 鼠标光标处, center: 屏幕中央）
final popupPositionProvider = pref<String>('popupPosition', 'cursor');
/// 置顶项目在列表中的显示位置（top: 顶部, bottom: 底部）
final pinPositionProvider = pref<String>('pinPosition', 'top');
/// 列表中图片预览区域的高度
final imageHeightProvider = pref<int>('imageHeight', 40);
/// 显示详细内容预览前的延迟毫秒数
final previewDelayProvider = pref<int>('previewDelay', 1500);
/// 搜索结果匹配文字的高亮样式（bold: 加粗, color: 颜色高亮）
final highlightMatchProvider = pref<String>('highlightMatch', 'bold');
/// 是否显示空格、换行等特殊不可见字符
final showSpecialCharsProvider = pref<bool>('showSpecialChars', false);
/// 是否在系统菜单栏显示应用图标
final showMenuBarIconProvider = pref<bool>('showMenuBarIcon', true);
/// 菜单栏图标的样式类型
final menuBarIconTypeProvider = pref<String>('menuBarIconType', 'clipboard');
/// 是否在菜单栏图标旁实时显示剪贴板文字预览
final showClipboardNearIconProvider = pref<bool>(
  'showClipboardNearIcon',
  false,
);
/// 搜索框的显示时机（always: 总是显示, typing: 输入时显示, never: 从不显示）
final showSearchBoxProvider = pref<String>('showSearchBox', 'always');
/// 是否显示来源应用程序的名称
final showAppNameProvider = pref<bool>('showAppName', false);
/// 是否显示来源应用程序的图标
final showAppIconProvider = pref<bool>('showAppIcon', true);
/// 是否显示列表底部的功能操作菜单
final showFooterMenuProvider = pref<bool>('showFooterMenu', true);
/// 剪贴板面板的默认窗口宽度
final windowWidthProvider = pref<double>('windowWidth', 350.0);
/// 应用主题模式（system: 跟随系统, light: 浅色, dark: 深色）
final themeModeProvider = pref<String>('themeMode', 'system');