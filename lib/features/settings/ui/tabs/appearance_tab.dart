import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';
import 'package:maccy/features/settings/ui/widgets/number_stepper.dart';

/// 设置：外观选项页。
///
/// 负责管理面板弹出位置、界面元素的可见性（如菜单栏图标、页脚菜单）以及
/// 搜索匹配的高亮样式等视觉参数。
class AppearanceTab extends ConsumerWidget {
  const AppearanceTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        children: [
          MacosSettingsGroup(
            title: 'Panel Configuration',
            children: [
              _buildDropdownTile(
                context,
                ref,
                label: 'Popup Position',
                subtitle: 'Where the clipboard history appears',
                icon: CupertinoIcons.cursor_rays,
                iconColor: CupertinoColors.activeBlue,
                currentValue: ref.watch(popupPositionProvider),
                options: {'cursor': 'Cursor', 'center': 'Center'},
                onSelected: (v) =>
                    ref.read(popupPositionProvider.notifier).set(v),
              ),
              _buildDropdownTile(
                context,
                ref,
                label: 'Pinned Position',
                subtitle: 'Where pinned items are displayed',
                icon: CupertinoIcons.pin,
                iconColor: CupertinoColors.systemOrange,
                currentValue: ref.watch(pinPositionProvider),
                options: {'top': 'Top', 'bottom': 'Bottom'},
                onSelected: (v) =>
                    ref.read(pinPositionProvider.notifier).set(v),
              ),
              MacosSettingsTile(
                label: 'Image Height',
                subtitle: 'Height of image previews in history',
                icon: CupertinoIcons.photo,
                iconColor: CupertinoColors.systemPink,
                trailing: NumberStepper(
                  value: ref.watch(imageHeightProvider),
                  onChanged: (v) =>
                      ref.read(imageHeightProvider.notifier).set(v),
                ),
              ),
              MacosSettingsTile(
                label: 'Preview Delay',
                subtitle: 'Delay before showing content preview (ms)',
                icon: CupertinoIcons.timer,
                iconColor: CupertinoColors.systemTeal,
                trailing: NumberStepper(
                  value: ref.watch(previewDelayProvider),
                  step: 100,
                  onChanged: (v) =>
                      ref.read(previewDelayProvider.notifier).set(v),
                ),
              ),
              _buildDropdownTile(
                context,
                ref,
                label: 'Highlight Match',
                subtitle: 'Visual style for search results',
                icon: CupertinoIcons.selection_pin_in_out,
                iconColor: CupertinoColors.systemYellow,
                currentValue: ref.watch(highlightMatchProvider),
                options: {'bold': 'Bold', 'color': 'Color'},
                onSelected: (v) =>
                    ref.read(highlightMatchProvider.notifier).set(v),
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
                  value: ref.watch(showSpecialCharsProvider),
                  onChanged: (v) => ref
                      .read(showSpecialCharsProvider.notifier)
                      .set(v ?? false),
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
                    _buildIconDropdown(
                      context,
                      ref,
                      ref.watch(menuBarIconTypeProvider),
                      (v) {
                        ref.read(menuBarIconTypeProvider.notifier).set(v);
                      },
                    ),
                    const SizedBox(width: 12),
                    CupertinoCheckbox(
                      value: ref.watch(showMenuBarIconProvider),
                      onChanged: (v) => ref
                          .read(showMenuBarIconProvider.notifier)
                          .set(v ?? false),
                    ),
                  ],
                ),
              ),
              MacosSettingsTile(
                label: 'Show Recent Copy in Menu Bar',
                subtitle: 'Display preview of last copied item',
                icon: CupertinoIcons.text_badge_star,
                iconColor: CupertinoColors.systemPurple,
                trailing: CupertinoCheckbox(
                  value: ref.watch(showRecentCopyInMenuBarProvider),
                  onChanged: (v) => ref
                      .read(showRecentCopyInMenuBarProvider.notifier)
                      .set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Search Field',
                subtitle: 'When to display the search input',
                icon: CupertinoIcons.search,
                iconColor: CupertinoColors.systemBlue,
                trailing: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    _buildSearchVisibilityDropdown(context, ref),
                    const SizedBox(width: 12),
                    CupertinoCheckbox(
                      value: ref.watch(showSearchProvider) != 'never',
                      onChanged: (v) => ref
                          .read(showSearchProvider.notifier)
                          .set((v ?? false) ? 'always' : 'never'),
                    ),
                  ],
                ),
              ),
              MacosSettingsTile(
                label: 'Show Title Before Search Field',
                subtitle: 'Display application name above search box',
                icon: CupertinoIcons.textformat,
                iconColor: CupertinoColors.systemIndigo,
                trailing: CupertinoCheckbox(
                  value: ref.watch(showTitleProvider),
                  onChanged: (v) =>
                      ref.read(showTitleProvider.notifier).set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Application Icons',
                subtitle: 'Show source app icons in history',
                icon: CupertinoIcons.app_badge,
                iconColor: CupertinoColors.systemIndigo,
                trailing: CupertinoCheckbox(
                  value: ref.watch(showAppIconProvider),
                  onChanged: (v) =>
                      ref.read(showAppIconProvider.notifier).set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Footer Menu',
                subtitle: 'Show action menu at the bottom',
                icon: CupertinoIcons.list_bullet_below_rectangle,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoCheckbox(
                  value: ref.watch(showFooterMenuProvider),
                  onChanged: (v) =>
                      ref.read(showFooterMenuProvider.notifier).set(v ?? false),
                ),
              ),
            ],
          ),
          // Footer warning
          if (!ref.watch(showFooterMenuProvider))
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 8, 16, 0),
              child: Text(
                'You can still open preferences from the menu bar icon.',
                style: TextStyle(
                  fontSize: 12,
                  color: isDark ? Colors.white38 : Colors.black38,
                ),
              ),
            ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }

  /// 构建搜索可见性下拉菜单。
  Widget _buildSearchVisibilityDropdown(BuildContext context, WidgetRef ref) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final searchEnabled = ref.watch(showSearchProvider) != 'never';
    final visibility = ref.watch(searchVisibilityProvider);

    return MenuAnchor(
      alignmentOffset: const Offset(0, 4),
      style: MenuStyle(
        backgroundColor: WidgetStatePropertyAll(
          isDark ? const Color(0xFF2D2D2D) : const Color(0xFFF2F2F2),
        ),
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(10),
            side: BorderSide(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.15)
                  : Colors.black.withValues(alpha: 0.1),
              width: 0.5,
            ),
          ),
        ),
        elevation: const WidgetStatePropertyAll(16),
        shadowColor: WidgetStatePropertyAll(
          Colors.black.withValues(alpha: isDark ? 0.5 : 0.2),
        ),
        padding: const WidgetStatePropertyAll(EdgeInsets.all(6)),
      ),
      builder: (context, controller, child) {
        return CupertinoButton(
          padding: EdgeInsets.zero,
          onPressed: searchEnabled
              ? () => controller.isOpen ? controller.close() : controller.open()
              : null,
          minimumSize: Size.zero,
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: isDark
                  ? Colors.white.withValues(alpha: searchEnabled ? 0.06 : 0.03)
                  : Colors.black.withValues(alpha: searchEnabled ? 0.04 : 0.02),
              borderRadius: BorderRadius.circular(6),
              border: Border.all(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.1)
                    : Colors.black.withValues(alpha: 0.1),
                width: 0.5,
              ),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  visibility == 'always'
                      ? 'Always'
                      : visibility == 'onType'
                          ? 'When Typing'
                          : 'Never',
                  style: TextStyle(
                    fontSize: 13,
                    color: isDark
                        ? Colors.white.withValues(alpha: searchEnabled ? 0.9 : 0.5)
                        : Colors.black.withValues(alpha: searchEnabled ? 0.87 : 0.5),
                  ),
                ),
                const SizedBox(width: 8),
                Icon(
                  CupertinoIcons.chevron_up_chevron_down,
                  size: 10,
                  color: isDark
                      ? Colors.white.withValues(alpha: searchEnabled ? 0.38 : 0.2)
                      : Colors.black.withValues(alpha: searchEnabled ? 0.38 : 0.2),
                ),
              ],
            ),
          ),
        );
      },
      menuChildren: ['always', 'onType', 'never'].map((mode) {
        return MenuItemButton(
          onPressed: () => ref.read(searchVisibilityProvider.notifier).set(mode),
          style: ButtonStyle(
            padding: const WidgetStatePropertyAll(
              EdgeInsets.symmetric(horizontal: 4),
            ),
            minimumSize: const WidgetStatePropertyAll(Size(120, 26)),
            fixedSize: const WidgetStatePropertyAll(Size.fromHeight(26)),
            overlayColor: WidgetStatePropertyAll(
              CupertinoColors.activeBlue.withValues(alpha: 0.9),
            ),
            foregroundColor: WidgetStateProperty.resolveWith(
              (states) =>
                  states.contains(WidgetState.hovered) ||
                      states.contains(WidgetState.pressed)
                  ? Colors.white
                  : (isDark ? Colors.white.withValues(alpha: 0.9) : Colors.black87),
            ),
            shape: WidgetStatePropertyAll(
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(5)),
            ),
          ),
          child: Row(
            children: [
              SizedBox(
                width: 20,
                child: visibility == mode
                    ? const Icon(CupertinoIcons.checkmark, size: 14)
                    : null,
              ),
              Text(
                mode == 'always'
                    ? 'Always'
                    : mode == 'onType'
                        ? 'When Typing'
                        : 'Never',
                style: const TextStyle(
                  fontSize: 13,
                  fontWeight: FontWeight.w400,
                ),
              ),
            ],
          ),
        );
      }).toList(),
    );
  }

  /// 构建带有下拉菜单选择器的设置行。
  Widget _buildDropdownTile(
    BuildContext context,
    WidgetRef ref, {
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
          backgroundColor: WidgetStatePropertyAll(
            isDark ? const Color(0xFF2D2D2D) : const Color(0xFFF2F2F2),
          ),
          shape: WidgetStatePropertyAll(
            RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(10),
              side: BorderSide(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.15)
                    : Colors.black.withValues(alpha: 0.1),
                width: 0.5,
              ),
            ),
          ),
          elevation: const WidgetStatePropertyAll(16),
          shadowColor: WidgetStatePropertyAll(
            Colors.black.withValues(alpha: isDark ? 0.5 : 0.2),
          ),
          padding: const WidgetStatePropertyAll(EdgeInsets.all(6)),
        ),
        builder: (context, controller, child) {
          return CupertinoButton(
            padding: EdgeInsets.zero,
            onPressed: () =>
                controller.isOpen ? controller.close() : controller.open(),
            minimumSize: Size.zero,
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              decoration: BoxDecoration(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.06)
                    : Colors.black.withValues(alpha: 0.04),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: isDark
                      ? Colors.white.withValues(alpha: 0.1)
                      : Colors.black.withValues(alpha: 0.1),
                  width: 0.5,
                ),
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    options[currentValue] ?? currentValue,
                    style: TextStyle(
                      fontSize: 13,
                      color: isDark
                          ? Colors.white.withValues(alpha: 0.9)
                          : Colors.black87,
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
        menuChildren: options.entries
            .map(
              (e) => _buildMenuItem(
                context,
                e.key,
                e.value,
                currentValue == e.key,
                onSelected,
              ),
            )
            .toList(),
      ),
    );
  }

  /// 构建下拉菜单项。
  Widget _buildMenuItem(
    BuildContext context,
    String value,
    String text,
    bool selected,
    ValueChanged<String> onSelected,
  ) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return MenuItemButton(
      onPressed: () => onSelected(value),
      style: ButtonStyle(
        padding: const WidgetStatePropertyAll(
          EdgeInsets.symmetric(horizontal: 4),
        ),
        minimumSize: const WidgetStatePropertyAll(Size(100, 26)),
        fixedSize: const WidgetStatePropertyAll(Size.fromHeight(26)),
        overlayColor: WidgetStatePropertyAll(
          CupertinoColors.activeBlue.withValues(alpha: 0.9),
        ),
        foregroundColor: WidgetStateProperty.resolveWith(
          (states) =>
              states.contains(WidgetState.hovered) ||
                  states.contains(WidgetState.pressed)
              ? Colors.white
              : (isDark ? Colors.white.withValues(alpha: 0.9) : Colors.black87),
        ),
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(borderRadius: BorderRadius.circular(5)),
        ),
      ),
      child: Row(
        children: [
          SizedBox(
            width: 20,
            child: selected
                ? const Icon(CupertinoIcons.checkmark, size: 14)
                : null,
          ),
          Text(
            text,
            style: const TextStyle(fontSize: 13, fontWeight: FontWeight.w400),
          ),
        ],
      ),
    );
  }

  /// 构建托盘图标类型的专用下拉选择器。
  Widget _buildIconDropdown(
    BuildContext context,
    WidgetRef ref,
    String current,
    ValueChanged<String> onSelected,
  ) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    final icons = {
      'clipboard': CupertinoIcons.doc_on_clipboard,
      'star': CupertinoIcons.star,
      'bell': CupertinoIcons.bell,
    };

    return MenuAnchor(
      alignmentOffset: const Offset(0, 4),
      style: MenuStyle(
        backgroundColor: WidgetStatePropertyAll(
          isDark ? const Color(0xFF2D2D2D) : const Color(0xFFF2F2F2),
        ),
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(10),
            side: BorderSide(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.15)
                  : Colors.black.withValues(alpha: 0.1),
              width: 0.5,
            ),
          ),
        ),
        elevation: const WidgetStatePropertyAll(16),
        shadowColor: WidgetStatePropertyAll(
          Colors.black.withValues(alpha: isDark ? 0.5 : 0.2),
        ),
        padding: const WidgetStatePropertyAll(EdgeInsets.all(6)),
      ),
      builder: (context, controller, child) {
        return CupertinoButton(
          padding: EdgeInsets.zero,
          onPressed: () =>
              controller.isOpen ? controller.close() : controller.open(),
          minimumSize: Size.zero,
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.06)
                  : Colors.black.withValues(alpha: 0.04),
              borderRadius: BorderRadius.circular(6),
              border: Border.all(
                color: isDark
                    ? Colors.white.withValues(alpha: 0.1)
                    : Colors.black.withValues(alpha: 0.1),
                width: 0.5,
              ),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  icons[current] ?? CupertinoIcons.doc_on_clipboard,
                  size: 14,
                  color: isDark
                      ? Colors.white.withValues(alpha: 0.9)
                      : Colors.black87,
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
      menuChildren: icons.entries.map((e) {
        return MenuItemButton(
          onPressed: () => onSelected(e.key),
          style: ButtonStyle(
            padding: const WidgetStatePropertyAll(
              EdgeInsets.symmetric(horizontal: 4),
            ),
            minimumSize: const WidgetStatePropertyAll(Size(80, 26)),
            fixedSize: const WidgetStatePropertyAll(Size.fromHeight(26)),
            overlayColor: WidgetStatePropertyAll(
              CupertinoColors.activeBlue.withValues(alpha: 0.9),
            ),
            foregroundColor: WidgetStateProperty.resolveWith(
              (states) =>
                  states.contains(WidgetState.hovered) ||
                      states.contains(WidgetState.pressed)
                  ? Colors.white
                  : (isDark ? Colors.white.withValues(alpha: 0.9) : Colors.black87),
            ),
            shape: WidgetStatePropertyAll(
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(5)),
            ),
          ),
          child: Row(
            children: [
              SizedBox(
                width: 20,
                child: current == e.key
                    ? const Icon(CupertinoIcons.checkmark, size: 14)
                    : null,
              ),
              Icon(e.value, size: 14),
              const SizedBox(width: 8),
              Text(
                e.key[0].toUpperCase() + e.key.substring(1),
                style: const TextStyle(fontSize: 13),
              ),
            ],
          ),
        );
      }).toList(),
    );
  }
}
