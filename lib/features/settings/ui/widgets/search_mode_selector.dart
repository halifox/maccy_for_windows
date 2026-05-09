import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';

/// 搜索模式选择器组件。
///
/// 用于设置界面，允许用户选择搜索模式。
/// 对应 Maccy 的 GeneralSettingsPane 中的搜索模式选择器。
class SearchModeSelector extends ConsumerWidget {
  const SearchModeSelector({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final searchMode = ref.watch(searchModeProvider);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'Search Mode',
          style: TextStyle(fontSize: 13, fontWeight: FontWeight.w500),
        ),
        const SizedBox(height: 8),
        DropdownButton<String>(
          value: searchMode,
          isExpanded: true,
          items: const [
            DropdownMenuItem(
              value: 'exact',
              child: Text('Exact - Case-insensitive substring match'),
            ),
            DropdownMenuItem(
              value: 'fuzzy',
              child: Text('Fuzzy - Approximate matching (like Spotlight)'),
            ),
            DropdownMenuItem(
              value: 'regex',
              child: Text('Regex - Regular expression pattern'),
            ),
            DropdownMenuItem(
              value: 'mixed',
              child: Text('Mixed - Try exact → regex → fuzzy'),
            ),
          ],
          onChanged: (value) {
            if (value != null) {
              ref.read(searchModeProvider.notifier).set(value);
            }
          },
        ),
        const SizedBox(height: 4),
        Text(
          _getDescription(searchMode),
          style: TextStyle(
            fontSize: 11,
            color: Colors.grey[600],
          ),
        ),
      ],
    );
  }

  String _getDescription(String mode) {
    switch (mode) {
      case 'exact':
        return 'Fast and precise. Searches for exact text matches (case-insensitive).';
      case 'fuzzy':
        return 'Flexible matching. Finds items even with typos or missing characters.';
      case 'regex':
        return 'Advanced pattern matching. Use regular expressions for complex searches.';
      case 'mixed':
        return 'Smart search. Tries exact match first, then regex, finally fuzzy.';
      default:
        return '';
    }
  }
}
