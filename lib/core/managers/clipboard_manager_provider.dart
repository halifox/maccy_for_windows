import 'dart:async';
import 'dart:io';

import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:drift/drift.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/database/database_provider.dart';
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
  void setSelfUpdate(bool value) {
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
    }

    try {
      const platform = MethodChannel('com.hali.clip/native_utils');
      await platform.invokeMethod('restoreAndPaste');
      if (Platform.isWindows) {
        await windowManager.hide();
      }
    } catch (e) {
      debugPrint('[ClipboardManager] 模拟粘贴失败: $e');
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
    final isPaused = ref.read(isPausedProvider);
    if (isPaused) return;
    final saveText = ref.read(saveTextProvider);
    final saveImage = ref.read(saveImagesProvider);
    final saveFile = ref.read(saveFilesProvider);

    final reader = await SystemClipboard.instance?.read();
    if (reader == null) return;

    final formats = Formats.standardFormats
        .where((element) => reader.canProvide(element))
        .toList();
    debugPrint(formats.toString());

    if (reader.canProvide(Formats.fileUri)) {
      if (!saveFile) return;
      reader.getValue(Formats.fileUri, (fileUri) {
        if (fileUri != null) {
          upsertClipboardEntry(fileUri.toFilePath(), 'file');
        }
      });
      return;
    }
    if (!reader.canProvide(Formats.fileUri) && reader.canProvide(Formats.png)) {
      if (!saveImage) return;
      reader.getFile(Formats.png, (file) {
        saveStreamToFile(file.getStream(), 'png');
      });
      return;
    }
    if (!reader.canProvide(Formats.fileUri) &&
        reader.canProvide(Formats.jpeg)) {
      if (!saveImage) return;
      reader.getFile(Formats.jpeg, (file) {
        saveStreamToFile(file.getStream(), 'jpeg');
      });
      return;
    }
    if (!reader.canProvide(Formats.fileUri) &&
        reader.canProvide(Formats.webp)) {
      if (!saveImage) return;
      reader.getFile(Formats.webp, (file) {
        saveStreamToFile(file.getStream(), 'webp');
      });
      return;
    }
    if (reader.canProvide(Formats.plainText)) {
      if (!saveText) return;
      reader.getValue(Formats.plainText, (text) {
        if (text != null) {
          upsertClipboardEntry(text, 'text');
        }
      });
    }
  }

  Future<void> upsertClipboardEntry(String content, String type) async {
    final db = ref.read(appDatabaseProvider);
    await db
        .into(db.clipboardEntries)
        .insert(
          ClipboardEntriesCompanion.insert(
            content: content,
            type: Value(type),
            createdAt: Value(DateTime.now()),
          ),
          onConflict: DoUpdate(
            (old) => ClipboardEntriesCompanion(
              createdAt: Value(DateTime.now()),
              type: Value(type),
            ),
            target: [db.clipboardEntries.content],
          ),
        );
  }

  /// 处理并持久化文本/HTML/URI 类型的剪贴板条目。

  /// 处理并持久化二进制类型的剪贴板条目。
  Future<void> saveStreamToFile(
    Stream<Uint8List> dataStream,
    String extension,
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
        print('写入失败: $e');
      } finally {
        // 3. 必须关闭 sink 确保缓冲区数据全部刷入硬盘
        await sink.close();
      }
    }

    await upsertClipboardEntry(filePath, 'image');
  }
}
