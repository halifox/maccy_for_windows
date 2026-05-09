import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:maccy/features/settings/ui/tabs/general_tab.dart';
import 'package:maccy/features/settings/ui/tabs/storage_tab.dart';
import 'package:maccy/features/settings/ui/tabs/appearance_tab.dart';
import 'package:maccy/features/settings/ui/tabs/pins_tab.dart';
import 'package:maccy/features/settings/ui/tabs/ignore_tab.dart';
import 'package:maccy/features/settings/ui/tabs/advanced_tab.dart';

/// 设置主页面。
///
/// 采用典型的 macOS 系统设置风格：左侧为图标驱动的导航菜单，右侧为对应的功能配置区域。
class SettingsPage extends HookConsumerWidget {
  const SettingsPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final selectedCategory = useState('General');
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    final sidebarBg = isDark
        ? const Color(0xFF2D2D2D)
        : const Color(0xFFE8E8E8);
    final contentBg = isDark
        ? const Color(0xFF1E1E1E)
        : const Color(0xFFF2F2F7);

    return Scaffold(
      backgroundColor: contentBg,
      body: Row(
        children: [
          Container(
            width: 220,
            decoration: BoxDecoration(
              color: sidebarBg,
              border: Border(
                right: BorderSide(
                  color: isDark ? Colors.black26 : Colors.black12,
                  width: 0.5,
                ),
              ),
            ),
            child: Column(
              children: [
                const SizedBox(height: 44),
                _SidebarItem(
                  icon: CupertinoIcons.settings,
                  label: 'General',
                  isSelected: selectedCategory.value == 'General',
                  onTap: () => selectedCategory.value = 'General',
                ),
                _SidebarItem(
                  icon: CupertinoIcons.tray_arrow_down,
                  label: 'Storage',
                  isSelected: selectedCategory.value == 'Storage',
                  onTap: () => selectedCategory.value = 'Storage',
                ),
                _SidebarItem(
                  icon: CupertinoIcons.paintbrush,
                  label: 'Appearance',
                  isSelected: selectedCategory.value == 'Appearance',
                  onTap: () => selectedCategory.value = 'Appearance',
                ),
                _SidebarItem(
                  icon: CupertinoIcons.pin,
                  label: 'Pins',
                  isSelected: selectedCategory.value == 'Pins',
                  onTap: () => selectedCategory.value = 'Pins',
                ),
                _SidebarItem(
                  icon: CupertinoIcons.slash_circle,
                  label: 'Ignore',
                  isSelected: selectedCategory.value == 'Ignore',
                  onTap: () => selectedCategory.value = 'Ignore',
                ),
                _SidebarItem(
                  icon: CupertinoIcons.settings_solid,
                  label: 'Advanced',
                  isSelected: selectedCategory.value == 'Advanced',
                  onTap: () => selectedCategory.value = 'Advanced',
                ),
                const Spacer(),
              ],
            ),
          ),
          Expanded(child: _buildContent(context, ref, selectedCategory.value)),
        ],
      ),
    );
  }

  /// 构建右侧的具体设置面板。
  ///
  /// 使用 [CustomScrollView] 和 [CupertinoSliverNavigationBar] 模仿原生系统的滚动拉伸效果。
  ///
  /// [category] 当前选中的设置分类。
  Widget _buildContent(BuildContext context, WidgetRef ref, String category) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return CustomScrollView(
      key: ValueKey(category),
      slivers: [
        CupertinoSliverNavigationBar(
          automaticallyImplyLeading: false,
          largeTitle: Text(
            category,
            style: TextStyle(
              fontFamily: '.AppleSystemUIFont',
              letterSpacing: -0.5,
              color: isDark ? Colors.white : Colors.black,
            ),
          ),
          backgroundColor: Colors.transparent,
          border: null,
          stretch: true,
        ),
        SliverToBoxAdapter(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(32, 10, 32, 32),
            child: switch (category) {
              'General' => const GeneralTab(),
              'Storage' => const StorageTab(),
              'Appearance' => const AppearanceTab(),
              'Pins' => const PinsTab(),
              'Ignore' => const IgnoreTab(),
              'Advanced' => const AdvancedTab(),
              _ => const SizedBox.shrink(),
            },
          ),
        ),
      ],
    );
  }
}

/// 设置侧边栏项目组件。
///
/// 字段说明:
/// [icon] 导航图标数据。
/// [label] 菜单名称文字。
/// [isSelected] 是否处于选中高亮状态。
/// [onTap] 点击切换回调。
class _SidebarItem extends StatelessWidget {

  const _SidebarItem({
    required this.icon,
    required this.label,
    required this.isSelected,
    required this.onTap,
  });
  final IconData icon;
  final String label;
  final bool isSelected;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    const blue = Color(0xFF007AFF);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return GestureDetector(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 14, vertical: 2),
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: isSelected ? blue : Colors.transparent,
          borderRadius: BorderRadius.circular(10),
        ),
        child: Row(
          children: [
            Icon(
              icon,
              size: 18,
              color: isSelected
                  ? Colors.white
                  : (isDark ? Colors.blueAccent : blue),
            ),
            const SizedBox(width: 12),
            Text(
              label,
              style: TextStyle(
                fontSize: 13,
                fontFamily: '.AppleSystemUIFont',
                fontWeight: isSelected ? FontWeight.w600 : FontWeight.w400,
                color: isSelected
                    ? Colors.white
                    : (isDark
                          ? Colors.white.withValues(alpha: 0.85)
                          : Colors.black87),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
