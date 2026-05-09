import 'dart:io';

import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/database/database.dart';
import 'package:intl/intl.dart';

/// 图片/内容预览弹窗组件。
///
/// 显示剪贴板条目的详细信息，包括图片预览、应用来源、复制时间、复制次数等。
class PreviewPopover extends ConsumerWidget {
  const PreviewPopover({
    required this.item,
    super.key,
  });

  final ClipboardEntry item;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final dateFormat = DateFormat('yyyy-MM-dd HH:mm:ss');

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
            // 图片或文本内容预览
            if (item.type == 'image')
              Flexible(
                child: ClipRRect(
                  borderRadius: BorderRadius.circular(5),
                  child: Image.file(
                    File(item.content),
                    fit: BoxFit.contain,
                    errorBuilder: (_, _, _) => const Icon(
                      Icons.broken_image,
                      size: 64,
                    ),
                  ),
                ),
              )
            else
              Flexible(
                child: SingleChildScrollView(
                  child: SelectableText(
                    item.content,
                    style: TextStyle(
                      fontSize: 13,
                      color: isDark ? Colors.white : Colors.black87,
                    ),
                  ),
                ),
              ),

            const SizedBox(height: 16),
            Divider(
              color: isDark ? Colors.white10 : Colors.black12,
              height: 1,
            ),
            const SizedBox(height: 12),

            // 元数据信息
            _InfoRow(
              label: 'Created',
              value: dateFormat.format(item.createdAt),
              isDark: isDark,
            ),

            if (item.isPinned) ...[
              const SizedBox(height: 12),
              Divider(
                color: isDark ? Colors.white10 : Colors.black12,
                height: 1,
              ),
              const SizedBox(height: 12),
              Text(
                'Press Alt+P to unpin',
                style: TextStyle(
                  fontSize: 11,
                  color: isDark ? Colors.white54 : Colors.black54,
                ),
              ),
            ],

            const SizedBox(height: 6),
            Text(
              'Press Alt+D to delete',
              style: TextStyle(
                fontSize: 11,
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
            color: isDark ? Colors.white70 : Colors.black54,
          ),
        ),
        Text(
          value,
          style: TextStyle(
            fontSize: 12,
            color: isDark ? Colors.white : Colors.black87,
          ),
        ),
      ],
    );
  }
}
