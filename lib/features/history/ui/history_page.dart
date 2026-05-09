import 'dart:async';
import 'dart:io';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/managers/window_manager_provider.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/history/ui/widgets/preview_popover.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

/// 历史记录主页面。
///
/// 应用程序的核心交互界面，包含搜索框、剪贴板条目列表、以及底部的操作菜单。
/// 该页面针对性能进行了优化，使用 RepaintBoundary 隔离重绘范围，并利用 Riverpod 的 select 进行精细化 Rebuild 控制。
class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  /// 构建历史记录页面 UI。
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final totalItems = ref.watch(
      filteredHistoryProvider.select((v) => v.value?.length ?? 0),
    );
    final isDark = Theme.of(context).brightness == Brightness.dark;

    // Match Maccy's exact background colors with transparency
    final bgColor = useMemoized(
      () => isDark
          ? const Color(0xFF1E1E1E).withOpacity(0.85)
          : const Color(0xFFF5F5F5).withOpacity(0.85),
      [isDark],
    );
    // Match Maccy's accent color with 0.8 opacity
    final highlightColor = isDark
        ? const Color(0xFF0A84FF).withOpacity(0.8)
        : const Color(0xFF007AFF).withOpacity(0.8);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: KeyboardListener(
        focusNode: useFocusNode(),
        onKeyEvent: (event) =>
            ref.read(historyControllerProvider.notifier).handleKeyEvent(event),
        child: DecoratedBox(
          decoration: BoxDecoration(
            color: bgColor,
            borderRadius: BorderRadius.circular(6), // Match Maccy's 6px radius
            border: Border.all(
              color: isDark ? Colors.black.withOpacity(0.5) : Colors.black12,
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
                      final history =
                          ref.watch(filteredHistoryProvider).value ?? [];
                      return ListView.builder(
                        padding: EdgeInsets.zero,
                        itemCount: history.length,
                        itemBuilder: (BuildContext context, int index) {
                          final item = history[index];
                          return _HistoryRow(
                            index: index,
                            item: item,
                            type: item.type,
                            content: item.content,
                            shortcut: index < 9 ? '${index + 1}' : null,
                            isPinned: item.isPinned,
                            selectionColor: highlightColor,
                            onTap: () => ref
                                .read(historyControllerProvider.notifier)
                                .selectItem(index),
                            onHover: () => ref
                                .read(historySelectedIndexProvider.notifier)
                                .set(index),
                            onPin: () => ref
                                .read(historyControllerProvider.notifier)
                                .togglePin(index),
                            onDelete: () => ref
                                .read(historyControllerProvider.notifier)
                                .deleteItem(index),
                          );
                        },
                      );
                    },
                  ),
                ),
              ),
              _FooterMenu(
                totalItems: totalItems,
                highlightColor: highlightColor,
              ),
            ],
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
          indent: 10, // Match Maccy's horizontal padding
          endIndent: 10,
          height: 6, // Match Maccy's separator padding
          color: isDark ? Colors.white10 : Colors.black12,
        ),
        _MenuRow(
          index: totalItems,
          label: 'Clear History',
          shortcut: '⌥⌘⌫',
          selectionColor: highlightColor,
          onTap: () =>
              ref.read(historyControllerProvider.notifier).clearHistory(),
          onHover: () =>
              ref.read(historySelectedIndexProvider.notifier).set(totalItems),
        ),
        _MenuRow(
          index: totalItems + 1,
          label: 'Settings...',
          shortcut: '⌘,',
          selectionColor: highlightColor,
          onTap: () {
            ref.read(appWindowManagerProvider.notifier).showSettings();
          },
          onHover: () => ref
              .read(historySelectedIndexProvider.notifier)
              .set(totalItems + 1),
        ),
        _MenuRow(
          index: totalItems + 2,
          label: 'Quit',
          shortcut: '⌘Q',
          selectionColor: highlightColor,
          onTap: () => ref.read(historyControllerProvider.notifier).quitApp(),
          onHover: () => ref
              .read(historySelectedIndexProvider.notifier)
              .set(totalItems + 2),
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

    ref.listen(historyFocusRequestProvider, (_, __) {
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

    return Container(
      padding: const EdgeInsets.fromLTRB(5, 5, 5, 5), // Match Maccy's 5px padding
      child: CupertinoSearchTextField(
        controller: searchController,
        focusNode: searchFocusNode,
        autofocus: true,
        placeholder: 'Search...',
        itemSize: 12,
        padding: const EdgeInsetsDirectional.fromSTEB(5, 4, 5, 4), // Adjust internal padding
        borderRadius: BorderRadius.circular(4), // Match Maccy's corner radius
        backgroundColor: isDark
            ? Colors.white10
            : Colors.black.withOpacity(0.06),
        style: TextStyle(
          fontSize: 12,
          color: isDark ? Colors.white : Colors.black,
          fontFamily: '.AppleSystemUIFont',
        ),
        onChanged: (value) {
          debouncer.run(() {
            ref.read(historySearchQueryProvider.notifier).set(value);
            ref.read(historySelectedIndexProvider.notifier).set(0);
          });
        },
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
/// [index] 条目在列表中的索引。
/// [item] 完整的剪贴板条目数据。
/// [type] 条目类型。
/// [content] 原始剪贴板文本。
/// [shortcut] 可用的快捷键文本（如 ⌘1）。
/// [isPinned] 是否置顶。
/// [selectionColor] 选中状态背景色。
/// [onTap] 点击（选择）回调。
/// [onHover] 悬停（导航聚焦）回调。
/// [onPin] 切换置顶回调。
/// [onDelete] 删除回调。
class _HistoryRow extends HookConsumerWidget {
  const _HistoryRow({
    required this.index,
    required this.item,
    required this.type,
    required this.content,
    this.shortcut,
    required this.isPinned,
    required this.selectionColor,
    required this.onTap,
    required this.onHover,
    required this.onPin,
    required this.onDelete,
  });

  final int index;
  final ClipboardEntry item;
  final String type;
  final String content;
  final String? shortcut;
  final bool isPinned;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;
  final VoidCallback onPin;
  final VoidCallback onDelete;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isSelected = ref.watch(
      historySelectedIndexProvider.select((val) => val == index),
    );
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final previewDelay = ref.watch(previewDelayProvider);

    final showPreview = useState(false);
    final hoverTimer = useRef<Timer?>(null);

    useEffect(() {
      return () {
        hoverTimer.value?.cancel();
      };
    }, []);

    return MouseRegion(
      onEnter: (_) {
        onHover();
        // Start timer for preview
        hoverTimer.value?.cancel();
        hoverTimer.value = Timer(Duration(milliseconds: previewDelay), () {
          showPreview.value = true;
        });
      },
      onExit: (_) {
        hoverTimer.value?.cancel();
        showPreview.value = false;
      },
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          GestureDetector(
            onTap: onTap,
            child: Container(
              height: 24, // Match Maccy's itemHeight
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 0),
              color: isSelected ? selectionColor : Colors.transparent,
              child: Row(
                children: [
                  if (isPinned)
                    const Icon(Icons.push_pin, size: 10, color: Colors.blueAccent),
                  Expanded(
                    child: Padding(
                      padding: EdgeInsets.only(left: isPinned ? 4 : 0),
                      child: buildContent(ref, isSelected, isDark),
                    ),
                  ),
                  if (isSelected) ...[
                    _HoverIcon(
                      icon: isPinned ? Icons.push_pin : Icons.push_pin_outlined,
                      onTap: onPin,
                      hoverColor: Colors.white,
                    ),
                    const SizedBox(width: 8),
                    _HoverIcon(
                      icon: Icons.delete_outline,
                      onTap: onDelete,
                      hoverColor: Colors.redAccent,
                    ),
                    const SizedBox(width: 8),
                  ],
                  if (shortcut != null)
                    Text(
                      '⌘$shortcut',
                      style: TextStyle(
                        fontSize: 12,
                        color: isSelected
                            ? Colors.white70
                            : (isDark ? Colors.white24 : Colors.black26),
                      ),
                    ),
                ],
              ),
            ),
          ),
          // Preview popover overlay
          if (showPreview.value)
            Positioned(
              left: 460, // Position to the right of the window
              top: -12,
              child: PreviewPopover(item: item),
            ),
        ],
      ),
    );
  }

  Widget buildContent(WidgetRef ref, bool isSelected, bool isDark) {
    if (type == 'image') {
      return Align(
        alignment: Alignment.centerLeft,
        child: Image.file(
          File(content),
          fit: BoxFit.contain,
          height: ref.read(imageHeightProvider).toDouble(),
          errorBuilder: (_, __, ___) => const Icon(Icons.broken_image, size: 32),
        ),
      );
    } else if (type == 'file') {
      return Row(
        children: [
          Icon(
            Icons.description_outlined,
            size: 14,
            color: isSelected
                ? Colors.white70
                : (isDark ? Colors.white54 : Colors.black54),
          ),
          const SizedBox(width: 6),
          Expanded(
            child: _TextContent(
              content: content,
              isSelected: isSelected,
              isDark: isDark,
            ),
          ),
        ],
      );
    } else {
      return _TextContent(
        content: content,
        isSelected: isSelected,
        isDark: isDark,
      );
    }
  }
}

class _TextContent extends HookConsumerWidget {
  const _TextContent({
    required this.content,
    required this.isSelected,
    required this.isDark,
  });

  final String content;
  final bool isSelected;
  final bool isDark;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final highlightMode = ref.watch(highlightMatchProvider);
    final searchQuery = ref.watch(historySearchQueryProvider);

    final displayContent = useMemoized(() {
      String text = content;
      if (text.length > 1000) text = text.substring(0, 1000);
      return text.trim().replaceAll('\n', ' ');
    }, [content]);

    final spans = useMemoized(() {
      final baseStyle = TextStyle(
        fontSize: 13,
        fontFamily: '.AppleSystemUIFont',
        color: isSelected
            ? Colors.white
            : (isDark ? Colors.white : Colors.black87),
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
          result.add(
            TextSpan(
              text: displayContent.substring(start, idx),
              style: baseStyle,
            ),
          );
        }

        final matchText = displayContent.substring(
          idx,
          idx + searchQuery.length,
        );
        result.add(
          TextSpan(
            text: matchText,
            style: baseStyle.copyWith(
              fontWeight: highlightMode == 'bold' || isSelected
                  ? FontWeight.bold
                  : FontWeight.w600,
              color: highlightMode == 'color' && !isSelected
                  ? matchColor
                  : null,
            ),
          ),
        );

        start = idx + searchQuery.length;
        idx = lowerContent.indexOf(lowerQuery, start);
      }

      if (start < displayContent.length) {
        result.add(
          TextSpan(text: displayContent.substring(start), style: baseStyle),
        );
      }
      return result;
    }, [displayContent, searchQuery, highlightMode, isSelected, isDark]);

    return Text.rich(
      TextSpan(children: spans),
      maxLines: 1,
      overflow: TextOverflow.ellipsis,
    );
  }
}

/// 带有悬停变色效果的图标组件。
///
/// 字段说明:
/// [icon] 图标数据。
/// [onTap] 点击回调。
/// [hoverColor] 鼠标悬停时的颜色。
/// [baseColor] 正常状态下的颜色。
class _HoverIcon extends HookWidget {
  const _HoverIcon({
    required this.icon,
    required this.onTap,
    this.hoverColor = Colors.white,
  });

  final IconData icon;
  final VoidCallback onTap;
  final Color hoverColor;
  final Color baseColor = Colors.white70;

  @override
  Widget build(BuildContext context) {
    final isHovered = useState(false);
    return MouseRegion(
      onEnter: (_) => isHovered.value = true,
      onExit: (_) => isHovered.value = false,
      cursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: onTap,
        child: Icon(
          icon,
          size: 14,
          color: isHovered.value ? hoverColor : baseColor,
        ),
      ),
    );
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
    final isSelected = ref.watch(
      historySelectedIndexProvider.select((val) => val == index),
    );
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MouseRegion(
      onHover: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 24, // Match Maccy's itemHeight
          padding: const EdgeInsets.symmetric(horizontal: 10), // Match Maccy's padding
          color: isSelected ? selectionColor : Colors.transparent,
          child: Row(
            children: [
              Expanded(
                child: Text(
                  label,
                  style: TextStyle(
                    fontSize: 13,
                    color: isSelected
                        ? Colors.white
                        : (isDark ? Colors.white : Colors.black87),
                  ),
                ),
              ),
              if (shortcut != null)
                Text(
                  shortcut!,
                  style: TextStyle(
                    fontSize: 12,
                    color: isSelected
                        ? Colors.white70
                        : (isDark ? Colors.white24 : Colors.black26),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}
