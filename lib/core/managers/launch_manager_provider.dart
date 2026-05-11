import 'dart:io';

import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:flutter/foundation.dart';
import 'package:launch_at_startup/launch_at_startup.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

part 'launch_manager_provider.g.dart';

/// 开机自启管理器。
///
/// 负责管理应用程序在操作系统登录时的自动启动配置。
/// 依赖于 launch_at_startup 包来实现跨平台支持。
@Riverpod(keepAlive: true)
class AppLaunchManager extends _$AppLaunchManager {
  @override
  FutureOr<void> build() async {
    // 只有在非 Web 和非调试模式下才启用开机自启设置
    if (kIsWeb || kDebugMode) {
      return;
    }

    final PackageInfo packageInfo = await PackageInfo.fromPlatform();

    launchAtStartup.setup(
      appName: packageInfo.appName,
      appPath: Platform.resolvedExecutable,
    );

    // 监听设置项的变化，实时同步至系统设置
    ref.listen(launchAtStartupProvider, (previous, next) async {
      await _syncStatus(next);
    }, fireImmediately: true);
  }

  /// 同步自启状态至系统。
  ///
  /// [enabled] 是否开启自启。
  Future<void> _syncStatus(bool enabled) async {
    try {
      if (enabled) {
        final isEnabled = await launchAtStartup.isEnabled();
        if (!isEnabled) {
          await launchAtStartup.enable();
        }
      } else {
        final isEnabled = await launchAtStartup.isEnabled();
        if (isEnabled) {
          await launchAtStartup.disable();
        }
      }
    } catch (e) {
      debugPrint(e.toString());
    }
  }
}
