import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:drift/drift.dart' show OrderingTerm;
import 'package:go_router/go_router.dart';
import '../../../core/database/database_provider.dart';
import '../../../core/managers/window_manager_provider.dart';
import '../../settings/providers/settings_provider.dart';
import '../../settings/providers/pins_provider.dart';
import '../../../core/managers/clipboard_manager_provider.dart';

class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final db = ref.watch(appDatabaseProvider);
    final settingsAsync = ref.watch(settingsProvider);
    final pinsAsync = ref.watch(pinsProvider);
    
    final searchController = useTextEditingController();
    final selectedIndex = useState(0);
    final focusNode = useFocusNode();

    // Data streams
    final historyStream = useMemoized(() {
      return (db.select(db.clipboardEntries)
            ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
            ..limit(50))
          .watch();
    }, [db]);

    final history = useStream(historyStream);
    final historyItems = history.data ?? [];
    final pinnedItems = pinsAsync.value ?? [];

    // Filtered combined items
    final filteredHistory = useMemoized(() {
      if (searchController.text.isEmpty) return historyItems;
      return historyItems.where((item) => item.content.toLowerCase().contains(searchController.text.toLowerCase())).toList();
    }, [historyItems, searchController.text]);

    final filteredPins = useMemoized(() {
      if (searchController.text.isEmpty) return pinnedItems;
      return pinnedItems.where((item) => item.title.toLowerCase().contains(searchController.text.toLowerCase()) || item.content.toLowerCase().contains(searchController.text.toLowerCase())).toList();
    }, [pinnedItems, searchController.text]);

    // Total interactive items
    final totalItems = filteredPins.length + filteredHistory.length;

    Future<void> selectItem(int index) async {
      String content = "";
      if (index < filteredPins.length) {
        content = filteredPins[index].content;
      } else if (index < totalItems) {
        content = filteredHistory[index - filteredPins.length].content;
      } else {
        return; // Menu item handled elsewhere
      }

      await Clipboard.setData(ClipboardData(text: content));
      await ref.read(appWindowManagerProvider.notifier).hide();
      await ref.read(appClipboardManagerProvider.notifier).simulatePaste();
    }

    void handleKeyEvent(KeyEvent event) {
      if (event is KeyDownEvent) {
        final settings = settingsAsync.value;
        int menuCount = (settings?.showFooterMenu ?? true) ? 4 : 0;

        if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
          selectedIndex.value = (selectedIndex.value + 1).clamp(0, totalItems + menuCount - 1);
        } else if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
          selectedIndex.value = (selectedIndex.value - 1).clamp(0, totalItems + menuCount - 1);
        } else if (event.logicalKey == LogicalKeyboardKey.enter) {
          if (selectedIndex.value < totalItems) {
            selectItem(selectedIndex.value);
          } else {
            final menuIdx = selectedIndex.value - totalItems;
            if (menuIdx == 0) db.delete(db.clipboardEntries).go();
            if (menuIdx == 1) {
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

    if (settingsAsync.value == null) return const SizedBox.shrink();
    final settings = settingsAsync.value!;
    final isDark = Theme.of(context).brightness == Brightness.dark;
    
    final bgColor = isDark ? const Color(0xFF2C2C2C).withOpacity(0.98) : const Color(0xFFEBEBEB).withOpacity(0.98);
    final labelColor = isDark ? Colors.white24 : const Color(0xFF9A9A9A);
    final highlightColor = isDark ? const Color(0xFF0058D0) : const Color(0xFF0063E1);

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
              // Header
              _buildHeader(context, isDark, labelColor, searchController, focusNode, selectedIndex),
              
              // List
              Expanded(
                child: ListView(
                  padding: EdgeInsets.zero,
                  children: [
                    // Pinned Items
                    for (int i = 0; i < filteredPins.length; i++)
                      _MaccyRow(
                        content: filteredPins[i].title,
                        shortcut: i < 9 ? '${i + 1}' : null,
                        isPinned: true,
                        isSelected: selectedIndex.value == i,
                        selectionColor: highlightColor,
                        onTap: () => selectItem(i),
                        onHover: () => selectedIndex.value = i,
                      ),
                    // History Items
                    for (int i = 0; i < filteredHistory.length; i++)
                      _MaccyRow(
                        content: filteredHistory[i].content,
                        shortcut: (i + filteredPins.length) < 9 ? '${i + filteredPins.length + 1}' : null,
                        isPinned: false,
                        isSelected: selectedIndex.value == (i + filteredPins.length),
                        selectionColor: highlightColor,
                        onTap: () => selectItem(i + filteredPins.length),
                        onHover: () => selectedIndex.value = i + filteredPins.length,
                      ),
                  ],
                ),
              ),
              
              if (settings.showFooterMenu) ...[
                Container(height: 0.5, color: isDark ? Colors.white10 : Colors.black12),
                const SizedBox(height: 2),
                _MaccyMenuRow(
                  label: 'Clear',
                  shortcut: '⌥⌘⌫',
                  isSelected: selectedIndex.value == totalItems,
                  selectionColor: highlightColor,
                  onTap: () => db.delete(db.clipboardEntries).go(),
                  onHover: () => selectedIndex.value = totalItems,
                ),
                                _MaccyMenuRow(
                                  label: 'Preferences...',
                                  shortcut: '⌘,',
                                  isSelected: selectedIndex.value == totalItems + 1,
                                  selectionColor: highlightColor,
                                  onTap: () {
                                    ref.read(appWindowManagerProvider.notifier).showSettings();
                                  },
                                  onHover: () => selectedIndex.value = totalItems + 1,
                                ),                _MaccyMenuRow(
                  label: 'Quit',
                  shortcut: '⌘Q',
                  isSelected: selectedIndex.value == totalItems + 3,
                  selectionColor: highlightColor,
                  onTap: () => exit(0),
                  onHover: () => selectedIndex.value = totalItems + 3,
                ),
                const SizedBox(height: 5),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(BuildContext context, bool isDark, Color labelColor, TextEditingController ctrl, FocusNode fn, ValueNotifier<int> sel) {
    final searchBg = isDark ? Colors.white10 : const Color(0xFFD1D1D1);
    return Container(
      height: 38,
      padding: const EdgeInsets.symmetric(horizontal: 14),
      child: Row(
        children: [
          Text('Maccy', style: TextStyle(fontSize: 13, fontFamily: '.AppleSystemUIFont', color: labelColor)),
          const SizedBox(width: 12),
          Expanded(
            child: Container(
              height: 22,
              alignment: Alignment.centerLeft,
              decoration: BoxDecoration(color: searchBg, borderRadius: BorderRadius.circular(4)),
              child: Row(
                children: [
                  const SizedBox(width: 6),
                  Icon(Icons.search, size: 13, color: labelColor),
                  const SizedBox(width: 4),
                  Expanded(
                    child: TextField(
                      controller: ctrl,
                      focusNode: fn,
                      autofocus: true,
                      cursorWidth: 1,
                      style: TextStyle(fontSize: 12, color: isDark ? Colors.white : Colors.black),
                      decoration: const InputDecoration(isCollapsed: true, border: InputBorder.none),
                      onChanged: (_) => sel.value = 0,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _MaccyRow extends StatelessWidget {
  final String content;
  final String? shortcut;
  final bool isPinned;
  final bool isSelected;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MaccyRow({required this.content, this.shortcut, required this.isPinned, required this.isSelected, required this.selectionColor, required this.onTap, required this.onHover});

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
              if (isPinned) const Icon(Icons.push_pin, size: 10, color: Colors.blueAccent),
              Expanded(
                child: Padding(
                  padding: EdgeInsets.only(left: isPinned ? 4 : 0),
                  child: Text(
                    content.trim().replaceAll('\n', ' '),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(fontSize: 13, fontFamily: '.AppleSystemUIFont', color: isSelected ? Colors.white : (isDark ? Colors.white : Colors.black87)),
                  ),
                ),
              ),
              if (shortcut != null)
                Text('⌘$shortcut', style: TextStyle(fontSize: 12, color: isSelected ? Colors.white70 : (isDark ? Colors.white24 : Colors.black26))),
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
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MaccyMenuRow({required this.label, this.shortcut, required this.isSelected, required this.selectionColor, required this.onTap, required this.onHover});

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
                child: Text(label, style: TextStyle(fontSize: 13, color: isSelected ? Colors.white : (isDark ? Colors.white : Colors.black87))),
              ),
              if (shortcut != null)
                Text(shortcut!, style: TextStyle(fontSize: 12, color: isSelected ? Colors.white70 : (isDark ? Colors.white24 : Colors.black26))),
            ],
          ),
        ),
      ),
    );
  }
}
