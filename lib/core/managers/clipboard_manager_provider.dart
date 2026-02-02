import 'dart:io';

import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:drift/drift.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:window_manager/window_manager.dart';

import '../../features/settings/providers/settings_provider.dart';
import '../database/database.dart';
import '../database/database_provider.dart';

part 'clipboard_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppClipboardManager extends _$AppClipboardManager with ClipboardListener {
  @override
  void build() {
    print("AppClipboardManager");
    clipboardWatcher.addListener(this);
    clipboardWatcher.start();
    ref.onDispose(() {
      clipboardWatcher.stop();
      clipboardWatcher.removeListener(this);
    });
  }

  Future<bool> checkAccessibilityPermissions() async {
    if (!Platform.isMacOS) return true;
    try {
      final result = await Process.run('osascript', ['-e', 'tell application "System Events" to return UI elements enabled']);
      return result.stdout.trim() == 'true';
    } catch (_) {
      return false;
    }
  }

  Future<void> requestAccessibilityPermissions() async {
    if (!Platform.isMacOS) return;

    // Open System Settings to the Accessibility Privacy page
    await Process.run('open', ['x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility']);
  }

  Future<void> simulatePaste() async {
    final settings = await ref.read(settingsProvider.future);
    if (!settings.autoPaste) return;

    if (Platform.isMacOS) {
      final hasPermission = await checkAccessibilityPermissions();
      if (!hasPermission) {
        print('Auto-paste: Missing Accessibility permissions on macOS.');
        return;
      }
    }

    ProcessResult result;
    if (Platform.isMacOS) {
    } else if (Platform.isWindows) {
      await windowManager.minimize();
      await Process.run('powershell', ['-Command', 'Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait("^v")']);
      await windowManager.hide();
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

      final lastEntry =
          await (db.select(db.clipboardEntries)
                ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
                ..limit(1))
              .getSingleOrNull();

      if (lastEntry?.content != content) {
        // 5. Enforce history limit
        final count = await db.clipboardEntries.count().getSingle();
        if (count >= settings.historyLimit) {
          final oldest =
              await (db.select(db.clipboardEntries)
                    ..orderBy([(t) => OrderingTerm.asc(t.createdAt)])
                    ..limit(1))
                  .getSingle();
          await (db.delete(db.clipboardEntries)..where((t) => t.id.equals(oldest.id))).go();
        }

        await db.into(db.clipboardEntries).insert(ClipboardEntriesCompanion.insert(content: content, type: const Value('text'), createdAt: Value(DateTime.now())));
      }
    }
  }
}
