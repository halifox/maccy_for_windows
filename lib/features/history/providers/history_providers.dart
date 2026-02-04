import 'dart:io';

import 'package:flutter/services.dart';
import 'package:haliclip/core/database/database.dart';
import 'package:haliclip/core/managers/clipboard_manager_provider.dart';
import 'package:haliclip/core/managers/window_manager_provider.dart';
import 'package:haliclip/features/history/repositories/history_repository.dart';
import 'package:haliclip/features/settings/providers/settings_provider.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:super_clipboard/super_clipboard.dart';

part 'history_providers.g.dart';

/// 历史记录搜索查询 Notifier。
///
/// 管理搜索框中的实时输入内容。
@riverpod
class HistorySearchQuery extends _$HistorySearchQuery {
  @override
  String build() => '';

  /// 更新搜索内容。
  void set(String query) => state = query;
}

/// 历史记录选中项索引 Notifier。
///
/// 管理列表中的焦点位置（键盘上下键导航的当前项）。
@riverpod
class HistorySelectedIndex extends _$HistorySelectedIndex {
  @override
  int build() => 0;

  /// 设置选中的索引。
  void set(int index) => state = index;

  /// 通过函数式更新器改变索引状态。
  void update(int Function(int) updater) => state = updater(state);
}

/// 历史记录焦点请求 Notifier。
///
/// 用于在窗口弹出时强制触发 UI 层的文本框 Focus 操作。
@riverpod
class HistoryFocusRequest extends _$HistoryFocusRequest {
  @override
  int build() => 0;

  /// 触发一次焦点获取请求。
  void request() => state++;
}

/// 经过过滤和排序的历史记录流。
///
/// 核心数据 Provider，监听搜索词、搜索模式及存储限制，实时从仓库获取数据库数据。
@riverpod
Stream<List<ClipboardEntry>> filteredHistory(Ref ref) {
  final query = ref.watch(historySearchQueryProvider);
  final searchMode = ref.watch(searchModeProvider);
  final limit = ref.watch(historyLimitProvider);
  final repository = ref.watch(historyRepositoryProvider);

  return repository.watchEntries(
    query: query,
    searchMode: searchMode,
    limit: limit,
  );
}

/// 历史记录交互控制器。
///
/// 整合了所有的业务操作逻辑，如条目点击（选择并自动粘贴）、删除、置顶、
/// 全局键盘事件处理（导航、确认、快捷键操作）以及应用的清理逻辑。
///
/// 字段说明:
/// [digitMap] 数字键位到列表索引的映射表，用于 Alt+数字 快速选择。
@riverpod
class HistoryController extends _$HistoryController {
  @override
  void build() {}

  /// 执行条目选择操作。
  ///
  /// 将选中的 [index] 对应的内容存入系统剪贴板，并触发模拟粘贴指令。
  Future<void> selectItem(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final item = history[index];
    final clipboardManager = ref.read(appClipboardManagerProvider.notifier);

    final clipboard = SystemClipboard.instance;
    if (clipboard == null) return;

    final itemWriter = DataWriterItem();
    if (item.type == 'file') {
      itemWriter.add(Formats.fileUri(Uri.file(item.content)));
    }
    if (item.type == 'image') {
      itemWriter.add(Formats.fileUri(Uri.file(item.content)));
    }
    itemWriter.add(Formats.plainText(item.content));
    clipboardManager.setSelfUpdate(true);
    await clipboard.write([itemWriter]);
    await clipboardManager.simulatePaste();
    clipboardManager.setSelfUpdate(false);
  }

  /// 删除指定的历史记录条目。
  Future<void> deleteItem(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final repository = ref.read(historyRepositoryProvider);
    await repository.deleteEntry(history[index].id);
  }

  /// 切换条目的置顶/取消置顶状态。
  Future<void> togglePin(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final repository = ref.read(historyRepositoryProvider);
    await repository.togglePin(history[index].id);
  }

  /// 手动添加历史条目（调试或扩展用）。
  Future<void> addItem(String content, {bool isPinned = false}) async {
    final repository = ref.read(historyRepositoryProvider);
    await repository.addEntry(content, isPinned: isPinned);
  }

  /// 清空所有历史记录。
  ///
  /// 若设置中开启了“同时清空系统剪贴板”，则会调用相应系统的命令行工具执行清空。
  Future<void> clearHistory() async {
    final repository = ref.read(historyRepositoryProvider);
    final clearSystem = ref.read(clearSystemClipboardProvider);
    await repository.deleteAllEntries();

    if (clearSystem) {
      if (Platform.isWindows) {
        const script =
            '[Windows.ApplicationModel.DataTransfer.Clipboard, Windows.ApplicationModel.DataTransfer, ContentType=WindowsRuntime]::ClearHistory()';
        await Process.run('powershell', ['-Command', script]);
      } else if (Platform.isMacOS) {
        await Process.run('sh', ['-c', 'pbcopy < /dev/null']);
      } else if (Platform.isLinux) {
        await Process.run('sh', [
          '-c',
          'xclip -selection clipboard /dev/null || xsel --clipboard --clear',
        ]);
      }
    }
  }

  /// 退出应用程序。
  ///
  /// 若配置了退出时清空，则执行历史清理后再结束进程。
  Future<void> quitApp() async {
    final clearOnExit = ref.read(clearOnExitProvider);
    if (clearOnExit) {
      await clearHistory();
    }
    exit(0);
  }

  final digitMap = {
    LogicalKeyboardKey.digit1: 0,
    LogicalKeyboardKey.digit2: 1,
    LogicalKeyboardKey.digit3: 2,
    LogicalKeyboardKey.digit4: 3,
    LogicalKeyboardKey.digit5: 4,
    LogicalKeyboardKey.digit6: 5,
    LogicalKeyboardKey.digit7: 6,
    LogicalKeyboardKey.digit8: 7,
    LogicalKeyboardKey.digit9: 8,
    LogicalKeyboardKey.digit0: 9,
  };

  /// 全局键盘事件分发处理。
  ///
  /// 处理上下键导航、回车确认、ESC隐藏以及 Alt+数字 快速选择逻辑。
  ///
  /// [event] 捕获到的原始键盘事件。
  void handleKeyEvent(KeyEvent event) {
    if (event is! KeyDownEvent && event is! KeyRepeatEvent) return;

    final logicalKey = event.logicalKey;
    final isAltPressed = HardwareKeyboard.instance.isAltPressed;
    final isArrowKey =
        logicalKey == LogicalKeyboardKey.arrowDown ||
        logicalKey == LogicalKeyboardKey.arrowUp;

    if (event is KeyRepeatEvent && !isArrowKey) return;

    final history = ref.read(filteredHistoryProvider).value ?? [];
    final selectedIndex = ref.read(historySelectedIndexProvider);
    final totalItems = history.length;
    final showFooter = ref.read(showFooterMenuProvider);

    final int menuCount = showFooter ? 3 : 0;
    final int maxIdx = totalItems + menuCount - 1;
    if (maxIdx < 0) return;

    switch (logicalKey) {
      case LogicalKeyboardKey.arrowDown:
        ref
            .read(historySelectedIndexProvider.notifier)
            .update((val) => (val + 1).clamp(0, maxIdx));
      case LogicalKeyboardKey.arrowUp:
        ref
            .read(historySelectedIndexProvider.notifier)
            .update((val) => (val - 1).clamp(0, maxIdx));
      case LogicalKeyboardKey.escape:
        ref.read(appWindowManagerProvider.notifier).hideHistory();
      case LogicalKeyboardKey.enter:
        if (selectedIndex < totalItems) {
          selectItem(selectedIndex);
        } else {
          final menuIdx = selectedIndex - totalItems;
          switch (menuIdx) {
            case 0:
              clearHistory();
            case 1:
              ref.read(appWindowManagerProvider.notifier).showSettings();
            case 2:
              quitApp();
          }
        }
      case _ when isAltPressed:
        if (digitMap.containsKey(logicalKey)) {
          final index = digitMap[logicalKey]!;
          if (index < totalItems) {
            selectItem(index);
          }
        }
    }
  }
}
