import 'dart:async';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import '../../../core/managers/window_manager_provider.dart';
import '../../settings/providers/settings_provider.dart';
import '../providers/history_providers.dart';

/// 历史记录页面，显示剪贴板历史列表、搜索框及底部操作菜单
class HistoryPage extends HookConsumerWidget {
  /// 构造函数
  const HistoryPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    // 性能优化：仅监听列表长度变化，条目内容变化不会导致整页 Rebuild
    final totalItems = ref.watch(filteredHistoryProvider.select((v) => v.value?.length ?? 0));
    final showFooterMenu = ref.watch(showFooterMenuProvider);

    final isDark = Theme.of(context).brightness == Brightness.dark;

    // 缓存不变的样式
    final bgColor = useMemoized(() => isDark ? const Color(0xFF2C2C2C).withOpacity(0.98) : const Color(0xFFEBEBEB).withOpacity(0.98), [isDark]);
    final highlightColor = isDark ? const Color(0xFF0058D0) : const Color(0xFF0063E1);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: KeyboardListener(
        focusNode: useFocusNode(), // 这里的 FocusNode 不需要监听
        onKeyEvent: (event) => ref.read(historyControllerProvider.notifier).handleKeyEvent(event),
        child: Container(
          decoration: BoxDecoration(
            color: bgColor,
            borderRadius: BorderRadius.circular(8),
            border: Border.all(color: isDark ? Colors.black.withOpacity(0.5) : Colors.black12, width: 0.5),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              const _HistoryHeader(),
              Expanded(
                // 性能优化：RepaintBoundary 隔离列表滚动/动画的重绘范围
                child: RepaintBoundary(
                  child: Consumer(
                    builder: (context, ref, child) {
                      final history = ref.watch(filteredHistoryProvider).value ?? [];
                      return ListView.builder(
                        padding: EdgeInsets.zero,
                        itemCount: history.length,
                        itemBuilder: (BuildContext context, int index) {
                          final item = history[index];
                          return _HistoryRow(
                            index: index,
                            content: item.content,
                            shortcut: index < 9 ? '${index + 1}' : null,
                            isPinned: item.isPinned,
                            selectionColor: highlightColor,
                            onTap: () => ref.read(historyControllerProvider.notifier).selectItem(index),
                            onHover: () => ref.read(historySelectedIndexProvider.notifier).set(index),
                            onPin: () => ref.read(historyControllerProvider.notifier).togglePin(index),
                            onDelete: () => ref.read(historyControllerProvider.notifier).deleteItem(index),
                          );
                        },
                      );
                    },
                  ),
                ),
              ),
              if (showFooterMenu) _FooterMenu(totalItems: totalItems, highlightColor: highlightColor),
            ],
          ),
        ),
      ),
    );
  }
}

class _FooterMenu extends ConsumerWidget {
  final int totalItems;
  final Color highlightColor;

  const _FooterMenu({required this.totalItems, required this.highlightColor});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Divider(indent: 8, endIndent: 8, height: 4, color: isDark ? Colors.white10 : Colors.black12),
        _MenuRow(
          index: totalItems,
          label: 'Clear History',
          shortcut: '⌥⌘⌫',
          selectionColor: highlightColor,
          onTap: () => ref.read(historyControllerProvider.notifier).clearHistory(),
          onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems),
        ),
        _MenuRow(
          index: totalItems + 1,
          label: 'Settings...',
          shortcut: '⌘,',
          selectionColor: highlightColor,
          onTap: () {
            ref.read(appWindowManagerProvider.notifier).showSettings();
          },
          onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems + 1),
        ),
        _MenuRow(
          index: totalItems + 2,
          label: 'Quit',
          shortcut: '⌘Q',
          selectionColor: highlightColor,
          onTap: () => ref.read(historyControllerProvider.notifier).quitApp(),
          onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems + 2),
        ),
        const SizedBox(height: 4),
      ],
    );
  }
}

class _HistoryHeader extends HookConsumerWidget {
  const _HistoryHeader();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final searchQuery = ref.watch(historySearchQueryProvider);
    final searchController = useTextEditingController(text: searchQuery);
    final searchFocusNode = useFocusNode();

    // 性能优化：搜索防抖逻辑
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
      padding: const EdgeInsets.fromLTRB(4, 4, 4, 4),
      child: CupertinoSearchTextField(
        controller: searchController,
        focusNode: searchFocusNode,
        autofocus: true,
        placeholder: 'Search...',
        itemSize: 12,
        padding: const EdgeInsetsDirectional.fromSTEB(4, 0, 4, 0),
        borderRadius: BorderRadius.circular(6),
        backgroundColor: isDark ? Colors.white10 : Colors.black.withOpacity(0.06),
        style: TextStyle(fontSize: 12, color: isDark ? Colors.white : Colors.black, fontFamily: '.AppleSystemUIFont'),
        onChanged: (value) {
          // 性能优化：防抖更新 Provider，减少数据库压力
          debouncer.run(() {
            ref.read(historySearchQueryProvider.notifier).set(value);
            ref.read(historySelectedIndexProvider.notifier).set(0);
          });
        },
      ),
    );
  }
}

class _Debouncer {
  final int milliseconds;
  VoidCallback? action;
  Timer? _timer;

  _Debouncer({required this.milliseconds});

  void run(VoidCallback action) {
    _timer?.cancel();
    _timer = Timer(Duration(milliseconds: milliseconds), action);
  }
}

class _HistoryRow extends HookConsumerWidget {
  final int index;
  final String content;
  final String? shortcut;
  final bool isPinned;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;
  final VoidCallback onPin;
  final VoidCallback onDelete;

  const _HistoryRow({
    required this.index,
    required this.content,
    this.shortcut,
    required this.isPinned,
    required this.selectionColor,
    required this.onTap,
    required this.onHover,
    required this.onPin,
    required this.onDelete,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isSelected = ref.watch(historySelectedIndexProvider.select((val) => val == index));
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final highlightMode = ref.watch(highlightMatchProvider);
    final searchQuery = ref.watch(historySearchQueryProvider);

    // 性能优化：预处理显示内容，避免重复 trim 和 replaceAll
    final displayContent = useMemoized(() {
      String text = content;
      if (text.length > 1000) text = text.substring(0, 1000);
      return text.trim().replaceAll('\n', ' ');
    }, [content]);

    // 性能优化：生成高亮 TextSpans
    final spans = useMemoized(() {
      final baseStyle = TextStyle(
        fontSize: 13,
        fontFamily: '.AppleSystemUIFont',
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
        // 添加匹配前的文本
        if (idx > start) {
          result.add(TextSpan(text: displayContent.substring(start, idx), style: baseStyle));
        }

        // 添加匹配的文本
        final matchText = displayContent.substring(idx, idx + searchQuery.length);
        result.add(TextSpan(
          text: matchText,
          style: baseStyle.copyWith(
            fontWeight: highlightMode == 'bold' || isSelected ? FontWeight.bold : FontWeight.w600,
            color: highlightMode == 'color' && !isSelected ? matchColor : null,
          ),
        ));

        start = idx + searchQuery.length;
        idx = lowerContent.indexOf(lowerQuery, start);
      }

      // 添加剩余文本
      if (start < displayContent.length) {
        result.add(TextSpan(text: displayContent.substring(start), style: baseStyle));
      }
      return result;
    }, [displayContent, searchQuery, highlightMode, isSelected, isDark]);

    return MouseRegion(
      onHover: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 24,
          padding: const EdgeInsets.symmetric(horizontal: 14),
          color: isSelected ? selectionColor : Colors.transparent,
          child: Row(
            children: [
              if (isPinned) const Icon(Icons.push_pin, size: 10, color: Colors.blueAccent),
              Expanded(
                child: Padding(
                  padding: EdgeInsets.only(left: isPinned ? 4 : 0),
                  child: Text.rich(
                    TextSpan(children: spans),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ),
              if (isSelected) ...[
                _HoverIcon(icon: isPinned ? Icons.push_pin : Icons.push_pin_outlined, onTap: onPin, hoverColor: Colors.white),
                const SizedBox(width: 8),
                _HoverIcon(icon: Icons.delete_outline, onTap: onDelete, hoverColor: Colors.redAccent),
                const SizedBox(width: 8),
              ],
              if (shortcut != null) Text('⌘$shortcut', style: TextStyle(fontSize: 12, color: isSelected ? Colors.white70 : (isDark ? Colors.white24 : Colors.black26))),
            ],
          ),
        ),
      ),
    );
  }
}

class _HoverIcon extends HookWidget {
  /// 图标数据
  final IconData icon;
  /// 点击回调
  final VoidCallback onTap;
  /// 悬停时的颜色
  final Color hoverColor;
  /// 基础颜色
  final Color baseColor;

  /// 构造函数
  const _HoverIcon({required this.icon, required this.onTap, this.hoverColor = Colors.white, this.baseColor = Colors.white70});

  @override
  Widget build(BuildContext context) {
    final isHovered = useState(false);
    return MouseRegion(
      onEnter: (_) => isHovered.value = true,
      onExit: (_) => isHovered.value = false,
      cursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () {
          // Prevent tap from reaching the parent row if needed
          // but GestureDetector usually stops propagation if handled
          onTap();
        },
        child: Icon(icon, size: 14, color: isHovered.value ? hoverColor : baseColor),
      ),
    );
  }
}

class _MenuRow extends ConsumerWidget {
  final int index;
  final String label;
  final String? shortcut;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MenuRow({required this.index, required this.label, this.shortcut, required this.selectionColor, required this.onTap, required this.onHover});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isSelected = ref.watch(historySelectedIndexProvider.select((val) => val == index));
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MouseRegion(
      onHover: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 24,
          padding: const EdgeInsets.symmetric(horizontal: 14),
          color: isSelected ? selectionColor : Colors.transparent,
          child: Row(
            children: [
              Expanded(
                child: Text(label, style: TextStyle(fontSize: 13, color: isSelected ? Colors.white : (isDark ? Colors.white : Colors.black87))),
              ),
              if (shortcut != null) Text(shortcut!, style: TextStyle(fontSize: 12, color: isSelected ? Colors.white70 : (isDark ? Colors.white24 : Colors.black26))),
            ],
          ),
        ),
      ),
    );
  }
}
