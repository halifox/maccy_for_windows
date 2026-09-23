# 变更记录

## Unreleased

### Added

- Added visual indicator for search clear button in history view

### Changed

- Added support for Ctrl+U shortcut to clear search input, with improved keyboard navigation and focus handling
- Improved search clear button interaction with mouse and keyboard and visual feedback
- Updated MSVC runtime detection to sort by version numerically, with improved compatibility checks and fallback logic, including enhanced error handling

### Fixed

- Added check to ensure MSVC runtime is not older than the toolset version

### Security

- Added option to fail if MSVC runtime DLLs cannot be bundled

