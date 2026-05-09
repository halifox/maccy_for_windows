import 'package:flutter/cupertino.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/tabs/general_tab.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';
import 'package:maccy/features/settings/ui/widgets/number_stepper.dart';

/// 设置：存储选项页。
///
/// 负责配置历史记录的保留上限、排序方式，以及选择需要捕获的内容类型（纯文本、图片、文件与文件夹）。
class StorageTab extends HookConsumerWidget {
  const StorageTab({super.key});

  String _getSortBySubtitle(String sortBy) {
    return switch (sortBy) {
      'lastCopiedAt' => 'Most recently copied items first',
      'firstCopiedAt' => 'Oldest items first',
      'numberOfCopies' => 'Most frequently copied items first',
      _ => 'Most recently copied items first',
    };
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return SingleChildScrollView(
      child: Column(
        children: [
          MacosSettingsGroup(
            title: 'Storage Policy',
            children: [
              MacosSettingsTile(
                label: 'History Size',
                subtitle: 'Maximum number of clipboard items to keep',
                icon: CupertinoIcons.list_number,
                iconColor: CupertinoColors.systemPurple,
                trailing: NumberStepper(
                  value: ref.watch(historyLimitProvider),
                  onChanged: (v) =>
                      ref.read(historyLimitProvider.notifier).set(v),
                  min: 1,
                  max: 999,
                ),
              ),
              MacosSettingsTile(
                label: 'Sort By',
                subtitle: _getSortBySubtitle(ref.watch(sortByProvider)),
                icon: CupertinoIcons.sort_down,
                iconColor: CupertinoColors.systemIndigo,
                trailing: _SortByMenu(),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Content Types',
            children: [
              MacosSettingsTile(
                label: 'Text Snippets',
                subtitle: 'Plain text, HTML, and RTF content',
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
                subtitle: 'PNG, JPEG, TIFF, and other image formats',
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
                subtitle: 'File paths and directory references',
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
          MacosSettingsGroup(
            title: 'Cleanup',
            children: [
              MacosSettingsTile(
                label: 'Clear on Exit',
                subtitle: 'Delete all history when quitting the application',
                icon: CupertinoIcons.trash,
                iconColor: CupertinoColors.systemRed,
                trailing: CupertinoCheckbox(
                  value: ref.watch(clearOnExitProvider),
                  onChanged: (v) =>
                      ref.read(clearOnExitProvider.notifier).set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Clear System Clipboard',
                subtitle: 'Also clear system clipboard when clearing history',
                icon: CupertinoIcons.clear,
                iconColor: CupertinoColors.systemGrey,
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
      ),
    );
  }
}

/// 排序方式下拉菜单
class _SortByMenu extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final sortBy = ref.watch(sortByProvider);

    final displayName = switch (sortBy) {
      'lastCopiedAt' => 'Last Copied',
      'firstCopiedAt' => 'First Copied',
      'numberOfCopies' => 'Copy Count',
      _ => 'Last Copied',
    };

    return MacosPopupMenu<String>(
      value: displayName,
      items: const ['lastCopiedAt', 'firstCopiedAt', 'numberOfCopies'],
      itemLabelBuilder: (v) => switch (v) {
        'lastCopiedAt' => 'Last Copied',
        'firstCopiedAt' => 'First Copied',
        'numberOfCopies' => 'Copy Count',
        _ => 'Last Copied',
      },
      onSelected: (v) => ref.read(sortByProvider.notifier).set(v),
      selectedItemBuilder: (v) => v == sortBy,
    );
  }
}
