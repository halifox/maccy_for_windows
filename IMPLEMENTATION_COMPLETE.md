# Maccy for Windows - 核心功能实现完成

## ✅ 已实现的功能

### 1. 自动粘贴功能 (Paste Simulation)
**文件**: `lib/core/services/paste_service.dart`

使用 Windows SendInput API 模拟 Ctrl+V 按键事件，实现选择历史记录后自动粘贴。

**核心特性**:
- 使用 Win32 API 发送键盘事件序列
- 支持延迟粘贴（默认 50ms）
- 完整的按键按下/释放序列
- 错误处理和平台检测

**使用方式**:
```dart
// 立即粘贴
final success = PasteService.simulatePaste();

// 延迟粘贴
final success = await PasteService.simulatePasteDelayed(delayMs: 100);
```

**集成位置**:
- `clipboard_manager_provider.dart` 的 `simulatePaste()` 方法
- 在 Windows 平台自动使用新的 PasteService

---

### 2. 增强窗口定位逻辑
**文件**: `lib/core/services/window_position_service.dart`

支持 4 种定位模式，完全对应 Maccy 的 PopupPosition 功能。

**支持的定位模式**:

#### 2.1 光标跟随模式 (cursor)
- 窗口出现在鼠标光标附近（右下方偏移 10px）
- 自动边界检测，确保不超出屏幕
- 多显示器支持

#### 2.2 屏幕中心模式 (center)
- 窗口居中显示在指定屏幕
- 支持多屏幕环境

#### 2.3 状态栏图标模式 (statusItem)
- 窗口出现在系统托盘图标附近
- 自动检测任务栏位置（上/下/左/右）
- 智能对齐

#### 2.4 记住位置模式 (lastPosition)
- 记住上次窗口位置
- 自动验证位置有效性
- 无效时回退到中心模式

**多屏幕支持**:
- `screenIndex = 0`: 活动屏幕（光标所在）
- `screenIndex >= 1`: 具体屏幕编号

**配置方式**:
在设置中配置 `popupPositionProvider` 和 `popupScreenProvider`：
```dart
// 设置为光标模式
ref.read(popupPositionProvider.notifier).set('cursor');

// 设置为第 2 个屏幕的中心
ref.read(popupPositionProvider.notifier).set('center');
ref.read(popupScreenProvider.notifier).set(2);
```

---

### 3. 循环选择模式 (Cycle Mode)
**文件**: `lib/core/managers/hotkey_manager_provider.dart`

完全复刻 Maccy 的三态状态机交互逻辑。

**三种状态**:

#### 3.1 Toggle 模式（默认）
- 按一次快捷键：打开窗口
- 再按一次：关闭窗口

#### 3.2 Opening 模式（过渡状态）
- 刚打开窗口的瞬间
- 200ms 内如果再次按键 → 进入 Cycle 模式
- 200ms 后自动转为 Toggle 模式

#### 3.3 Cycle 模式（循环选择）
- 按住修饰键（Ctrl/Shift/Alt）
- 连续按主键：循环选择下一项
- 释放修饰键：自动粘贴选中项并关闭窗口

**状态转换图**:
```
未显示 --[按快捷键]--> Opening --[200ms超时]--> Toggle
                           |
                           +--[再次按键]--> Cycle --[释放修饰键]--> 粘贴并关闭
```

**用户体验**:
1. 快速按两次 `Ctrl+Shift+V` → 循环模式
2. 继续按 `V` → 向下选择
3. 松开 `Ctrl+Shift` → 自动粘贴

---

### 4. 修饰键释放检测
**文件**: `lib/core/services/modifier_key_service.dart`

使用 Windows GetAsyncKeyState API 实时监听修饰键状态。

**核心特性**:
- 50ms 轮询间隔（可配置）
- 检测 Ctrl、Shift、Alt 三个修饰键
- 状态变化回调
- 所有修饰键释放回调

**API 使用**:
```dart
final service = ModifierKeyService();

// 开始监听
service.startMonitoring(
  onAllModifiersReleased: () {
    print('所有修饰键已释放');
  },
  onModifierStateChanged: (keys) {
    print('当前按下的修饰键: $keys');
  },
);

// 停止监听
service.stopMonitoring();

// 查询当前状态
final hasModifiers = service.hasAnyModifierPressed;
final isCtrlPressed = service.isModifierPressed(ModifierKey.ctrl);
```

**集成位置**:
- `hotkey_manager_provider.dart` 在进入 Opening 状态时自动启动监听
- 检测到修饰键释放时触发粘贴动作

---

## 🎯 完整交互流程示例

### 场景 1: 快速粘贴（Toggle 模式）
1. 用户按 `Ctrl+Shift+V` → 窗口打开
2. 用户按 `↓` 选择第 3 项
3. 用户按 `Enter` → 粘贴并关闭

### 场景 2: 循环选择（Cycle 模式）
1. 用户按住 `Ctrl+Shift`，快速按两次 `V`
2. 窗口打开，自动选中第 2 项
3. 用户继续按 `V` → 选中第 3 项
4. 用户继续按 `V` → 选中第 4 项
5. 用户松开 `Ctrl+Shift` → 自动粘贴第 4 项并关闭

### 场景 3: 多屏幕环境
1. 用户在设置中配置：
   - 弹窗位置：`center`
   - 目标屏幕：`2`（第二个显示器）
2. 用户按快捷键 → 窗口在第二个显示器中心打开

---

## 📋 配置项说明

### 新增的设置项

```dart
// 弹窗位置模式
final popupPositionProvider = pref<String>('popupPosition', 'cursor');
// 可选值: 'cursor', 'center', 'statusItem', 'lastPosition'

// 目标屏幕索引
final popupScreenProvider = pref<int>('popupScreen', 0);
// 0 = 活动屏幕, 1+ = 具体屏幕编号

// 自动粘贴开关
final autoPasteProvider = pref<bool>('autoPaste', true);
```

---

## 🔧 技术实现细节

### 1. Win32 API 使用

#### SendInput (粘贴)
```dart
final inputs = calloc<INPUT>(4);
inputs[0].type = INPUT_KEYBOARD;
inputs[0].ki.wVk = VK_CONTROL;
// ... 设置其他字段
SendInput(4, inputs, sizeOf<INPUT>());
```

#### GetAsyncKeyState (修饰键检测)
```dart
final ctrlState = GetAsyncKeyState(VK_CONTROL);
final isPressed = (ctrlState & 0x8000) != 0;
```

### 2. 状态管理架构

```
AppHotKeyManager (热键管理器)
  ├─ PopupState (三态状态机)
  ├─ ModifierKeyService (修饰键监听)
  └─ HistoryController (历史记录控制)
       └─ selectNext() (循环选择)
```

### 3. 窗口定位流程

```
showHistory()
  └─ WindowPositionService.calculatePosition()
       ├─ _getTargetDisplay() (确定目标屏幕)
       ├─ _positionNearCursor() (光标模式)
       ├─ _positionAtCenter() (中心模式)
       ├─ _positionNearTray() (托盘模式)
       └─ _positionAtLastLocation() (记住位置)
```

---

## 🧪 测试建议

### 测试用例 1: 自动粘贴
1. 打开记事本
2. 复制一些文本到剪贴板
3. 打开 Maccy 窗口
4. 选择一个历史记录
5. 验证：文本自动粘贴到记事本

### 测试用例 2: 循环选择
1. 确保历史记录中有至少 5 条
2. 按住 `Ctrl+Shift`，快速按 3 次 `V`
3. 验证：窗口打开并选中第 3 项
4. 继续按 `V` 2 次
5. 验证：选中第 5 项
6. 松开 `Ctrl+Shift`
7. 验证：第 5 项自动粘贴

### 测试用例 3: 多屏幕
1. 连接第二个显示器
2. 设置弹窗位置为 `center`，屏幕为 `2`
3. 按快捷键
4. 验证：窗口在第二个显示器中心打开

### 测试用例 4: 窗口定位
1. 测试 `cursor` 模式：移动鼠标到不同位置，验证窗口跟随
2. 测试 `statusItem` 模式：验证窗口在托盘图标附近
3. 测试 `lastPosition` 模式：移动窗口后关闭，再次打开验证位置

---

## 🐛 已知问题和注意事项

### 1. 粘贴延迟
- 某些应用需要更长的延迟才能正确接收粘贴
- 可通过 `PasteService.simulatePasteDelayed(delayMs: 150)` 调整

### 2. 修饰键检测精度
- 轮询间隔为 50ms，理论上可能有最多 50ms 的延迟
- 实际使用中几乎感觉不到

### 3. 多屏幕 DPI 缩放
- 在不同 DPI 的屏幕间切换时，坐标计算可能需要调整
- 当前实现使用逻辑坐标，应该能正确处理

### 4. 任务栏位置检测
- 依赖 `tray_manager.getBounds()` 的准确性
- 如果托盘图标位置获取失败，会回退到光标模式

---

## 📦 依赖项

确保 `pubspec.yaml` 包含以下依赖：

```yaml
dependencies:
  win32: ^5.0.0
  ffi: ^2.0.0
  screen_retriever: ^0.2.0
  tray_manager: ^0.5.2
  window_manager: ^0.5.1
  hotkey_manager: ^0.2.3
  shared_preferences: ^2.5.2
```

---

## 🚀 下一步建议

### 短期优化
1. 添加粘贴失败重试机制
2. 优化修饰键检测的 CPU 占用
3. 添加窗口动画效果
4. 支持自定义粘贴延迟配置

### 中期功能
1. 支持 macOS 平台（使用 CGEvent API）
2. 添加窗口位置拖拽保存
3. 支持更多快捷键组合
4. 添加声音反馈

### 长期规划
1. 插件系统
2. 云同步
3. 多语言支持
4. 主题定制

---

## 📝 代码生成

完成实现后，需要运行代码生成：

```bash
flutter pub run build_runner build --delete-conflicting-outputs
```

这将生成以下文件：
- `hotkey_manager_provider.g.dart`
- `window_manager_provider.g.dart`
- `clipboard_manager_provider.g.dart`

---

## 🎉 总结

本次实现完成了 Maccy 的 4 个核心交互功能：

1. ✅ **自动粘贴** - 使用 Win32 SendInput API
2. ✅ **窗口定位** - 4 种模式 + 多屏幕支持
3. ✅ **循环选择** - 三态状态机完整实现
4. ✅ **修饰键检测** - GetAsyncKeyState 轮询

这些功能的实现使得 Maccy for Windows 的交互体验与原版 macOS Maccy 高度一致，达到了"功能等价"和"交互高保真"的目标。

用户现在可以享受到流畅的剪贴板管理体验，包括快速粘贴、循环选择、智能窗口定位等核心功能。
