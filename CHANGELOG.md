# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com) and the
project adheres to [Semantic Versioning](https://semver.org).

## 0.2.0 - 2026-09-05

### Changed

- **Windows**: `customFrame` redesigned from pure borderless (client
  collapses to the full window rect) to **equal-width NC bands** — the
  client shrinks by one DPI-scaled band on all four sides, giving:
  - self-drawn titlebar **plus** the system border (Win11 1px border and
    rounded corners survive, `WS_CAPTION` is never touched);
  - uniform 4-edge + 4-corner native resize bands (including the top edge,
    which upstream window_manager's `hidden` branch leaves dead on Win11);
  - no themed-border flash during drag-resize (the band is real
    non-client area, so the resize loop repaints the band itself);
  - fullscreen (both the plugin's own and an external `WS_OVERLAPPEDWINDOW`
    strip) collapses the band entirely — no NC area, no resize.
- No child-window subclass needed anymore (edge hit-testing is native);
  the child subclass from 0.1.x is gone.

## 0.1.1 - 2026-09-05

### Fixed

- **Windows**: the themed border flashed back during a drag-resize. The
  graft now drops `WS_CAPTION` (keeping `WS_THICKFRAME` for native edge
  resizing) while enabled and restores the exact pre-graft style on
  disable — the Chromium/Electron frameless recipe. Without it,
  `DefWindowProc` repainted the non-client area computed from
  `WS_CAPTION` inside the resize modal loop, ignoring the
  `WM_NCCALCSIZE` return 0 mapping.

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
