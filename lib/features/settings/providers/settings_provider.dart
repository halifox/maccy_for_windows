import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

// 预初始化 Provider
/// 共享首选项 Provider，在应用启动时通过 appStartup 进行初始化
/// 我们将其保留为同步 Provider，以便设置项可以同步读取
final sharedPrefsProvider = Provider<SharedPreferences>((ref) {
  // 注意：这个抛错仅用于调试，实际在 appStartup 之后，所有访问此 Provider 的地方
  // 都应该在 appStartupProvider 的 data 状态之后
  throw UnimplementedError('sharedPrefsProvider must be accessed after appStartup');
});

/// 自动持久化的 Notifier 基类，支持多种基本数据类型
class PersistentNotifier<T> extends Notifier<T> {
  /// 持久化存储的 Key
  final String key;
  /// 默认值
  final T defaultValue;
  late final _prefs = ref.read(sharedPrefsProvider);

  /// 构造函数
  PersistentNotifier(this.key, this.defaultValue);

  @override
  T build() {
    return _get(key) ?? defaultValue;
  }

  /// 更新状态并持久化到本地存储
  void set(T value) {
    if (state == value) return;
    state = value;
    _set(key, value);
  }

  /// 从首选项中读取数据
  T? _get(String key) {
    if (T == String) return _prefs.getString(key) as T?;
    if (T == int) return _prefs.getInt(key) as T?;
    if (T == bool) return _prefs.getBool(key) as T?;
    if (T == double) return _prefs.getDouble(key) as T?;
    if (T == List<String>) return _prefs.getStringList(key) as T?;
    return null;
  }

  /// 将数据保存到首选项中
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

/// 快速创建一个持久化 Notifier Provider 的工具函数
NotifierProvider<PersistentNotifier<T>, T> pref<T>(String key, T defaultValue) {
  return NotifierProvider<PersistentNotifier<T>, T>(() => PersistentNotifier<T>(key, defaultValue));
}

// --- Launch & Update ---
final launchAtStartupProvider = pref<bool>('launchAtStartup', false);
final autoCheckUpdatesProvider = pref<bool>('autoCheckUpdates', true);
final hotkeyOpenProvider = pref<String>('hotkeyOpen', '{"modifiers":["alt"],"key":"V"}');
final hotkeyPinProvider = pref<String>('hotkeyPin', '');
final hotkeyDeleteProvider = pref<String>('hotkeyDelete', '');

// --- Behavior ---
final autoPasteProvider = pref<bool>('autoPaste', true);
final pastePlainProvider = pref<bool>('pastePlain', false);
final searchModeProvider = pref<String>('searchMode', 'fuzzy');
final isPausedProvider = pref<bool>('isPaused', false);

// --- Storage ---
final historyLimitProvider = pref<int>('historyLimit', 200);
final saveTextProvider = pref<bool>('saveText', true);
final saveImagesProvider = pref<bool>('saveImages', true);
final saveFilesProvider = pref<bool>('saveFiles', true);
final clearOnExitProvider = pref<bool>('clearOnExit', false);
final clearSystemClipboardProvider = pref<bool>('clearSystemClipboard', false);

// --- UI & Appearance ---
final popupPositionProvider = pref<String>('popupPosition', 'cursor');
final pinPositionProvider = pref<String>('pinPosition', 'top');
final imageHeightProvider = pref<int>('imageHeight', 40);
final previewDelayProvider = pref<int>('previewDelay', 1500);
final highlightMatchProvider = pref<String>('highlightMatch', 'bold');
final showSpecialCharsProvider = pref<bool>('showSpecialChars', false);
final showMenuBarIconProvider = pref<bool>('showMenuBarIcon', true);
final menuBarIconTypeProvider = pref<String>('menuBarIconType', 'clipboard');
final showClipboardNearIconProvider = pref<bool>('showClipboardNearIcon', false);
final showSearchBoxProvider = pref<String>('showSearchBox', 'always');
final showAppNameProvider = pref<bool>('showAppName', false);
final showAppIconProvider = pref<bool>('showAppIcon', true);
final showFooterMenuProvider = pref<bool>('showFooterMenu', true);
final windowWidthProvider = pref<double>('windowWidth', 350.0);
final themeModeProvider = pref<String>('themeMode', 'system');
