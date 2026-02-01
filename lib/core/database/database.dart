import 'package:drift/drift.dart';
import 'package:drift_flutter/drift_flutter.dart';

part 'database.g.dart';

class ClipboardEntries extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get content => text()();
  TextColumn get type => text().withDefault(const Constant('text'))();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
  BoolColumn get isPinned => boolean().withDefault(const Constant(false))();
}

class AppSettings extends Table {
  IntColumn get id => integer().autoIncrement()();
  
  // 通用
  BoolColumn get launchAtStartup => boolean().withDefault(const Constant(false))();
  TextColumn get hotkeyOpen => text().withDefault(const Constant('Alt+V'))();
  BoolColumn get autoPaste => boolean().withDefault(const Constant(true))();
  BoolColumn get pastePlain => boolean().withDefault(const Constant(false))();
  TextColumn get searchMode => text().withDefault(const Constant('fuzzy'))(); // exact, fuzzy

  // 存储
  IntColumn get historyLimit => integer().withDefault(const Constant(200))();
  BoolColumn get saveText => boolean().withDefault(const Constant(true))();
  BoolColumn get saveImages => boolean().withDefault(const Constant(true))();
  BoolColumn get saveFiles => boolean().withDefault(const Constant(true))();

  // 外观
  TextColumn get popupPosition => text().withDefault(const Constant('cursor'))(); // cursor, center
  TextColumn get pinPosition => text().withDefault(const Constant('top'))(); // top, bottom
  IntColumn get imageHeight => integer().withDefault(const Constant(40))();
  IntColumn get previewDelay => integer().withDefault(const Constant(1500))();
  BoolColumn get showAppIcon => boolean().withDefault(const Constant(true))();
  BoolColumn get showAppName => boolean().withDefault(const Constant(false))();
  BoolColumn get showFooterMenu => boolean().withDefault(const Constant(true))();
  RealColumn get windowWidth => real().withDefault(const Constant(350.0))();
  TextColumn get themeMode => text().withDefault(const Constant('system'))();

  // 忽略 & 高级
  BoolColumn get isPaused => boolean().withDefault(const Constant(false))();
  BoolColumn get clearOnExit => boolean().withDefault(const Constant(false))();
  BoolColumn get clearSystemClipboard => boolean().withDefault(const Constant(false))();
  TextColumn get ignoreAppsJson => text().withDefault(const Constant('[]'))();
}

// 专门为固定项目准备的表
class Pins extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get title => text()();
  TextColumn get content => text()();
  TextColumn get hotkey => text().nullable()();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
}

@DriftDatabase(tables: [ClipboardEntries, AppSettings, Pins])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 4;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (m) async {
        await m.createAll();
      },
      onUpgrade: (m, from, to) async {
        if (from < 4) {
          // 为简单起见，在开发阶段如果版本变动较大，直接重建所有表或添加缺失列
          // 这里我们使用 createTable 确保 Pins 表存在
          await m.createTable(pins);
          // 实际生产中应逐个 addColumn，此处假设用户愿意重置或我们手动补齐
          try {
            await m.addColumn(appSettings, appSettings.hotkeyOpen);
            await m.addColumn(appSettings, appSettings.pastePlain);
            await m.addColumn(appSettings, appSettings.searchMode);
            await m.addColumn(appSettings, appSettings.saveText);
            await m.addColumn(appSettings, appSettings.saveImages);
            await m.addColumn(appSettings, appSettings.saveFiles);
            await m.addColumn(appSettings, appSettings.popupPosition);
            await m.addColumn(appSettings, appSettings.pinPosition);
            await m.addColumn(appSettings, appSettings.imageHeight);
            await m.addColumn(appSettings, appSettings.previewDelay);
            await m.addColumn(appSettings, appSettings.showAppIcon);
            await m.addColumn(appSettings, appSettings.showAppName);
            await m.addColumn(appSettings, appSettings.showFooterMenu);
            await m.addColumn(appSettings, appSettings.isPaused);
            await m.addColumn(appSettings, appSettings.clearOnExit);
            await m.addColumn(appSettings, appSettings.clearSystemClipboard);
            await m.addColumn(appSettings, appSettings.ignoreAppsJson);
          } catch (_) {}
        }
      },
    );
  }

  static QueryExecutor _openConnection() {
    return driftDatabase(
      name: 'clipboard_db',
      native: const DriftNativeOptions(
        shareAcrossIsolates: true,
      ),
    );
  }
}