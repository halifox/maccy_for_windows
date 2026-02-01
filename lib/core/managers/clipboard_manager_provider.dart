import 'dart:io';
import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import '../database/database_provider.dart';
import '../database/database.dart';
import 'package:drift/drift.dart';
import '../../features/settings/providers/settings_provider.dart';

part 'clipboard_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppClipboardManager extends _$AppClipboardManager with ClipboardListener {
  @override
  void build() {
    clipboardWatcher.addListener(this);
  }

  void start() => clipboardWatcher.start();
  void stop() => clipboardWatcher.stop();

  Future<void> simulatePaste() async {
    final settings = await ref.read(settingsProvider.future);
    if (!settings.autoPaste) return;

    await Future.delayed(const Duration(milliseconds: 150));

    if (Platform.isMacOS) {
      await Process.run('osascript', [
        '-e',
        'tell application "System Events" to keystroke "v" using {command down}'
      ]);
    } else if (Platform.isWindows) {
      await Process.run('powershell', [
        '-Command',
        r'$wshell = New-Object -ComObject WScript.Shell; $wshell.SendKeys("^v")'
      ]);
    }
  }

  @override
  void onClipboardChanged() async {
    final settings = await ref.read(settingsProvider.future);
    
    // 1. Check if recording is paused
    if (settings.isPaused) return;

    // 2. Read clipboard data
    // Note: Drift currently supports text well. Images/Files would require blob storage.
    ClipboardData? data = await Clipboard.getData(Clipboard.kTextPlain);
    
    if (data != null && data.text != null && data.text!.isNotEmpty) {
      String content = data.text!;

      // 3. Filter by type (Text)
      if (!settings.saveText) return;

      // 4. Regex filtering (Placeholder for now)
      // if (settings.regexFilter != null && RegExp(settings.regexFilter!).hasMatch(content)) return;

      final db = ref.read(appDatabaseProvider);
      
      final lastEntry = await (db.select(db.clipboardEntries)
        ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
        ..limit(1))
        .getSingleOrNull();
        
      if (lastEntry?.content != content) {
        // 5. Enforce history limit
        final count = await db.clipboardEntries.count().getSingle();
        if (count >= settings.historyLimit) {
          final oldest = await (db.select(db.clipboardEntries)
            ..orderBy([(t) => OrderingTerm.asc(t.createdAt)])
            ..limit(1))
            .getSingle();
          await (db.delete(db.clipboardEntries)..where((t) => t.id.equals(oldest.id))).go();
        }

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
