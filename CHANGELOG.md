# 变更记录

## Unreleased

### Changed

- Updated MSVC runtime detection to sort by version numerically, with improved compatibility checks and fallback logic

### Fixed

- Added check to ensure MSVC runtime is not older than the toolset version

### Security

- Added option to fail if MSVC runtime DLLs cannot be bundled

