import 'package:flutter/material.dart';
import 'package:maccy/core/constants/ui_constants.dart';

/// 键盘快捷键显示组件。
///
/// 完全复刻 Maccy 的 KeyboardShortcutView.swift。
/// 布局：[修饰符 55px] [间距 1px] [字符 12px]
class KeyboardShortcutWidget extends StatelessWidget {
  const KeyboardShortcutWidget({
    required this.shortcut,
    required this.isSelected,
    required this.isDark,
    super.key,
  });

  final String shortcut;
  final bool isSelected;
  final bool isDark;

  @override
  Widget build(BuildContext context) {
    // 分离修饰符和字符
    // 例如：'⌘1' -> modifiers: '⌘', character: '1'
    final modifiers = shortcut.length > 1 ? shortcut.substring(0, shortcut.length - 1) : '';
    final character = shortcut.isNotEmpty ? shortcut[shortcut.length - 1] : '';

    return Opacity(
      opacity: character.isEmpty ? 0.0 : MaccyUIConstants.shortcutOpacity,
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          // 修饰符区域（右对齐）
          SizedBox(
            width: MaccyUIConstants.shortcutModifiersWidth,
            child: Text(
              modifiers,
              textAlign: TextAlign.right,
              style: TextStyle(
                fontSize: MaccyUIConstants.shortcutFontSize,
                fontFamily: MaccyUIConstants.systemFontFamilyWindows,
                color: isSelected
                    ? Colors.white
                    : (isDark ? Colors.white : Colors.black87),
              ),
            ),
          ),
          // 间距
          const SizedBox(width: MaccyUIConstants.shortcutSpacing),
          // 字符区域（居中）
          SizedBox(
            width: MaccyUIConstants.shortcutCharacterWidth,
            child: Text(
              character,
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: MaccyUIConstants.shortcutFontSize,
                fontFamily: MaccyUIConstants.systemFontFamilyWindows,
                color: isSelected
                    ? Colors.white
                    : (isDark ? Colors.white : Colors.black87),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
