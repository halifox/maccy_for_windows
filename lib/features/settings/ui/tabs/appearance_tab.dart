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

    return SingleChildScrollView(
      child: Column(
        children: [
          MacosSettingsGroup(
            title: 'Panel Configuration',
            children: [
              _buildDropdownTile(
                context,
                label: 'Popup Position',
                subtitle: 'Where the clipboard history appears',
                icon: CupertinoIcons.cursor_rays,
                iconColor: CupertinoColors.activeBlue,
                currentValue: settings.popupPosition,
                options: {'cursor': 'Cursor', 'center': 'Center'},
                onSelected: (v) => notifier.updateSettings(AppSettingsCompanion(popupPosition: Value(v))),
              ),
              _buildDropdownTile(
                context,
                label: 'Pinned Position',
                subtitle: 'Where pinned items are displayed',
                icon: CupertinoIcons.pin,
                iconColor: CupertinoColors.systemOrange,
                currentValue: settings.pinPosition,
                options: {'top': 'Top', 'bottom': 'Bottom'},
                onSelected: (v) => notifier.updateSettings(AppSettingsCompanion(pinPosition: Value(v))),
              ),
              MacosSettingsTile(
                label: 'Image Height',
                subtitle: 'Height of image previews in history',
                icon: CupertinoIcons.photo,
                iconColor: CupertinoColors.systemPink,
                trailing: _NumberStepper(
                  value: settings.imageHeight,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(imageHeight: Value(v))),
                ),
              ),
              MacosSettingsTile(
                label: 'Preview Delay',
                subtitle: 'Delay before showing content preview (ms)',
                icon: CupertinoIcons.timer,
                iconColor: CupertinoColors.systemTeal,
                trailing: _NumberStepper(
                  value: settings.previewDelay,
                  step: 100,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(previewDelay: Value(v))),
                ),
              ),
              _buildDropdownTile(
                context,
                label: 'Highlight Match',
                subtitle: 'Visual style for search results',
                icon: CupertinoIcons.selection_pin_in_out,
                iconColor: CupertinoColors.systemYellow,
                currentValue: settings.highlightMatch,
                options: {'bold': 'Bold', 'color': 'Color'},
                onSelected: (v) => notifier.updateSettings(AppSettingsCompanion(highlightMatch: Value(v))),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Interface Elements',
            children: [
              MacosSettingsTile(
                label: 'Special Characters',
                subtitle: 'Show whitespace and line breaks',
                icon: CupertinoIcons.paragraph,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoCheckbox(
                  value: settings.showSpecialChars,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showSpecialChars: Value(v ?? false))),
                ),
              ),
              MacosSettingsTile(
                label: 'Menu Bar Icon',
                subtitle: 'Show app in system menu bar',
                icon: CupertinoIcons.macwindow,
                iconColor: CupertinoColors.activeBlue,
                trailing: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    _buildIconDropdown(context, settings.menuBarIconType, (v) {
                      notifier.updateSettings(AppSettingsCompanion(menuBarIconType: Value(v)));
                    }),
                    const SizedBox(width: 12),
                    CupertinoCheckbox(
                      value: settings.showMenuBarIcon,
                      onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showMenuBarIcon: Value(v ?? false))),
                    ),
                  ],
                ),
              ),
              MacosSettingsTile(
                label: 'Clipboard in Menu Bar',
                subtitle: 'Show preview text near menu bar icon',
                icon: CupertinoIcons.text_insert,
                iconColor: CupertinoColors.systemIndigo,
                trailing: CupertinoCheckbox(
                  value: settings.showClipboardNearIcon,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showClipboardNearIcon: Value(v ?? false))),
                ),
              ),
              _buildDropdownTile(
                context,
                label: 'Search Box',
                subtitle: 'When to display the search input',
                icon: CupertinoIcons.search,
                iconColor: CupertinoColors.systemBlue,
                currentValue: settings.showSearchBox,
                options: {'always': 'Always', 'typing': 'When Typing', 'never': 'Never'},
                onSelected: (v) => notifier.updateSettings(AppSettingsCompanion(showSearchBox: Value(v))),
              ),
              MacosSettingsTile(
                label: 'Source App Name',
                subtitle: 'Show application name before search box',
                icon: CupertinoIcons.info_circle,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoCheckbox(
                  value: settings.showAppName,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showAppName: Value(v ?? false))),
                ),
              ),
              MacosSettingsTile(
                label: 'Application Icons',
                subtitle: 'Show source app icons in history',
                icon: CupertinoIcons.app_badge,
                iconColor: CupertinoColors.systemIndigo,
                trailing: CupertinoCheckbox(
                  value: settings.showAppIcon,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showAppIcon: Value(v ?? false))),
                ),
              ),
              MacosSettingsTile(
                label: 'Footer Menu',
                subtitle: 'Show action menu at the bottom',
                icon: CupertinoIcons.list_bullet_below_rectangle,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoCheckbox(
                  value: settings.showFooterMenu,
                  onChanged: (v) => notifier.updateSettings(AppSettingsCompanion(showFooterMenu: Value(v ?? false))),
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }

  Widget _buildDropdownTile(
    BuildContext context, {
    required String label,
    required String subtitle,
    required IconData icon,
    required Color iconColor,
    required String currentValue,
    required Map<String, String> options,
    required ValueChanged<String> onSelected,
  }) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MacosSettingsTile(
      label: label,
      subtitle: subtitle,
      icon: icon,
      iconColor: iconColor,
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
            onPressed: () => controller.isOpen ? controller.close() : controller.open(), minimumSize: Size(0, 0),
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              decoration: BoxDecoration(
                color: isDark ? Colors.white.withOpacity(0.06) : Colors.black.withOpacity(0.04),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1), width: 0.5),
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    options[currentValue] ?? currentValue,
                    style: TextStyle(fontSize: 13, color: isDark ? Colors.white.withOpacity(0.9) : Colors.black87),
                  ),
                  const SizedBox(width: 8),
                  Icon(CupertinoIcons.chevron_up_chevron_down, size: 10, color: isDark ? Colors.white38 : Colors.black38),
                ],
              ),
            ),
          );
        },
        menuChildren: options.entries.map((e) => _buildMenuItem(context, e.key, e.value, currentValue == e.key, onSelected)).toList(),
      ),
    );
  }

  Widget _buildMenuItem(BuildContext context, String value, String text, bool selected, ValueChanged<String> onSelected) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MenuItemButton(
      onPressed: () => onSelected(value),
      style: ButtonStyle(
        padding: const WidgetStatePropertyAll(EdgeInsets.symmetric(horizontal: 4)),
        minimumSize: const WidgetStatePropertyAll(Size(100, 26)),
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
          SizedBox(width: 20, child: selected ? const Icon(CupertinoIcons.checkmark, size: 14) : null),
          Text(text, style: const TextStyle(fontSize: 13, fontWeight: FontWeight.w400)),
        ],
      ),
    );
  }

  Widget _buildIconDropdown(BuildContext context, String current, ValueChanged<String> onSelected) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final icons = {
      'clipboard': CupertinoIcons.doc_on_clipboard,
      'star': CupertinoIcons.star,
      'bell': CupertinoIcons.bell,
    };

    return MenuAnchor(
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
          onPressed: () => controller.isOpen ? controller.close() : controller.open(), minimumSize: Size(0, 0),
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: isDark ? Colors.white.withOpacity(0.06) : Colors.black.withOpacity(0.04),
              borderRadius: BorderRadius.circular(6),
              border: Border.all(color: isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1), width: 0.5),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(icons[current] ?? CupertinoIcons.doc_on_clipboard, size: 14, color: isDark ? Colors.white.withOpacity(0.9) : Colors.black87),
                const SizedBox(width: 8),
                Icon(CupertinoIcons.chevron_up_chevron_down, size: 10, color: isDark ? Colors.white38 : Colors.black38),
              ],
            ),
          ),
        );
      },
      menuChildren: icons.entries.map((e) {
        return MenuItemButton(
          onPressed: () => onSelected(e.key),
          style: ButtonStyle(
            padding: const WidgetStatePropertyAll(EdgeInsets.symmetric(horizontal: 4)),
            minimumSize: const WidgetStatePropertyAll(Size(80, 26)),
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
              SizedBox(width: 20, child: current == e.key ? const Icon(CupertinoIcons.checkmark, size: 14) : null),
              Icon(e.value, size: 14),
              const SizedBox(width: 8),
              Text(e.key[0].toUpperCase() + e.key.substring(1), style: const TextStyle(fontSize: 13)),
            ],
          ),
        );
      }).toList(),
    );
  }
}

class _NumberStepper extends StatelessWidget {
  final int value;
  final int step;
  final ValueChanged<int> onChanged;
  const _NumberStepper({required this.value, this.step = 1, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    final color = isDark ? Colors.white.withOpacity(0.06) : Colors.black.withOpacity(0.04);
    final borderColor = isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1);

    return Container(
      decoration: BoxDecoration(color: color, borderRadius: BorderRadius.circular(6), border: Border.all(color: borderColor, width: 0.5)),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Padding(padding: const EdgeInsets.symmetric(horizontal: 10), child: Text('$value', style: const TextStyle(fontSize: 13))),
          Container(width: 0.5, height: 20, color: borderColor),
          Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              _StepperButton(icon: CupertinoIcons.chevron_up, onTap: () => onChanged(value + step)),
              Container(width: 20, height: 0.5, color: borderColor),
              _StepperButton(icon: CupertinoIcons.chevron_down, onTap: () => onChanged(value - step > 0 ? value - step : 0)),
            ],
          ),
        ],
      ),
    );
  }
}

class _StepperButton extends StatelessWidget {
  final IconData icon;
  final VoidCallback onTap;
  const _StepperButton({required this.icon, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 24, height: 12,
        color: Colors.transparent,
        child: Icon(icon, size: 8, color: isDark ? Colors.white54 : Colors.black54),
      ),
    );
  }
}