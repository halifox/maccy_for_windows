import 'package:flutter/cupertino.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../widgets/macos_settings_widgets.dart';

class IgnoreTab extends ConsumerWidget {
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
