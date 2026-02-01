import 'dart:io';
import 'package:flutter/material.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:window_manager/window_manager.dart';
import 'package:screen_retriever/screen_retriever.dart';

part 'window_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppWindowManager extends _$AppWindowManager with WindowListener {
  @override
  void build() {
    windowManager.addListener(this);
  }

  @override
  void onWindowBlur() {
    hide();
  }

  Future<void> showAtCursor() async {
    Offset cursorPosition = await screenRetriever.getCursorScreenPoint();
    
    // Default size for the clipboard history window
    const Size windowSize = Size(350, 450);
    
    await windowManager.setHasShadow(true);
    await windowManager.setSize(windowSize);
    await windowManager.setPosition(Offset(cursorPosition.dx, cursorPosition.dy));

    await windowManager.show();
    await windowManager.focus();
  }

  Future<void> showSettings() async {
    const Size settingsSize = Size(500, 600);
    await windowManager.setSize(settingsSize);
    await windowManager.setMinimumSize(settingsSize);
    await windowManager.center();
    await windowManager.show();
    await windowManager.focus();
  }

  Future<void> hide() async {
    await windowManager.hide();
  }
}
