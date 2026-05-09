import 'package:fuzzy/fuzzy.dart';
import 'package:maccy/core/database/database.dart' as db;

/// 搜索模式枚举
///
/// 对应 Maccy 的 Search.Mode
enum SearchMode {
  /// 精确匹配（不区分大小写）
  exact,

  /// 模糊搜索（Fuzzy）
  fuzzy,

  /// 正则表达式
  regexp,

  /// 混合模式（依次尝试 exact → regexp → fuzzy）
  mixed,
}

/// 搜索结果
class SearchResult {
  const SearchResult({
    required this.item,
    required this.score,
    required this.ranges,
  });

  /// 匹配的历史记录项
  final db.ClipboardEntry item;

  /// 匹配分数（0.0-1.0，越高越匹配）
  final double score;

  /// 匹配的字符范围（用于高亮显示）
  final List<MatchRange> ranges;
}

/// 匹配范围
class MatchRange {
  const MatchRange(this.start, this.end);

  final int start;
  final int end;

  @override
  String toString() => '[$start, $end)';
}

/// 高级搜索服务
///
/// 实现 Maccy 的四种搜索模式：exact, fuzzy, regexp, mixed
class AdvancedSearchService {
  /// 执行搜索
  ///
  /// [query] 搜索关键词
  /// [items] 待搜索的历史记录列表
  /// [mode] 搜索模式
  List<SearchResult> search(
    String query,
    List<db.ClipboardEntry> items,
    SearchMode mode,
  ) {
    if (query.isEmpty) {
      return items
          .map((item) => SearchResult(item: item, score: 1.0, ranges: const []))
          .toList();
    }

    switch (mode) {
      case SearchMode.exact:
        return _exactSearch(query, items);
      case SearchMode.fuzzy:
        return _fuzzySearch(query, items);
      case SearchMode.regexp:
        return _regexpSearch(query, items);
      case SearchMode.mixed:
        return _mixedSearch(query, items);
    }
  }

  /// 精确匹配搜索
  ///
  /// 不区分大小写，查找包含关键词的项目
  List<SearchResult> _exactSearch(String query, List<db.ClipboardEntry> items) {
    final lowerQuery = query.toLowerCase();
    final results = <SearchResult>[];

    for (final item in items) {
      final title = item.content;
      final lowerTitle = title.toLowerCase();
      if (lowerTitle.contains(lowerQuery)) {
        final ranges = _findExactMatchRanges(lowerTitle, lowerQuery);
        results.add(SearchResult(
          item: item,
          score: 1.0,
          ranges: ranges,
        ));
      }
    }

    return results;
  }

  /// 模糊搜索
  ///
  /// 使用 Fuzzy 算法，允许字符顺序不完全匹配
  /// 对应 Maccy 的 Fuse.js 实现（threshold=0.7）
  List<SearchResult> _fuzzySearch(String query, List<db.ClipboardEntry> items) {
    // 限制搜索文本长度（性能优化）
    final searchableItems = items.map((item) {
      final title = item.content.length > 5000
          ? item.content.substring(0, 5000)
          : item.content;
      return _SearchableItem(item, title);
    }).toList();

    final fuzzy = Fuzzy<_SearchableItem>(
      searchableItems,
      options: FuzzyOptions(
        keys: [
          WeightedKey(
            name: 'title',
            getter: (item) => item.title,
            weight: 1.0,
          ),
        ],
        threshold: 0.3, // fuzzy 包的阈值是反向的，0.3 对应 Maccy 的 0.7
        distance: 100,
        shouldSort: true,
      ),
    );

    final fuzzyResults = fuzzy.search(query);
    final results = <SearchResult>[];

    for (final result in fuzzyResults) {
      final ranges = result.matches.isNotEmpty
          ? result.matches.first.matchedIndices
              .map((range) => MatchRange(range.start, range.end + 1))
              .toList()
          : <MatchRange>[];

      results.add(SearchResult(
        item: result.item.item,
        score: 1.0 - result.score, // 转换为相似度分数
        ranges: ranges,
      ));
    }

    return results;
  }

  /// 正则表达式搜索
  List<SearchResult> _regexpSearch(String query, List<db.ClipboardEntry> items) {
    try {
      final regex = RegExp(query, caseSensitive: false);
      final results = <SearchResult>[];

      for (final item in items) {
        final match = regex.firstMatch(item.content);
        if (match != null) {
          final ranges = [MatchRange(match.start, match.end)];
          results.add(SearchResult(
            item: item,
            score: 1.0,
            ranges: ranges,
          ));
        }
      }

      return results;
    } catch (e) {
      // 正则表达式无效，返回空结果
      return [];
    }
  }

  /// 混合模式搜索
  ///
  /// 依次尝试 exact → regexp → fuzzy，返回第一个有结果的模式
  List<SearchResult> _mixedSearch(String query, List<db.ClipboardEntry> items) {
    // 1. 尝试精确匹配
    var results = _exactSearch(query, items);
    if (results.isNotEmpty) return results;

    // 2. 尝试正则表达式
    results = _regexpSearch(query, items);
    if (results.isNotEmpty) return results;

    // 3. 回退到模糊搜索
    return _fuzzySearch(query, items);
  }

  /// 查找精确匹配的字符范围
  List<MatchRange> _findExactMatchRanges(String text, String query) {
    final ranges = <MatchRange>[];
    var startIndex = 0;

    while (true) {
      final index = text.indexOf(query, startIndex);
      if (index == -1) break;

      ranges.add(MatchRange(index, index + query.length));
      startIndex = index + query.length;
    }

    return ranges;
  }
}

/// 内部辅助类，用于 Fuzzy 搜索
class _SearchableItem {
  _SearchableItem(this.item, this.title);

  final db.ClipboardEntry item;
  final String title;
}
