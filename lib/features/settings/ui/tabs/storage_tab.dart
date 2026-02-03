import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../providers/settings_provider.dart';
import '../widgets/macos_settings_widgets.dart';
import '../widgets/number_stepper.dart';

/// 存储设置选项卡，配置历史记录限制和保存的内容类型
class StorageTab extends HookConsumerWidget {
  /// 构造函数
  const StorageTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return Column(
      children: [
        MacosSettingsGroup(
          title: 'Storage Policy',
          children: [
            MacosSettingsTile(
              label: 'Keep History',
              subtitle: 'Limit the number of stored clipboard items',
              icon: CupertinoIcons.list_number,
              iconColor: CupertinoColors.systemPurple,
              trailing: NumberStepper(
                value: ref.watch(historyLimitProvider),
                onChanged: (v) => ref.read(historyLimitProvider.notifier).set(v),
              ),
            ),
          ],
        ),
        MacosSettingsGroup(
          title: 'Content to Save',
          children: [
            MacosSettingsTile(
              label: 'Text Snippets',
              icon: CupertinoIcons.text_alignleft,
              iconColor: CupertinoColors.systemBlue,
              trailing: CupertinoCheckbox(
                value: ref.watch(saveTextProvider),
                onChanged: (v) => ref.read(saveTextProvider.notifier).set(v ?? false),
              ),
            ),
            MacosSettingsTile(
              label: 'Images',
              icon: CupertinoIcons.photo,
              iconColor: CupertinoColors.systemPink,
              trailing: CupertinoCheckbox(
                value: ref.watch(saveImagesProvider),
                onChanged: (v) => ref.read(saveImagesProvider.notifier).set(v ?? false),
              ),
            ),
            MacosSettingsTile(
              label: 'Files & Folders',
              icon: CupertinoIcons.doc,
              iconColor: CupertinoColors.systemOrange,
              trailing: CupertinoCheckbox(
                value: ref.watch(saveFilesProvider),
                onChanged: (v) => ref.read(saveFilesProvider.notifier).set(v ?? false),
              ),
            ),
          ],
        ),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          child: Text(
            'Older items are automatically deleted once the limit is reached.',
            style: TextStyle(fontSize: 11, color: isDark ? Colors.white24 : Colors.black38),
          ),
        ),
      ],
    );
  }
}
