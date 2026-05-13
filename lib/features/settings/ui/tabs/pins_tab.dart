import 'package:drift/drift.dart' as drift;
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/database/database_provider.dart';

/// 设置：置顶项目管理选项卡。
///
/// 允许用户集中查看、排序或批量取消置顶的剪贴板条目。
class PinsTab extends HookConsumerWidget {
  const PinsTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final db = ref.watch(appDatabaseProvider);
    final pinnedItemsStream = useMemoized(() => _watchPinnedItems(db), [db]);
    final pinnedItemsSnapshot = useStream(pinnedItemsStream);
    final selectedId = useState<int?>(null);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    final items = pinnedItemsSnapshot.data ?? [];

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          if (items.isEmpty)
            Center(
              child: Padding(
                padding: const EdgeInsets.symmetric(vertical: 64),
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      CupertinoIcons.pin_slash,
                      size: 64,
                      color: isDark ? Colors.white24 : Colors.black26,
                    ),
                    const SizedBox(height: 16),
                    Text(
                      'No pinned items',
                      style: TextStyle(
                        fontSize: 16,
                        color: isDark ? Colors.white38 : Colors.black38,
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Pin items from the history view using Alt+P',
                      style: TextStyle(
                        fontSize: 13,
                        color: isDark ? Colors.white24 : Colors.black26,
                      ),
                    ),
                  ],
                ),
              ),
            )
          else
            Container(
              margin: const EdgeInsets.only(bottom: 16),
              decoration: BoxDecoration(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.03)
                    : Colors.black.withValues(alpha: 0.02),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(
                  color: isDark
                      ? Colors.white.withValues(alpha: 0.1)
                      : Colors.black.withValues(alpha: 0.1),
                ),
              ),
              child: Column(
                children: [
                  // Table header
                  Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 8,
                    ),
                    decoration: BoxDecoration(
                      color: isDark
                          ? Colors.white.withValues(alpha: 0.05)
                          : Colors.black.withValues(alpha: 0.03),
                      borderRadius: const BorderRadius.only(
                        topLeft: Radius.circular(8),
                        topRight: Radius.circular(8),
                      ),
                    ),
                    child: Row(
                      children: [
                        SizedBox(
                          width: 80,
                          child: Text(
                            'Key',
                            style: TextStyle(
                              fontSize: 13,
                              fontWeight: FontWeight.w600,
                              color: isDark ? Colors.white70 : Colors.black87,
                            ),
                          ),
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          flex: 2,
                          child: Text(
                            'Alias',
                            style: TextStyle(
                              fontSize: 13,
                              fontWeight: FontWeight.w600,
                              color: isDark ? Colors.white70 : Colors.black87,
                            ),
                          ),
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          flex: 3,
                          child: Text(
                            'Content',
                            style: TextStyle(
                              fontSize: 13,
                              fontWeight: FontWeight.w600,
                              color: isDark ? Colors.white70 : Colors.black87,
                            ),
                          ),
                        ),
                        const SizedBox(width: 40),
                      ],
                    ),
                  ),
                  // Table rows
                  ...items.asMap().entries.map((entry) {
                    final index = entry.key;
                    final item = entry.value;
                    final isSelected = selectedId.value == item.id;
                    final isLast = index == items.length - 1;

                    return _PinTableRow(
                      item: item,
                      isSelected: isSelected,
                      isLast: isLast,
                      onTap: () => selectedId.value = item.id,
                      onDelete: () async {
                        await _unpinItem(db, item.id);
                        if (selectedId.value == item.id) {
                          selectedId.value = null;
                        }
                      },
                      onUpdatePin: (newPin) async {
                        await _updateItemPin(db, item.id, newPin);
                      },
                      onUpdateTitle: (newTitle) async {
                        await _updateItemTitle(db, item.id, newTitle);
                      },
                      onUpdateAlias: (newAlias) async {
                        await _updateItemAlias(db, item.id, newAlias);
                      },
                    );
                  }),
                ],
              ),
            ),
          // Description
          Padding(
            padding: const EdgeInsets.only(top: 16),
            child: Text(
              'Customize pinned items by changing their keyboard shortcuts, aliases, or content. '
              'Press Delete to unpin selected items.',
              style: TextStyle(
                fontSize: 12,
                color: isDark ? Colors.white38 : Colors.black38,
              ),
            ),
          ),
        ],
      ),
    );
  }

  /// 监听所有固定项。
  Stream<List<HistoryItem>> _watchPinnedItems(AppDatabase db) {
    return (db.select(db.historyItems)
          ..where((t) => t.pin.isNotNull())
          ..orderBy([(t) => drift.OrderingTerm.asc(t.pin)]))
        .watch();
  }

  /// 取消固定。
  Future<void> _unpinItem(AppDatabase db, int itemId) async {
    await (db.update(db.historyItems)..where((t) => t.id.equals(itemId))).write(
      const HistoryItemsCompanion(pin: drift.Value(null)),
    );
  }

  /// 更新固定快捷键。
  Future<void> _updateItemPin(AppDatabase db, int itemId, String newPin) async {
    await (db.update(db.historyItems)..where((t) => t.id.equals(itemId))).write(
      HistoryItemsCompanion(pin: drift.Value(newPin)),
    );
  }

  /// 更新标题。
  Future<void> _updateItemTitle(
    AppDatabase db,
    int itemId,
    String newTitle,
  ) async {
    await (db.update(db.historyItems)..where((t) => t.id.equals(itemId))).write(
      HistoryItemsCompanion(title: drift.Value(newTitle)),
    );
  }
}

/// 更新别名。
Future<void> _updateItemAlias(
  AppDatabase db,
  int itemId,
  String newAlias,
) async {
  await (db.update(db.historyItems)..where((t) => t.id.equals(itemId))).write(
    HistoryItemsCompanion(
      alias: drift.Value(newAlias.isEmpty ? null : newAlias),
    ),
  );
}

/// 固定项表格行。
class _PinTableRow extends StatelessWidget {
  const _PinTableRow({
    required this.item,
    required this.isSelected,
    required this.isLast,
    required this.onTap,
    required this.onDelete,
    required this.onUpdatePin,
    required this.onUpdateTitle,
    required this.onUpdateAlias,
  });

  final HistoryItem item;
  final bool isSelected;
  final bool isLast;
  final VoidCallback onTap;
  final VoidCallback onDelete;
  final ValueChanged<String> onUpdatePin;
  final ValueChanged<String> onUpdateTitle;
  final ValueChanged<String> onUpdateAlias;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Padding(
      padding: .symmetric(vertical: 4),
      child: Row(
        children: [
          const SizedBox(width: 8),
          // Key column
          _PinKeyPicker(
            currentPin: item.pin ?? '',
            onChanged: onUpdatePin,
          ),

          // SizedBox(
          //   width: 80,
          //   child: _PinKeyPicker(
          //     currentPin: item.pin ?? '',
          //     onChanged: onUpdatePin,
          //   ),
          // ),
          const SizedBox(width: 8),
          // Alias column
          Expanded(
            flex: 2,
            child: _EditableTextField(
              initialValue: item.alias ?? '',
              onChanged: onUpdateAlias,
            ),
          ),
          const SizedBox(width: 8),
          // Content column
          Expanded(
            flex: 3,
            child: _EditableTextField(
              initialValue: item.title,
              onChanged: onUpdateTitle,
            ),
          ),
          // Delete button
          const SizedBox(width: 8),

          CupertinoButton(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
            color: isDark
                ? Colors.white.withValues(alpha: 0.1)
                : Colors.black.withValues(alpha: 0.05),
            borderRadius: BorderRadius.circular(6),
            onPressed: onDelete,
            minimumSize: Size.zero,
            child: Text(
              'delete',
              style: TextStyle(
                fontSize: 12,
                color: isDark ? Colors.white : Colors.black87,
              ),
            ),
          ),
          const SizedBox(width: 8),
        ],
      ),
    );
  }
}

/// 固定快捷键选择器。
class _PinKeyPicker extends StatelessWidget {
  const _PinKeyPicker({required this.currentPin, required this.onChanged});

  final String currentPin;
  final ValueChanged<String> onChanged;

  static const _availablePins = [
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    'N',
    'O',
    'P',
    'R',
    'S',
    'T',
    'U',
    'X',
    'Y',
  ];

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return MenuAnchor(
      alignmentOffset: const Offset(0, 4),
      style: MenuStyle(
        backgroundColor: WidgetStatePropertyAll(
          isDark ? const Color(0xFF2D2D2D) : const Color(0xFFF2F2F2),
        ),
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(10),
            side: BorderSide(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.15)
                  : Colors.black.withValues(alpha: 0.1),
              width: 0.5,
            ),
          ),
        ),
        elevation: const WidgetStatePropertyAll(16),
        shadowColor: WidgetStatePropertyAll(
          Colors.black.withValues(alpha: isDark ? 0.5 : 0.2),
        ),
        padding: const WidgetStatePropertyAll(EdgeInsets.all(6)),
      ),
      builder: (context, controller, child) {
        return CupertinoButton(
          padding: EdgeInsets.zero,
          onPressed: () =>
              controller.isOpen ? controller.close() : controller.open(),
          minimumSize: Size.zero,
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.06)
                  : Colors.black.withValues(alpha: 0.04),
              borderRadius: BorderRadius.circular(6),
              border: Border.all(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.1)
                    : Colors.black.withValues(alpha: 0.1),
                width: 0.5,
              ),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  'Alt+${currentPin.toUpperCase()}',
                  style: TextStyle(
                    fontSize: 13,
                    fontFamily: 'Courier New',
                    color: isDark
                        ? Colors.white.withValues(alpha: 0.9)
                        : Colors.black87,
                  ),
                ),
                const SizedBox(width: 4),
                Icon(
                  CupertinoIcons.chevron_up_chevron_down,
                  size: 10,
                  color: isDark ? Colors.white38 : Colors.black38,
                ),
              ],
            ),
          ),
        );
      },
      menuChildren: _availablePins.map((pin) {
        return MenuItemButton(
          onPressed: () => onChanged(pin),
          style: ButtonStyle(
            padding: const WidgetStatePropertyAll(
              EdgeInsets.symmetric(horizontal: 4),
            ),
            minimumSize: const WidgetStatePropertyAll(Size(60, 26)),
            fixedSize: const WidgetStatePropertyAll(Size.fromHeight(26)),
            overlayColor: WidgetStatePropertyAll(
              CupertinoColors.activeBlue.withValues(alpha: 0.9),
            ),
            foregroundColor: WidgetStateProperty.resolveWith(
              (states) =>
                  states.contains(WidgetState.hovered) ||
                      states.contains(WidgetState.pressed)
                  ? Colors.white
                  : (isDark
                        ? Colors.white.withValues(alpha: 0.9)
                        : Colors.black87),
            ),
            shape: WidgetStatePropertyAll(
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(5)),
            ),
          ),
          child: Row(
            children: [
              SizedBox(
                width: 20,
                child: currentPin == pin
                    ? const Icon(CupertinoIcons.checkmark, size: 14)
                    : null,
              ),
              Text(
                'Alt+$pin',
                style: const TextStyle(
                  fontSize: 13,
                  fontFamily: 'Courier New',
                  fontWeight: FontWeight.w400,
                ),
              ),
            ],
          ),
        );
      }).toList(),
    );
  }
}

/// 可编辑文本框。
class _EditableTextField extends HookWidget {
  const _EditableTextField({
    required this.initialValue,
    required this.onChanged,
  });

  final String initialValue;
  final ValueChanged<String> onChanged;

  @override
  Widget build(BuildContext context) {
    final controller = useTextEditingController(text: initialValue);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return CupertinoTextField(
      controller: controller,
      style: TextStyle(
        fontSize: 12,
        color: isDark ? Colors.white.withValues(alpha: 0.87) : Colors.black87,
      ),
      decoration: BoxDecoration(
        color: isDark
            ? Colors.white.withValues(alpha: 0.05)
            : Colors.black.withValues(alpha: 0.03),
        borderRadius: BorderRadius.circular(4),
        border: Border.all(
          color: isDark
              ? Colors.white.withValues(alpha: 0.1)
              : Colors.black.withValues(alpha: 0.1),
        ),
      ),
      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 4),
      onChanged: onChanged,
      // onSubmitted: onChanged,
      // onEditingComplete: () => onChanged(controller.text),
    );
  }
}
