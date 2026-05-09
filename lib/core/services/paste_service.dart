import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:win32/win32.dart';

/// 自动粘贴服务。
///
/// 使用 Windows SendInput API 模拟 Ctrl+V 按键事件，实现选择历史记录后自动粘贴到目标应用。
/// 这是 Maccy 核心交互体验的关键组件。
class PasteService {
  /// 模拟 Ctrl+V 按键组合。
  ///
  /// 通过 Win32 SendInput API 发送键盘事件序列：
  /// 1. Ctrl Down
  /// 2. V Down
  /// 3. V Up
  /// 4. Ctrl Up
  ///
  /// 返回是否成功发送输入事件。
  static bool simulatePaste() {
    if (!Platform.isWindows) {
      return false;
    }

    final inputs = calloc<INPUT>(4);

    try {
      // Ctrl Down
      inputs[0].type = INPUT_KEYBOARD;
      inputs[0].ki.wVk = VK_CONTROL;
      inputs[0].ki.wScan = 0;
      inputs[0].ki.dwFlags = 0;
      inputs[0].ki.time = 0;
      inputs[0].ki.dwExtraInfo = 0;

      // V Down
      inputs[1].type = INPUT_KEYBOARD;
      inputs[1].ki.wVk = 0x56; // 'V' key
      inputs[1].ki.wScan = 0;
      inputs[1].ki.dwFlags = 0;
      inputs[1].ki.time = 0;
      inputs[1].ki.dwExtraInfo = 0;

      // V Up
      inputs[2].type = INPUT_KEYBOARD;
      inputs[2].ki.wVk = 0x56;
      inputs[2].ki.wScan = 0;
      inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
      inputs[2].ki.time = 0;
      inputs[2].ki.dwExtraInfo = 0;

      // Ctrl Up
      inputs[3].type = INPUT_KEYBOARD;
      inputs[3].ki.wVk = VK_CONTROL;
      inputs[3].ki.wScan = 0;
      inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
      inputs[3].ki.time = 0;
      inputs[3].ki.dwExtraInfo = 0;

      final result = SendInput(4, inputs, sizeOf<INPUT>());
      return result == 4;
    } finally {
      free(inputs);
    }
  }

  /// 延迟执行粘贴操作。
  ///
  /// 在某些应用中，需要等待剪贴板内容完全写入后再执行粘贴。
  ///
  /// [delayMs] 延迟毫秒数，默认 50ms。
  static Future<bool> simulatePasteDelayed({int delayMs = 50}) async {
    await Future.delayed(Duration(milliseconds: delayMs));
    return simulatePaste();
  }

  /// 模拟 Ctrl+C 复制操作（用于测试或特殊场景）。
  static bool simulateCopy() {
    if (!Platform.isWindows) {
      return false;
    }

    final inputs = calloc<INPUT>(4);

    try {
      // Ctrl Down
      inputs[0].type = INPUT_KEYBOARD;
      inputs[0].ki.wVk = VK_CONTROL;
      inputs[0].ki.dwFlags = 0;

      // C Down
      inputs[1].type = INPUT_KEYBOARD;
      inputs[1].ki.wVk = 0x43; // 'C' key
      inputs[1].ki.dwFlags = 0;

      // C Up
      inputs[2].type = INPUT_KEYBOARD;
      inputs[2].ki.wVk = 0x43;
      inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

      // Ctrl Up
      inputs[3].type = INPUT_KEYBOARD;
      inputs[3].ki.wVk = VK_CONTROL;
      inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

      final result = SendInput(4, inputs, sizeOf<INPUT>());
      return result == 4;
    } finally {
      free(inputs);
    }
  }
}
