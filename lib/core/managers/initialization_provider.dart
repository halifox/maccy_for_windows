import 'package:maccy/core/managers/tray_manager_provider.dart';
import 'package:maccy/core/managers/window_manager_provider.dart';
import 'package:maccy/core/managers/launch_manager_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'package:maccy/core/managers/clipboard_manager_provider.dart';
import 'package:maccy/core/managers/hotkey_manager_provider.dart';

part 'initialization_provider.g.dart';

/// 应用程序启动初始化流程编排器。
///
/// 作为应用启动的“哨兵”，负责协调所有核心后台管理服务的并行初始化过程。
/// 只有当本 Provider 完成后，主 UI 才会从闪屏页切换到主业务页面。
///
/// [ref] Riverpod 引用，用于触发各管理器的实例化。
@riverpod
Future<void> appStartup(Ref ref) async {
  debugPrint('[AppStartup] 开始编排后台服务初始化流程...');

  await Future.wait([
    ref.watch(appWindowManagerProvider.future),
    ref.watch(appTrayManagerProvider.future),
    ref.watch(appHotKeyManagerProvider.future),
    ref.watch(appClipboardManagerProvider.future),
    ref.watch(appLaunchManagerProvider.future),
  ]);

  debugPrint('[AppStartup] 核心后台服务 (窗口、托盘、热键、剪贴板、自启) 已全部初始化完成');
}
