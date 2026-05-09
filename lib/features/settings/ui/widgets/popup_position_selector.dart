import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/services/screen_service.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';

/// 弹出位置选择器组件。
///
/// 用于设置界面，允许用户选择窗口弹出位置。
/// 对应 Maccy 的 AppearanceSettingsPane 中的 PopupAt 配置。
class PopupPositionSelector extends ConsumerWidget {
  const PopupPositionSelector({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final position = ref.watch(popupPositionProvider);
    final screenIndex = ref.watch(popupScreenProvider);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'Popup Position',
          style: TextStyle(fontSize: 13, fontWeight: FontWeight.w500),
        ),
        const SizedBox(height: 8),
        Row(
          children: [
            Expanded(
              child: DropdownButton<String>(
                value: position,
                isExpanded: true,
                items: const [
                  DropdownMenuItem(
                    value: 'cursor',
                    child: Text('Cursor - Follow mouse position'),
                  ),
                  DropdownMenuItem(
                    value: 'center',
                    child: Text('Center - Screen center'),
                  ),
                  DropdownMenuItem(
                    value: 'statusItem',
                    child: Text('Status Item - Near system tray'),
                  ),
                  DropdownMenuItem(
                    value: 'lastPosition',
                    child: Text('Last Position - Remember location'),
                  ),
                ],
                onChanged: (value) {
                  if (value != null) {
                    ref.read(popupPositionProvider.notifier).set(value);
                  }
                },
              ),
            ),
            const SizedBox(width: 16),
            // 屏幕选择器（仅在 center 或 lastPosition 模式下显示）
            if (position == 'center' || position == 'lastPosition')
              SizedBox(
                width: 150,
                child: FutureBuilder<List<Map<String, dynamic>>>(
                  future: ScreenService.getAllScreensInfo(),
                  builder: (context, snapshot) {
                    if (!snapshot.hasData) {
                      return const SizedBox.shrink();
                    }

                    final screens = snapshot.data!;
                    if (screens.length <= 1) {
                      return const SizedBox.shrink();
                    }

                    return DropdownButton<int>(
                      value: screenIndex,
                      isExpanded: true,
                      items: [
                        const DropdownMenuItem(
                          value: 0,
                          child: Text('Active Screen'),
                        ),
                        ...screens.map((screen) {
                          final index = screen['index'] as int;
                          final name = screen['name'] as String;
                          final isPrimary = screen['isPrimary'] as bool;
                          return DropdownMenuItem(
                            value: index,
                            child: Text(
                              isPrimary ? '$name (Primary)' : name,
                            ),
                          );
                        }),
                      ],
                      onChanged: (value) {
                        if (value != null) {
                          ref.read(popupScreenProvider.notifier).set(value);
                        }
                      },
                    );
                  },
                ),
              ),
          ],
        ),
        const SizedBox(height: 4),
        Text(
          _getDescription(position),
          style: TextStyle(
            fontSize: 11,
            color: Colors.grey[600],
          ),
        ),
      ],
    );
  }

  String _getDescription(String position) {
    switch (position) {
      case 'cursor':
        return 'Window appears near the mouse cursor (like Spotlight).';
      case 'center':
        return 'Window appears at the center of the selected screen.';
      case 'statusItem':
        return 'Window appears near the system tray icon.';
      case 'lastPosition':
        return 'Window remembers and appears at the last used position.';
      default:
        return '';
    }
  }
}
