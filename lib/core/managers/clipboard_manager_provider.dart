import 'dart:async';
import 'dart:io';

import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:drift/drift.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/database/database_provider.dart';
import 'package:maccy/core/services/clipboard_filter_service.dart';
import 'package:maccy/core/services/foreground_app_service.dart';
import 'package:maccy/core/services/rich_text_service.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:stream_transform/stream_transform.dart';
import 'package:super_clipboard/super_clipboard.dart';
import 'package:window_manager/window_manager.dart';

part 'clipboard_manager_provider.g.dart';

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 字段说明:
/// [_cleanupTimer] 定时清理任务的计时器，用于定期执行数据库容量缩减。
@Riverpod(keepAlive: true)
class AppClipboardManager extends _$AppClipboardManager with ClipboardListener {
  Timer? _cleanupTimer;
  final _clipboardUpdateController = StreamController<void>();
  bool _isSelfUpdate = false;

  /// 设置是否为自身更新操作。
  ///
  /// 当应用自身修改剪贴板时，应将此标志设为 true，以避免监听器重复处理。
  set isSelfUpdate(bool value) {
    _isSelfUpdate = value;
  }

  /// 初始化剪贴板管理器。
  ///
  /// 启动监听插件并配置定时清理任务。
  @override
  FutureOr<void> build() async {
    clipboardWatcher.addListener(this);
    await clipboardWatcher.start();
    _cleanupTimer = Timer.periodic(const Duration(minutes: 5), (_) {
      final limit = ref.read(historyLimitProvider);
      _pruneHistory(limit);
    });

    _clipboardUpdateController.stream
        .debounce(const Duration(milliseconds: 200))
        .listen((_) {
          _processClipboardChange();
        });

    ref.onDispose(() {
      clipboardWatcher.stop();
      clipboardWatcher.removeListener(this);
      _cleanupTimer?.cancel();
      _clipboardUpdateController.close();
    });
    debugPrint('[ClipboardManager] 服务已启动');
  }

  /// 清理超出限制的历史记录。
  ///
  /// 检查当前数据库记录总数，若超过 [limit] 则删除较旧的非置顶记录。
  ///
  /// [limit] 允许保留的最大记录条数。
  Future<void> _pruneHistory(int limit) async {
    final db = ref.read(appDatabaseProvider);
    final entriesCount = await db.clipboardEntries.count().getSingle();

    if (entriesCount > limit) {
      final entriesToKeep =
          await (db.select(db.clipboardEntries)
                ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
                ..limit(limit))
              .get();

      if (entriesToKeep.isNotEmpty) {
        final oldestIdToKeep = entriesToKeep.last.id;
        await (db.delete(
          db.clipboardEntries,
        )..where((t) => t.id.isSmallerThanValue(oldestIdToKeep))).go();
        debugPrint('[ClipboardManager] 数据库清理完成，保留最新 $limit 条记录');
      }
    }
  }

  /// 检查是否拥有 macOS 辅助功能权限。
  ///
  /// 模拟粘贴功能在 macOS 上需要该权限才能注入按键事件。
  ///
  /// 返回 [bool] 是否拥有权限。
  Future<bool> checkAccessibilityPermissions() async {
    if (!Platform.isMacOS) return true;
    try {
      final result = await Process.run('osascript', [
        '-e',
        'tell application "System Events" to return UI elements enabled',
      ]);
      return result.stdout.trim() == 'true';
    } catch (e) {
      debugPrint('[ClipboardManager] 检查权限异常: $e');
      return false;
    }
  }

  /// 打开系统设置以请求 macOS 辅助功能权限。
  Future<void> requestAccessibilityPermissions() async {
    if (!Platform.isMacOS) return;
    await Process.run('open', [
      'x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility',
    ]);
  }

  /// 模拟系统粘贴操作。
  ///
  /// 根据当前平台，通过 Native 管道（CGEvent 或 SendInput）发送粘贴快捷键指令，
  /// 旨在将选中的历史条目直接注入到之前的活跃窗口中。
  Future<void> simulatePaste() async {
    final autoPaste = ref.read(autoPasteProvider);
    if (!autoPaste) return;

    if (Platform.isMacOS) {
      final hasPermission = await checkAccessibilityPermissions();
      if (!hasPermission) {
        debugPrint('[ClipboardManager] 粘贴失败: 缺少 macOS 辅助功能权限');
        return;
      }

      try {
        const platform = MethodChannel('com.hali.clip/native_utils');
        await platform.invokeMethod('restoreAndPaste');
      } catch (e) {
        debugPrint('[ClipboardManager] macOS 模拟粘贴失败: $e');
      }
    } else if (Platform.isWindows) {
      // 使用 Win32 SendInput API 模拟粘贴
      try {
        // 先隐藏窗口，恢复之前的活跃窗口
        await windowManager.hide();

        // 等待窗口完全隐藏并焦点切换
        await Future<void>.delayed(const Duration(milliseconds: 100));

        // 执行粘贴
        await _simulateWindowsPaste();
      } catch (e) {
        debugPrint('[ClipboardManager] Windows 模拟粘贴失败: $e');
      }
    }
  }

  /// 系统剪贴板内容变化时的回调。
  ///
  /// 负责响应系统剪贴板事件，判断当前应用是否处于暂停监听状态，
  /// 并根据用户设置决定是否保存文本、图片或文件内容。
  @override
  Future<void> onClipboardChanged() async {
    if (_isSelfUpdate) return;
    _clipboardUpdateController.add(null);
  }

  Future<void> _processClipboardChange() async {
    debugPrint('Processing Clipboard Change');

    // 1. 检查是否暂停监听
    final isPaused = ref.read(ignoreEventsProvider);
    if (isPaused) return;

    // 2. 获取前台应用名称（Windows）
    String? appName;
    if (Platform.isWindows) {
      appName = ForegroundAppService.getForegroundAppName();
      debugPrint('[ClipboardManager] 来源应用: $appName');

      // 3. 应用过滤检查
      final ignoredApps = ref.read(ignoredAppsProvider);
      final isWhitelistMode = ref.read(ignoreAllAppsExceptListedProvider);

      if (ClipboardFilterService.shouldIgnoreApp(
        appName,
        ignoredApps: ignoredApps,
        isWhitelistMode: isWhitelistMode,
      )) {
        debugPrint('[ClipboardManager] 应用被过滤: $appName');
        return;
      }
    }

    // 4. 检查内容类型设置
    final saveText = ref.read(saveTextProvider);
    final saveImage = ref.read(saveImagesProvider);
    final saveFile = ref.read(saveFilesProvider);

    final reader = await SystemClipboard.instance?.read();
    if (reader == null) return;

    final formats = Formats.standardFormats
        .where((element) => reader.canProvide(element))
        .toList();
    debugPrint(formats.toString());

    // 5. 处理文件类型
    if (reader.canProvide(Formats.fileUri)) {
      if (!saveFile) return;
      reader.getValue(Formats.fileUri, (fileUri) {
        if (fileUri != null) {
          upsertClipboardEntry(fileUri.toFilePath(), 'file', appName);
        }
      });
      return;
    }

    // 6. 处理图片类型
    if (!reader.canProvide(Formats.fileUri) && reader.canProvide(Formats.png)) {
      if (!saveImage) return;
      reader.getFile(Formats.png, (file) {
        saveStreamToFile(file.getStream(), 'png', appName);
      });
      return;
    }
    if (!reader.canProvide(Formats.fileUri) &&
        reader.canProvide(Formats.jpeg)) {
      if (!saveImage) return;
      reader.getFile(Formats.jpeg, (file) {
        saveStreamToFile(file.getStream(), 'jpeg', appName);
      });
      return;
    }
    if (!reader.canProvide(Formats.fileUri) &&
        reader.canProvide(Formats.webp)) {
      if (!saveImage) return;
      reader.getFile(Formats.webp, (file) {
        saveStreamToFile(file.getStream(), 'webp', appName);
      });
      return;
    }

    // 7. 处理文本类型
    if (reader.canProvide(Formats.plainText)) {
      if (!saveText) return;
      reader.getValue(Formats.plainText, (text) {
        if (text != null) {
          // 8. 正则表达式过滤
          final regexPatterns = ref.read(ignoreRegexpProvider);
          if (ClipboardFilterService.shouldIgnoreContent(text, regexPatterns)) {
            debugPrint('[ClipboardManager] 内容被正则过滤');
            return;
          }

          // 9. 读取富文本格式（仅 Windows）
          String? htmlContent;
          String? rtfContent;
          if (Platform.isWindows) {
            try {
              htmlContent = RichTextService.readHtmlFromClipboard();
              rtfContent = RichTextService.readRtfFromClipboard();
              if (htmlContent != null) {
                debugPrint('[ClipboardManager] 检测到 HTML 格式');
              }
              if (rtfContent != null) {
                debugPrint('[ClipboardManager] 检测到 RTF 格式');
              }
            } catch (e) {
              debugPrint('[ClipboardManager] 读取富文本失败: $e');
            }
          }

          upsertClipboardEntry(
            text,
            'text',
            appName,
            htmlContent: htmlContent,
            rtfContent: rtfContent,
          );
        }
      });
    }
  }

  Future<void> upsertClipboardEntry(
    String content,
    String type,
    String? appName, {
    String? htmlContent,
    String? rtfContent,
  }) async {
    final db = ref.read(appDatabaseProvider);

    // 检查是否已存在
    final existing = await (db.select(db.clipboardEntries)
          ..where((t) => t.content.equals(content)))
        .getSingleOrNull();

    if (existing != null) {
      // 已存在，更新复制次数和时间
      await (db.update(db.clipboardEntries)
            ..where((t) => t.id.equals(existing.id)))
          .write(
        ClipboardEntriesCompanion(
          copyCount: Value(existing.copyCount + 1),
          lastCopiedAt: Value(DateTime.now()),
          appName: Value(appName),
          htmlContent: Value(htmlContent),
          rtfContent: Value(rtfContent),
        ),
      );
      debugPrint('[ClipboardManager] 更新已存在条目，复制次数: ${existing.copyCount + 1}');
    } else {
      // 新条目，插入
      await db.into(db.clipboardEntries).insert(
            ClipboardEntriesCompanion.insert(
              content: content,
              type: Value(type),
              createdAt: Value(DateTime.now()),
              appName: Value(appName),
              copyCount: const Value(1),
              firstCopiedAt: Value(DateTime.now()),
              lastCopiedAt: Value(DateTime.now()),
              htmlContent: Value(htmlContent),
              rtfContent: Value(rtfContent),
            ),
          );
      debugPrint('[ClipboardManager] 插入新条目');
    }
  }

  /// 处理并持久化文本/HTML/URI 类型的剪贴板条目。

  /// 处理并持久化二进制类型的剪贴板条目。
  Future<void> saveStreamToFile(
    Stream<Uint8List> dataStream,
    String extension,
    String? appName,
  ) async {
    // 保存到本地文件
    final appDir = await getApplicationDocumentsDirectory();
    final storageDir = Directory(p.join(appDir.path, 'clipboard_storage'));
    if (!await storageDir.exists()) {
      await storageDir.create(recursive: true);
    }

    final name = '${DateTime.now().millisecondsSinceEpoch}';
    final fileName = '$name.$extension';
    final filePath = p.join(storageDir.path, fileName);
    final file = File(filePath);

    // 如果文件不存在则写入
    if (!await file.exists()) {
      final sink = file.openWrite();

      try {
        // 2. 将数据流直接对接给 sink
        await sink.addStream(dataStream);
      } catch (e) {
        debugPrint('写入失败: $e');
      } finally {
        // 3. 必须关闭 sink 确保缓冲区数据全部刷入硬盘
        await sink.close();
      }
    }

    await upsertClipboardEntry(filePath, 'image', appName);
  }

  /// Windows 平台模拟粘贴操作
  Future<void> _simulateWindowsPaste() async {
    try {
      const platform = MethodChannel('com.hali.clip/native_utils');
      await platform.invokeMethod('simulatePaste');
    } catch (e) {
      debugPrint('[ClipboardManager] Windows 粘贴失败: $e');
    }
  }
}
