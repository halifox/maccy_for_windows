import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import '../database/database_provider.dart';
import '../database/database.dart';
import 'package:drift/drift.dart';

part 'clipboard_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppClipboardManager extends _$AppClipboardManager with ClipboardListener {
  @override
  void build() {
    clipboardWatcher.addListener(this);
  }

  void start() {
    clipboardWatcher.start();
  }

  void stop() {
    clipboardWatcher.stop();
  }

  @override
  void onClipboardChanged() async {
    ClipboardData? data = await Clipboard.getData(Clipboard.kTextPlain);
    if (data != null && data.text != null && data.text!.isNotEmpty) {
      final content = data.text!;
      
      final db = ref.read(appDatabaseProvider);
      
      // Check if last entry is the same to avoid duplicates
      final lastEntry = await (db.select(db.clipboardEntries)
        ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
        ..limit(1))
        .getSingleOrNull();
        
      if (lastEntry?.content != content) {
        await db.into(db.clipboardEntries).insert(
          ClipboardEntriesCompanion.insert(
            content: content,
            type: const Value('text'),
            createdAt: Value(DateTime.now()),
          ),
        );
      }
    }
  }
}
