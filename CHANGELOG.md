# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com) and the
project adheres to [Semantic Versioning](https://semver.org).

## 0.1.0 - 2026-09-05

Initial public release.

### Added

- Verbatim port of window_manager 0.5.2's full Dart facade and event system
  (`windowManager`, `WindowListener`, 15 window events, 60+ methods), with
  the native Windows / Linux / macOS implementations ported mechanically.
- **Custom frame takeover** (`WindowOptions(customFrame: true)` or
  `windowManager.setCustomFrame(bool)`, default off):
  - Windows: borderless `WM_NCCALCSIZE` (no themed border, no Win11 8px top
    inset), per-monitor-DPI four-edge/four-corner `WM_NCHITTEST` resizing
    (top edge included), cooperative `WM_GETMINMAXINFO` (minimum/maximum
    size enforced natively; maximized windows clamp to the monitor work
    area), and resizing force-disabled in fullscreen — covering both the
    plugin's own fullscreen state and an external fullscreen path that
    strips `WS_OVERLAPPEDWINDOW`.
  - Linux: reversible `gtk_window_set_decorated` toggle with snapshot
    restore.
  - macOS: reversible `fullSizeContentView` + transparent-titlebar toggle
    that keeps the `.titled` bit so the window stays key-capable.
- C++-only Windows implementation — the package has **no `win32` Dart
  dependency**, unlocking `win32 6.x`/`file_picker 12.x`/
  `flutter_secure_storage 11.x` in downstream apps.
- `DragToMoveArea`, `DragToResizeArea`, `WindowCaption` widgets from the
  upstream port.
