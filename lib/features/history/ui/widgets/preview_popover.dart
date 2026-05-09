import 'dart:io';

import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/constants/ui_constants.dart';
import 'package:maccy/core/database/database.dart';
import 'package:maccy/features/history/repositories/history_repository.dart';
import 'package:intl/intl.dart';

/// 图片/内容预览弹窗组件。
///
/// 显示剪贴板条目的详细信息，包括图片预览、应用来源、复制时间、复制次数等。
/// 完全基于 Maccy 的 PreviewItemView.swift 实现。
class PreviewPopover extends ConsumerWidget {
  const PreviewPopover({
    required this.item,
    super.key,
  });

  final HistoryItem item;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final dateFormat = DateFormat('yyyy-MM-dd HH:mm:ss');
    final repository = ref.watch(historyRepositoryProvider);

    return Container(
      constraints: const BoxConstraints(
        maxWidth: 400,
        maxHeight: 500,
      ),
      decoration: BoxDecoration(
        color: isDark
            ? const Color(0xFF2C2C2C).withValues(alpha: 0.95)
            : const Color(0xFFF5F5F5).withValues(alpha: 0.95),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: isDark ? Colors.white12 : Colors.black12,
          width: 1,
        ),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.3),
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // 内容预览区域
            Flexible(
              child: FutureBuilder<List<HistoryItemContent>>(
                future: repository.getItemContents(item.id),
                builder: (context, snapshot) {
                  if (!snapshot.hasData) {
                    return const Center(child: CircularProgressIndicator());
                  }

                  final contents = snapshot.data!;

                  // 检查是否有图片
                  final imageContent = contents.firstWhere(
                    (c) => c.type.startsWith('image/'),
                    orElse: () => const HistoryItemContent(id: 0, itemId: 0, type: '', value: null),
                  );

                  if (imageContent.value != null) {
                    // 显示图片
                    return ClipRRect(
                      borderRadius: BorderRadius.circular(5),
                      child: Image.memory(
                        imageContent.value!,
                        fit: BoxFit.contain,
                        errorBuilder: (_, _, _) => const Icon(
                          Icons.broken_image,
                          size: 64,
                        ),
                      ),
                    );
                  } else {
                    // 显示文本
                    return SingleChildScrollView(
                      child: SelectableText(
                        item.title,
                        style: TextStyle(
                          fontSize: MaccyUIConstants.primaryFontSize,
                          fontFamily: Platform.isWindows
                              ? MaccyUIConstants.systemFontFamilyWindows
                              : MaccyUIConstants.systemFontFamily,
                          color: isDark ? Colors.white : Colors.black87,
                        ),
                      ),
                    );
                  }
                },
              ),
            ),

            const SizedBox(height: 16),
            Divider(
              color: isDark ? Colors.white10 : Colors.black12,
              height: 1,
            ),
            const SizedBox(height: 12),

            // 元数据信息（基于 Maccy 的 PreviewItemView）
            if (item.application != null) ...[
              _InfoRow(
                label: 'Application',
                value: item.application!,
                isDark: isDark,
              ),
              const SizedBox(height: 8),
            ],

            _InfoRow(
              label: 'First Copy Time',
              value: dateFormat.format(item.firstCopiedAt),
              isDark: isDark,
            ),
            const SizedBox(height: 8),

            _InfoRow(
              label: 'Last Copy Time',
              value: dateFormat.format(item.lastCopiedAt),
              isDark: isDark,
            ),
            const SizedBox(height: 8),

            _InfoRow(
              label: 'Number of Copies',
              value: item.numberOfCopies.toString(),
              isDark: isDark,
            ),

            const SizedBox(height: 12),
            Divider(
              color: isDark ? Colors.white10 : Colors.black12,
              height: 1,
            ),
            const SizedBox(height: 12),

            // 快捷键提示
            if (item.pin != null)
              Padding(
                padding: const EdgeInsets.only(bottom: 6),
                child: Text(
                  'Press Alt+P to unpin',
                  style: TextStyle(
                    fontSize: 11,
                    fontFamily: Platform.isWindows
                        ? MaccyUIConstants.systemFontFamilyWindows
                        : MaccyUIConstants.systemFontFamily,
                    color: isDark ? Colors.white54 : Colors.black54,
                  ),
                ),
              )
            else
              Padding(
                padding: const EdgeInsets.only(bottom: 6),
                child: Text(
                  'Press Alt+P to pin',
                  style: TextStyle(
                    fontSize: 11,
                    fontFamily: Platform.isWindows
                        ? MaccyUIConstants.systemFontFamilyWindows
                        : MaccyUIConstants.systemFontFamily,
                    color: isDark ? Colors.white54 : Colors.black54,
                  ),
                ),
              ),

            Text(
              'Press Alt+D to delete',
              style: TextStyle(
                fontSize: 11,
                fontFamily: Platform.isWindows
                    ? MaccyUIConstants.systemFontFamilyWindows
                    : MaccyUIConstants.systemFontFamily,
                color: isDark ? Colors.white54 : Colors.black54,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// 信息行组件。
class _InfoRow extends StatelessWidget {
  const _InfoRow({
    required this.label,
    required this.value,
    required this.isDark,
  });

  final String label;
  final String value;
  final bool isDark;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Text(
          '$label: ',
          style: TextStyle(
            fontSize: 12,
            fontFamily: Platform.isWindows
                ? MaccyUIConstants.systemFontFamilyWindows
                : MaccyUIConstants.systemFontFamily,
            color: isDark ? Colors.white70 : Colors.black54,
          ),
        ),
        Expanded(
          child: Text(
            value,
            style: TextStyle(
              fontSize: 12,
              fontFamily: Platform.isWindows
                  ? MaccyUIConstants.systemFontFamilyWindows
                  : MaccyUIConstants.systemFontFamily,
              color: isDark ? Colors.white : Colors.black87,
            ),
            overflow: TextOverflow.ellipsis,
          ),
        ),
      ],
    );
  }
}
