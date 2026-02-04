import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';

/// macOS 风格的设置组容器。
///
/// 为一组相关的设置项提供带有标题、圆角背景和内部分割线的统一样式。
///
/// 字段说明:
/// [children] 内部排列的设置行列表。
/// [title] 可选的组标题文字。
class MacosSettingsGroup extends StatelessWidget {

  const MacosSettingsGroup({super.key, required this.children, this.title});
  final List<Widget> children;
  final String? title;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        if (title != null)
          Padding(
            padding: const EdgeInsets.only(left: 12, bottom: 8, top: 4),
            child: Text(
              title!.toUpperCase(),
              style: TextStyle(
                fontSize: 11,
                fontWeight: FontWeight.w500,
                color: isDark ? Colors.white30 : Colors.black38,
                letterSpacing: 0.5,
              ),
            ),
          ),
        Container(
          margin: const EdgeInsets.only(bottom: 24),
          decoration: BoxDecoration(
            color: isDark ? const Color(0xFF2C2C2E) : Colors.white,
            borderRadius: BorderRadius.circular(10),
            boxShadow: [
              if (!isDark)
                BoxShadow(
                  color: Colors.black.withOpacity(0.03),
                  blurRadius: 4,
                  offset: const Offset(0, 1),
                ),
            ],
            border: Border.all(
              color: isDark
                  ? Colors.white.withOpacity(0.05)
                  : Colors.black.withOpacity(0.05),
              width: 0.5,
            ),
          ),
          child: ClipRRect(
            borderRadius: BorderRadius.circular(10),
            child: Column(children: _separateWithDividers(children, isDark)),
          ),
        ),
      ],
    );
  }

  /// 在各个子组件之间插入细分割线，并设置左侧缩进。
  List<Widget> _separateWithDividers(List<Widget> widgets, bool isDark) {
    if (widgets.length <= 1) return widgets;
    final List<Widget> result = [];
    for (int i = 0; i < widgets.length; i++) {
      result.add(widgets[i]);
      if (i < widgets.length - 1) {
        result.add(
          Divider(
            height: 0.5,
            thickness: 0.5,
            indent: 48,
            endIndent: 0,
            color: isDark ? Colors.white10 : Colors.black.withOpacity(0.05),
          ),
        );
      }
    }
    return result;
  }
}

/// macOS 风格的设置行组件。
///
/// 展示图标、标题、副标题，并在右侧放置自定义的操作组件（如 Checkbox 或 Dropdown）。
///
/// 字段说明:
/// [label] 主标题文字。
/// [subtitle] 可选的说明文字。
/// [icon] 左侧显示的图标。
/// [iconColor] 图标容器背景色。
/// [trailing] 右侧的交互 Widget。
class MacosSettingsTile extends StatelessWidget {

  const MacosSettingsTile({
    super.key,
    required this.label,
    this.subtitle,
    this.icon,
    this.iconColor,
    required this.trailing,
  });
  final String label;
  final String? subtitle;
  final IconData? icon;
  final Color? iconColor;
  final Widget trailing;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return Container(
      constraints: const BoxConstraints(minHeight: 44),
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        children: [
          if (icon != null) ...[
            Container(
              padding: const EdgeInsets.all(4),
              decoration: BoxDecoration(
                color: iconColor ?? CupertinoColors.systemBlue,
                borderRadius: BorderRadius.circular(6),
              ),
              child: Icon(icon, size: 16, color: Colors.white),
            ),
            const SizedBox(width: 12),
          ],
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  label,
                  style: TextStyle(
                    fontSize: 13,
                    color: isDark
                        ? Colors.white.withOpacity(0.9)
                        : Colors.black87,
                    fontWeight: FontWeight.w400,
                  ),
                ),
                if (subtitle != null)
                  Text(
                    subtitle!,
                    style: TextStyle(
                      fontSize: 11,
                      color: isDark ? Colors.white38 : Colors.black45,
                    ),
                  ),
              ],
            ),
          ),
          trailing,
        ],
      ),
    );
  }
}
