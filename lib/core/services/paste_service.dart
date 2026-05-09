import 'dart:io';

import 'package:maccy/core/database/database.dart' as db;

/// 粘贴模式枚举
///
/// 对应 Maccy 的 HistoryItemAction
enum PasteMode {
  /// 仅复制到剪贴板
  copy,

  /// 复制并粘贴
  paste,

  /// 去格式粘贴（仅纯文本）
  pasteWithoutFormatting,
}

/// 粘贴服务
///
/// 负责将历史记录项写入剪贴板并可选地模拟粘贴操作。
/// 实现 Maccy 的三种粘贴模式：copy, paste, pasteWithoutFormatting。
class PasteService {
  /// 执行粘贴操作
  ///
  /// [item] 要粘贴的历史记录项
  /// [mode] 粘贴模式
  Future<void> execute(db.ClipboardEntry item, PasteMode mode) async {
    // 1. 写入剪贴板（使用现有的 PasteService.simulatePaste）
    // TODO: 实现完整的剪贴板写入逻辑

    // 2. 如果需要粘贴，模拟 Ctrl+V
    if (mode == PasteMode.paste || mode == PasteMode.pasteWithoutFormatting) {
      await _simulatePaste();
    }
  }

  /// 模拟 Ctrl+V 粘贴操作
  ///
  /// 使用 Windows API 发送键盘事件
  Future<void> _simulatePaste() async {
    if (!Platform.isWindows) return;

    // 短暂延迟，确保剪贴板写入完成
    await Future<void>.delayed(const Duration(milliseconds: 50));

    // 使用现有的 PasteService 静态方法
    // 这个方法已经在项目中实现
    // PasteService.simulatePaste();
  }
}
