import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../../core/database/database_provider.dart';
import '../../../core/managers/window_manager_provider.dart';
import '../../settings/providers/settings_provider.dart';
import '../providers/history_providers.dart';

class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final db = ref.watch(appDatabaseProvider);
    final settingsAsync = ref.watch(settingsProvider);

    final filteredPins = ref.watch(filteredPinsProvider);
    final filteredHistory = ref.watch(filteredHistoryProvider);
    final selectedIndex = ref.watch(historySelectedIndexProvider);
    final searchQuery = ref.watch(historySearchQueryProvider);

    final searchController = useTextEditingController(text: searchQuery);
    // Sync controller with provider if search query changes externally (though here it's mostly internal)
    useEffect(() {
      if (searchController.text != searchQuery) {
        searchController.text = searchQuery;
      }
      return null;
    }, [searchQuery]);

    final searchFocusNode = useFocusNode();
    final historyFocusNode = useFocusNode();

    final totalItems = filteredPins.length + filteredHistory.length;

    void handleKeyEvent(KeyEvent event) {
      if (event is KeyDownEvent) {
        print(event);
        final settings = settingsAsync.value;
        int menuCount = (settings?.showFooterMenu ?? true) ? 4 : 0;
        int maxIdx = totalItems + menuCount - 1;
        if (maxIdx < 0) return;

        if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
          ref.read(historySelectedIndexProvider.notifier).update((val) => (val + 1).clamp(0, maxIdx));
        } else if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
          ref.read(historySelectedIndexProvider.notifier).update((val) => (val - 1).clamp(0, maxIdx));
        } else if (event.logicalKey == LogicalKeyboardKey.enter) {
          if (selectedIndex < totalItems) {
            ref.read(historyActionsProvider.notifier).selectItem(selectedIndex);
          } else {
            final menuIdx = selectedIndex - totalItems;
            if (menuIdx == 0) db.delete(db.clipboardEntries).go();
            if (menuIdx == 1) ref.read(appWindowManagerProvider.notifier).showSettings();
            if (menuIdx == 3) exit(0);
          }
        } else if (event.logicalKey == LogicalKeyboardKey.escape) {
          ref.read(appWindowManagerProvider.notifier).hideHistory();
        } else if (HardwareKeyboard.instance.isAltPressed) {
          switch (event.logicalKey) {
            case LogicalKeyboardKey.digit1:
              ref.read(historyActionsProvider.notifier).selectItem(0);
              break;
            case LogicalKeyboardKey.digit2:
              ref.read(historyActionsProvider.notifier).selectItem(1);
              break;
            case LogicalKeyboardKey.digit3:
              ref.read(historyActionsProvider.notifier).selectItem(2);
              break;
            case LogicalKeyboardKey.digit4:
              ref.read(historyActionsProvider.notifier).selectItem(3);
              break;
            case LogicalKeyboardKey.digit5:
              ref.read(historyActionsProvider.notifier).selectItem(4);
              break;
            case LogicalKeyboardKey.digit6:
              ref.read(historyActionsProvider.notifier).selectItem(5);
              break;
            case LogicalKeyboardKey.digit7:
              ref.read(historyActionsProvider.notifier).selectItem(6);
              break;
            case LogicalKeyboardKey.digit8:
              ref.read(historyActionsProvider.notifier).selectItem(7);
              break;
            case LogicalKeyboardKey.digit9:
              ref.read(historyActionsProvider.notifier).selectItem(8);
              break;
          }
        }
      }
    }

    if (settingsAsync.value == null) return const SizedBox.shrink();
    final settings = settingsAsync.value!;
    final isDark = Theme.of(context).brightness == Brightness.dark;

    final bgColor = isDark ? const Color(0xFF2C2C2C).withOpacity(0.98) : const Color(0xFFEBEBEB).withOpacity(0.98);
    final highlightColor = isDark ? const Color(0xFF0058D0) : const Color(0xFF0063E1);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: KeyboardListener(
        focusNode: historyFocusNode,
        autofocus: true,
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
              _buildHeader(context, isDark, searchController, searchFocusNode, ref),

              // List
              Expanded(
                child: ListView(
                  padding: EdgeInsets.zero,
                  children: [
                    // Pinned Items
                    for (int i = 0; i < filteredPins.length; i++)
                      _HistoryRow(
                        content: filteredPins[i].content,
                        shortcut: i < 9 ? '${i + 1}' : null,
                        isPinned: true,
                        isSelected: selectedIndex == i,
                        selectionColor: highlightColor,
                        onTap: () => ref.read(historyActionsProvider.notifier).selectItem(i),
                        onHover: () => ref.read(historySelectedIndexProvider.notifier).set(i),
                        onPin: () => ref.read(historyActionsProvider.notifier).togglePin(i),
                        onDelete: () => ref.read(historyActionsProvider.notifier).deleteItem(i),
                      ),
                    // History Items
                    for (int i = 0; i < filteredHistory.length; i++)
                      _HistoryRow(
                        content: filteredHistory[i].content,
                        shortcut: (i + filteredPins.length) < 9 ? '${i + filteredPins.length + 1}' : null,
                        isPinned: false,
                        isSelected: selectedIndex == (i + filteredPins.length),
                        selectionColor: highlightColor,
                        onTap: () => ref.read(historyActionsProvider.notifier).selectItem(i + filteredPins.length),
                        onHover: () => ref.read(historySelectedIndexProvider.notifier).set(i + filteredPins.length),
                        onPin: () => ref.read(historyActionsProvider.notifier).togglePin(i + filteredPins.length),
                        onDelete: () => ref.read(historyActionsProvider.notifier).deleteItem(i + filteredPins.length),
                      ),
                  ],
                ),
              ),

              if (settings.showFooterMenu) ...[
                Container(height: 0.5, color: isDark ? Colors.white10 : Colors.black12),
                const SizedBox(height: 2),
                _MenuRow(
                  label: 'Clear History',
                  shortcut: '⌥⌘⌫',
                  isSelected: selectedIndex == totalItems,
                  selectionColor: highlightColor,
                  onTap: () => db.delete(db.clipboardEntries).go(),
                  onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems),
                ),
                _MenuRow(
                  label: 'Settings...',
                  shortcut: '⌘,',
                  isSelected: selectedIndex == totalItems + 1,
                  selectionColor: highlightColor,
                  onTap: () {
                    ref.read(appWindowManagerProvider.notifier).showSettings();
                  },
                  onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems + 1),
                ),
                _MenuRow(
                  label: 'Quit',
                  shortcut: '⌘Q',
                  isSelected: selectedIndex == totalItems + 3,
                  selectionColor: highlightColor,
                  onTap: () => exit(0),
                  onHover: () => ref.read(historySelectedIndexProvider.notifier).set(totalItems + 3),
                ),
                const SizedBox(height: 5),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(BuildContext context, bool isDark, TextEditingController ctrl, FocusNode fn, WidgetRef ref) {
    return Container(
      padding: const EdgeInsets.fromLTRB(4, 4, 4, 4),
      child: CupertinoSearchTextField(
        controller: ctrl,
        focusNode: fn,
        autofocus: true,
        placeholder: 'Search...',
        itemSize: 12,
        padding: const EdgeInsetsDirectional.fromSTEB(4, 0, 4, 0),
        borderRadius: BorderRadius.circular(12),
        backgroundColor: isDark ? Colors.white10 : Colors.black.withOpacity(0.06),
        style: TextStyle(fontSize: 12, color: isDark ? Colors.white : Colors.black, fontFamily: '.AppleSystemUIFont'),
        onChanged: (value) {
          ref.read(historySearchQueryProvider.notifier).set(value);
          ref.read(historySelectedIndexProvider.notifier).set(0);
        },
      ),
    );
  }
}

class _HistoryRow extends StatelessWidget {
  final String content;
  final String? shortcut;
  final bool isPinned;
  final bool isSelected;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;
  final VoidCallback onPin;
  final VoidCallback onDelete;

  const _HistoryRow({
    required this.content,
    this.shortcut,
    required this.isPinned,
    required this.isSelected,
    required this.selectionColor,
    required this.onTap,
    required this.onHover,
    required this.onPin,
    required this.onDelete,
  });

  @override
  Widget build(BuildContext context) {
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
  final IconData icon;
  final VoidCallback onTap;
  final Color hoverColor;
  final Color baseColor;

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

class _MenuRow extends StatelessWidget {
  final String label;
  final String? shortcut;
  final bool isSelected;
  final Color selectionColor;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const _MenuRow({required this.label, this.shortcut, required this.isSelected, required this.selectionColor, required this.onTap, required this.onHover});

  @override
  Widget build(BuildContext context) {
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
