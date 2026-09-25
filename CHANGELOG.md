# Changelog

## [1.0.5](https://github.com/halifox/maccy_for_windows/compare/v1.0.4...v1.0.5) (2026-09-25)


### Bug Fixes

* **release-test:** exercise patch release ([5c457d9](https://github.com/halifox/maccy_for_windows/commit/5c457d9dd2529826747e31c4e33d9944291b67b9))
* **release-test:** exercise patch release ([eb391eb](https://github.com/halifox/maccy_for_windows/commit/eb391ebb4eb305e3aff56e5641ff0477af22530b))

## [1.0.4](https://github.com/halifox/maccy_for_windows/compare/v1.0.3...v1.0.4) (2026-09-25)


### Bug Fixes

* **release-test:** exercise patch release ([bc276e2](https://github.com/halifox/maccy_for_windows/commit/bc276e2c3ff713689b03e352f482f20566ebf427))
* **release-test:** exercise patch release ([704a75f](https://github.com/halifox/maccy_for_windows/commit/704a75f3ac6003830a9905345f10dd10ef8ba8ee))

## 变更记录

## Unreleased

### Breaking Changes

- Removed direct clipboard monitor usage from main window, replaced with application controller
- Removed direct update checker usage from main window, replaced with application controller

### Added

- Added application controller to manage clipboard and preview operations, with improved event handling and reduced direct dependencies
- Added experimental tray icon management with improved icon loading and updating
- Added visual indicator for search clear button in history view, with improved interaction and feedback

### Changed

- Updated clipboard handling to use new application controller architecture
- Improved tray icon management with new class-based implementation
- Refactored clipboard monitoring with safer global memory handling
- Added support for Ctrl+U shortcut to clear search input with improved keyboard navigation and focus handling, now works with new application controller architecture
- Improved search clear button interaction with mouse and keyboard with visual feedback
- Updated MSVC runtime detection to sort by version numerically with improved compatibility checks and fallback logic

### Fixed

- Fixed clipboard write operation to use new application controller architecture
- Fixed clipboard clear operation to use new application controller architecture
- Added check to ensure MSVC runtime is not older than the toolset version

### Security

- Added secure handling of global memory buffers in clipboard operations
- Added option to fail if MSVC runtime DLLs cannot be bundled
