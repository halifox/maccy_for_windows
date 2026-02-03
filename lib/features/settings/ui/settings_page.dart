import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'tabs/general_tab.dart';
import 'tabs/storage_tab.dart';
import 'tabs/appearance_tab.dart';
import 'tabs/pins_tab.dart';
import 'tabs/ignore_tab.dart';
import 'tabs/advanced_tab.dart';

/// 设置页面，采用 macOS 风格的侧边栏布局
class SettingsPage extends HookConsumerWidget {
  /// 构造函数
  const SettingsPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final selectedCategory = useState('General');
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    final sidebarBg = isDark ? const Color(0xFF2D2D2D) : const Color(0xFFE8E8E8);
    final contentBg = isDark ? const Color(0xFF1E1E1E) : const Color(0xFFF2F2F7);

    return Scaffold(
      backgroundColor: contentBg,
      body: Row(
        children: [
          // macOS Sidebar
          Container(
            width: 220,
            decoration: BoxDecoration(
              color: sidebarBg,
              border: Border(right: BorderSide(color: isDark ? Colors.black26 : Colors.black12, width: 0.5)),
            ),
            child: Column(
              children: [
                const SizedBox(height: 44),
                _SidebarItem(icon: CupertinoIcons.settings, label: 'General', isSelected: selectedCategory.value == 'General', onTap: () => selectedCategory.value = 'General'),
                _SidebarItem(icon: CupertinoIcons.tray_arrow_down, label: 'Storage', isSelected: selectedCategory.value == 'Storage', onTap: () => selectedCategory.value = 'Storage'),
                _SidebarItem(icon: CupertinoIcons.paintbrush, label: 'Appearance', isSelected: selectedCategory.value == 'Appearance', onTap: () => selectedCategory.value = 'Appearance'),
                _SidebarItem(icon: CupertinoIcons.pin, label: 'Pins', isSelected: selectedCategory.value == 'Pins', onTap: () => selectedCategory.value = 'Pins'),
                _SidebarItem(icon: CupertinoIcons.slash_circle, label: 'Ignore', isSelected: selectedCategory.value == 'Ignore', onTap: () => selectedCategory.value = 'Ignore'),
                _SidebarItem(icon: CupertinoIcons.settings_solid, label: 'Advanced', isSelected: selectedCategory.value == 'Advanced', onTap: () => selectedCategory.value = 'Advanced'),
                const Spacer(),
              ],
            ),
          ),
          // Right Content Area
          Expanded(
            child: _buildContent(context, ref, selectedCategory.value),
          ),
        ],
      ),
    );
  }

  /// 构建右侧内容区域，根据选中的分类显示不同的 Tab
  Widget _buildContent(BuildContext context, WidgetRef ref, String category) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    return CustomScrollView(
      key: ValueKey(category), // 关键：强制 CustomScrollView 在分类切换时重新创建
      slivers: [
        CupertinoSliverNavigationBar(
          automaticallyImplyLeading: false,
          largeTitle: Text(
            category, 
            style: TextStyle(
              fontFamily: '.AppleSystemUIFont', 
              letterSpacing: -0.5,
              color: isDark ? Colors.white : Colors.black, // 适配深浅模式
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

/// 侧边栏项目组件
class _SidebarItem extends StatelessWidget {
  /// 图标
  final IconData icon;
  /// 标签文本
  final String label;
  /// 是否被选中
  final bool isSelected;
  /// 点击回调
  final VoidCallback onTap;

  /// 构造函数
  const _SidebarItem({required this.icon, required this.label, required this.isSelected, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final blue = const Color(0xFF007AFF);
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
            Icon(icon, size: 18, color: isSelected ? Colors.white : (isDark ? Colors.blueAccent : blue)),
            const SizedBox(width: 12),
            Text(
              label,
              style: TextStyle(
                fontSize: 13,
                fontFamily: '.AppleSystemUIFont',
                fontWeight: isSelected ? FontWeight.w600 : FontWeight.w400,
                color: isSelected ? Colors.white : (isDark ? Colors.white.withOpacity(0.85) : Colors.black87),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
