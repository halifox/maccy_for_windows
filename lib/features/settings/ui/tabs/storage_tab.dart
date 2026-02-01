import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../providers/settings_provider.dart';
import '../../../../core/database/database.dart';
import 'package:drift/drift.dart' show Value;
import '../widgets/macos_settings_widgets.dart';

class StorageTab extends ConsumerWidget {
  final AppSetting settings;
  const StorageTab({super.key, required this.settings});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final notifier = ref.read(settingsProvider.notifier);
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
              trailing: Row(
                children: [
                  Text('${settings.historyLimit}', style: const TextStyle(fontSize: 13, color: CupertinoColors.systemGrey)),
                  const SizedBox(width: 8),
                  CupertinoButton(
                    padding: EdgeInsets.zero,
                    minSize: 0,
                    child: const Icon(CupertinoIcons.chevron_up_chevron_down, size: 14, color: CupertinoColors.systemGrey),
                    onPressed: () {
                      // 弹出选择菜单或直接 +100
                      int next = (settings.historyLimit + 50);
                      if (next > 1000) next = 50;
                      notifier.updateHistoryLimit(next);
                    },
                  ),
                ],
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
                value: settings.saveText,
                onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(saveText: Value(v!))),
              ),
            ),
            MacosSettingsTile(
              label: 'Images',
              icon: CupertinoIcons.photo,
              iconColor: CupertinoColors.systemPink,
              trailing: CupertinoCheckbox(
                value: settings.saveImages,
                onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(saveImages: Value(v!))),
              ),
            ),
            MacosSettingsTile(
              label: 'Files & Folders',
              icon: CupertinoIcons.doc,
              iconColor: CupertinoColors.systemOrange,
              trailing: CupertinoCheckbox(
                value: settings.saveFiles,
                onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(saveFiles: Value(v!))),
              ),
            ),
          ],
        ),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          child: Text(
            'Saved data size is approximately 16 MB.\nOlder items are automatically deleted once the limit is reached.',
            style: TextStyle(fontSize: 11, color: isDark ? Colors.white24 : Colors.black38),
          ),
        ),
      ],
    );
  }
}