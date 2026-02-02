import 'dart:io';

import 'package:clipboard/core/managers/clipboard_manager_provider.dart';
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

  String _getSearchModeName(String mode) {
    return switch (mode) {
      'exact' => 'Exact',
      'fuzzy' => 'Fuzzy',
      'regex' => 'Regex',
      'mixed' => 'Mixed',
      _ => 'Fuzzy',
    };
  }

  String _getSearchModeSubtitle(String mode) {
    return switch (mode) {
      'exact' => 'Matches exact text',
      'fuzzy' => 'Finds similar text',
      'regex' => 'Use regular expressions',
      'mixed' => 'Combine fuzzy and exact',
      _ => 'Finds similar text',
    };
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final notifier = ref.read(settingsProvider.notifier);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        children: [
          MacosSettingsGroup(
            title: 'Launch & Update',
            children: [
              MacosSettingsTile(
                label: 'Launch at login',
                subtitle: 'Start application when you log in',
                icon: CupertinoIcons.power,
                iconColor: CupertinoColors.systemBlue,
                trailing: CupertinoCheckbox(
                  value: settings.launchAtStartup,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(launchAtStartup: Value(v ?? false))),
                ),
              ),
              MacosSettingsTile(
                label: 'Auto-check for updates',
                subtitle: 'Keep application up to date',
                icon: CupertinoIcons.arrow_2_circlepath,
                iconColor: CupertinoColors.systemGreen,
                trailing: CupertinoCheckbox(
                  value: settings.autoCheckUpdates,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(autoCheckUpdates: Value(v ?? false))),
                ),
              ),
              MacosSettingsTile(
                label: 'Check for Updates',
                icon: CupertinoIcons.refresh,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoButton(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                  color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.05),
                  borderRadius: BorderRadius.circular(6),
                  onPressed: () {}, minimumSize: Size(0, 0),
                  child: Text('Check Now', style: TextStyle(fontSize: 12, color: isDark ? Colors.white : Colors.black87)),
                ),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Keyboard Shortcuts',
            children: [
              MacosSettingsTile(
                label: 'Open Clipboard',
                subtitle: 'Toggle clipboard history visibility',
                icon: CupertinoIcons.keyboard,
                iconColor: CupertinoColors.systemOrange,
                trailing: _HotkeyDisplay(value: settings.hotkeyOpen),
              ),
              MacosSettingsTile(
                label: 'Pin Item',
                subtitle: 'Quickly pin the selected item',
                icon: CupertinoIcons.pin,
                iconColor: CupertinoColors.systemRed,
                trailing: _HotkeyDisplay(value: settings.hotkeyPin),
              ),
              MacosSettingsTile(
                label: 'Delete Item',
                subtitle: 'Remove the selected item from history',
                icon: CupertinoIcons.trash,
                iconColor: CupertinoColors.systemRed,
                trailing: _HotkeyDisplay(value: settings.hotkeyDelete),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Search',
            children: [
              MacosSettingsTile(
                label: 'Search Mode',
                subtitle: _getSearchModeSubtitle(settings.searchMode),
                icon: CupertinoIcons.search,
                iconColor: CupertinoColors.systemBlue,
                trailing: MenuAnchor(
                  alignmentOffset: const Offset(0, 4),
                  style: MenuStyle(
                    backgroundColor: WidgetStatePropertyAll(isDark ? const Color(0xFF2D2D2D) : const Color(0xFFF2F2F2)),
                    shape: WidgetStatePropertyAll(
                      RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(10),
                        side: BorderSide(color: isDark ? Colors.white.withOpacity(0.15) : Colors.black.withOpacity(0.1), width: 0.5),
                      ),
                    ),
                    elevation: const WidgetStatePropertyAll(16),
                    shadowColor: WidgetStatePropertyAll(Colors.black.withOpacity(isDark ? 0.5 : 0.2)),
                    padding: const WidgetStatePropertyAll(EdgeInsets.all(6)),
                  ),
                  builder: (context, controller, child) {
                    return CupertinoButton(
                      padding: EdgeInsets.zero,
                      onPressed: () {
                        if (controller.isOpen) {
                          controller.close();
                        } else {
                          controller.open();
                        }
                      }, minimumSize: Size(0, 0),
                      child: Container(
                        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                        decoration: BoxDecoration(
                          color: isDark ? Colors.white.withOpacity(0.06) : Colors.black.withOpacity(0.04),
                          borderRadius: BorderRadius.circular(6),
                          border: Border.all(
                            color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1),
                            width: 0.5,
                          ),
                        ),
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Text(
                              _getSearchModeName(settings.searchMode),
                              style: TextStyle(
                                fontSize: 13,
                                color: isDark ? Colors.white.withOpacity(0.9) : Colors.black87,
                                fontWeight: FontWeight.w400,
                              ),
                            ),
                            const SizedBox(width: 8),
                            Icon(
                              CupertinoIcons.chevron_up_chevron_down,
                              size: 10,
                              color: isDark ? Colors.white38 : Colors.black38,
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                  menuChildren: [
                    _buildMacosMenuItem(context, 'exact', 'Exact', settings.searchMode == 'exact', notifier),
                    _buildMacosMenuItem(context, 'fuzzy', 'Fuzzy', settings.searchMode == 'fuzzy', notifier),
                    _buildMacosMenuItem(context, 'regex', 'Regex', settings.searchMode == 'regex', notifier),
                    _buildMacosMenuItem(context, 'mixed', 'Mixed', settings.searchMode == 'mixed', notifier),
                  ],
                ),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Behavior',
            children: [
              MacosSettingsTile(
                label: 'Auto-paste',
                subtitle: 'Paste automatically when selecting an item',
                icon: CupertinoIcons.doc_on_clipboard,
                iconColor: CupertinoColors.systemOrange,
                trailing: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    if (Platform.isMacOS)
                      FutureBuilder<bool>(
                        future: ref.read(appClipboardManagerProvider.notifier).checkAccessibilityPermissions(),
                        builder: (context, snapshot) {
                          if (snapshot.data == false) {
                            return Padding(
                              padding: const EdgeInsets.only(right: 8.0),
                              child: CupertinoButton(
                                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                                color: CupertinoColors.systemRed.withOpacity(0.1),
                                borderRadius: BorderRadius.circular(4),
                                onPressed: () => ref.read(appClipboardManagerProvider.notifier).requestAccessibilityPermissions(),
                                minimumSize: Size(0, 0),
                                child: const Text('Grant Access', style: TextStyle(fontSize: 10, color: CupertinoColors.systemRed)),
                              ),
                            );
                          }
                          return const SizedBox.shrink();
                        },
                      ),
                    CupertinoCheckbox(
                      value: settings.autoPaste,
                      onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(autoPaste: Value(v ?? false))),
                    ),
                  ],
                ),
              ),
              MacosSettingsTile(
                label: 'Pure text paste',
                subtitle: 'Always strip formatting from clipboard',
                icon: CupertinoIcons.text_quote,
                iconColor: CupertinoColors.systemGreen,
                trailing: CupertinoCheckbox(
                  value: settings.pastePlain,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(pastePlain: Value(v ?? false))),
                ),
              ),
            ],
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _ModifierDesc('• Hold ⌥ to only copy.'),
                _ModifierDesc('• Hold ⌘ to copy and paste.'),
                _ModifierDesc('• Hold ⇧⌘ to copy and match format.'),
              ],
            ),
          ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }

  Widget _buildMacosMenuItem(BuildContext context, String value, String text, bool selected, SettingsNotifier notifier) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MenuItemButton(
      onPressed: () => notifier.updateSettings(AppSettingsCompanion(searchMode: Value(value))),
      style: ButtonStyle(
        padding: const WidgetStatePropertyAll(EdgeInsets.symmetric(horizontal: 4)),
        minimumSize: const WidgetStatePropertyAll(Size(120, 26)),
        fixedSize: const WidgetStatePropertyAll(Size.fromHeight(26)),
        overlayColor: WidgetStatePropertyAll(CupertinoColors.activeBlue.withValues(alpha: 0.9)),
        foregroundColor: WidgetStateProperty.resolveWith(
          (states) => states.contains(WidgetState.hovered) || states.contains(WidgetState.pressed)
              ? Colors.white
              : (isDark ? Colors.white.withOpacity(0.9) : Colors.black87),
        ),
        shape: WidgetStatePropertyAll(RoundedRectangleBorder(borderRadius: BorderRadius.circular(5))),
      ),
      child: Row(
        children: [
          SizedBox(
            width: 20,
            child: selected ? const Icon(CupertinoIcons.checkmark, size: 14) : null,
          ),
          Text(text, style: const TextStyle(fontSize: 13, fontWeight: FontWeight.w400)),
        ],
      ),
    );
  }
}

class _HotkeyDisplay extends StatelessWidget {
  final String value;
  const _HotkeyDisplay({required this.value});

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
      decoration: BoxDecoration(
        color: isDark ? Colors.white.withOpacity(0.05) : Colors.black.withOpacity(0.05),
        borderRadius: BorderRadius.circular(4),
        border: Border.all(color: isDark ? Colors.white10 : Colors.black, width: 0.5),
      ),
      child: Text(
        value.isEmpty ? 'Not Set' : value,
        style: TextStyle(
          fontSize: 12,
          color: value.isEmpty ? CupertinoColors.systemGrey : (isDark ? Colors.white70 : Colors.black87),
        ),
      ),
    );
  }
}

class _ModifierDesc extends StatelessWidget {
  final String text;
  const _ModifierDesc(this.text);

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return Text(
      text,
      style: TextStyle(fontSize: 11, color: isDark ? Colors.white24 : Colors.black38, height: 1.5),
    );
  }
}


// class _HotkeyRecorderTile extends StatelessWidget {
//   final String value;
//   final ValueChanged<String> onChanged;
//
//   const _HotkeyRecorderTile({required this.value, required this.onChanged});
//
//   @override
//   Widget build(BuildContext context) {
//     final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
//     return Container(
//       padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
//       decoration: BoxDecoration(
//         color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.05),
//         borderRadius: BorderRadius.circular(6),
//         border: Border.all(color: isDark ? Colors.white10 : Colors.black),
//       ),
//       child: Row(
//         mainAxisSize: MainAxisSize.min,
//         children: [
//           Text(
//             value.isEmpty ? '设置快捷键' : value,
//             style: TextStyle(
//               fontSize: 12,
//               color: value.isEmpty ? (isDark ? Colors.white38 : Colors.black38) : (isDark ? Colors.white : Colors.black87),
//             ),
//           ),
//           if (value.isNotEmpty) ...[
//             const SizedBox(width: 6),
//             GestureDetector(
//               onTap: () => onChanged(''),
//               child: Icon(CupertinoIcons.clear_circled_solid, size: 14, color: isDark ? Colors.white38 : Colors.black38),
//             ),
//           ],
//         ],
//       ),
//     );
//   }
// }

// class _ModifierDescription extends StatelessWidget {
//   final String text;
//   final bool isDark;
//   const _ModifierDescription({required this.text, required this.isDark});
//
//   @override
//   Widget build(BuildContext context) {
//     return Text(
//       text,
//       style: TextStyle(fontSize: 11, color: isDark ? Colors.white38 : Colors.black38, height: 1.5),
//     );
//   }
// }

