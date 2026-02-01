import 'package:riverpod_annotation/riverpod_annotation.dart';
import '../../../core/database/database.dart';
import '../../../core/database/database_provider.dart';
import 'package:drift/drift.dart';

part 'pins_provider.g.dart';

@Riverpod(keepAlive: true)
class PinsNotifier extends _$PinsNotifier {
  @override
  Stream<List<Pin>> build() {
    final db = ref.watch(appDatabaseProvider);
    return (db.select(db.pins)..orderBy([(t) => OrderingTerm.desc(t.createdAt)])).watch();
  }

  Future<void> addPin(String title, String content, {String? hotkey}) async {
    final db = ref.read(appDatabaseProvider);
    await db.into(db.pins).insert(
      PinsCompanion.insert(
        title: title,
        content: content,
        hotkey: Value(hotkey),
      ),
    );
  }

  Future<void> deletePin(int id) async {
    final db = ref.read(appDatabaseProvider);
    await (db.delete(db.pins)..where((t) => t.id.equals(id))).go();
  }
}
