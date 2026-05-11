import 'dart:async';
import 'dart:convert';

import 'package:clipboard_watcher/clipboard_watcher.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';
import 'package:maccy/core/services/clipboard_filter_service.dart';
import 'package:maccy/core/services/foreground_app_service.dart';
import 'package:maccy/core/services/rich_text_service.dart';
import 'package:maccy/features/history/repositories/history_repository.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:path/path.dart' as p;
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:stream_transform/stream_transform.dart';
import 'package:super_clipboard/super_clipboard.dart';

part 'clipboard_manager_provider.g.dart';

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 实现 Maccy 的多格式内容存储逻辑。
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
      // 清理任务由 repository 的 _limitHistorySize 自动处理
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
  }

  /// 模拟系统粘贴操作。
  ///
  /// 根据当前平台，通过 Native 管道（CGEvent 或 SendInput）发送粘贴快捷键指令，
  /// 旨在将选中的历史条目直接注入到之前的活跃窗口中。
  Future<void> simulatePaste() async {
    final autoPaste = ref.read(autoPasteProvider);
    if (!autoPaste) return;

    await _simulateWindowsPaste();
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

  /// 处理剪贴板变化（实现 Maccy 的多格式逻辑）。
  Future<void> _processClipboardChange() async {
    // 1. 检查是否暂停监听
    final isPaused = ref.read(ignoreEventsProvider);
    if (isPaused) return;

    // 2. 获取前台应用名称
    String? appName;
    appName = ForegroundAppService.getForegroundAppName();

    // 3. 应用过滤检查
    final ignoredApps = ref.read(ignoredAppsProvider);
    final isWhitelistMode = ref.read(ignoreAllAppsExceptListedProvider);

    if (ClipboardFilterService.shouldIgnoreApp(
      appName,
      ignoredApps: ignoredApps,
      isWhitelistMode: isWhitelistMode,
    )) {
      return;
    }

    // 4. 读取剪贴板内容
    final reader = await SystemClipboard.instance?.read();
    if (reader == null) return;

    // 5. 收集所有可用格式的内容（Maccy 的多格式存储）
    final contents = <HistoryItemContentData>[];
    String? titleText;

    // 5.1 文本格式
    if (reader.canProvide(Formats.plainText)) {
      final text = await reader.readValue(Formats.plainText);
      if (text != null && text.isNotEmpty) {
        // 正则表达式过滤
        final regexPatterns = ref.read(ignoreRegexpProvider);
        if (ClipboardFilterService.shouldIgnoreContent(text, regexPatterns)) {
          return;
        }

        contents.add(HistoryItemContentData(
          type: 'text/plain',
          value: Uint8List.fromList(utf8.encode(text)),
        ));
        titleText = text;
      }
    }

    // 5.2 HTML 格式
    if (reader.canProvide(Formats.htmlText)) {
      final html = await reader.readValue(Formats.htmlText);
      if (html != null && html.isNotEmpty) {
        contents.add(HistoryItemContentData(
          type: 'text/html',
          value: Uint8List.fromList(utf8.encode(html)),
        ));
      }
    }

    // 5.3 RTF 格式
    try {
      final rtf = RichTextService.readRtfFromClipboard();
      if (rtf != null && rtf.isNotEmpty) {
        contents.add(HistoryItemContentData(
          type: 'text/rtf',
          value: Uint8List.fromList(utf8.encode(rtf)),
        ));
      }
    } catch (e) {
      debugPrint(e.toString());
    }

    // 5.4 图片格式
    if (reader.canProvide(Formats.png)) {
      try {
        final imageData = await _readImageData(reader, Formats.png);
        if (imageData != null) {
          contents.add(HistoryItemContentData(
            type: 'image/png',
            value: imageData,
          ));
          titleText ??= '[图片]';
        }
      } catch (e) {
        debugPrint(e.toString());
      }
    } else if (reader.canProvide(Formats.jpeg)) {
      try {
        final imageData = await _readImageData(reader, Formats.jpeg);
        if (imageData != null) {
          contents.add(HistoryItemContentData(
            type: 'image/jpeg',
            value: imageData,
          ));
          titleText ??= '[图片]';
        }
      } catch (e) {
        debugPrint(e.toString());
      }
    }

    // 5.5 文件格式
    if (reader.canProvide(Formats.fileUri)) {
      try {
        final fileUri = await reader.readValue(Formats.fileUri);
        if (fileUri != null) {
          final filePath = fileUri.toFilePath();
          contents.add(HistoryItemContentData(
            type: 'file',
            value: Uint8List.fromList(utf8.encode(filePath)),
          ));
          titleText = filePath;
        }
      } catch (e) {
        debugPrint(e.toString());
      }
    }

    // 6. 如果没有任何内容，返回
    if (contents.isEmpty) {
      return;
    }

    // 7. 生成标题
    final title = _generateTitle(titleText ?? '', contents);

    // 8. 保存到数据库（使用 Maccy 的去重逻辑）
    final repository = ref.read(historyRepositoryProvider);
    await repository.addOrUpdateEntry(
      contents: contents,
      application: appName,
      title: title,
    );
  }

  /// 读取图片数据。
  Future<Uint8List?> _readImageData(ClipboardReader reader, SimpleFileFormat format) async {
    final completer = Completer<Uint8List?>();

    reader.getFile(format, (file) async {
      try {
        final stream = file.getStream();
        final chunks = <int>[];
        await for (final chunk in stream) {
          chunks.addAll(chunk);
        }
        completer.complete(Uint8List.fromList(chunks));
      } catch (e) {
        completer.completeError(e);
      }
    });

    return completer.future;
  }

  /// 生成标题（实现 Maccy 的 generateTitle 逻辑）。
  String _generateTitle(String primaryText, List<HistoryItemContentData> contents) {
    // 优先级：文件 > 文本 > 图片
    final fileContent = contents.firstWhere(
      (c) => c.type == 'file',
      orElse: () => HistoryItemContentData(type: '', value: null),
    );

    if (fileContent.value != null) {
      final path = utf8.decode(fileContent.value!);
      return p.basename(path);
    }

    if (primaryText.isNotEmpty) {
      // 限制长度为 1000 字符（Maccy 的限制）
      var title = primaryText.length > 1000 ? primaryText.substring(0, 1000) : primaryText;

      // 特殊符号显示
      final showSpecialSymbols = ref.read(showSpecialCharsProvider);
      if (showSpecialSymbols) {
        // 替换前导空格
        title = title.replaceAllMapped(RegExp('^ +'), (m) => '·' * m.group(0)!.length);
        // 替换尾随空格
        title = title.replaceAllMapped(RegExp(r' +$'), (m) => '·' * m.group(0)!.length);
        // 替换换行和制表符
        title = title.replaceAll('\n', '⏎').replaceAll('\t', '⇥');
      } else {
        title = title.trim();
      }

      return title;
    }

    // 图片类型
    final hasImage = contents.any((c) => c.type.startsWith('image/'));
    if (hasImage) {
      return '[图片]';
    }

    return '[未知内容]';
  }

  /// Windows 平台模拟粘贴操作
  Future<void> _simulateWindowsPaste() async {
    try {
      const platform = MethodChannel('com.hali.clip/native_utils');
      await platform.invokeMethod('restoreAndPaste');
    } catch (e) {
      debugPrint(e.toString());
    }
  }
}
