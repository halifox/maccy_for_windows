import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/widgets/macos_settings_widgets.dart';

/// 设置：忽略列表选项卡。
///
/// 用于配置剪贴板监控的黑名单，此处设置的应用程序或内容模式将不会被 Maccy 捕获。
/// 包含三个子标签：应用程序过滤、剪贴板类型过滤、正则表达式过滤。
class IgnoreTab extends HookConsumerWidget {
  const IgnoreTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final selectedTab = useState(0);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      mainAxisSize: MainAxisSize.min, 
      children: [
        // Tab 切换器
        Container(
          margin: const EdgeInsets.only(bottom: 16),
          decoration: BoxDecoration(
            color: isDark
                ? Colors.white.withValues(alpha: 0.05)
                : Colors.black.withValues(alpha: 0.03),
            borderRadius: BorderRadius.circular(8),
          ),
          child: CupertinoSlidingSegmentedControl<int>(
            groupValue: selectedTab.value,
            onValueChanged: (value) => selectedTab.value = value ?? 0,
            children: const {
              0: Padding(
                padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                child: Text('Applications', style: TextStyle(fontSize: 13)),
              ),
              1: Padding(
                padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                child: Text('Types', style: TextStyle(fontSize: 13)),
              ),
              2: Padding(
                padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                child: Text('Regex', style: TextStyle(fontSize: 13)),
              ),
            },
          ),
        ),
        // Tab 内容
        switch (selectedTab.value) {
          0 => const _ApplicationsTab(),
          1 => const _PasteboardTypesTab(),
          2 => const _RegexTab(),
          _ => const SizedBox.shrink(),
        },
      ],
    );
  }
}

/// 应用程序过滤子标签
class _ApplicationsTab extends HookConsumerWidget {
  const _ApplicationsTab();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final apps = ref.watch(ignoredAppsProvider);
    final isWhitelistMode = ref.watch(ignoreAllAppsExceptListedProvider);
    final textController = useTextEditingController();
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          MacosSettingsGroup(
            title: 'Mode',
            children: [
              MacosSettingsTile(
                label: 'Blacklist Mode',
                subtitle: 'Ignore clipboard from listed applications',
                icon: CupertinoIcons.xmark_circle,
                iconColor: CupertinoColors.systemRed,
                trailing: CupertinoCheckbox(
                  value: !isWhitelistMode,
                  onChanged: (v) => ref
                      .read(ignoreAllAppsExceptListedProvider.notifier)
                      .set(!(v ?? true)),
                ),
              ),
              MacosSettingsTile(
                label: 'Whitelist Mode',
                subtitle: 'Only capture clipboard from listed applications',
                icon: CupertinoIcons.checkmark_circle,
                iconColor: CupertinoColors.systemGreen,
                trailing: CupertinoCheckbox(
                  value: isWhitelistMode,
                  onChanged: (v) => ref
                      .read(ignoreAllAppsExceptListedProvider.notifier)
                      .set(v ?? false),
                ),
              ),
            ],
          ),
          MacosSettingsGroup(
            title: isWhitelistMode
                ? 'Allowed Applications (${apps.length})'
                : 'Ignored Applications (${apps.length})',
            children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    // 添加应用输入框
                    Row(
                      children: [
                        Expanded(
                          child: CupertinoTextField(
                            controller: textController,
                            placeholder: 'Application name or bundle ID',
                            padding: const EdgeInsets.symmetric(
                              horizontal: 12,
                              vertical: 8,
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
                          ),
                        ),
                        const SizedBox(width: 8),
                        CupertinoButton(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 16,
                            vertical: 8,
                          ),
                          color: CupertinoColors.activeBlue,
                          borderRadius: BorderRadius.circular(6),
                          onPressed: () {
                            final text = textController.text.trim();
                            if (text.isNotEmpty && !apps.contains(text)) {
                              ref
                                  .read(ignoredAppsProvider.notifier)
                                  .set([...apps, text]);
                              textController.clear();
                            }
                          },
                          child: const Text(
                            'Add',
                            style: TextStyle(fontSize: 13),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    // 应用列表
                    if (apps.isEmpty)
                      Center(
                        child: Padding(
                          padding: const EdgeInsets.all(32),
                          child: Column(
                            children: [
                              Icon(
                                CupertinoIcons.app,
                                size: 48,
                                color: isDark
                                    ? Colors.white24
                                    : Colors.black26,
                              ),
                              const SizedBox(height: 12),
                              Text(
                                isWhitelistMode
                                    ? 'All applications allowed'
                                    : 'No applications ignored',
                                style: TextStyle(
                                  color: isDark
                                      ? Colors.white38
                                      : Colors.black38,
                                ),
                              ),
                            ],
                          ),
                        ),
                      )
                    else
                      ...apps.map((app) => _ListItem(
                            text: app,
                            onDelete: () {
                              final updated = apps.where((a) => a != app).toList();
                              ref.read(ignoredAppsProvider.notifier).set(updated);
                            },
                          )),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Text(
              'Examples: chrome.exe, notepad.exe, com.google.Chrome',
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

/// 剪贴板类型过滤子标签
class _PasteboardTypesTab extends HookConsumerWidget {
  const _PasteboardTypesTab();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final types = ref.watch(ignoredPasteboardTypesProvider);
    final textController = useTextEditingController();
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          MacosSettingsGroup(
            title: 'Ignored Pasteboard Types (${types.length})',
            children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Text(
                      'Prevent capturing clipboard data from password managers and other sensitive applications.',
                      style: TextStyle(
                        fontSize: 12,
                        color: isDark ? Colors.white54 : Colors.black54,
                      ),
                    ),
                    const SizedBox(height: 16),
                    // 添加类型输入框
                    Row(
                      children: [
                        Expanded(
                          child: CupertinoTextField(
                            controller: textController,
                            placeholder: 'Pasteboard type identifier',
                            padding: const EdgeInsets.symmetric(
                              horizontal: 12,
                              vertical: 8,
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
                          ),
                        ),
                        const SizedBox(width: 8),
                        CupertinoButton(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 16,
                            vertical: 8,
                          ),
                          color: CupertinoColors.activeBlue,
                          borderRadius: BorderRadius.circular(6),
                          onPressed: () {
                            final text = textController.text.trim();
                            if (text.isNotEmpty && !types.contains(text)) {
                              ref
                                  .read(ignoredPasteboardTypesProvider.notifier)
                                  .set([...types, text]);
                              textController.clear();
                            }
                          },
                          child: const Text(
                            'Add',
                            style: TextStyle(fontSize: 13),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    // 类型列表
                    if (types.isEmpty)
                      Center(
                        child: Padding(
                          padding: const EdgeInsets.all(32),
                          child: Column(
                            children: [
                              Icon(
                                CupertinoIcons.doc_text,
                                size: 48,
                                color: isDark
                                    ? Colors.white24
                                    : Colors.black26,
                              ),
                              const SizedBox(height: 12),
                              Text(
                                'No types ignored',
                                style: TextStyle(
                                  color: isDark
                                      ? Colors.white38
                                      : Colors.black38,
                                ),
                              ),
                            ],
                          ),
                        ),
                      )
                    else
                      ...types.map((type) => _ListItem(
                            text: type,
                            onDelete: () {
                              final updated = types.where((t) => t != type).toList();
                              ref
                                  .read(ignoredPasteboardTypesProvider.notifier)
                                  .set(updated);
                            },
                          )),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Text(
              'Default ignored types include password managers like 1Password, KeeWeb, etc.',
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

/// 正则表达式过滤子标签
class _RegexTab extends HookConsumerWidget {
  const _RegexTab();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final patterns = ref.watch(ignoreRegexpProvider);
    final textController = useTextEditingController();
    final errorText = useState<String?>(null);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          MacosSettingsGroup(
            title: 'Regular Expression Patterns (${patterns.length})',
            children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Text(
                      'Ignore clipboard content matching these regular expressions.',
                      style: TextStyle(
                        fontSize: 12,
                        color: isDark ? Colors.white54 : Colors.black54,
                      ),
                    ),
                    const SizedBox(height: 16),
                    // 添加正则输入框
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Expanded(
                              child: CupertinoTextField(
                                controller: textController,
                                placeholder: 'Regular expression pattern',
                                padding: const EdgeInsets.symmetric(
                                  horizontal: 12,
                                  vertical: 8,
                                ),
                                decoration: BoxDecoration(
                                  color: isDark
                                      ? Colors.white.withValues(alpha: 0.05)
                                      : Colors.black.withValues(alpha: 0.03),
                                  borderRadius: BorderRadius.circular(6),
                                  border: Border.all(
                                    color: errorText.value != null
                                        ? CupertinoColors.systemRed
                                        : (isDark
                                            ? Colors.white.withValues(alpha: 0.1)
                                            : Colors.black.withValues(alpha: 0.1)),
                                  ),
                                ),
                                onChanged: (_) => errorText.value = null,
                              ),
                            ),
                            const SizedBox(width: 8),
                            CupertinoButton(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 16,
                                vertical: 8,
                              ),
                              color: CupertinoColors.activeBlue,
                              borderRadius: BorderRadius.circular(6),
                              onPressed: () {
                                final text = textController.text.trim();
                                if (text.isEmpty) return;

                                // 验证正则表达式
                                try {
                                  RegExp(text);
                                  if (!patterns.contains(text)) {
                                    ref
                                        .read(ignoreRegexpProvider.notifier)
                                        .set([...patterns, text]);
                                    textController.clear();
                                    errorText.value = null;
                                  }
                                } catch (e) {
                                  errorText.value = 'Invalid regex pattern';
                                }
                              },
                              child: const Text(
                                'Add',
                                style: TextStyle(fontSize: 13),
                              ),
                            ),
                          ],
                        ),
                        if (errorText.value != null)
                          Padding(
                            padding: const EdgeInsets.only(top: 4, left: 12),
                            child: Text(
                              errorText.value!,
                              style: const TextStyle(
                                fontSize: 11,
                                color: CupertinoColors.systemRed,
                              ),
                            ),
                          ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    // 正则列表
                    if (patterns.isEmpty)
                      Center(
                        child: Padding(
                          padding: const EdgeInsets.all(32),
                          child: Column(
                            children: [
                              Icon(
                                CupertinoIcons.textformat,
                                size: 48,
                                color: isDark
                                    ? Colors.white24
                                    : Colors.black26,
                              ),
                              const SizedBox(height: 12),
                              Text(
                                'No patterns configured',
                                style: TextStyle(
                                  color: isDark
                                      ? Colors.white38
                                      : Colors.black38,
                                ),
                              ),
                            ],
                          ),
                        ),
                      )
                    else
                      ...patterns.map((pattern) => _ListItem(
                            text: pattern,
                            isMonospace: true,
                            onDelete: () {
                              final updated =
                                  patterns.where((p) => p != pattern).toList();
                              ref
                                  .read(ignoreRegexpProvider.notifier)
                                  .set(updated);
                            },
                          )),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Examples:',
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                    color: isDark ? Colors.white70 : Colors.black87,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  r'• ^\d{4}-\d{4}-\d{4}-\d{4}$ - Credit card numbers',
                  style: TextStyle(
                    fontSize: 11,
                    fontFamily: 'Courier New',
                    color: isDark ? Colors.white38 : Colors.black38,
                  ),
                ),
                Text(
                  '• password|secret|token - Sensitive keywords',
                  style: TextStyle(
                    fontSize: 11,
                    fontFamily: 'Courier New',
                    color: isDark ? Colors.white38 : Colors.black38,
                  ),
                ),
                Text(
                  r'• ^[A-Z0-9]{32}$ - API keys (32 chars)',
                  style: TextStyle(
                    fontSize: 11,
                    fontFamily: 'Courier New',
                    color: isDark ? Colors.white38 : Colors.black38,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

/// 通用列表项组件
class _ListItem extends StatelessWidget {
  const _ListItem({
    required this.text,
    required this.onDelete,
    this.isMonospace = false,
  });

  final String text;
  final VoidCallback onDelete;
  final bool isMonospace;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: isDark
            ? Colors.white.withValues(alpha: 0.03)
            : Colors.black.withValues(alpha: 0.02),
        borderRadius: BorderRadius.circular(6),
        border: Border.all(
          color: isDark
              ? Colors.white.withValues(alpha: 0.05)
              : Colors.black.withValues(alpha: 0.05),
        ),
      ),
      child: Row(
        children: [
          Expanded(
            child: Text(
              text,
              style: TextStyle(
                fontSize: 13,
                fontFamily: isMonospace ? 'Courier New' : null,
                color: isDark ? Colors.white.withValues(alpha: 0.87) : Colors.black.withValues(alpha: 0.87),
              ),
            ),
          ),
          CupertinoButton(
            padding: EdgeInsets.zero,
            onPressed: onDelete, minimumSize: const Size(24, 24),
            child: Icon(
              CupertinoIcons.xmark_circle_fill,
              size: 18,
              color: isDark
                  ? Colors.white.withValues(alpha: 0.3)
                  : Colors.black.withValues(alpha: 0.3),
            ),
          ),
        ],
      ),
    );
  }
}
