import 'package:clipboard/core/managers/tray_manager_provider.dart';
import 'package:clipboard/core/managers/window_manager_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'clipboard_manager_provider.dart';
import 'hotkey_manager_provider.dart';

part 'initialization_provider.g.dart';

/// 应用程序启动初始化 Provider，用于预先加载并运行所有的后台管理服务（窗口、托盘、快捷键、剪贴板监听）。
@riverpod
Future<void> appStartup(Ref ref) async {
  debugPrint('🏗️ AppStartup: 开始编排初始化流程...');

  // 2. 启动并等待其他服务初始化
  // 对于 AsyncNotifier，我们使用 .future 等待异步 build 完成
  // 对于同步 Notifier (如 AppWindowManager)，直接 watch 即可触发实例化

  debugPrint('🏗️ AppStartup: 正在启动后台服务 (托盘、热键、剪贴板)...');
  await Future.wait([
    ref.watch(appWindowManagerProvider.future),
    ref.watch(appTrayManagerProvider.future),
    ref.watch(appHotKeyManagerProvider.future),
    ref.watch(appClipboardManagerProvider.future),
  ]);

  debugPrint('🏁 AppStartup: 所有核心服务已就绪。');
}
