# 设置页面功能缺失分析

基于 Maccy v2.6.1 源代码的完整对比分析。

---

## 📊 总体对比

| Tab | Maccy 功能数 | Flutter 已实现 | 缺失功能 | 完成度 |
|-----|-------------|---------------|---------|--------|
| **General** | 11 | 8 | 3 | 73% |
| **Storage** | 7 | 5 | 2 | 71% |
| **Appearance** | 16 | 11 | 5 | 69% |
| **Pins** | 1 (表格编辑器) | 0 | 1 | 0% |
| **Ignore** | 3 | 3 | 0 | 100% |
| **Advanced** | 7 | 4 | 3 | 57% |

**总体完成度：62% (31/50 功能)**

---

## 1️⃣ General Tab

### ✅ 已实现 (8/11)
- Launch at login
- Auto-check for updates
- Check Now button
- Open Clipboard hotkey
- Pin Item hotkey
- Delete Item hotkey
- Search Mode dropdown
- Auto-paste checkbox
- Pure text paste checkbox

### ❌ 缺失功能 (3/11)

#### 1.1 Modifiers Description Text
**位置：** Behavior 分组底部  
**Maccy 源码：** GeneralSettingsPane.swift:89-95
```swift
Text(String(
  format: NSLocalizedString("Modifiers", tableName: "GeneralSettings", comment: ""),
  copyModifier, pasteModifier, pasteWithoutFormatting
))
```
**功能：** 动态显示当前修饰键的作用，例如：
- "Hold ⌥ to copy item instead of pasting."
- "Hold ⌥⇧ to paste item without formatting."

**实现难度：** 简单

---

#### 1.2 Notifications and Sounds Link
**位置：** 最底部独立分组  
**Maccy 源码：** GeneralSettingsPane.swift:98-104
```swift
Settings.Section(title: "") {
  if let notificationsURL = notificationsURL {
    Link(destination: notificationsURL, label: {
      Text("NotificationsAndSounds", tableName: "GeneralSettings")
    })
  }
}
```
**功能：** 打开系统通知设置页面的链接  
**Windows 适配：** 需要打开 Windows 通知设置

**实现难度：** 中等

---

#### 1.3 Ignore Only Next Event
**位置：** 应该在 Advanced Tab，但 Maccy 没有单独的 Advanced Tab  
**注意：** 您的 Flutter 实现已经在 Advanced Tab 中实现了此功能！

---

## 2️⃣ Storage Tab

### ✅ 已实现 (5/7)
- Save Text checkbox
- Save Images checkbox
- Save Files checkbox
- History Size stepper
- Sort By dropdown
- Clear on Exit checkbox
- Clear System Clipboard checkbox

### ❌ 缺失功能 (2/7)

#### 2.1 Save Description Text
**位置：** Save 分组底部  
**Maccy 源码：** StorageSettingsPane.swift:90-92
```swift
Text("SaveDescription", tableName: "StorageSettings")
  .controlSize(.small)
  .foregroundStyle(.gray)
```
**内容：** "Maccy will save only the types you enable above."

**实现难度：** 简单

---

#### 2.2 Current Storage Size Display
**位置：** Size 输入框右侧  
**Maccy 源码：** StorageSettingsPane.swift:102-108
```swift
Text(storageSize)
  .controlSize(.small)
  .foregroundStyle(.gray)
  .help(Text("CurrentSizeTooltip", tableName: "StorageSettings"))
  .onAppear {
    storageSize = Storage.shared.size
  }
```
**功能：** 显示当前数据库大小，例如 "2.3 MB (142 items)"

**实现难度：** 中等（需要查询数据库）

---

## 3️⃣ Appearance Tab

### ✅ 已实现 (11/16)
- Popup Position dropdown
- Pinned Position dropdown
- Image Height stepper
- Preview Delay stepper
- Highlight Match dropdown
- Special Characters checkbox
- Menu Bar Icon checkbox + icon picker
- Search Box dropdown
- Application Icons checkbox
- Footer Menu checkbox

### ❌ 缺失功能 (5/16)

#### 3.1 Popup Screen Picker (Multi-monitor)
**位置：** Popup Position 选项中  
**Maccy 源码：** AppearanceSettingsPane.swift:53-80
```swift
Picker("", selection: $popupAt) {
  ForEach(PopupPosition.allCases) { position in
    if position == .center || position == .lastPosition, screens.count > 1 {
      screenPicker(for: position)
    } else {
      Text(position.description)
    }
  }
}
```
**功能：** 多显示器时可以选择在哪个屏幕显示  
**选项：** Active Screen, Screen 1, Screen 2, etc.

**实现难度：** 中等

---

#### 3.2 Show Recent Copy in Menu Bar
**位置：** Interface Elements 分组  
**Maccy 源码：** AppearanceSettingsPane.swift:149-151
```swift
Defaults.Toggle(key: .showRecentCopyInMenuBar) {
  Text("ShowRecentCopyInMenuBar", tableName: "AppearanceSettings")
}
```
**功能：** 在菜单栏图标旁边显示最近复制的内容预览

**实现难度：** 简单

---

#### 3.3 Search Visibility Mode
**位置：** Search Box 选项右侧  
**Maccy 源码：** AppearanceSettingsPane.swift:157-165
```swift
Picker("", selection: $searchVisibility) {
  ForEach(SearchVisibility.allCases) { type in
    Text(type.description)
  }
}
.disabled(!showSearch)
```
**选项：** Always, When Typing, Never  
**注意：** 您当前的 `showSearchProvider` 只有 3 个选项，但没有独立的 visibility picker

**实现难度：** 简单

---

#### 3.4 Show Title Before Search Field
**位置：** Interface Elements 分组  
**Maccy 源码：** AppearanceSettingsPane.swift:167-169
```swift
Defaults.Toggle(key: .showTitle) {
  Text("ShowTitleBeforeSearchField", tableName: "AppearanceSettings")
}
```
**功能：** 在搜索框前显示应用名称/标题

**实现难度：** 简单

---

#### 3.5 Open Preferences Warning
**位置：** Footer Menu 下方  
**Maccy 源码：** AppearanceSettingsPane.swift:177-180
```swift
Text("OpenPreferencesWarning", tableName: "AppearanceSettings")
  .opacity(showFooter ? 0 : 1)
  .controlSize(.small)
  .foregroundStyle(.gray)
```
**内容：** "You can still open preferences from the menu bar icon."  
**显示条件：** 仅当 Footer Menu 关闭时显示

**实现难度：** 简单

---

## 4️⃣ Pins Tab

### ❌ 完全缺失 (0/1)

#### 4.1 Pins Table Editor
**Maccy 源码：** PinsSettingsPane.swift:114-162

**功能：**
- 显示所有固定项的表格
- 3 列：
  - **Key**: 快捷键选择器 (⌘0-9, ⌘A-Z)
  - **Alias**: 可编辑的别名/标题
  - **Content**: 可编辑的内容（仅文本类型）
- 支持删除固定项 (Delete 键)
- 底部描述文字

**表格结构：**
```swift
Table(items, selection: $selection) {
  TableColumn(Text("Key")) { item in
    PinPickerView(item: item, availablePins: availablePins)
  }
  .width(60)
  
  TableColumn(Text("Alias")) { item in
    PinTitleView(item: item)
  }
  
  TableColumn(Text("Content")) { item in
    PinValueView(item: item)
  }
}
```

**实现难度：** 高（需要数据库查询 + 表格编辑器）

---

## 5️⃣ Ignore Tab

### ✅ 完全实现 (3/3)
- Applications 子标签 ✅
- Pasteboard Types 子标签 ✅
- Regular Expressions 子标签 ✅

**完成度：100%** 🎉

---

## 6️⃣ Advanced Tab

### ✅ 已实现 (4/7)
- Clipboard Check Interval stepper
- Pause Clipboard Monitoring checkbox
- Ignore Only Next Event checkbox
- Clear History on Exit checkbox
- Clear System Clipboard checkbox

### ❌ 缺失功能 (3/7)

**注意：** Maccy 的 Advanced Tab 非常简单，只有 2 个 toggle 和大量描述文字。您的 Flutter 实现反而更丰富！

#### 6.1 Turn Off Description
**Maccy 源码：** AdvancedSettingsPane.swift:10-12
```swift
Text("TurnOffDescription", tableName: "AdvancedSettings")
  .fixedSize(horizontal: false, vertical: true)
  .foregroundStyle(.gray)
  .controlSize(.small)
```
**内容：** "Maccy will stop recording clipboard changes until you turn it back on."

**实现难度：** 简单

---

#### 6.2 Shell Script Examples
**Maccy 源码：** AdvancedSettingsPane.swift:14-29

**功能：** 显示如何通过命令行控制 Maccy 的示例

**示例 1 - 暂停/恢复：**
```bash
# Pause
defaults write org.p0deje.Maccy ignoreEvents true
# Resume
defaults write org.p0deje.Maccy ignoreEvents false
```

**示例 2 - 忽略下一次事件：**
```bash
defaults write org.p0deje.Maccy ignoreOnlyNextEvent true
```

**实现难度：** 简单（纯文本显示）

---

#### 6.3 Turn Off Via Menu Icon Description
**Maccy 源码：** AdvancedSettingsPane.swift:20-23
```swift
Text("TurnOffViaMenuIconDescription", tableName: "AdvancedSettings")
  .fixedSize(horizontal: false, vertical: true)
  .foregroundStyle(.gray)
  .controlSize(.small)
```
**内容：** "You can also pause/resume from the menu bar icon."

**实现难度：** 简单

---

## 🎯 实现优先级建议

### 🔴 高优先级（影响核心功能）
1. **Pins Table Editor** - 这是唯一完全缺失的 Tab
2. **Current Storage Size Display** - 用户需要知道数据库大小
3. **Modifiers Description Text** - 帮助用户理解快捷键行为

### 🟡 中优先级（增强用户体验）
4. **Show Recent Copy in Menu Bar** - 常用功能
5. **Show Title Before Search Field** - UI 完整性
6. **Search Visibility Mode** - 已有基础，只需调整
7. **Popup Screen Picker** - 多显示器用户需要

### 🟢 低优先级（锦上添花）
8. **Save Description Text** - 纯提示文字
9. **Open Preferences Warning** - 纯提示文字
10. **Shell Script Examples** - 高级用户功能
11. **Turn Off Descriptions** - 纯提示文字
12. **Notifications Link** - Windows 适配复杂

---

## 📝 实现计划

### Phase 1: 补充简单文本和提示（1-2 小时）
- [ ] General: Modifiers Description Text
- [ ] Storage: Save Description Text
- [ ] Appearance: Open Preferences Warning
- [ ] Appearance: Show Title Before Search Field checkbox
- [ ] Appearance: Show Recent Copy in Menu Bar checkbox
- [ ] Advanced: Turn Off Description
- [ ] Advanced: Shell Script Examples
- [ ] Advanced: Turn Off Via Menu Icon Description

### Phase 2: 中等复杂度功能（2-3 小时）
- [ ] Storage: Current Storage Size Display
- [ ] Appearance: Search Visibility Mode picker
- [ ] Appearance: Popup Screen Picker (multi-monitor)

### Phase 3: 复杂功能（4-6 小时）
- [ ] Pins: Complete Table Editor
  - [ ] 查询数据库中 pin != null 的条目
  - [ ] 实现 3 列表格
  - [ ] Key 列：快捷键选择器
  - [ ] Alias 列：可编辑文本框
  - [ ] Content 列：可编辑文本框（仅文本类型）
  - [ ] 删除功能
  - [ ] 底部描述文字

---

## 🚀 快速开始

建议从 Phase 1 开始，因为这些都是简单的文本添加，可以快速提升完成度。

Phase 2 的功能需要一些逻辑处理，但不涉及复杂的 UI。

Phase 3 的 Pins Table Editor 是最复杂的，需要完整的 CRUD 功能和表格 UI。

---

## 📊 完成后的最终状态

实现所有缺失功能后：
- **General Tab**: 100% (11/11)
- **Storage Tab**: 100% (7/7)
- **Appearance Tab**: 100% (16/16)
- **Pins Tab**: 100% (1/1)
- **Ignore Tab**: 100% (3/3) ✅ 已完成
- **Advanced Tab**: 100% (7/7)

**总体完成度：100% (50/50 功能)** 🎉
