import 'dart:ui';

import 'package:clipboard/core/managers/window_manager_provider.dart';
import 'package:clipboard/features/settings/providers/settings_provider.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'app.dart';

/// 应用程序入口函数
void main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();

  // 设置 Flutter 框架层异常捕获
  FlutterError.onError = (details) {
    FlutterError.presentError(details);
    debugPrint('❌ Flutter Error: ${details.exception}\n${details.stack}');
  };

  // 设置平台/异步异常捕获
  PlatformDispatcher.instance.onError = (error, stack) {
    debugPrint('❌ Platform Error: $error\n$stack');
    return true; // 表示已处理
  };
  // 1. 首先初始化窗口管理器插件 (这是核心，必须最先完成)
  await AppWindowManager.init();
  final prefs = await SharedPreferences.getInstance();

  runApp(
    ProviderScope(
      overrides: [
        sharedPrefsProvider.overrideWithValue(prefs),
      ],
      child: const HaliClipApp(),
    ),
  );
}
