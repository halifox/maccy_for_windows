import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../providers/settings_provider.dart';
import '../../../../core/database/database.dart';
import 'package:drift/drift.dart' show Value;
import '../widgets/macos_settings_widgets.dart';

class GeneralTab extends ConsumerWidget {
  final AppSetting settings;
  const GeneralTab({super.key, required this.settings});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final notifier = ref.read(settingsProvider.notifier);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Column(
      children: [
        MacosSettingsGroup(title: 'Startup', children: [
          MacosSettingsTile(
            label: 'Launch at login',
            icon: CupertinoIcons.power,
            iconColor: CupertinoColors.systemBlue,
            trailing: CupertinoSwitch(
              value: settings.launchAtStartup,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(launchAtStartup: Value(v))),
            ),
          ),
          MacosSettingsTile(
            label: 'Auto-check for updates',
            icon: CupertinoIcons.arrow_2_circlepath,
            iconColor: CupertinoColors.systemGreen,
            trailing: CupertinoSwitch(
              value: true,
              onChanged: (v) {},
            ),
          ),
        ]),
        MacosSettingsGroup(title: 'Behavior', children: [
          MacosSettingsTile(
            label: 'Auto-paste',
            subtitle: 'Paste automatically when selecting an item',
            icon: CupertinoIcons.doc_on_clipboard,
            iconColor: CupertinoColors.systemOrange,
            trailing: CupertinoSwitch(
              value: settings.autoPaste,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(autoPaste: Value(v))),
            ),
          ),
          MacosSettingsTile(
            label: 'Pure text paste',
            subtitle: 'Always strip formatting',
            icon: CupertinoIcons.text_quote,
            iconColor: CupertinoColors.systemGreen,
            trailing: CupertinoSwitch(
              value: settings.pastePlain,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(pastePlain: Value(v))),
            ),
          ),
        ]),
        // macOS 26 behavior description style
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          child: Text(
            'Custom behavior when selecting items:\n• Hold ⌥ to only copy.\n• Hold ⌘ to copy and paste.\n• Hold ⇧⌘ to copy and match format.',
            style: TextStyle(fontSize: 11, color: isDark ? Colors.white24 : Colors.black38, height: 1.4),
          ),
        ),
      ],
    );
  }
}
