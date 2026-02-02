import 'package:clipboard/features/history/providers/history_providers.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../../../core/database/database.dart';

class PinsTab extends ConsumerWidget {
  const PinsTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final pinsAsync = ref.watch(historyEntriesProvider); // Watch all entries but we will filter pins
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // macOS 26 Table Container
          Container(
            decoration: BoxDecoration(
              color: isDark ? const Color(0xFF1E1E1E) : Colors.white,
              borderRadius: BorderRadius.circular(10),
              border: Border.all(
                color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1),
                width: 0.5,
              ),
              boxShadow: [
                if (!isDark)
                  BoxShadow(
                    color: Colors.black.withOpacity(0.05),
                    blurRadius: 10,
                    offset: const Offset(0, 2),
                  ),
              ],
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                _buildHeader(isDark),
                pinsAsync.when(
                  data: (entries) {
                    final pins = entries.where((e) => e.isPinned).toList();
                    return pins.isEmpty
                      ? const _EmptyState()
                      : ListView.builder(
                          shrinkWrap: true,
                          padding: EdgeInsets.zero,
                          physics: const NeverScrollableScrollPhysics(),
                          itemCount: pins.length,
                          itemBuilder: (context, index) {
                            return _PinTableRow(
                              entry: pins[index],
                              isLast: index == pins.length - 1,
                            );
                          },
                        );
                  },
                  loading: () => const Padding(
                    padding: EdgeInsets.all(40.0),
                    child: CupertinoActivityIndicator(),
                  ),
                  error: (err, _) => Padding(
                    padding: const EdgeInsets.all(20.0),
                    child: Text('Error: $err', style: const TextStyle(color: CupertinoColors.systemRed, fontSize: 12)),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          _AddAction(isDark: isDark),
          const SizedBox(height: 32),
          Text(
            '您可以管理您的固定项目。',
            style: TextStyle(fontSize: 11, color: isDark ? Colors.white24 : Colors.black38),
          ),
        ],
      ),
    );
  }

  Widget _buildHeader(bool isDark) {
    return Container(
      height: 32,
      decoration: BoxDecoration(
        color: isDark ? Colors.white.withOpacity(0.03) : Colors.black.withOpacity(0.02),
        borderRadius: const BorderRadius.vertical(top: Radius.circular(10)),
        border: Border(
          bottom: BorderSide(
            color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05),
            width: 0.5,
          ),
        ),
      ),
      child: Row(
        children: [
          _buildHeaderCell('内容', isDark: isDark, isLast: true),
        ],
      ),
    );
  }

  Widget _buildHeaderCell(String text, {double? width, required bool isDark, bool isLast = false}) {
    final content = Container(
      alignment: Alignment.centerLeft,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Text(
        text,
        style: TextStyle(
          fontSize: 11,
          fontWeight: FontWeight.w500,
          color: isDark ? Colors.white38 : Colors.black45,
        ),
      ),
    );

    final cell = width != null ? SizedBox(width: width, child: content) : Expanded(child: content);

    if (isLast) return cell;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        cell,
        Container(
          width: 0.5,
          height: 16,
          color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05),
        ),
      ],
    );
  }
}

class _PinTableRow extends ConsumerWidget {
  final ClipboardEntry entry;
  final bool isLast;
  const _PinTableRow({required this.entry, required this.isLast});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    final borderColor = isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05);

    return GestureDetector(
      onTap: () => _showEditDialog(context, entry),
      behavior: HitTestBehavior.opaque,
      child: Container(
        height: 36,
        decoration: BoxDecoration(
          border: isLast ? null : Border(bottom: BorderSide(color: borderColor, width: 0.5)),
        ),
        child: Row(
          children: [
            _buildCell(entry.content.replaceAll('\n', ' '), isDark: isDark, isLast: true),
            Padding(
              padding: const EdgeInsets.only(right: 8),
              child: CupertinoButton(
                padding: EdgeInsets.zero,
                onPressed: () => ref.read(historyActionsProvider.notifier).deleteItemById(entry.id), minimumSize: Size(0, 0),
                child: Icon(CupertinoIcons.minus_circle_fill, size: 14, color: CupertinoColors.systemRed.withValues(alpha: 0.6)),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildCell(String text, {double? width, required bool isDark, bool isLast = false, bool isBold = false, bool isDim = false}) {
    final content = Container(
      alignment: Alignment.centerLeft,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      child: Text(
        text,
        maxLines: 1,
        overflow: TextOverflow.ellipsis,
        style: TextStyle(
          fontSize: 12,
          fontWeight: isBold ? FontWeight.w500 : FontWeight.w400,
          color: isDim 
            ? (isDark ? Colors.white24 : Colors.black38) 
            : (isDark ? Colors.white.withOpacity(0.9) : Colors.black87),
        ),
      ),
    );

    final cell = width != null ? SizedBox(width: width, child: content) : Expanded(child: content);
    if (isLast) return cell;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        cell,
        Container(
          width: 0.5,
          height: 36,
          color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05),
        ),
      ],
    );
  }
}

class _AddAction extends StatelessWidget {
  final bool isDark;
  const _AddAction({required this.isDark});

  @override
  Widget build(BuildContext context) {
    return CupertinoButton(
      padding: EdgeInsets.zero,
      onPressed: () => _showEditDialog(context, null),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.03),
          borderRadius: BorderRadius.circular(6),
          border: Border.all(color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1), width: 0.5),
        ),
        child: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(CupertinoIcons.plus, size: 14),
            SizedBox(width: 4),
            Text('添加', style: TextStyle(fontSize: 12, fontWeight: FontWeight.w500)),
          ],
        ),
      ),
    );
  }
}

void _showEditDialog(BuildContext context, ClipboardEntry? entry) {
  showCupertinoDialog(
    context: context,
    builder: (context) => _PinEditDialog(entry: entry),
  );
}

class _PinEditDialog extends HookConsumerWidget {
  final ClipboardEntry? entry;
  const _PinEditDialog({this.entry});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final contentController = useTextEditingController(text: entry?.content);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return CupertinoAlertDialog(
      title: Text(entry == null ? '添加固定项目' : '编辑项目'),
      content: Padding(
        padding: const EdgeInsets.only(top: 12),
        child: Column(
          children: [
            CupertinoTextField(
              controller: contentController,
              placeholder: '内容',
              maxLines: 4,
              style: const TextStyle(fontSize: 13),
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
              decoration: BoxDecoration(
                color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(color: isDark ? Colors.white10 : Colors.black12),
              ),
            ),
          ],
        ),
      ),
      actions: [
        CupertinoDialogAction(
          onPressed: () => Navigator.pop(context),
          child: const Text('取消'),
        ),
        CupertinoDialogAction(
          isDefaultAction: true,
          onPressed: () {
            if (contentController.text.isNotEmpty) {
              if (entry == null) {
                ref.read(historyActionsProvider.notifier).addItem(
                  contentController.text,
                  isPinned: true,
                );
              } else {
                // If editing existing, we can add a method to update it if needed, 
                // but for now, we follow the table structure.
              }
              Navigator.pop(context);
            }
          },
          child: const Text('保存'),
        ),
      ],
    );
  }
}

class _EmptyState extends StatelessWidget {
  const _EmptyState();
  @override
  Widget build(BuildContext context) {
    return const Padding(
      padding: EdgeInsets.all(48.0),
      child: Center(
        child: Text(
          '暂无固定项目',
          style: TextStyle(fontSize: 12, color: CupertinoColors.systemGrey),
        ),
      ),
    );
  }
}