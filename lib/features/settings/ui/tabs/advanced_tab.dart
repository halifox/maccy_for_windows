import 'package:flutter/cupertino.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:haliclip/features/settings/providers/settings_provider.dart';
import 'package:haliclip/features/settings/ui/widgets/macos_settings_widgets.dart';

/// 设置：高级选项页。
///
/// 包含录制状态控制（暂停/恢复）以及隐私清理相关的逻辑配置。
class AdvancedTab extends ConsumerWidget {
  const AdvancedTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Column(
      children: [
        MacosSettingsGroup(
          title: 'Recording',
          children: [
            MacosSettingsTile(
              label: 'Pause capture',
              subtitle: 'Stop recording new clipboard items',
              icon: CupertinoIcons.pause_circle,
              iconColor: CupertinoColors.systemGrey,
              trailing: CupertinoCheckbox(
                value: ref.watch(isPausedProvider),
                onChanged: (v) =>
                    ref.read(isPausedProvider.notifier).set(v ?? false),
              ),
            ),
          ],
        ),
        MacosSettingsGroup(
          title: 'Privacy',
          children: [
            MacosSettingsTile(
              label: 'Clear on exit',
              icon: CupertinoIcons.trash,
              iconColor: CupertinoColors.systemRed,
              trailing: CupertinoCheckbox(
                value: ref.watch(clearOnExitProvider),
                onChanged: (v) =>
                    ref.read(clearOnExitProvider.notifier).set(v ?? false),
              ),
            ),
            MacosSettingsTile(
              label: 'Clear system clipboard',
              subtitle: 'Also clear system clipboard when history is cleared',
              icon: CupertinoIcons.clear_circled,
              iconColor: CupertinoColors.systemOrange,
              trailing: CupertinoCheckbox(
                value: ref.watch(clearSystemClipboardProvider),
                onChanged: (v) => ref
                    .read(clearSystemClipboardProvider.notifier)
                    .set(v ?? false),
              ),
            ),
          ],
        ),
      ],
    );
  }
}
