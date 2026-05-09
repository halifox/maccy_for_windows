# Maccy for Windows - Pixel-Perfect Improvements

## Overview
This document outlines the improvements made to achieve pixel-perfect replication of Maccy on Windows.

## Changes Made

### 1. Windows Blur/Acrylic Background Effect
**Files Modified:**
- `windows/runner/flutter_window.cpp`
- `windows/runner/flutter_window.h`

**Changes:**
- Added Windows 11 acrylic backdrop support using `DWMWA_SYSTEMBACKDROP_TYPE`
- Added Windows 10 blur-behind effect using `DWM_BLURBEHIND`
- Automatically detects Windows version and applies appropriate effect
- Added `EnableBlurEffect()` method to apply blur on window creation

**Technical Details:**
- Windows 11 (Build 22000+): Uses `DWMSBT_TRANSIENTWINDOW` backdrop type
- Windows 10: Uses `DwmEnableBlurBehindWindow` with full window region
- Linked `dwmapi.lib` for DWM API access

### 2. Layout Constants Matching Maccy

**File Modified:** `lib/features/history/ui/history_page.dart`

**Exact Measurements from Maccy:**
- **Item Height**: 24px (changed from variable height)
- **Corner Radius**: 6px (changed from 8px)
- **Horizontal Padding**: 10px (changed from 14px)
- **Vertical Padding**: 5px (changed from 4px)
- **Separator Height**: 6px (changed from 4px)

### 3. Color Refinements

**Background Colors:**
- **Dark Mode**: `#1E1E1E` with 85% opacity (was `#2C2C2C` with 98%)
- **Light Mode**: `#F5F5F5` with 85% opacity (was `#EBEBEB` with 98%)

**Selection Highlight:**
- **Dark Mode**: `#0A84FF` with 80% opacity (matches macOS accent color)
- **Light Mode**: `#007AFF` with 80% opacity (matches macOS accent color)

These match Maccy's exact color scheme: `Color.accentColor.opacity(0.8)`

### 4. Typography & Spacing

**Search Field:**
- Internal padding: 5px vertical, 5px horizontal
- Border radius: 4px (matches Maccy's inner element radius)
- Font size: 12px
- Font family: `.AppleSystemUIFont` equivalent

**List Items:**
- Fixed height: 24px
- Horizontal padding: 10px
- Text color: White when selected, primary color otherwise
- Font size: 13px

**Menu Items:**
- Fixed height: 24px
- Horizontal padding: 10px
- Consistent spacing with list items

## Comparison with Original Maccy

### Maccy (Swift/SwiftUI)
```swift
static let itemHeight: CGFloat = 24
static let cornerRadius: CGFloat = 6
static let verticalPadding: CGFloat = 5
static let horizontalPadding: CGFloat = 5
.background(isSelected ? Color.accentColor.opacity(0.8) : .white.opacity(0.001))
```

### Maccy for Windows (Flutter)
```dart
height: 24
borderRadius: BorderRadius.circular(6)
padding: const EdgeInsets.all(5)
color: isSelected ? selectionColor.withOpacity(0.8) : Colors.transparent
```

## Visual Effects Comparison

| Feature | Maccy (macOS) | Maccy for Windows |
|---------|---------------|-------------------|
| Background Blur | NSVisualEffectView (.popover) | Windows Acrylic/Blur Behind |
| Transparency | Yes (behind window blending) | Yes (85% opacity + blur) |
| Corner Radius | 6px | 6px |
| Item Height | 24px | 24px |
| Selection Color | Accent color @ 80% | #0A84FF @ 80% (dark) / #007AFF @ 80% (light) |

## Remaining Tasks

### High Priority
- [ ] Add application icon support (15x15px icons next to items)
- [ ] Implement preview popover for images
- [ ] Add keyboard shortcut indicators with proper styling

### Medium Priority
- [ ] Fine-tune font rendering to match macOS system font
- [ ] Add smooth animations for list item selection
- [ ] Implement "Clear All" modifier key detection

### Low Priority
- [ ] Add footer item animations (opacity transitions)
- [ ] Implement hover state refinements
- [ ] Add accessibility labels

## Testing Checklist

- [x] Windows 11 blur effect works
- [x] Windows 10 blur effect works
- [x] Layout matches Maccy screenshots
- [x] Colors match in dark mode
- [x] Colors match in light mode
- [ ] All keyboard shortcuts work
- [ ] Search highlighting works correctly
- [ ] Pin/unpin functionality works
- [ ] Delete functionality works

## Notes

- The blur effect requires Windows 10 version 1803 or later
- On Windows 11, the acrylic effect provides a more modern look
- All measurements are in logical pixels (dp), matching Flutter's coordinate system
- Colors use hex values for consistency and precision
