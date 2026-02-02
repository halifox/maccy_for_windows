import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import '../../../core/database/database.dart';
import '../../../core/database/database_provider.dart';
import '../../../core/managers/clipboard_manager_provider.dart';
import '../../../core/managers/window_manager_provider.dart';
import 'package:drift/drift.dart';

part 'history_providers.g.dart';

@riverpod
class HistorySearchQuery extends _$HistorySearchQuery {
  @override
  String build() => '';
  
  void set(String query) => state = query;
}

@riverpod
class HistorySelectedIndex extends _$HistorySelectedIndex {
  @override
  int build() => 0;
  
  void set(int index) => state = index;
  
  void update(int Function(int) updater) => state = updater(state);
}

@riverpod
Stream<List<ClipboardEntry>> historyEntries(Ref ref) {
  final db = ref.watch(appDatabaseProvider);
  return (db.select(db.clipboardEntries)
            ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
            ..limit(50))
          .watch();
}

@riverpod
List<ClipboardEntry> filteredPins(Ref ref) {
  final history = ref.watch(historyEntriesProvider).value ?? [];
  final query = ref.watch(historySearchQueryProvider);
  
  final pins = history.where((item) => item.isPinned).toList();
  
  if (query.isEmpty) return pins;
  final lowercaseQuery = query.toLowerCase();
  return pins.where((item) => 
    item.content.toLowerCase().contains(lowercaseQuery)
  ).toList();
}

@riverpod
List<ClipboardEntry> filteredHistory(Ref ref) {
  final history = ref.watch(historyEntriesProvider).value ?? [];
  final query = ref.watch(historySearchQueryProvider);
  
  final unpinnedHistory = history.where((item) => !item.isPinned).toList();
  
  if (query.isEmpty) return unpinnedHistory;
  final lowercaseQuery = query.toLowerCase();
  return unpinnedHistory.where((item) => 
    item.content.toLowerCase().contains(lowercaseQuery)
  ).toList();
}

@riverpod
class HistoryActions extends _$HistoryActions {
  @override
  void build() {
    print('HistoryActions');
    ref.onDispose(() {
      print('HistoryActions::onDispose');

    },);
  }

  Future<void> selectItem(int index) async {
    final fPins = ref.read(filteredPinsProvider);
    final fHistory = ref.read(filteredHistoryProvider);
    final totalItems = fPins.length + fHistory.length;

    String content = "";
    if (index < fPins.length) {
      content = fPins[index].content;
    } else if (index < totalItems) {
      content = fHistory[index - fPins.length].content;
    } else {
      return;
    }

    await Clipboard.setData(ClipboardData(text: content));
    // await ref.read(appWindowManagerProvider.notifier).hideHistory();
    await ref.read(appClipboardManagerProvider.notifier).simulatePaste();
  }

  Future<void> deleteItem(int index) async {
    final fPins = ref.read(filteredPinsProvider);
    final fHistory = ref.read(filteredHistoryProvider);
    final totalItems = fPins.length + fHistory.length;

    final db = ref.read(appDatabaseProvider);
    if (index < fPins.length) {
      final item = fPins[index];
      await (db.delete(db.clipboardEntries)..where((t) => t.id.equals(item.id))).go();
    } else if (index < totalItems) {
      final item = fHistory[index - fPins.length];
      await (db.delete(db.clipboardEntries)..where((t) => t.id.equals(item.id))).go();
    }
  }

  Future<void> togglePin(int index) async {
    final fPins = ref.read(filteredPinsProvider);
    final fHistory = ref.read(filteredHistoryProvider);
    final totalItems = fPins.length + fHistory.length;

    final db = ref.read(appDatabaseProvider);
    if (index < fPins.length) {
      final item = fPins[index];
      await (db.update(db.clipboardEntries)..where((t) => t.id.equals(item.id)))
          .write(const ClipboardEntriesCompanion(isPinned: Value(false)));
    } else if (index < totalItems) {
      final item = fHistory[index - fPins.length];
      await (db.update(db.clipboardEntries)..where((t) => t.id.equals(item.id)))
          .write(const ClipboardEntriesCompanion(isPinned: Value(true)));
    }
  }

  Future<void> addItem(String content, {bool isPinned = false}) async {
    final db = ref.read(appDatabaseProvider);
    await db.into(db.clipboardEntries).insert(
      ClipboardEntriesCompanion.insert(
        content: content,
        isPinned: Value(isPinned),
      ),
    );
  }

  Future<void> deleteItemById(int id) async {
    final db = ref.read(appDatabaseProvider);
    await (db.delete(db.clipboardEntries)..where((t) => t.id.equals(id))).go();
  }
}
