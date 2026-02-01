import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../../core/database/database.dart';
import '../../../core/database/database_provider.dart';
import 'package:drift/drift.dart';

part 'settings_provider.g.dart';

@Riverpod(keepAlive: true)
class SettingsNotifier extends _$SettingsNotifier {
  @override
  Future<AppSetting> build() async {
    // 统一 Ref 使用：通过 this.ref 访问
    final db = ref.watch(appDatabaseProvider);
    final setting = await db.select(db.appSettings).getSingleOrNull();
    
    if (setting == null) {
      final id = await db.into(db.appSettings).insert(
        AppSettingsCompanion.insert(
          launchAtStartup: const Value(false),
          autoPaste: const Value(true),
          pastePlain: const Value(false),
          searchMode: const Value('fuzzy'),
          historyLimit: const Value(200),
          saveText: const Value(true),
          saveImages: const Value(true),
          saveFiles: const Value(true),
          popupPosition: const Value('cursor'),
          pinPosition: const Value('top'),
          imageHeight: const Value(40),
          previewDelay: const Value(1500),
          showAppIcon: const Value(true),
          showAppName: const Value(false),
          showFooterMenu: const Value(true),
          windowWidth: const Value(350.0),
          themeMode: const Value('system'),
          isPaused: const Value(false),
          clearOnExit: const Value(false),
          clearSystemClipboard: const Value(false),
          ignoreAppsJson: const Value('[]'),
        ),
      );
      return (db.select(db.appSettings)..where((t) => t.id.equals(id))).getSingle();
    }
    return setting;
  }

  /// 通用的更新方法
  Future<void> updateSettings(AppSettingsCompanion companion) async {
    final db = ref.read(appDatabaseProvider);
    final current = state.value;
    if (current == null) return;

    await (db.update(db.appSettings)..where((t) => t.id.equals(current.id)))
        .write(companion);
    
    // Riverpod 3.0/4.0 推荐 invalidateSelf 触发 build 重新运行以保持响应式
    ref.invalidateSelf();
  }

  /// 快捷更新历史限制
  Future<void> updateHistoryLimit(int limit) => 
      updateSettings(AppSettingsCompanion(historyLimit: Value(limit)));
}