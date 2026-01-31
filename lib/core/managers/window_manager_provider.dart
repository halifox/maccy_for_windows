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
    const Size windowSize = Size(350, 500);
    
    // Adjust position so it doesn't go off screen
    // For simplicity, we just put it near the cursor
    await windowManager.setBounds(Rect.fromLTWH(
      cursorPosition.dx,
      cursorPosition.dy,
      windowSize.width,
      windowSize.height,
    ));

    if (Platform.isMacOS) {
      await windowManager.setVisualEffect(WindowVisualEffect(
        effect: WindowVisualEffectState.active,
      ));
    }

    await windowManager.show();
    await windowManager.focus();
  }

  Future<void> hide() async {
    await windowManager.hide();
  }
}
