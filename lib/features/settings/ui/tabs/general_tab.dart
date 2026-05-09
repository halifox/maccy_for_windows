import 'dart:io';

import 'package:maccy/core/managers/clipboard_manager_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'package:maccy/core/models/hotkey_config.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';

/// 设置：常规选项页。
///
/// 包含基础的应用配置，如开机自启、自动更新、全局快捷键定义、搜索模式选择以及
/// 关键的“自动粘贴”行为开关。
class GeneralTab extends ConsumerWidget {
  const GeneralTab({super.key});

  /// 获取搜索模式的描述文本。
  ///
  /// [mode] 搜索模式标识符。
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
                  value: ref.watch(launchAtStartupProvider),
                  onChanged: (v) => ref
                      .read(launchAtStartupProvider.notifier)
                      .set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Auto-check for updates',
                subtitle: 'Keep application up to date',
                icon: CupertinoIcons.arrow_2_circlepath,
                iconColor: CupertinoColors.systemGreen,
                trailing: CupertinoCheckbox(
                  value: ref.watch(autoCheckUpdatesProvider),
                  onChanged: (v) => ref
                      .read(autoCheckUpdatesProvider.notifier)
                      .set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Check for Updates',
                icon: CupertinoIcons.refresh,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoButton(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 12,
                    vertical: 4,
                  ),
                  color: isDark
                      ? Colors.white.withValues(alpha: 0.1)
                      : Colors.black.withValues(alpha: 0.05),
                  borderRadius: BorderRadius.circular(6),
                  onPressed: () {},
                  minimumSize: Size.zero,
                  child: Text(
                    'Check Now',
                    style: TextStyle(
                      fontSize: 12,
                      color: isDark ? Colors.white : Colors.black87,
                    ),
                  ),
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
                trailing: _HotkeySelector(),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Search',
            children: [
              MacosSettingsTile(
                label: 'Search Mode',
                subtitle: _getSearchModeSubtitle(ref.watch(searchModeProvider)),
                icon: CupertinoIcons.search,
                iconColor: CupertinoColors.systemBlue,
                trailing: _SearchModeMenu(),
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
                        future: ref
                            .read(appClipboardManagerProvider.notifier)
                            .checkAccessibilityPermissions(),
                        builder: (context, snapshot) {
                          if (snapshot.data == false) {
                            return Padding(
                              padding: const EdgeInsets.only(right: 8.0),
                              child: CupertinoButton(
                                padding: const EdgeInsets.symmetric(
                                  horizontal: 8,
                                  vertical: 2,
                                ),
                                color: CupertinoColors.systemRed.withValues(
                                  alpha: 0.1,
                                ),
                                borderRadius: BorderRadius.circular(4),
                                onPressed: () => ref
                                    .read(appClipboardManagerProvider.notifier)
                                    .requestAccessibilityPermissions(),
                                minimumSize: Size.zero,
                                child: const Text(
                                  'Grant Access',
                                  style: TextStyle(
                                    fontSize: 10,
                                    color: CupertinoColors.systemRed,
                                  ),
                                ),
                              ),
                            );
                          }
                          return const SizedBox.shrink();
                        },
                      ),
                    CupertinoCheckbox(
                      value: ref.watch(autoPasteProvider),
                      onChanged: (v) =>
                          ref.read(autoPasteProvider.notifier).set(v ?? false),
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
                  value: ref.watch(pastePlainProvider),
                  onChanged: (v) =>
                      ref.read(pastePlainProvider.notifier).set(v ?? false),
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }
}

/// 快捷键选择器组件。
///
/// 允许用户通过勾选修饰键（Ctrl, Alt, Shift, Win/Cmd）并选择主键位来实时更新唤起热键。
class _HotkeySelector extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final config = ref.watch(hotkeyOpenProvider);
    final isDark = Theme.of(context).brightness == Brightness.dark;

    /// 更新热键配置并同步至持久化存储。
    void update(List<String> newMods, String newKey) {
      ref
          .read(hotkeyOpenProvider.notifier)
          .set(AppHotKeyConfig(modifiers: newMods, key: newKey));
    }

    /// 构建单个修饰键的复选框行。
    Widget buildModifier(String label, String value) {
      final selected = config.modifiers.contains(value);
      return Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          const SizedBox(width: 4),
          CupertinoCheckbox(
            value: selected,
            onChanged: (v) {
              final next = List<String>.from(config.modifiers);
              if (v ?? false) {
                if (!next.contains(value)) next.add(value);
              } else {
                next.remove(value);
              }
              update(next, config.key);
            },
          ),
          const SizedBox(width: 4),
          Text(
            label,
            style: TextStyle(
              fontSize: 12,
              color: isDark ? Colors.white70 : Colors.black87,
            ),
          ),
          const SizedBox(width: 8),
        ],
      );
    }

    final isMac = Platform.isMacOS;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            buildModifier(isMac ? '⌘' : 'Win', 'meta'),
            buildModifier(isMac ? '⌥' : 'Alt', 'alt'),
          ],
        ),
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            buildModifier(isMac ? '⌃' : 'Ctrl', 'control'),
            buildModifier(isMac ? '⇧' : 'Shift', 'shift'),
          ],
        ),
        const SizedBox(width: 8),
        _KeyPicker(
          value: config.key,
          onChanged: (v) => update(config.modifiers, v),
        ),
      ],
    );
  }
}

/// 搜索模式下拉菜单。
class _SearchModeMenu extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final mode = ref.watch(searchModeProvider);

    return MacosPopupMenu<String>(
      value: mode[0].toUpperCase() + mode.substring(1),
      items: const ['exact', 'fuzzy', 'regex', 'mixed'],
      itemLabelBuilder: (v) => v[0].toUpperCase() + v.substring(1),
      onSelected: (v) => ref.read(searchModeProvider.notifier).set(v),
      selectedItemBuilder: (v) => v == mode,
    );
  }
}

/// 通用的 macOS 风格弹出菜单按钮。
///
/// 字段说明:
/// [value] 当前选中的展示文字。
/// [items] 菜单项列表。
/// [itemLabelBuilder] 菜单项展示文本的构建器。
/// [onSelected] 选中回调。
/// [selectedItemBuilder] 用于判断某个项是否处于选中状态的逻辑。
class MacosPopupMenu<T> extends StatelessWidget {

  const MacosPopupMenu({
    super.key,
    required this.value,
    required this.items,
    required this.itemLabelBuilder,
    required this.onSelected,
    this.selectedItemBuilder,
  });
  final String value;
  final List<T> items;
  final String Function(T) itemLabelBuilder;
  final ValueChanged<T> onSelected;
  final bool Function(T)? selectedItemBuilder;

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

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
                Text(
                  value,
                  style: TextStyle(
                    fontSize: 13,
                    color: isDark
                        ? Colors.white.withValues(alpha: 0.9)
                        : Colors.black87,
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
      menuChildren: items.map((item) {
        final selected = selectedItemBuilder?.call(item) ?? false;
        return MenuItemButton(
          onPressed: () => onSelected(item),
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
                child: selected
                    ? const Icon(CupertinoIcons.checkmark, size: 14)
                    : null,
              ),
              Text(
                itemLabelBuilder(item),
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
}

/// 键位选择器（用于热键设置）。
class _KeyPicker extends StatelessWidget {

  const _KeyPicker({required this.value, required this.onChanged});
  final String value;
  final ValueChanged<String> onChanged;

  @override
  Widget build(BuildContext context) {
    final keys = [
      ...List.generate(26, (i) => String.fromCharCode(65 + i)),
      ...List.generate(10, (i) => i.toString()),
    ];

    return MacosPopupMenu<String>(
      value: value,
      items: keys,
      itemLabelBuilder: (v) => v,
      onSelected: onChanged,
      selectedItemBuilder: (v) => v == value,
    );
  }
}
