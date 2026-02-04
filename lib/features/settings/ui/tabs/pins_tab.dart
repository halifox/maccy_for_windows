import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

/// 设置：置顶项目管理选项卡。
///
/// 允许用户集中查看、排序或批量取消置顶的剪贴板条目。
class PinsTab extends ConsumerWidget {
  const PinsTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return const SingleChildScrollView(
      padding: EdgeInsets.all(24),
      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: []),
    );
  }
}
