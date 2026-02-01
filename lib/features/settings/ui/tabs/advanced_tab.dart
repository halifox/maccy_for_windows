import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../providers/settings_provider.dart';
import '../../../../core/database/database.dart';
import 'package:drift/drift.dart' show Value;
import '../widgets/macos_settings_widgets.dart';

class AdvancedTab extends ConsumerWidget {
  final AppSetting settings;
  const AdvancedTab({super.key, required this.settings});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final notifier = ref.read(settingsProvider.notifier);
    return Column(
      children: [
        MacosSettingsGroup(title: 'Recording', children: [
          MacosSettingsTile(
            label: 'Pause capture',
            subtitle: 'Stop recording new clipboard items',
            icon: CupertinoIcons.pause_circle,
            iconColor: CupertinoColors.systemGrey,
            trailing: CupertinoSwitch(
              value: settings.isPaused,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(isPaused: Value(v))),
            ),
          ),
        ]),
        MacosSettingsGroup(title: 'Privacy', children: [
          MacosSettingsTile(
            label: 'Clear on exit',
            icon: CupertinoIcons.trash,
            iconColor: CupertinoColors.systemRed,
            trailing: CupertinoSwitch(
              value: settings.clearOnExit,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(clearOnExit: Value(v))),
            ),
          ),
          MacosSettingsTile(
            label: 'Clear system clipboard',
            subtitle: 'Also clear system clipboard when history is cleared',
            icon: CupertinoIcons.clear_circled,
            iconColor: CupertinoColors.systemOrange,
            trailing: CupertinoSwitch(
              value: settings.clearSystemClipboard,
              onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(clearSystemClipboard: Value(v))),
            ),
          ),
        ]),
      ],
    );
  }
}
