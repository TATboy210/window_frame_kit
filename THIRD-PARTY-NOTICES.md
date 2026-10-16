# Third-Party Notices

window_frame_kit is a fork-and-graft package. Substantial portions of its
code are derived from the following upstream projects; their copyright
notices are also stacked in [LICENSE](LICENSE) and every intentional
deviation is tracked in [DEVIATIONS.md](DEVIATIONS.md).

## window_manager 0.5.2

- https://github.com/leanflutter/window_manager
- Copyright (c) 2022-present LiJianying
- License: MIT
- Scope: the full Dart facade (`lib/`), the Windows plugin and window
  manager implementation (`windows/`), the Linux GTK plugin
  (`linux/`), and the macOS AppKit plugin (`macos/`) are verbatim
  ports with mechanical package-name normalization; the frame-graft
  additions are marked `// FRAME:` and ledgered.

## bitsdojo_window 0.1.6

- https://github.com/bitsdojo/bitsdojo_window
- Copyright (c) 2020-2021 Bogdan Hobeanu
- License: MIT
- Scope: the frame-takeover techniques are re-implemented (no code copied
  verbatim) in `windows/frame_geometry.h` and
  `windows/frame_controller.{h,cpp}`: the full-client `WM_NCCALCSIZE`
  mapping with the client top pushed 1px above the window plus a 1px top
  `DwmExtendFrameIntoClientArea` margin, `WM_NCHITTEST` resize bands sized
  `SM_CXSIZEFRAME + SM_CXPADDEDBORDER` for the window DPI, the
  FLUTTERVIEW child returning `HTTRANSPARENT` on band hits, and swallowing
  `WM_ERASEBKGND`. bitsdojo's unconditional `return 0` in
  `WM_GETMINMAXINFO` is deliberately NOT reproduced.

## Techniques referenced (no code copied)

- Windows Terminal (MIT, (c) Microsoft Corporation) and Chromium (BSD-3,
  (c) The Chromium Authors): maximized client = monitor work area with
  1px left free on auto-hide taskbar edges (`ABM_GETAUTOHIDEBAREX`), and
  `WVR_REDRAW` from `WM_NCCALCSIZE` during an interactive size-move.
  These are publicly documented Win32 behaviors re-implemented from the
  description; no source from either project is included.
