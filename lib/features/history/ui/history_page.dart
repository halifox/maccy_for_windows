import 'dart:async';
import 'dart:io';
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/constants/ui_constants.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/managers/window_manager_provider.dart';
import 'package:maccy/core/utils/text_formatter.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/history/ui/history_actions.dart';
import 'package:maccy/features/history/ui/history_intents.dart';
import 'package:maccy/features/history/ui/widgets/keyboard_shortcut_widget.dart';
import 'package:maccy/features/history/ui/widgets/preview_popover.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';

/// 历史记录主页面。
///
/// 应用程序的核心交互界面，包含搜索框、剪贴板条目列表、以及底部的操作菜单。
/// 该页面针对性能进行了优化，使用 RepaintBoundary 隔离重绘范围，并利用 Riverpod 的 select 进行精细化 Rebuild 控制。
class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  /// 构建历史记录页面 UI。
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final totalItems = ref.watch(filteredHistoryProvider.select((v) => v.value?.length ?? 0));
    final isDark = Theme.of(context).brightness == Brightness.dark;

    // 确保有选中项，如果没有则选中第一项
    ref.listen(filteredHistoryProvider, (previous, next) {
      final history = next.value ?? [];
      final selectedId = ref.read(historySelectedIdProvider);
      if (history.isNotEmpty && (selectedId == null || !history.any((item) => item.id == selectedId))) {
        ref.read(historySelectedIdProvider.notifier).value = history.first.id;
      }
    });

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Shortcuts(
        shortcuts: const <ShortcutActivator, Intent>{
          SingleActivator(LogicalKeyboardKey.arrowDown): NavigateDownIntent(),
          SingleActivator(LogicalKeyboardKey.arrowUp): NavigateUpIntent(),
          SingleActivator(LogicalKeyboardKey.enter): SelectItemIntent(),
          SingleActivator(LogicalKeyboardKey.escape): CloseWindowIntent(),
          SingleActivator(LogicalKeyboardKey.keyP, control: true): TogglePinIntent(),
          SingleActivator(LogicalKeyboardKey.comma, control: true): OpenSettingsIntent(),
          SingleActivator(LogicalKeyboardKey.keyQ, control: true): QuitAppIntent(),
          //quick
          SingleActivator(LogicalKeyboardKey.digit1, alt: true): QuickSelectIntent(0),
          SingleActivator(LogicalKeyboardKey.digit2, alt: true): QuickSelectIntent(1),
          SingleActivator(LogicalKeyboardKey.digit3, alt: true): QuickSelectIntent(2),
          SingleActivator(LogicalKeyboardKey.digit4, alt: true): QuickSelectIntent(3),
          SingleActivator(LogicalKeyboardKey.digit5, alt: true): QuickSelectIntent(4),
          SingleActivator(LogicalKeyboardKey.digit6, alt: true): QuickSelectIntent(5),
          SingleActivator(LogicalKeyboardKey.digit7, alt: true): QuickSelectIntent(6),
          SingleActivator(LogicalKeyboardKey.digit8, alt: true): QuickSelectIntent(7),
          SingleActivator(LogicalKeyboardKey.digit9, alt: true): QuickSelectIntent(8),
          SingleActivator(LogicalKeyboardKey.digit0, alt: true): QuickSelectIntent(9),
          //quick pin
          SingleActivator(LogicalKeyboardKey.keyA, alt: true): QuickPinSelectIntent('A'),
          SingleActivator(LogicalKeyboardKey.keyB, alt: true): QuickPinSelectIntent('B'),
          SingleActivator(LogicalKeyboardKey.keyC, alt: true): QuickPinSelectIntent('C'),
          SingleActivator(LogicalKeyboardKey.keyD, alt: true): QuickPinSelectIntent('D'),
          SingleActivator(LogicalKeyboardKey.keyE, alt: true): QuickPinSelectIntent('E'),
          SingleActivator(LogicalKeyboardKey.keyF, alt: true): QuickPinSelectIntent('F'),
          SingleActivator(LogicalKeyboardKey.keyG, alt: true): QuickPinSelectIntent('G'),
          SingleActivator(LogicalKeyboardKey.keyH, alt: true): QuickPinSelectIntent('H'),
          SingleActivator(LogicalKeyboardKey.keyI, alt: true): QuickPinSelectIntent('I'),
          SingleActivator(LogicalKeyboardKey.keyJ, alt: true): QuickPinSelectIntent('J'),
          SingleActivator(LogicalKeyboardKey.keyK, alt: true): QuickPinSelectIntent('K'),
          SingleActivator(LogicalKeyboardKey.keyL, alt: true): QuickPinSelectIntent('L'),
          SingleActivator(LogicalKeyboardKey.keyM, alt: true): QuickPinSelectIntent('M'),
          SingleActivator(LogicalKeyboardKey.keyN, alt: true): QuickPinSelectIntent('N'),
          SingleActivator(LogicalKeyboardKey.keyO, alt: true): QuickPinSelectIntent('O'),
          SingleActivator(LogicalKeyboardKey.keyP, alt: true): QuickPinSelectIntent('P'),
          SingleActivator(LogicalKeyboardKey.keyQ, alt: true): QuickPinSelectIntent('Q'),
          SingleActivator(LogicalKeyboardKey.keyR, alt: true): QuickPinSelectIntent('R'),
          SingleActivator(LogicalKeyboardKey.keyS, alt: true): QuickPinSelectIntent('S'),
          SingleActivator(LogicalKeyboardKey.keyT, alt: true): QuickPinSelectIntent('T'),
          SingleActivator(LogicalKeyboardKey.keyU, alt: true): QuickPinSelectIntent('U'),
          SingleActivator(LogicalKeyboardKey.keyV, alt: true): QuickPinSelectIntent('V'),
          SingleActivator(LogicalKeyboardKey.keyW, alt: true): QuickPinSelectIntent('W'),
          SingleActivator(LogicalKeyboardKey.keyX, alt: true): QuickPinSelectIntent('X'),
          SingleActivator(LogicalKeyboardKey.keyY, alt: true): QuickPinSelectIntent('Y'),
          SingleActivator(LogicalKeyboardKey.keyZ, alt: true): QuickPinSelectIntent('Z'),
        },
        child: Actions(
          actions: <Type, Action<Intent>>{
            NavigateDownIntent: NavigateDownAction(ref),
            NavigateUpIntent: NavigateUpAction(ref),
            SelectItemIntent: SelectItemAction(ref),
            CloseWindowIntent: CloseWindowAction(ref),
            TogglePinIntent: TogglePinAction(ref),
            OpenSettingsIntent: OpenSettingsAction(ref),
            QuitAppIntent: QuitAppAction(ref),
            QuickSelectIntent: QuickSelectAction(ref),
            QuickPinSelectIntent: QuickPinSelectAction(ref),
          },
          child: Focus(
            autofocus: true,
            child: ClipRRect(
              borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 20, sigmaY: 20),
                child: DecoratedBox(
                  decoration: BoxDecoration(
                    color: isDark
                        ? const Color(0xFF1E1E1E).withValues(alpha: 0.85)
                        : const Color(0xFFF5F5F5).withValues(alpha: 0.85),
                    borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
                    border: Border.all(
                      color: isDark ? Colors.black.withValues(alpha: 0.5) : Colors.black12,
                      width: 0.5,
                    ),
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      const _HistoryHeader(),
                      Expanded(
                        child: RepaintBoundary(
                          child: Consumer(
                            builder: (context, ref, child) {
                              final pinnedItems = ref.watch(pinnedHistoryProvider).value ?? [];
                              final unpinnedItems = ref.watch(unpinnedHistoryProvider).value ?? [];

                              return CustomScrollView(
                                slivers: [
                                  SliverList(
                                    delegate: SliverChildBuilderDelegate(
                                      (context, listIndex) {
                                        final item = pinnedItems[listIndex];
                                        return _HistoryRow(
                                          item: item,
                                          shortcut: item.pin,
                                          selectionColor: isDark
                                              ? const Color(0xFF0A84FF).withValues(alpha: 0.8)
                                              : const Color(0xFF007AFF).withValues(alpha: 0.8),
                                          onTap: () => ref.read(historyControllerProvider.notifier).selectItem(item.id),
                                          onHover: () => ref.read(historySelectedIdProvider.notifier).value = item.id,
                                          onPin: () => ref.read(historyControllerProvider.notifier).togglePin(item.id),
                                          onDelete: () => ref.read(historyControllerProvider.notifier).deleteItem(item.id),
                                        );
                                      },
                                      childCount: pinnedItems.length,
                                    ),
                                  ),
                                  SliverList(
                                    delegate: SliverChildBuilderDelegate(
                                      (context, listIndex) {
                                        final item = unpinnedItems[listIndex];
                                        final shortcut = listIndex < 10 ? '${(listIndex + 1) % 10}' : null;
                                        return _HistoryRow(
                                          item: item,
                                          shortcut: shortcut,
                                          selectionColor: isDark
                                              ? const Color(0xFF0A84FF).withValues(alpha: 0.8)
                                              : const Color(0xFF007AFF).withValues(alpha: 0.8),
                                          onTap: () => ref.read(historyControllerProvider.notifier).selectItem(item.id),
                                          onHover: () => ref.read(historySelectedIdProvider.notifier).value = item.id,
                                          onPin: () => ref.read(historyControllerProvider.notifier).togglePin(item.id),
                                          onDelete: () => ref.read(historyControllerProvider.notifier).deleteItem(item.id),
                                        );
                                      },
                                      childCount: unpinnedItems.length,
                                    ),
                                  ),
                                ],
                              );
                            },
                          ),
                        ),
                      ),
                      _FooterMenu(
                        totalItems: totalItems,
                        highlightColor: isDark
                            ? const Color(0xFF0A84FF).withValues(alpha: 0.8)
                            : const Color(0xFF007AFF).withValues(alpha: 0.8),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

/// 页面底部菜单组件。
///
/// 展示清空历史、进入设置、退出应用等快捷操作。
///
/// 字段说明:
/// [totalItems] 当前历史列表的总条目数，用于计算菜单项的索引。
/// [highlightColor] 选中项的高亮背景色。
class _FooterMenu extends ConsumerWidget {
  const _FooterMenu({required this.totalItems, required this.highlightColor});

  final int totalItems;
  final Color highlightColor;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Divider(
          indent: MaccyUIConstants.dividerHorizontalPadding,
          endIndent: MaccyUIConstants.dividerHorizontalPadding,
          height: MaccyUIConstants.verticalSeparatorPadding,
          color: isDark ? Colors.white10 : Colors.black12,
        ),
        _MenuRow(
          index: totalItems,
          label: 'Clear History',
          shortcut: 'Alt+Win+Del',
          selectionColor: highlightColor,
          onTap: () => ref.read(historyControllerProvider.notifier).clearHistory(),
          onHover: () => ref.read(historySelectedIdProvider.notifier).value = null,
        ),
        _MenuRow(
          index: totalItems + 1,
          label: 'Settings...',
          shortcut: 'Ctrl+,',
          selectionColor: highlightColor,
          onTap: () {
            ref.read(appWindowManagerProvider.notifier).showSettings();
          },
          onHover: () => ref.read(historySelectedIdProvider.notifier).value = null,
        ),
        _MenuRow(
          index: totalItems + 2,
          label: 'Quit',
          shortcut: 'Ctrl+Q',
          selectionColor: highlightColor,
          onTap: () => ref.read(historyControllerProvider.notifier).quitApp(),
          onHover: () => ref.read(historySelectedIdProvider.notifier).value = null,
        ),
        const SizedBox(height: 4),
      ],
    );
  }
}

/// 历史记录顶部头部组件（搜索框）。
///
/// 提供实时搜索过滤功能，集成了防抖逻辑（Debouncer）以优化数据库查询压力。
class _HistoryHeader extends HookConsumerWidget {
  const _HistoryHeader();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final searchQuery = ref.watch(historySearchQueryProvider);
    final searchController = useTextEditingController(text: searchQuery);
    final searchFocusNode = useFocusNode();
    final debouncer = useMemoized(() => _Debouncer(milliseconds: 150));

    ref.listen(historyFocusRequestProvider, (_, _) {
      searchFocusNode.requestFocus();
    });

    useEffect(() {
      if (searchController.text != searchQuery) {
        searchController.text = searchQuery;
      }
      return null;
    }, [searchQuery]);

    useEffect(() {
      searchFocusNode.requestFocus();
      return null;
    }, []);

    useOnAppLifecycleStateChange((oldState, newState) {
      if (newState == AppLifecycleState.resumed) {
        searchFocusNode.requestFocus();
      }
    });

    return Padding(
      padding: const EdgeInsets.all(MaccyUIConstants.searchFieldPadding),
      child: Container(
        height: MaccyUIConstants.searchFieldHeight,
        decoration: BoxDecoration(
          color: isDark ? Colors.white.withValues(alpha: 0.1) : Colors.black.withValues(alpha: 0.06),
          borderRadius: BorderRadius.circular(MaccyUIConstants.searchFieldCornerRadius),
        ),
        child: Row(
          children: [
            // 搜索图标
            Padding(
              padding: const EdgeInsets.only(left: MaccyUIConstants.searchFieldInternalHorizontalPadding),
              child: Icon(
                Icons.search,
                size: MaccyUIConstants.searchFieldIconSize,
                color: (isDark ? Colors.white : Colors.black).withValues(
                  alpha: MaccyUIConstants.searchFieldIconOpacity,
                ),
              ),
            ),
            // 输入框
            Expanded(
              child: TextField(
                controller: searchController,
                focusNode: searchFocusNode,
                autofocus: true,
                decoration: const InputDecoration(
                  hintText: 'Search...',
                  border: InputBorder.none,
                  isDense: true,
                  contentPadding: EdgeInsets.symmetric(
                    horizontal: MaccyUIConstants.searchFieldInternalHorizontalPadding,
                  ),
                ),
                style: TextStyle(
                  fontSize: MaccyUIConstants.searchFieldFontSize,
                  color: isDark ? Colors.white : Colors.black,
                  fontFamily: Platform.isWindows
                      ? MaccyUIConstants.systemFontFamilyWindows
                      : MaccyUIConstants.systemFontFamily,
                ),
                onChanged: (value) {
                  debouncer.run(() {
                    ref.read(historySearchQueryProvider.notifier).value = value;
                    final history = ref.read(filteredHistoryProvider).value ?? [];
                    if (history.isNotEmpty) {
                      ref.read(historySelectedIdProvider.notifier).value = history.first.id;
                    }
                  });
                },
              ),
            ),
            // 清除按钮
            if (searchQuery.isNotEmpty)
              GestureDetector(
                onTap: () {
                  searchController.clear();
                  ref.read(historySearchQueryProvider.notifier).value = '';
                  final history = ref.read(filteredHistoryProvider).value ?? [];
                  if (history.isNotEmpty) {
                    ref.read(historySelectedIdProvider.notifier).value = history.first.id;
                  }
                },
                child: Padding(
                  padding: const EdgeInsets.only(right: MaccyUIConstants.searchFieldInternalHorizontalPadding),
                  child: Icon(
                    Icons.cancel,
                    size: MaccyUIConstants.searchFieldIconSize,
                    color: (isDark ? Colors.white : Colors.black).withValues(
                      alpha: MaccyUIConstants.searchFieldClearButtonOpacity,
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}

/// 函数防抖执行器。
///
/// 用于在频繁调用的事件中延迟执行目标动作。
///
/// 字段说明:
/// [milliseconds] 延迟执行的毫秒数。
/// [action] 待执行的动作回调。
/// [_timer] 内部计时器。
class _Debouncer {
  _Debouncer({required this.milliseconds});

  final int milliseconds;
  VoidCallback? action;
  Timer? _timer;

  /// 执行防抖包装后的动作。
  ///
  /// [action] 目标回调函数。
  void run(VoidCallback action) {
    _timer?.cancel();
    _timer = Timer(Duration(milliseconds: milliseconds), action);
  }
}

/// 历史记录条目行组件。
///
/// 展示单条剪贴板内容，支持关键词搜索高亮、置顶状态标识、删除/置顶交互按钮、预览弹窗。
///
/// 字段说明:
/// [item] 完整的剪贴板条目数据。
/// [shortcut] 可用的快捷键文本（如 Win+1）。
/// [selectionColor] 选中状态背景色。
/// [onTap] 点击（选择）回调。
/// [onHover] 悬停（导航聚焦）回调。
/// [onPin] 切换置顶回调。
/// [onDelete] 删除回调。
class _HistoryRow extends HookConsumerWidget {
  const _HistoryRow({
    required this.item,
    this.shortcut,
    required this.selectionColor,
    required this.onTap,
    required this.onHover,
    required this.onPin,
    required this.onDelete,
  });

  final HistoryItem item;
  final String? shortcut;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;
  final VoidCallback onPin;
  final VoidCallback onDelete;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isSelected = ref.watch(historySelectedIdProvider.select((val) => val == item.id));
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final previewDelay = ref.watch(previewDelayProvider);

    final overlayController = useMemoized(() => OverlayPortalController());
    final hoverTimer = useRef<Timer?>(null);

    useEffect(() {
      return () {
        hoverTimer.value?.cancel();
      };
    }, []);

    return MouseRegion(
      onEnter: (_) {
        onHover();
        hoverTimer.value?.cancel();
        hoverTimer.value = Timer(Duration(milliseconds: previewDelay), () {
          overlayController.show();
        });
      },
      onExit: (_) {
        hoverTimer.value?.cancel();
        hoverTimer.value = Timer(const Duration(milliseconds: 200), () {
          overlayController.hide();
        });
      },
      child: OverlayPortal(
        controller: overlayController,
        overlayChildBuilder: (context) {
          return Positioned(
            right: 10,
            top: 45,
            child: MouseRegion(
              onEnter: (_) => hoverTimer.value?.cancel(),
              onExit: (_) => overlayController.hide(),
              child: PreviewPopover(item: item),
            ),
          );
        },
        child: GestureDetector(
          onTap: onTap,
          child: Container(
            height: MaccyUIConstants.itemHeight,
            padding: const EdgeInsets.symmetric(horizontal: MaccyUIConstants.contentLeadingPadding, vertical: 0),
            decoration: BoxDecoration(
              color: isSelected ? selectionColor : Colors.transparent,
              borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
            ),
            child: Row(
              children: [
                Expanded(child: buildContent(ref, isSelected, isDark)),
                if (shortcut != null)
                  Padding(
                    padding: const EdgeInsets.only(right: MaccyUIConstants.shortcutTrailingPadding),
                    child: KeyboardShortcutWidget(shortcut: 'Alt+$shortcut', isSelected: isSelected, isDark: isDark),
                  )
                else
                  const SizedBox(width: MaccyUIConstants.shortcutPlaceholderWidth),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget buildContent(WidgetRef ref, bool isSelected, bool isDark) {
    // 如果是 pin 项且有别名，显示别名；否则显示 title
    final displayText = (item.pin != null && item.alias != null && item.alias!.isNotEmpty) ? item.alias! : item.title;
    return _TextContent(content: displayText, isSelected: isSelected, isDark: isDark);
  }
}

class _TextContent extends HookConsumerWidget {
  const _TextContent({required this.content, required this.isSelected, required this.isDark});

  final String content;
  final bool isSelected;
  final bool isDark;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final highlightMode = ref.watch(highlightMatchProvider);
    final searchQuery = ref.watch(historySearchQueryProvider);
    final showSpecialChars = ref.watch(showSpecialCharsProvider);

    final displayContent = useMemoized(() {
      // 使用 TextFormatter 格式化特殊字符
      String text = TextFormatter.formatForDisplay(content, showSpecialChars: showSpecialChars);

      // 限制长度
      if (text.length > 1000) text = text.substring(0, 1000);

      // 如果不显示特殊字符，将换行替换为空格以便单行显示
      if (!showSpecialChars) {
        text = text.replaceAll('\n', ' ');
      }

      return text;
    }, [content, showSpecialChars]);

    final spans = useMemoized(() {
      final baseStyle = TextStyle(
        fontSize: MaccyUIConstants.primaryFontSize,
        fontFamily: Platform.isWindows ? MaccyUIConstants.systemFontFamilyWindows : MaccyUIConstants.systemFontFamily,
        color: isSelected ? Colors.white : (isDark ? Colors.white : Colors.black87),
      );

      if (searchQuery.isEmpty) {
        return [TextSpan(text: displayContent, style: baseStyle)];
      }

      final List<TextSpan> result = [];
      final lowerContent = displayContent.toLowerCase();
      final lowerQuery = searchQuery.toLowerCase();
      int start = 0;
      int idx = lowerContent.indexOf(lowerQuery);

      if (idx == -1) {
        return [TextSpan(text: displayContent, style: baseStyle)];
      }

      final matchColor = isDark ? Colors.orangeAccent : Colors.deepOrange;

      while (idx != -1) {
        if (idx > start) {
          result.add(TextSpan(text: displayContent.substring(start, idx), style: baseStyle));
        }

        final matchText = displayContent.substring(idx, idx + searchQuery.length);
        result.add(
          TextSpan(
            text: matchText,
            style: baseStyle.copyWith(
              fontWeight: highlightMode == 'bold' || isSelected ? FontWeight.bold : FontWeight.w600,
              color: highlightMode == 'color' && !isSelected ? matchColor : null,
            ),
          ),
        );

        start = idx + searchQuery.length;
        idx = lowerContent.indexOf(lowerQuery, start);
      }

      if (start < displayContent.length) {
        result.add(TextSpan(text: displayContent.substring(start), style: baseStyle));
      }
      return result;
    }, [displayContent, searchQuery, highlightMode, isSelected, isDark]);

    return Text.rich(TextSpan(children: spans), maxLines: 1, overflow: TextOverflow.ellipsis);
  }
}

/// 菜单行组件（用于底部菜单）。
///
/// 字段说明:
/// [index] 菜单项索引。
/// [label] 标签文本。
/// [shortcut] 快捷键说明文字。
/// [selectionColor] 选中时的背景色。
/// [onTap] 点击回调。
/// [onHover] 悬停回调。
class _MenuRow extends ConsumerWidget {
  const _MenuRow({
    required this.index,
    required this.label,
    this.shortcut,
    required this.selectionColor,
    required this.onTap,
    required this.onHover,
  });

  final int index;
  final String label;
  final String? shortcut;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isSelected = ref.watch(historySelectedIdProvider.select((val) => val == null && val == index));
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MouseRegion(
      onHover: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: MaccyUIConstants.itemHeight,
          padding: const EdgeInsets.symmetric(horizontal: MaccyUIConstants.contentLeadingPadding),
          decoration: BoxDecoration(
            color: isSelected ? selectionColor : Colors.transparent,
            borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
          ),
          child: Row(
            children: [
              Expanded(
                child: Text(
                  label,
                  style: TextStyle(
                    fontSize: MaccyUIConstants.primaryFontSize,
                    fontFamily: Platform.isWindows
                        ? MaccyUIConstants.systemFontFamilyWindows
                        : MaccyUIConstants.systemFontFamily,
                    color: isSelected ? Colors.white : (isDark ? Colors.white : Colors.black87),
                  ),
                ),
              ),
              if (shortcut != null)
                Padding(
                  padding: const EdgeInsets.only(right: MaccyUIConstants.shortcutTrailingPadding),
                  child: KeyboardShortcutWidget(shortcut: shortcut!, isSelected: isSelected, isDark: isDark),
                )
              else
                const SizedBox(width: MaccyUIConstants.shortcutPlaceholderWidth),
            ],
          ),
        ),
      ),
    );
  }
}
