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
  IntColumn get historyLimit => integer().withDefault(const Constant(200))();
  BoolColumn get launchAtStartup => boolean().withDefault(const Constant(false))();
  TextColumn get hotkeyJson => text().nullable()();
}

@DriftDatabase(tables: [ClipboardEntries, AppSettings])
class AppDatabase extends _$AppDatabase {
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 2;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (m) async {
        await m.createAll();
      },
      onUpgrade: (m, from, to) async {
        if (from < 2) {
          await m.createTable(appSettings);
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