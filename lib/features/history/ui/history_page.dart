import 'dart:io';
import 'package:clipboard/core/managers/clipboard_manager_provider.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:drift/drift.dart' show OrderingTerm;
import 'package:go_router/go_router.dart';
import '../../../core/database/database_provider.dart';
import '../../../core/managers/window_manager_provider.dart';

class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final db = ref.watch(appDatabaseProvider);
    final searchController = useTextEditingController();
    final selectedIndex = useState(0);
    final focusNode = useFocusNode();

    final historyStream = useMemoized(() {
      return (db.select(db.clipboardEntries)
            ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
            ..limit(50))
          .watch();
    }, [db]);

    final history = useStream(historyStream);
    final items = history.data ?? [];

    final filteredItems = useMemoized(() {
      if (searchController.text.isEmpty) return items;
      return items
          .where((item) => item.content.toLowerCase().contains(searchController.text.toLowerCase()))
          .toList();
    }, [items, searchController.text]);

    Future<void> selectItem(int index) async {
      if (index >= 0 && index < filteredItems.length) {
        final item = filteredItems[index];
        await Clipboard.setData(ClipboardData(text: item.content));
        // 先隐藏窗口
        await ref.read(appWindowManagerProvider.notifier).hide();
        // 再触发模拟粘贴
        await ref.read(appClipboardManagerProvider.notifier).simulatePaste();
      }
    }

    void handleClear() async {
      await db.delete(db.clipboardEntries).go();
      selectedIndex.value = 0;
    }

    void handleKeyEvent(KeyEvent event) {
      if (event is KeyDownEvent) {
        if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
          selectedIndex.value = (selectedIndex.value + 1).clamp(0, filteredItems.length + 3);
        } else if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
          selectedIndex.value = (selectedIndex.value - 1).clamp(0, filteredItems.length + 3);
        } else if (event.logicalKey == LogicalKeyboardKey.enter) {
          if (selectedIndex.value < filteredItems.length) {
            selectItem(selectedIndex.value);
          } else {
            final menuIdx = selectedIndex.value - filteredItems.length;
            if (menuIdx == 0) handleClear();
            if (menuIdx == 1) {
              context.push('/settings');
              ref.read(appWindowManagerProvider.notifier).showSettings();
            }
            if (menuIdx == 3) exit(0);
          }
        } else if (event.logicalKey == LogicalKeyboardKey.escape) {
          ref.read(appWindowManagerProvider.notifier).hide();
        } else {
          final label = event.logicalKey.keyLabel;
          if (RegExp(r'^[1-9]$').hasMatch(label)) {
            selectItem(int.parse(label) - 1);
          }
        }
      }
    }

    final isDark = Theme.of(context).brightness == Brightness.dark;
    final bgColor = isDark ? const Color(0xFF2C2C2C).withOpacity(0.98) : const Color(0xFFEBEBEB).withOpacity(0.98);
    final labelColor = isDark ? Colors.white24 : const Color(0xFF9A9A9A);
    final searchBg = isDark ? Colors.white10 : const Color(0xFFD1D1D1);
    final highlightColor = isDark ? const Color(0xFF0058D0) : const Color(0xFF0063E1);
    final lineDividerColor = isDark ? Colors.white10 : const Color(0xFFD0D0D0);
    final textColor = isDark ? Colors.white.withOpacity(0.9) : Colors.black.withOpacity(0.8);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: KeyboardListener(
        focusNode: FocusNode(),
        onKeyEvent: handleKeyEvent,
        child: Container(
          decoration: BoxDecoration(
            color: bgColor,
            borderRadius: BorderRadius.circular(8),
            border: Border.all(color: isDark ? Colors.black.withOpacity(0.5) : Colors.black12, width: 0.5),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              // Pixel-perfect Search Header
              Container(
                height: 38,
                padding: const EdgeInsets.symmetric(horizontal: 14),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    Text(
                      'Maccy',
                      style: TextStyle(
                        fontSize: 13,
                        fontFamily: '.AppleSystemUIFont',
                        color: labelColor,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Container(
                        height: 22,
                        alignment: Alignment.centerLeft, // Force vertical centering
                        decoration: BoxDecoration(
                          color: searchBg,
                          borderRadius: BorderRadius.circular(4),
                        ),
                        child: Row(
                          crossAxisAlignment: CrossAxisAlignment.center,
                          children: [
                            const SizedBox(width: 6),
                            Icon(Icons.search, size: 13, color: labelColor),
                            const SizedBox(width: 4),
                            Expanded(
                              child: TextField(
                                controller: searchController,
                                focusNode: focusNode,
                                autofocus: true,
                                cursorWidth: 1,
                                cursorHeight: 14,
                                cursorColor: isDark ? Colors.white70 : Colors.black87,
                                textAlignVertical: TextAlignVertical.center, // Center input text
                                style: TextStyle(
                                  fontSize: 12.5, 
                                  fontFamily: '.AppleSystemUIFont',
                                  color: isDark ? Colors.white : Colors.black,
                                  decoration: TextDecoration.none,
                                ),
                                decoration: const InputDecoration(
                                  isCollapsed: true, // No internal padding
                                  border: InputBorder.none,
                                  contentPadding: EdgeInsets.zero,
                                ),
                                onChanged: (_) => selectedIndex.value = 0,
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              Expanded(
                child: ListView.builder(
                  padding: EdgeInsets.zero,
                  itemCount: filteredItems.length,
                  itemBuilder: (context, index) {
                    final item = filteredItems[index];
                    final isSelected = index == selectedIndex.value;
                    return _MaccyRow(
                      content: item.content,
                      shortcut: index < 9 ? '${index + 1}' : null,
                      isSelected: isSelected,
                      selectionColor: highlightColor,
                      textColor: textColor,
                      shortcutColor: labelColor,
                      onTap: () => selectItem(index),
                      onHover: () => selectedIndex.value = index,
                    );
                  },
                ),
              ),
              Container(height: 0.5, color: lineDividerColor),
              const SizedBox(height: 2),
              _MaccyMenuRow(
                label: 'Clear',
                shortcut: '⌥⌘⌫',
                isSelected: selectedIndex.value == filteredItems.length,
                selectionColor: highlightColor,
                textColor: textColor,
                shortcutColor: labelColor,
                onTap: handleClear,
                onHover: () => selectedIndex.value = filteredItems.length,
              ),
              _MaccyMenuRow(
                label: 'Preferences...', 
                shortcut: '⌘,',
                isSelected: selectedIndex.value == filteredItems.length + 1,
                selectionColor: highlightColor,
                textColor: textColor,
                shortcutColor: labelColor,
                onTap: () {
                  context.push('/settings');
                  ref.read(appWindowManagerProvider.notifier).showSettings();
                },
                onHover: () => selectedIndex.value = filteredItems.length + 1,
              ),
              _MaccyMenuRow(
                label: 'About',
                isSelected: selectedIndex.value == filteredItems.length + 2,
                selectionColor: highlightColor,
                textColor: textColor,
                shortcutColor: labelColor,
                onTap: () {},
                onHover: () => selectedIndex.value = filteredItems.length + 2,
              ),
              _MaccyMenuRow(
                label: 'Quit',
                shortcut: '⌘Q',
                isSelected: selectedIndex.value == filteredItems.length + 3,
                selectionColor: highlightColor,
                textColor: textColor,
                shortcutColor: labelColor,
                onTap: () => exit(0),
                onHover: () => selectedIndex.value = filteredItems.length + 3,
              ),
              const SizedBox(height: 5),
            ],
          ),
        ),
      ),
    );
  }
}

class _MaccyRow extends StatelessWidget {
  final String content;
  final String? shortcut;
  final bool isSelected;
  final Color selectionColor;
  final Color textColor;
  final Color shortcutColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MaccyRow({
    required this.content,
    this.shortcut,
    required this.isSelected,
    required this.selectionColor,
    required this.textColor,
    required this.shortcutColor,
    required this.onTap,
    required this.onHover,
  });

  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      onEnter: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 22,
          padding: const EdgeInsets.symmetric(horizontal: 14),
          color: isSelected ? selectionColor : Colors.transparent,
          child: Row(
            children: [
              Expanded(
                child: Text(
                  content.trim().replaceAll('\n', ' '),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: TextStyle(
                    fontSize: 13,
                    fontFamily: '.AppleSystemUIFont',
                    color: isSelected ? Colors.white : textColor,
                  ),
                ),
              ),
              if (shortcut != null)
                Text(
                  '⌘$shortcut',
                  style: TextStyle(
                    fontSize: 12,
                    fontFamily: '.AppleSystemUIFont',
                    color: isSelected ? Colors.white70 : shortcutColor,
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

class _MaccyMenuRow extends StatelessWidget {
  final String label;
  final String? shortcut;
  final bool isSelected;
  final Color selectionColor;
  final Color textColor;
  final Color shortcutColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MaccyMenuRow({
    required this.label,
    this.shortcut,
    required this.isSelected,
    required this.selectionColor,
    required this.textColor,
    required this.shortcutColor,
    required this.onTap,
    required this.onHover,
  });

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MouseRegion(
      onEnter: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 22,
          padding: const EdgeInsets.symmetric(horizontal: 14),
          color: isSelected ? selectionColor : Colors.transparent,
          child: Row(
            children: [
              Expanded(
                child: Text(
                  label,
                  style: TextStyle(
                    fontSize: 13,
                    fontFamily: '.AppleSystemUIFont',
                    color: isSelected ? Colors.white : textColor,
                  ),
                ),
              ),
              if (shortcut != null)
                Text(
                  shortcut!,
                  style: TextStyle(
                    fontSize: 12,
                    fontFamily: '.AppleSystemUIFont',
                    color: isSelected ? Colors.white70 : shortcutColor,
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}