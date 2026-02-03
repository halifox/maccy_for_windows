import 'dart:async';
import 'dart:io';

import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:drift/drift.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:window_manager/window_manager.dart';

import '../../features/settings/providers/settings_provider.dart';
import '../database/database.dart';
import '../database/database_provider.dart';

part 'clipboard_manager_provider.g.dart';

/// 剪贴板管理器，负责监听系统剪贴板变化、处理数据过滤并持久化到数据库。
@Riverpod(keepAlive: true)
class AppClipboardManager extends _$AppClipboardManager with ClipboardListener {
  /// 定时清理任务计时器
  Timer? _cleanupTimer;

  @override
  FutureOr<void> build() async {
    debugPrint('🚀 AppClipboardManager: 开始初始化剪贴板监听...');
    clipboardWatcher.addListener(this);
    await clipboardWatcher.start();

    // 启动定时清理器：每 5 分钟执行一次后台数据库缩减
    _cleanupTimer = Timer.periodic(const Duration(minutes: 5), (_) {
      debugPrint('🕒 AppClipboardManager: 执行定时数据库清理任务...');
      final limit = ref.read(historyLimitProvider);
      _pruneHistory(limit);
    });

    ref.onDispose(() {
      debugPrint('🛑 AppClipboardManager: 正在停止监听并清理资源...');
      clipboardWatcher.stop();
      clipboardWatcher.removeListener(this);
      _cleanupTimer?.cancel();
    });
    debugPrint('✅ AppClipboardManager: 剪贴板服务已启动');
  }

  /// 清理超出限制的历史记录
  Future<void> _pruneHistory(int limit) async {
    debugPrint('🧹 AppClipboardManager: 检查历史记录数量，限制为 $limit...');
    final db = ref.read(appDatabaseProvider);
    // 只统计非固定的条目，或者简单地按总量清理
    final entriesCount = await db.clipboardEntries.count().getSingle();
    
    if (entriesCount > limit) {
      debugPrint('🧹 AppClipboardManager: 当前条目 $entriesCount 超出限制，正在执行清理...');
      // 找到按时间排序第 limit 条记录的 ID，作为清理阈值
      final entriesToKeep = await (db.select(db.clipboardEntries)
            ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
            ..limit(limit))
          .get();

      if (entriesToKeep.isNotEmpty) {
        final oldestIdToKeep = entriesToKeep.last.id;
        await (db.delete(db.clipboardEntries)
              ..where((t) => t.id.isSmallerThanValue(oldestIdToKeep)))
            .go();
        debugPrint('✅ AppClipboardManager: 定期清理完成，已保留最新的 $limit 条记录。');
      }
    } else {
      debugPrint('✅ AppClipboardManager: 当前条目未超出限制，无需清理。');
    }
  }

  /// 检查是否拥有 macOS 辅助功能权限（用于模拟粘贴）。
  Future<bool> checkAccessibilityPermissions() async {
    if (!Platform.isMacOS) return true;
    debugPrint('🔍 AppClipboardManager: 正在检查 macOS 辅助功能权限...');
    try {
      final result = await Process.run('osascript', ['-e', 'tell application "System Events" to return UI elements enabled']);
      final hasPermission = result.stdout.trim() == 'true';
      debugPrint('🔍 AppClipboardManager: macOS 辅助功能权限状态: $hasPermission');
      return hasPermission;
    } catch (e) {
      debugPrint('❌ AppClipboardManager: 检查权限出错: $e');
      return false;
    }
  }

  /// 打开系统设置以请求 macOS 辅助功能权限。
  Future<void> requestAccessibilityPermissions() async {
    if (!Platform.isMacOS) return;
    debugPrint('📢 AppClipboardManager: 正在请求 macOS 辅助功能权限，打开系统设置...');

    // Open System Settings to the Accessibility Privacy page
    await Process.run('open', ['x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility']);
  }

  /// 模拟系统粘贴操作（Ctrl+V 或 Command+V）。
  /// 在自动粘贴功能开启时，会尝试将当前剪贴板内容注入到之前的活跃窗口。
  Future<void> simulatePaste() async {
    final autoPaste = ref.read(autoPasteProvider);
    debugPrint('📋 AppClipboardManager: 尝试模拟粘贴 (自动粘贴: $autoPaste)...');
    if (!autoPaste) return;

    if (Platform.isMacOS) {
      final hasPermission = await checkAccessibilityPermissions();
      if (!hasPermission) {
        debugPrint('⚠️ AppClipboardManager: macOS 缺少辅助功能权限，无法粘贴');
        return;
      }
    }

    if (Platform.isMacOS) {
      debugPrint('📋 AppClipboardManager: macOS 正在通过 Native (CGEvent) 执行模拟粘贴...');
      try {
        const platform = MethodChannel('com.hali.clip/native_utils');
        await platform.invokeMethod('restoreAndPaste');
        debugPrint('✅ AppClipboardManager: Native 粘贴指令已发送');
      } catch (e) {
        debugPrint('❌ AppClipboardManager: Native 粘贴失败: $e');
        // 如果原生失败，可以考虑回退到 osascript
      }
    } else if (Platform.isWindows) {
      debugPrint('📋 AppClipboardManager: Windows 正在通过 Native (SendInput) 执行模拟粘贴...');
      try {
        const platform = MethodChannel('com.hali.clip/native_utils');
        await platform.invokeMethod('restoreAndPaste');
        await windowManager.hide();
        debugPrint('✅ AppClipboardManager: Windows Native 粘贴指令已发送');
      } catch (e) {
        debugPrint('❌ AppClipboardManager: Windows Native 粘贴失败: $e');
      }
    }
  }

  /// 当系统剪贴板内容发生变化时的回调处理
  @override
  void onClipboardChanged() async {
    final isPaused = ref.read(isPausedProvider);
    debugPrint('🔔 AppClipboardManager: 检测到剪贴板变化 (暂停状态: $isPaused)');
    if (isPaused) return;

    final historyLimit = ref.read(historyLimitProvider);
    final saveText = ref.read(saveTextProvider);
    final saveImages = ref.read(saveImagesProvider);
    final saveFiles = ref.read(saveFilesProvider);

    // 1. 处理文本 (Text Snippets)
    if (saveText) {
      ClipboardData? data = await Clipboard.getData(Clipboard.kTextPlain);
      if (data != null && data.text != null && data.text!.trim().isNotEmpty) {
        debugPrint('📝 AppClipboardManager: 捕获到文本内容 (长度: ${data.text!.length})');
        await _handleTextEntry(data.text!, historyLimit);
      }
    }

    // 2. TODO: 处理图片 (Images)
    if (saveImages) {
      debugPrint('🖼️ AppClipboardManager: 图片捕获功能暂未实现');
    }

    // 3. TODO: 处理文件/文件夹 (Files & Folders)
    if (saveFiles) {
      debugPrint('📁 AppClipboardManager: 文件捕获功能暂未实现');
    }
  }

  /// 处理并持久化文本类型的剪贴板条目
  Future<void> _handleTextEntry(String content, int historyLimit) async {
    debugPrint('💾 AppClipboardManager: 正在保存文本条目到数据库...');
    final db = ref.read(appDatabaseProvider);

    // 使用 Upsert 原子操作：尝试插入，如果内容冲突（已存在）则更新时间
    await db.into(db.clipboardEntries).insert(
          ClipboardEntriesCompanion.insert(
            content: content,
            type: const Value('text'),
            createdAt: Value(DateTime.now()),
          ),
          onConflict: DoUpdate(
            (old) => ClipboardEntriesCompanion(
              createdAt: Value(DateTime.now()),
            ),
            target: [db.clipboardEntries.content],
          ),
        );
    debugPrint('✅ AppClipboardManager: 文本条目已更新/存入数据库');
  }
}
