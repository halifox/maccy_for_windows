import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';

/// 设置：高级选项页。
///
/// 包含剪贴板检查间隔、录制状态控制（暂停/恢复）以及隐私清理相关的逻辑配置。
class AdvancedTab extends ConsumerWidget {
  const AdvancedTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        children: [
          MacosSettingsGroup(
            title: 'Performance',
            children: [
              MacosSettingsTile(
                label: 'Clipboard Check Interval',
                subtitle: 'How often to check for clipboard changes (seconds)',
                icon: CupertinoIcons.timer,
                iconColor: CupertinoColors.systemBlue,
                trailing: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(
                        horizontal: 12,
                        vertical: 6,
                      ),
                      decoration: BoxDecoration(
                        color: isDark
                            ? Colors.white.withValues(alpha: 0.05)
                            : Colors.black.withValues(alpha: 0.03),
                        borderRadius: BorderRadius.circular(6),
                        border: Border.all(
                          color: isDark
                              ? Colors.white.withValues(alpha: 0.1)
                              : Colors.black.withValues(alpha: 0.1),
                        ),
                      ),
                      child: Text(
                        '${ref.watch(clipboardCheckIntervalProvider).toStringAsFixed(1)}s',
                        style: TextStyle(
                          fontSize: 13,
                          color: isDark ? Colors.white.withValues(alpha: 0.87) : Colors.black.withValues(alpha: 0.87),
                        ),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        CupertinoButton(
                          padding: EdgeInsets.zero,
                          onPressed: () {
                            final current = ref.read(clipboardCheckIntervalProvider);
                            if (current < 5.0) {
                              ref
                                  .read(clipboardCheckIntervalProvider.notifier)
                                  .set((current + 0.1).clamp(0.1, 5.0));
                            }
                          }, minimumSize: const Size(20, 20),
                          child: Icon(
                            CupertinoIcons.chevron_up,
                            size: 12,
                            color: isDark ? Colors.white54 : Colors.black54,
                          ),
                        ),
                        CupertinoButton(
                          padding: EdgeInsets.zero,
                          onPressed: () {
                            final current = ref.read(clipboardCheckIntervalProvider);
                            if (current > 0.1) {
                              ref
                                  .read(clipboardCheckIntervalProvider.notifier)
                                  .set((current - 0.1).clamp(0.1, 5.0));
                            }
                          }, minimumSize: const Size(20, 20),
                          child: Icon(
                            CupertinoIcons.chevron_down,
                            size: 12,
                            color: isDark ? Colors.white54 : Colors.black54,
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: 'Recording Control',
            children: [
              MacosSettingsTile(
                label: 'Turn Off Clipboard Monitoring',
                subtitle: 'Stop recording clipboard changes',
                icon: CupertinoIcons.pause_circle,
                iconColor: CupertinoColors.systemOrange,
                trailing: CupertinoCheckbox(
                  value: ref.watch(ignoreEventsProvider),
                  onChanged: (v) =>
                      ref.read(ignoreEventsProvider.notifier).set(v ?? false),
                ),
              ),
              MacosSettingsTile(
                label: 'Ignore Only Next Event',
                subtitle: 'Skip only the next clipboard change',
                icon: CupertinoIcons.forward,
                iconColor: CupertinoColors.systemGrey,
                trailing: CupertinoCheckbox(
                  value: ref.watch(ignoreOnlyNextEventProvider),
                  onChanged: (v) => ref
                      .read(ignoreOnlyNextEventProvider.notifier)
                      .set(v ?? false),
                ),
              ),
            ],
          ),
          // Turn off descriptions
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 8, 16, 16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Maccy will stop recording clipboard changes until you turn it back on.',
                  style: TextStyle(
                    fontSize: 12,
                    color: isDark ? Colors.white38 : Colors.black38,
                  ),
                ),
                const SizedBox(height: 8),
                Text(
                  'You can also pause/resume from the menu bar icon.',
                  style: TextStyle(
                    fontSize: 12,
                    color: isDark ? Colors.white38 : Colors.black38,
                  ),
                ),
                const SizedBox(height: 12),
                Text(
                  'Shell script examples:',
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                    color: isDark ? Colors.white54 : Colors.black54,
                  ),
                ),
                const SizedBox(height: 4),
                Container(
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: isDark
                        ? Colors.white.withValues(alpha: 0.05)
                        : Colors.black.withValues(alpha: 0.03),
                    borderRadius: BorderRadius.circular(4),
                  ),
                  child: DefaultTextStyle(
                    style: TextStyle(
                      fontSize: 11,
                      fontFamily: 'Courier New',
                      color: isDark ? Colors.white70 : Colors.black87,
                    ),
                    child: const Text(
                      '# Pause monitoring\n'
                      r'reg add "HKCU\Software\Maccy" /v ignoreEvents /t REG_DWORD /d 1 /f' '\n\n'
                      '# Resume monitoring\n'
                      r'reg add "HKCU\Software\Maccy" /v ignoreEvents /t REG_DWORD /d 0 /f' '\n\n'
                      '# Ignore only next event\n'
                      r'reg add "HKCU\Software\Maccy" /v ignoreOnlyNextEvent /t REG_DWORD /d 1 /f',
                    ),
                  ),
                ),
              ],
            ),
          ),
          MacosSettingsGroup(
            title: 'Privacy & Cleanup',
            children: [
              MacosSettingsTile(
                label: 'Clear History on Exit',
                subtitle: 'Delete all clipboard history when quitting',
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
                icon: CupertinoIcons.clear_circled,
                iconColor: CupertinoColors.systemOrange,
                trailing: CupertinoCheckbox(
                  value: ref.watch(clearSystemClipboardProvider),
                  onChanged: (v) => ref
                      .read(clearSystemClipboardProvider.notifier)
                      .set(v ?? false),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Text(
              'Note: Lower check intervals provide faster clipboard detection but use more system resources.',
              style: TextStyle(
                fontSize: 12,
                color: isDark ? Colors.white38 : Colors.black38,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
