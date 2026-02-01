import 'dart:io';
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

  /// 模拟系统粘贴快捷键
  Future<void> simulatePaste() async {
    // 等待窗口隐藏和焦点切换
    await Future.delayed(const Duration(milliseconds: 100));

    if (Platform.isMacOS) {
      // macOS: 使用 AppleScript 模拟 Cmd+V
      await Process.run('osascript', [
        '-e',
        'tell application "System Events" to keystroke "v" using {command down}'
      ]);
    } else if (Platform.isWindows) {
      // Windows: 使用 PowerShell 模拟 Ctrl+V
      await Process.run('powershell', [
        '-Command',
        r'$wshell = New-Object -ComObject WScript.Shell; $wshell.SendKeys("^v")'
      ]);
    } else if (Platform.isLinux) {
      // Linux: 尝试使用 xdotool (需预装)
      await Process.run('xdotool', ['key', 'ctrl+v']);
    }
  }

  @override
  void onClipboardChanged() async {
    ClipboardData? data = await Clipboard.getData(Clipboard.kTextPlain);
    if (data != null && data.text != null && data.text!.isNotEmpty) {
      final content = data.text!;
      final db = ref.read(appDatabaseProvider);
      
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