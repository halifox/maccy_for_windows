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
  BoolColumn get autoCheckUpdates => boolean().withDefault(const Constant(true))();
  TextColumn get hotkeyOpen => text().withDefault(const Constant('Alt+V'))();
  TextColumn get hotkeyPin => text().withDefault(const Constant(''))();
  TextColumn get hotkeyDelete => text().withDefault(const Constant(''))();
  BoolColumn get autoPaste => boolean().withDefault(const Constant(true))();
  BoolColumn get pastePlain => boolean().withDefault(const Constant(false))();
  TextColumn get searchMode => text().withDefault(const Constant('fuzzy'))(); // exact, fuzzy, regex, mixed

  // 存储
  IntColumn get historyLimit => integer().withDefault(const Constant(200))();
  BoolColumn get saveText => boolean().withDefault(const Constant(true))();
  BoolColumn get saveImages => boolean().withDefault(const Constant(true))();
  BoolColumn get saveFiles => boolean().withDefault(const Constant(true))();

  TextColumn get popupPosition => text().withDefault(const Constant('cursor'))(); // cursor, center
  TextColumn get pinPosition => text().withDefault(const Constant('top'))(); // top, bottom
  IntColumn get imageHeight => integer().withDefault(const Constant(40))();
  IntColumn get previewDelay => integer().withDefault(const Constant(1500))();
  TextColumn get highlightMatch => text().withDefault(const Constant('bold'))(); // bold, color
  
  BoolColumn get showSpecialChars => boolean().withDefault(const Constant(false))();
  BoolColumn get showMenuBarIcon => boolean().withDefault(const Constant(true))();
  TextColumn get menuBarIconType => text().withDefault(const Constant('clipboard'))();
  BoolColumn get showClipboardNearIcon => boolean().withDefault(const Constant(false))();
  TextColumn get showSearchBox => text().withDefault(const Constant('always'))(); // always, never, typing
  BoolColumn get showAppName => boolean().withDefault(const Constant(false))();
  BoolColumn get showAppIcon => boolean().withDefault(const Constant(true))();
  BoolColumn get showFooterMenu => boolean().withDefault(const Constant(true))();
  RealColumn get windowWidth => real().withDefault(const Constant(350.0))();
  TextColumn get themeMode => text().withDefault(const Constant('system'))();

  // 忽略 & 高级
  BoolColumn get isPaused => boolean().withDefault(const Constant(false))();
  BoolColumn get clearOnExit => boolean().withDefault(const Constant(false))();
  BoolColumn get clearSystemClipboard => boolean().withDefault(const Constant(false))();
  TextColumn get ignoreAppsJson => text().withDefault(const Constant('[]'))();
}

@DriftDatabase(tables: [ClipboardEntries, AppSettings])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 5;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (m) async {
        await m.createAll();
      },
      onUpgrade: (m, from, to) async {
        if (from < 5) {
          // Migration logic to handle isPinned addition if it wasn't there in previous versions
          // and potentially migrate data from Pins table if it existed.
          // For now, focusing on schema cleanup.
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.isPinned);
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