import 'dart:ui';

import 'package:maccy/core/managers/window_manager_provider.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:maccy/app.dart';

/// 应用程序入口函数。
///
/// 负责 Flutter 绑定的初始化、全局异常捕获配置、核心插件（如窗口管理器）的预初始化、
/// 以及本地持久化存储（SharedPreferences）的加载。
///
/// [args] 命令行参数。
void main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();

  FlutterError.onError = (details) {
    FlutterError.presentError(details);
  };

  PlatformDispatcher.instance.onError = (error, stack) {
    return true;
  };

  await AppWindowManager.init();
  final prefs = await SharedPreferences.getInstance();

  runApp(
    ProviderScope(
      overrides: [sharedPrefsProvider.overrideWithValue(prefs)],
      child: const MaccyApp(),
    ),
  );
}
