import 'package:riverpod_annotation/riverpod_annotation.dart';
import '../../../core/database/database.dart';
import '../../../core/database/database_provider.dart';
import 'package:drift/drift.dart';

part 'settings_provider.g.dart';

@Riverpod(keepAlive: true)
class SettingsNotifier extends _$SettingsNotifier {
  @override
  Future<AppSetting> build() async {
    print('SettingsNotifier: building...');
    try {
      final db = ref.watch(appDatabaseProvider);
      print('SettingsNotifier: fetching setting...');
      final setting = await db.select(db.appSettings).getSingleOrNull();
      print('SettingsNotifier: setting is $setting');
      
      if (setting == null) {
        print('SettingsNotifier: creating default setting...');
        final id = await db.into(db.appSettings).insert(
          AppSettingsCompanion.insert(
            historyLimit: const Value(200),
            launchAtStartup: const Value(false),
          ),
        );
        print('SettingsNotifier: created default setting with id $id');
        return AppSetting(
          id: id,
          historyLimit: 200,
          launchAtStartup: false,
          hotkeyJson: null,
        );
      }
      return setting;
    } catch (e, stack) {
      print('SettingsNotifier error: $e');
      print(stack);
      rethrow;
    }
  }

  Future<void> updateHistoryLimit(int limit) async {
    final db = ref.read(appDatabaseProvider);
    await (db.update(db.appSettings)..where((t) => t.id.equals(state.value!.id)))
        .write(AppSettingsCompanion(historyLimit: Value(limit)));
    ref.invalidateSelf();
  }

  Future<void> toggleLaunchAtStartup(bool value) async {
    final db = ref.read(appDatabaseProvider);
    await (db.update(db.appSettings)..where((t) => t.id.equals(state.value!.id)))
        .write(AppSettingsCompanion(launchAtStartup: Value(value)));
    ref.invalidateSelf();
  }
}
