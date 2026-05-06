import 'package:flutter/cupertino.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';
import 'package:maccy/features/settings/ui/widgets/number_stepper.dart';

/// 设置：存储选项页。
///
/// 负责配置历史记录的保留上限，以及选择需要捕获的内容类型（纯文本、图片、文件与文件夹）。
class StorageTab extends HookConsumerWidget {
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
                onChanged: (v) =>
                    ref.read(historyLimitProvider.notifier).set(v),
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
                onChanged: (v) =>
                    ref.read(saveTextProvider.notifier).set(v ?? false),
              ),
            ),
            MacosSettingsTile(
              label: 'Images',
              icon: CupertinoIcons.photo,
              iconColor: CupertinoColors.systemPink,
              trailing: CupertinoCheckbox(
                value: ref.watch(saveImagesProvider),
                onChanged: (v) =>
                    ref.read(saveImagesProvider.notifier).set(v ?? false),
              ),
            ),
            MacosSettingsTile(
              label: 'Files & Folders',
              icon: CupertinoIcons.doc,
              iconColor: CupertinoColors.systemOrange,
              trailing: CupertinoCheckbox(
                value: ref.watch(saveFilesProvider),
                onChanged: (v) =>
                    ref.read(saveFilesProvider.notifier).set(v ?? false),
              ),
            ),
          ],
        ),
      ],
    );
  }
}
