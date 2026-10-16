# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com) and the
project adheres to [Semantic Versioning](https://semver.org).

## 0.3.0 - 2026-10-16

### Breaking

- Requires **Flutter 3.47 / Dart 3.13** and **macOS 12** (unchanged) because
  `screen_retriever` was bumped to `^0.3.0` (nativeapi based). Window
  placement (`WindowOptions(center: true)`, `setAlignment`) now reads
  `DisplayManager` directly; results for named alignments are unchanged.
  Linux builds now also need `libx11-dev` and `libxi-dev` (nativeapi
  build hook, found via `pkg-config`).
- **Windows `customFrame` redesign.** The whole window is client area
  again (0.2.x kept real non-client bands on the left/right/bottom and a
  1px strip on top). Consequences:
  - **Windows 10 shows no accent/theme-colored border line** on any edge
    (normal, maximized, inactive) - the 0.2.x 1px top strip was exactly
    that line.
  - **Windows 11** keeps the DWM rounded corners and 1px border
    (`DWMWCP_ROUND` is requested only on build >= 22000, detected at
    runtime via `RtlGetVersion`).
  - `WS_CAPTION` is removed while the frame is enabled and restored on
    disable (code reading `GWL_STYLE` sees the difference).
  - The `WM_GETMINMAXINFO` interception is gone; the upstream
    window_manager branch still applies `setMinimumSize` /
    `setMaximumSize`. If you relied on 0.2.x clamping behavior there,
    nothing changes for normal apps.

### Added

- **Equal resize thickness** on all four edges and corners on Windows
  (`SM_CXSIZEFRAME + SM_CXPADDEDBORDER` for the window's current DPI,
  so it follows `WM_DPICHANGED` and multi-monitor moves). The Flutter
  view is subclassed (`SetWindowSubclass`) and returns `HTTRANSPARENT` on
  band hits so the top level answers. No bands when maximized,
  fullscreen or non-resizable.
- Maximized windows fill the monitor work area exactly; on monitors with an
  auto-hide taskbar 1px is left free on that edge so the taskbar can still
  be revealed (Windows Terminal / Chromium recipe).
- Smoother interactive resize: `WM_ERASEBKGND` swallowed, `WVR_REDRAW`
  during size-move (no stale-pixel BitBlt), no non-client repaint on
  activation changes.
- `windowManager.isCustomFrame()` (all platforms).
- `windowManager.detectWindowManagerConflict()`; `ensureInitialized()`
  logs a warning once when the original `window_manager` plugin is also
  registered.
- `ResizeDiagnostics` (debug/profile, opt-in): frame times and
  surface-vs-window size mismatches during interactive resize.
- Native gtest suite for the pure frame geometry
  (`windows/frame_geometry.h`): Win10/Win11 policy, normal / maximized /
  fullscreen, DPI 96-192, edges vs corners.
- Example integration test (`example/integration_test/frame_kit_test.dart`)
  run in CI on Windows, macOS and Linux (xvfb).
- CI: windows-11-arm, macos-15-intel, Flutter beta early-warning job;
  tag-triggered pub.dev publishing via OIDC; SECURITY.md, issue templates.

### Known limitations

- Windows 11 snap layouts flyout on a self-drawn maximize button is not
  provided (needs `HTMAXBUTTON` hit-testing over a Dart-drawn button);
  Win+Z and drag-to-top-edge snapping work.

## 0.2.2 - 2026-09-06

### Fixed

- **Windows**: corner codes (`HTTOPLEFT` / `HTTOPRIGHT`) win inside the top
  band, so top-corner resizing behaves like the native side-band corners.

## 0.2.1 - 2026-09-05

### Fixed

- **Windows**: the top band shrank by the full resize width, making the
  system paint a titlebar-black strip above the self-drawn titlebar. The
  top now keeps exactly **1px** of non-client area (the DWM border line,
  bitsdojo's `top -= 1` recipe), and top-edge resize is served by a
  self-computed `HTTOP` over the client top band.

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
