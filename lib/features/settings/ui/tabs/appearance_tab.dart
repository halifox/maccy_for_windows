import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../providers/settings_provider.dart';
import '../../../../core/database/database.dart';
import 'package:drift/drift.dart' show Value;
import '../widgets/macos_settings_widgets.dart';

class AppearanceTab extends ConsumerWidget {
  final AppSetting settings;
  const AppearanceTab({super.key, required this.settings});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final notifier = ref.read(settingsProvider.notifier);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Column(
      children: [
        MacosSettingsGroup(
          title: 'Panel Configuration',
          children: [
            MacosSettingsTile(
              label: 'Popup Position',
              subtitle: 'Where the clipboard history appears',
              icon: CupertinoIcons.cursor_rays,
              iconColor: CupertinoColors.activeBlue,
              trailing: CupertinoButton(
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                minSize: 0,
                color: isDark ? Colors.white10 : Colors.black12,
                borderRadius: BorderRadius.circular(6),
                child: Text(
                  settings.popupPosition.toUpperCase(),
                  style: TextStyle(fontSize: 11, color: isDark ? Colors.white70 : Colors.black87, fontWeight: FontWeight.w600),
                ),
                onPressed: () {
                  final next = settings.popupPosition == 'cursor' ? 'center' : 'cursor';
                  notifier.updateSettings(AppSettingsCompanion(popupPosition: Value(next)));
                }, 
              ),
            ),
            MacosSettingsTile(
              label: 'Panel Width',
              icon: CupertinoIcons.arrow_left_right,
              iconColor: CupertinoColors.systemTeal,
              trailing: SizedBox(
                width: 140,
                child: CupertinoSlider(
                  value: settings.windowWidth,
                  min: 300, max: 600,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(windowWidth: Value(v))),
                ),
              ),
            ),
          ],
        ),
        MacosSettingsGroup(
          title: 'Interface Elements',
          children: [
            MacosSettingsTile(
              label: 'Application Icons',
              subtitle: 'Show source app icons in history',
              icon: CupertinoIcons.app_badge,
              iconColor: CupertinoColors.systemIndigo,
              trailing: CupertinoSwitch(
                value: settings.showAppIcon,
                onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showAppIcon: Value(v))),
              ),
            ),
            MacosSettingsTile(
              label: 'Footer Menu',
              subtitle: 'Show action menu at the bottom',
              icon: CupertinoIcons.list_bullet_below_rectangle,
              iconColor: CupertinoColors.systemGrey,
              trailing: CupertinoSwitch(
                value: settings.showFooterMenu,
                onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showFooterMenu: Value(v))),
              ),
            ),
          ],
        ),
      ],
    );
  }
}