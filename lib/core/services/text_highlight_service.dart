import 'package:flutter/material.dart';
import 'package:maccy/core/services/advanced_search_service.dart';

/// 文本高亮服务
///
/// 实现 Maccy 的搜索结果高亮功能，支持两种高亮样式：
/// 1. bold - 加粗匹配文本
/// 2. color - 彩色高亮匹配文本
class TextHighlightService {
  /// 高亮样式枚举
  static const String styleBold = 'bold';
  static const String styleColor = 'color';

  /// 生成带高亮的富文本
  ///
  /// [text] 原始文本
  /// [ranges] 匹配范围列表
  /// [style] 高亮样式（'bold' 或 'color'）
  TextSpan buildHighlightedText(
    String text,
    List<MatchRange> ranges, {
    String style = styleBold,
    TextStyle? baseStyle,
  }) {
    if (ranges.isEmpty) {
      return TextSpan(text: text, style: baseStyle);
    }

    final spans = <TextSpan>[];
    var lastEnd = 0;

    // 合并重叠的范围
    final mergedRanges = _mergeRanges(ranges);

    for (final range in mergedRanges) {
      // 添加未匹配的文本
      if (range.start > lastEnd) {
        spans.add(TextSpan(
          text: text.substring(lastEnd, range.start),
          style: baseStyle,
        ));
      }

      // 添加匹配的文本（高亮）
      final matchedText = text.substring(range.start, range.end);
      spans.add(TextSpan(
        text: matchedText,
        style: _getHighlightStyle(style, baseStyle),
      ));

      lastEnd = range.end;
    }

    // 添加剩余的文本
    if (lastEnd < text.length) {
      spans.add(TextSpan(
        text: text.substring(lastEnd),
        style: baseStyle,
      ));
    }

    return TextSpan(children: spans);
  }

  /// 获取高亮样式
  TextStyle _getHighlightStyle(String style, TextStyle? baseStyle) {
    if (style == styleBold) {
      return (baseStyle ?? const TextStyle()).copyWith(
        fontWeight: FontWeight.bold,
      );
    } else {
      // color 样式
      return (baseStyle ?? const TextStyle()).copyWith(
        backgroundColor: Colors.yellow.withValues(alpha: 0.3),
        fontWeight: FontWeight.w600,
      );
    }
  }

  /// 合并重叠的匹配范围
  List<MatchRange> _mergeRanges(List<MatchRange> ranges) {
    if (ranges.isEmpty) return [];

    // 按起始位置排序
    final sorted = List<MatchRange>.from(ranges)
      ..sort((a, b) => a.start.compareTo(b.start));

    final merged = <MatchRange>[sorted.first];

    for (var i = 1; i < sorted.length; i++) {
      final current = sorted[i];
      final last = merged.last;

      if (current.start <= last.end) {
        // 重叠或相邻，合并
        merged[merged.length - 1] = MatchRange(
          last.start,
          current.end > last.end ? current.end : last.end,
        );
      } else {
        // 不重叠，添加新范围
        merged.add(current);
      }
    }

    return merged;
  }

  /// 截断长文本并保留匹配部分
  ///
  /// 用于在列表中显示长文本时，确保匹配部分可见
  /// 对应 Maccy 的 String+Shortened.swift
  String shortenText(
    String text,
    List<MatchRange> ranges, {
    int maxLength = 100,
    String ellipsis = '...',
  }) {
    if (text.length <= maxLength) return text;

    if (ranges.isEmpty) {
      // 没有匹配，直接截断
      return text.substring(0, maxLength) + ellipsis;
    }

    // 找到第一个匹配的位置
    final firstMatch = ranges.first;
    final matchCenter = (firstMatch.start + firstMatch.end) ~/ 2;

    // 计算截取范围，确保匹配部分在中间
    var start = matchCenter - maxLength ~/ 2;
    var end = start + maxLength;

    if (start < 0) {
      start = 0;
      end = maxLength;
    } else if (end > text.length) {
      end = text.length;
      start = end - maxLength;
      if (start < 0) start = 0;
    }

    var result = text.substring(start, end);

    // 添加省略号
    if (start > 0) result = ellipsis + result;
    if (end < text.length) result = result + ellipsis;

    return result;
  }

  /// 显示特殊字符
  ///
  /// 对应 Maccy 的 showSpecialSymbols 配置
  /// 将不可见字符转换为可见符号：
  /// - 空格 → ·
  /// - 换行 → ⏎
  /// - Tab → ⇥
  String showSpecialCharacters(String text) {
    return text
        .replaceAll(' ', '·')
        .replaceAll('\n', '⏎\n')
        .replaceAll('\t', '⇥');
  }

  /// 隐藏特殊字符（恢复原始文本）
  String hideSpecialCharacters(String text) {
    return text
        .replaceAll('·', ' ')
        .replaceAll('⏎', '')
        .replaceAll('⇥', '\t');
  }
}
