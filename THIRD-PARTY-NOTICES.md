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
- Scope: the frame-takeover concepts (borderless `WM_NCCALCSIZE` route and
  native `WM_NCHITTEST` edge resizing) are re-implemented in
  `windows/frame_controller.{h,cpp}`; bitsdojo's unconditional
  `return 0` in `WM_GETMINMAXINFO` is deliberately NOT reproduced
  (cooperative merge instead).
