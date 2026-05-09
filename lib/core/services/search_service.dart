import 'package:fuzzy/fuzzy.dart';
import 'package:maccy/core/database/database.dart';

/// 搜索模式枚举。
///
/// 对应 Maccy 的 Search.Mode。
enum SearchMode {
  exact,  // 精确匹配（不区分大小写）
  fuzzy,  // 模糊搜索
  regexp, // 正则表达式
  mixed,  // 混合模式：依次尝试 exact → regexp → fuzzy
}

/// 搜索结果包装类。
///
/// 包含匹配的条目和相关性评分（用于模糊搜索排序）。
class SearchResult { // 匹配字符的索引位置（用于高亮）

  SearchResult({
    required this.entry,
    this.score = 0.0,
    this.matchIndices = const [],
  });
  final ClipboardEntry entry;
  final double score;
  final List<int> matchIndices;
}

/// 搜索服务。
///
/// 提供多种搜索模式，对应 Maccy 的 Search.swift 实现。
///
/// 核心功能：
/// - exact: 不区分大小写的子串匹配
/// - fuzzy: 基于 Fuse.js 算法的模糊搜索（threshold: 0.7）
/// - regexp: 正则表达式搜索
/// - mixed: 智能混合搜索（先精确，再正则，最后模糊）
class SearchService {
  /// 模糊搜索的最大文本长度限制。
  ///
  /// 对应 Maccy 的 fuzzySearchLimit = 5000。
  /// 超过此长度的文本会被截断以提升性能。
  static const int fuzzySearchLimit = 5000;

  /// 模糊搜索的相似度阈值。
  ///
  /// 对应 Maccy 的 Fuse(threshold: 0.7)。
  /// 值越小匹配越严格，0.0 = 完全匹配，1.0 = 匹配所有。
  static const double fuzzyThreshold = 0.3;

  /// 执行搜索。
  ///
  /// [query] 搜索关键词
  /// [items] 待搜索的条目列表
  /// [mode] 搜索模式
  ///
  /// 返回匹配的条目列表（模糊搜索会按相关性排序）。
  static List<ClipboardEntry> search({
    required String query,
    required List<ClipboardEntry> items,
    required SearchMode mode,
  }) {
    if (query.isEmpty) return items;

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

  /// 精确搜索（不区分大小写）。
  ///
  /// 对应 Maccy 的 simpleSearch(options: .caseInsensitive)。
  static List<ClipboardEntry> _exactSearch(
    String query,
    List<ClipboardEntry> items,
  ) {
    final lowerQuery = query.toLowerCase();
    return items.where((item) {
      return item.content.toLowerCase().contains(lowerQuery);
    }).toList();
  }

  /// 模糊搜索。
  ///
  /// 对应 Maccy 的 fuzzySearch，使用 Fuse 算法。
  ///
  /// 实现细节：
  /// 1. 截断超长文本（> 5000 字符）
  /// 2. 使用 fuzzy 包进行模糊匹配
  /// 3. 按相关性评分排序（分数越低越相关）
  static List<ClipboardEntry> _fuzzySearch(
    String query,
    List<ClipboardEntry> items,
  ) {
    // 准备搜索数据
    final searchableItems = items.map((item) {
      var content = item.content;
      if (content.length > fuzzySearchLimit) {
        content = content.substring(0, fuzzySearchLimit);
      }
      return content;
    }).toList();

    // 配置 Fuzzy 搜索
    final fuzzy = Fuzzy<String>(
      searchableItems,
      options: FuzzyOptions(
        threshold: fuzzyThreshold,
        isCaseSensitive: false,
        shouldSort: true,
        findAllMatches: false,
        minMatchCharLength: 1,
        location: 0,
        distance: 100,
      ),
    );

    // 执行搜索
    final results = fuzzy.search(query);

    // 映射回原始条目并按评分排序
    return results.map((result) {
      return items[result.score.toInt()];
    }).toList();
  }

  /// 正则表达式搜索。
  ///
  /// 对应 Maccy 的 simpleSearch(options: .regularExpression)。
  static List<ClipboardEntry> _regexpSearch(
    String query,
    List<ClipboardEntry> items,
  ) {
    try {
      final regex = RegExp(query, caseSensitive: false);
      return items.where((item) {
        return regex.hasMatch(item.content);
      }).toList();
    } catch (e) {
      // 正则表达式语法错误，返回空结果
      return [];
    }
  }

  /// 混合搜索。
  ///
  /// 对应 Maccy 的 mixedSearch。
  ///
  /// 搜索策略：
  /// 1. 先尝试精确匹配（最快）
  /// 2. 若无结果，尝试正则表达式
  /// 3. 若仍无结果，尝试模糊搜索（最慢但最宽松）
  static List<ClipboardEntry> _mixedSearch(
    String query,
    List<ClipboardEntry> items,
  ) {
    // 1. 精确搜索
    var results = _exactSearch(query, items);
    if (results.isNotEmpty) return results;

    // 2. 正则搜索
    results = _regexpSearch(query, items);
    if (results.isNotEmpty) return results;

    // 3. 模糊搜索
    results = _fuzzySearch(query, items);
    return results;
  }

  /// 获取搜索匹配的高亮范围。
  ///
  /// 用于在 UI 中高亮显示匹配的文本。
  ///
  /// [query] 搜索关键词
  /// [content] 内容文本
  /// [mode] 搜索模式
  ///
  /// 返回匹配位置的起始索引列表。
  static List<int> getMatchIndices({
    required String query,
    required String content,
    required SearchMode mode,
  }) {
    if (query.isEmpty) return [];

    switch (mode) {
      case SearchMode.exact:
        return _getExactMatchIndices(query, content);
      case SearchMode.regexp:
        return _getRegexpMatchIndices(query, content);
      case SearchMode.fuzzy:
      case SearchMode.mixed:
        // 模糊搜索的高亮较复杂，暂时使用精确匹配的高亮
        return _getExactMatchIndices(query, content);
    }
  }

  /// 获取精确匹配的索引位置。
  static List<int> _getExactMatchIndices(String query, String content) {
    final indices = <int>[];
    final lowerContent = content.toLowerCase();
    final lowerQuery = query.toLowerCase();

    int index = lowerContent.indexOf(lowerQuery);
    while (index != -1) {
      indices.add(index);
      index = lowerContent.indexOf(lowerQuery, index + 1);
    }

    return indices;
  }

  /// 获取正则匹配的索引位置。
  static List<int> _getRegexpMatchIndices(String query, String content) {
    try {
      final regex = RegExp(query, caseSensitive: false);
      final matches = regex.allMatches(content);
      return matches.map((m) => m.start).toList();
    } catch (e) {
      return [];
    }
  }
}
