# Visual Replication Guide - Maccy for Windows

## Design System

### Color Palette

#### Dark Mode
```
Background:     #1E1E1E @ 85% opacity
Selection:      #0A84FF @ 80% opacity
Text Primary:   #FFFFFF
Text Secondary: #FFFFFF @ 70% opacity
Text Tertiary:  #FFFFFF @ 24% opacity
Divider:        #FFFFFF @ 10% opacity
Border:         #000000 @ 50% opacity
```

#### Light Mode
```
Background:     #F5F5F5 @ 85% opacity
Selection:      #007AFF @ 80% opacity
Text Primary:   #000000 @ 87% opacity
Text Secondary: #000000 @ 70% opacity
Text Tertiary:  #000000 @ 26% opacity
Divider:        #000000 @ 12% opacity
Border:         #000000 @ 12% opacity
```

### Typography

```
Search Field:
  - Font Size: 12px
  - Font Family: System UI Font
  - Placeholder Color: Secondary text color

List Items:
  - Font Size: 13px
  - Font Family: System UI Font
  - Line Height: 24px (single line)
  - Max Lines: 1
  - Overflow: Ellipsis

Keyboard Shortcuts:
  - Font Size: 12px
  - Font Family: System UI Font
  - Color: Tertiary text color
```

### Spacing & Layout

```
Window:
  - Default Size: 450x450px
  - Min Size: 300x200px
  - Corner Radius: 6px
  - Border Width: 0.5px

Content Padding:
  - Vertical: 5px
  - Horizontal: 5px

Search Field:
  - Height: ~28px (auto with padding)
  - Padding: 5px all sides
  - Margin Bottom: 5px
  - Corner Radius: 4px

List Items:
  - Height: 24px (fixed)
  - Padding Horizontal: 10px
  - Padding Vertical: 0px
  - Spacing Between: 0px

Dividers:
  - Height: 6px (includes padding)
  - Indent: 10px
  - Thickness: 1px

Footer Menu:
  - Item Height: 24px
  - Padding Horizontal: 10px
  - Top Divider: 6px height
```

### Visual Effects

#### Background Blur
- **Windows 11**: Acrylic material with transient window backdrop
- **Windows 10**: Blur-behind effect with full window region
- **Opacity**: 85% for background color + system blur

#### Shadows
- Window shadow: System default (enabled via `setHasShadow(true)`)
- No internal shadows on items

#### Animations
- List item selection: Instant (no animation)
- Window show/hide: System default
- Scroll: System default smooth scroll

### Component Specifications

#### Search Field
```
┌─────────────────────────────────────┐
│  🔍 Search...                       │  Height: ~28px
└─────────────────────────────────────┘
Padding: 5px | Border Radius: 4px
Background: White @ 10% (dark) / Black @ 6% (light)
```

#### List Item (Unselected)
```
┌─────────────────────────────────────┐
│  Clipboard text content...      ⌘1 │  Height: 24px
└─────────────────────────────────────┘
Padding: 0px 10px | Background: Transparent
```

#### List Item (Selected)
```
┌─────────────────────────────────────┐
│  Clipboard text content...  📌 🗑 ⌘1│  Height: 24px
└─────────────────────────────────────┘
Padding: 0px 10px | Background: Accent @ 80%
Text: White | Icons: White @ 70%
```

#### Footer Menu Item
```
┌─────────────────────────────────────┐
│  Clear History              ⌥⌘⌫    │  Height: 24px
└─────────────────────────────────────┘
Padding: 0px 10px | Same styling as list items
```

### Icon Specifications

#### Pin Icon
- Size: 10px
- Color: Blue accent (#2196F3)
- Position: Left of text, 4px margin

#### Hover Icons (Delete, Pin)
- Size: 14px
- Color: White @ 70% (base), White @ 100% (hover)
- Hover Color (Delete): Red accent (#F44336)
- Spacing: 8px between icons

#### File Type Icons
- Size: 14px
- Color: Matches text color
- Margin Right: 6px

### Keyboard Shortcuts Display

```
Format: ⌘1, ⌘2, ⌥⌘⌫, ⌘,, ⌘Q
Font Size: 12px
Color: Tertiary text color
Alignment: Right
```

### Image Preview

```
┌─────────────────────────────────────┐
│  [Image Preview]                    │  Height: Variable (setting)
└─────────────────────────────────────┘
Max Height: Configurable (default: 40px)
Fit: Contain
Alignment: Left
```

## Pixel-Perfect Checklist

### Layout ✓
- [x] Window corner radius: 6px
- [x] Item height: 24px
- [x] Horizontal padding: 10px
- [x] Vertical padding: 5px
- [x] Divider height: 6px

### Colors ✓
- [x] Background opacity: 85%
- [x] Selection color: Accent @ 80%
- [x] Text colors match
- [x] Border colors match

### Effects ✓
- [x] Windows blur/acrylic enabled
- [x] Transparency working
- [x] Window shadow enabled

### Typography ✓
- [x] Font sizes correct
- [x] Line heights correct
- [x] Text overflow handling

### Interactions ⚠️
- [x] Hover states
- [x] Selection states
- [ ] Keyboard navigation
- [ ] Smooth scrolling

### Features ⚠️
- [x] Search functionality
- [x] Pin/unpin items
- [x] Delete items
- [ ] Application icons
- [ ] Image previews
- [ ] Preview popover

## Browser Testing

Test the following scenarios:
1. Dark mode vs Light mode
2. Different window sizes
3. Long text overflow
4. Image items
5. File items
6. Pinned items at top
7. Search with highlighting
8. Keyboard shortcuts
9. Hover interactions
10. Multi-monitor positioning

## Performance Targets

- Window show time: < 100ms
- Search response: < 50ms (with debounce)
- Scroll FPS: 60fps
- Memory usage: < 100MB for 1000 items
