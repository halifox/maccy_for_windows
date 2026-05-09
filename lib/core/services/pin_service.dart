import 'package:maccy/core/database/database.dart';
import 'package:maccy/features/history/repositories/history_repository.dart';

/// 固定项服务
///
/// 实现 Maccy 的固定项（Pin）功能，允许用户将常用的剪贴板项固定到列表顶部或底部，
/// 并为其分配字母快捷键（b-y，排除 a/q/v/w/z）。
///
/// 注意：由于数据模型使用 isPinned + pinOrder，快捷键映射在此服务中处理。
class PinService {
  PinService(this._repository);

  final HistoryRepository _repository;

  /// 可用的固定快捷键
  ///
  /// 对应 Maccy 的 KeyShortcut.availableKeys
  /// 排除了 a(全选), q(退出), v(粘贴), w(关闭), z(撤销)
  static const availableKeys = [
    'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'r', 's', 't', 'u', 'x', 'y',
  ];

  /// 字母键到数字顺序的映射
  static const keyToOrder = {
    'b': 0, 'c': 1, 'd': 2, 'e': 3, 'f': 4,
    'g': 5, 'h': 6, 'i': 7, 'j': 8, 'k': 9,
    'l': 10, 'm': 11, 'n': 12, 'o': 13, 'p': 14,
    'r': 15, 's': 16, 't': 17, 'u': 18, 'x': 19, 'y': 20,
  };

  /// 数字顺序到字母键的映射
  static const orderToKey = {
    0: 'b', 1: 'c', 2: 'd', 3: 'e', 4: 'f',
    5: 'g', 6: 'h', 7: 'i', 8: 'j', 9: 'k',
    10: 'l', 11: 'm', 12: 'n', 13: 'o', 14: 'p',
    15: 'r', 16: 's', 17: 't', 18: 'u', 19: 'x', 20: 'y',
  };

  /// 切换固定状态
  ///
  /// 如果项目已固定，则取消固定；否则分配快捷键并固定。
  Future<void> togglePin(HistoryItem item) async {
    if (item.pin != null) {
      await unpin(item);
    } else {
      await pin(item);
    }
  }

  /// 固定项目
  ///
  /// 自动分配可用的 pinOrder
  Future<void> pin(HistoryItem item) async {
    final pinKey = await _findAvailablePin();
    if (pinKey == null) {
      throw Exception('没有可用的固定位置（最多 21 个固定项）');
    }

    await _repository.togglePin(item.id);
  }

  /// 取消固定
  Future<void> unpin(HistoryItem item) async {
    await _repository.togglePin(item.id);
  }

  /// 为项目分配特定的快捷键
  ///
  /// 如果快捷键已被占用，会先释放原有项目的固定状态
  Future<void> pinWithKey(HistoryItem item, String key) async {
    if (!availableKeys.contains(key)) {
      throw ArgumentError('无效的快捷键: $key');
    }

    // 检查是否已被占用
    final existing = await _repository.getItemByPin(key);
    if (existing != null && existing.id != item.id) {
      await unpin(existing);
    }

    await _repository.togglePin(item.id);
  }

  /// 查找可用的 pin 字符
  Future<String?> _findAvailablePin() async {
    return _repository.getNextAvailablePin();
  }

  /// 获取所有固定项
  Future<List<HistoryItem>> getPinnedItems() async {
    return _repository.getPinnedItems();
  }

  /// 获取指定快捷键对应的项目
  Future<HistoryItem?> getItemByKey(String key) async {
    if (!availableKeys.contains(key)) return null;
    return _repository.getItemByPin(key);
  }

  /// 获取项目的快捷键
  String? getKeyForItem(HistoryItem item) {
    return item.pin;
  }

  /// 检查快捷键是否可用
  Future<bool> isKeyAvailable(String key) async {
    if (!availableKeys.contains(key)) return false;
    final item = await _repository.getItemByPin(key);
    return item == null;
  }

  /// 获取可用快捷键数量
  Future<int> getAvailableKeyCount() async {
    final pinnedItems = await _repository.getPinnedItems();
    return availableKeys.length - pinnedItems.length;
  }
}
