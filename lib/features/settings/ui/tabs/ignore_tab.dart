import 'package:flutter/cupertino.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../widgets/macos_settings_widgets.dart';

/// 忽略列表选项卡，用于配置不监听剪贴板的黑名单应用
class IgnoreTab extends ConsumerWidget {
  /// 构造函数
  const IgnoreTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Column(
      children: [
        MacosSettingsGroup(title: 'Blacklist', children: [
          const Padding(
            padding: EdgeInsets.all(32.0),
            child: Center(
              child: Column(
                children: [
                  Icon(CupertinoIcons.slash_circle, size: 48, color: CupertinoColors.systemGrey),
                  SizedBox(height: 16),
                  Text('All applications are monitored', style: TextStyle(color: CupertinoColors.systemGrey)),
                ],
              ),
            ),
          ),
        ]),
      ],
    );
  }
}
