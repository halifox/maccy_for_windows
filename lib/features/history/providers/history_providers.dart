import 'dart:io';

import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../../../core/database/database.dart';
import '../../../core/managers/clipboard_manager_provider.dart';
import '../../../core/managers/window_manager_provider.dart';
import '../../settings/providers/settings_provider.dart';
import '../repositories/history_repository.dart';

part 'history_providers.g.dart';

/// 历史记录搜索查询 Provider
@riverpod
class HistorySearchQuery extends _$HistorySearchQuery {
  @override
  String build() => '';

  /// 设置搜索查询字符串
  void set(String query) => state = query;
}

/// 历史记录当前选中索引 Provider
@riverpod
class HistorySelectedIndex extends _$HistorySelectedIndex {
  @override
  int build() => 0;

  /// 设置选中索引
  void set(int index) => state = index;

  /// 通过更新器更新选中索引
  void update(int Function(int) updater) => state = updater(state);
}

/// 历史记录焦点请求 Provider，用于触发搜索框获取焦点
@riverpod
class HistoryFocusRequest extends _$HistoryFocusRequest {
  @override
  int build() => 0;

  /// 发起焦点获取请求
  void request() => state++;
}

/// 经过过滤和排序的历史记录流 Provider
@riverpod
Stream<List<ClipboardEntry>> filteredHistory(Ref ref) {
  final query = ref.watch(historySearchQueryProvider);
  final searchMode = ref.watch(searchModeProvider);
  final limit = ref.watch(historyLimitProvider);
  final repository = ref.watch(historyRepositoryProvider);

  return repository.watchEntries(query: query, searchMode: searchMode, limit: limit);
}

/// 历史记录控制器，负责处理用户交互操作（选择、删除、置顶、清空等）
@riverpod
class HistoryController extends _$HistoryController {
  @override
  void build() {}

  /// 选择指定索引的项目并尝试自动粘贴
  Future<void> selectItem(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final content = history[index].content;
    final clipboardManager = ref.read(appClipboardManagerProvider.notifier);

    await Clipboard.setData(ClipboardData(text: content));
    await clipboardManager.simulatePaste();
  }

  /// 删除指定索引的历史记录条目
  Future<void> deleteItem(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final repository = ref.read(historyRepositoryProvider);
    await repository.deleteEntry(history[index].id);
  }

  /// 切换指定索引条目的置顶状态
  Future<void> togglePin(int index) async {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (index >= history.length) return;

    final repository = ref.read(historyRepositoryProvider);
    await repository.togglePin(history[index].id);
  }

  /// 手动添加一条历史记录
  Future<void> addItem(String content, {bool isPinned = false}) async {
    final repository = ref.read(historyRepositoryProvider);
    await repository.addEntry(content, isPinned: isPinned);
  }

  /// 清空所有历史记录，并根据设置清空系统剪贴板
  Future<void> clearHistory() async {
    final repository = ref.read(historyRepositoryProvider);
    final clearSystem = ref.read(clearSystemClipboardProvider);
    await repository.deleteAllEntries();
    // 如果设置了清除历史时同时清除系统剪贴板
    if (clearSystem) {
      if (Platform.isWindows) {
        const script = '[Windows.ApplicationModel.DataTransfer.Clipboard, Windows.ApplicationModel.DataTransfer, ContentType=WindowsRuntime]::ClearHistory()';
        await Process.run('powershell', ['-Command', script]);
      } else if (Platform.isMacOS) {
        await Process.run('sh', ['-c', 'pbcopy < /dev/null']);
      } else if (Platform.isLinux) {
        // await Process.run('xclip', ['-selection', 'clipboard', '/dev/null']);
        await Process.run('sh', ['-c', 'xclip -selection clipboard /dev/null || xsel --clipboard --clear']);
      }
    }
  }

  /// 退出应用程序，并根据设置清空历史
  Future<void> quitApp() async {
    final clearOnExit = ref.read(clearOnExitProvider);
    if (clearOnExit) {
      await clearHistory();
    }
    exit(0);
  }

  /// 数字键位映射表，用于 Alt+数字 快速选择
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

  /// 处理全局键盘事件，支持上下键导航、回车确认、ESC隐藏等操作
  void handleKeyEvent(KeyEvent event) {
    if (event is! KeyDownEvent && event is! KeyRepeatEvent) return;

    final logicalKey = event.logicalKey;
    final isAltPressed = HardwareKeyboard.instance.isAltPressed;
    final isArrowKey = logicalKey == LogicalKeyboardKey.arrowDown || logicalKey == LogicalKeyboardKey.arrowUp;

    if (event is KeyRepeatEvent && !isArrowKey) return;

    final history = ref.read(filteredHistoryProvider).value ?? [];
    final selectedIndex = ref.read(historySelectedIndexProvider);
    final totalItems = history.length;
    final showFooter = ref.read(showFooterMenuProvider);

    int menuCount = showFooter ? 3 : 0; // Clear, Settings, Quit
    int maxIdx = totalItems + menuCount - 1;
    if (maxIdx < 0) return;

    switch (logicalKey) {
      case LogicalKeyboardKey.arrowDown:
        ref.read(historySelectedIndexProvider.notifier).update((val) => (val + 1).clamp(0, maxIdx));
      case LogicalKeyboardKey.arrowUp:
        ref.read(historySelectedIndexProvider.notifier).update((val) => (val - 1).clamp(0, maxIdx));
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
