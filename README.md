# window_frame_kit

A Flutter desktop plugin that combines **frame takeover** (self-drawn title
bar, frameless window, native equal-width edge resizing) with the **full
window management API** of `window_manager` (event streams, geometry,
always-on-top, close prevention, and more) in a single package, for
Windows, Linux, and macOS.

Frame plugins and window-manager plugins normally fight over the same
native window messages; window_frame_kit merges them into one plugin with
**one** window-procedure delegate, and ships a **C++-only Windows
implementation with no `win32` Dart dependency**.

## Platforms

| Platform | Architectures | Status |
|----------|---------------|--------|
| Windows 10 / 11 | x64 (arm64: see CI coverage) | Reference implementation; CI build + native tests + integration test |
| macOS 12+ | arm64, x64 | CI build + integration test |
| Linux (GTK 3, X11 / Wayland) | x64 (arm64: see CI coverage) | CI build + integration test under Xvfb |

Desktop only — there are no mobile or web targets.

## Getting started

```yaml
dependencies:
  window_frame_kit: ^0.3.0
```

Requires Flutter 3.47+ (Dart 3.13+). Do **not** also depend on
`window_manager` — this package exposes the same `windowManager` facade
(see [Coexistence](#coexistence-with-window_manager)).

```dart
import 'package:window_frame_kit/window_frame_kit.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();
  const options = WindowOptions(
    size: Size(1280, 720),
    minimumSize: Size(854, 480),
    titleBarStyle: TitleBarStyle.hidden,
    customFrame: true, // frame takeover, applied before first show
  );
  windowManager.waitUntilReadyToShow(options, () async {
    await windowManager.show();
    await windowManager.focus();
  });
  runApp(const MyApp());
}
```

Draw your own title bar and wrap it in `DragToMoveArea` (drag to move,
double-click to maximize/restore). `windowManager.setCustomFrame(bool)`
toggles the frame at runtime; `windowManager.isCustomFrame()` reports it.

## What `customFrame` does per platform

### Windows

| | Windows 10 | Windows 11 |
|---|---|---|
| System border / accent line | **none** on any edge (normal, maximized, inactive) | DWM 1px border kept |
| Corners | square (Win10 has no rounding) | native rounded (`DWMWCP_ROUND`) |
| Drop shadow | DWM shadow (1px top frame margin) | DWM shadow |
| Resize bands | equal on all 4 edges + 4 corners | same |

The OS version is decided **at runtime** (`RtlGetVersion`, build >= 22000
is Windows 11); Win11-only DWM attributes are never sent to Windows 10.

How it works (see `windows/frame_geometry.h`, unit-tested without a
window):

- `WM_NCCALCSIZE` makes the whole window client area, pushing the client
  top 1px above the window so the 1px DWM shadow margin is never visible.
  There is no non-client strip left for Windows 10 to paint in the accent
  color. `WS_CAPTION` is removed while enabled (so the resize loop cannot
  repaint a themed caption) and restored on disable; `WS_THICKFRAME` and
  `WS_MAXIMIZEBOX` stay, which Aero Snap needs.
- `WM_NCHITTEST` computes resize bands of
  `SM_CXSIZEFRAME + SM_CXPADDEDBORDER` for the window's **current** DPI on
  every edge and corner (8px at 100%, 12px at 150%, 16px at 200%), so the
  top edge is exactly as easy to grab as the others and the band follows
  per-monitor DPI changes. The Flutter view is subclassed and returns
  `HTTRANSPARENT` on band hits.
- Maximized: the client is the monitor work area (the window itself
  overhangs the monitor, as all maximized windows do); on an edge with an
  auto-hide taskbar 1px is left free so the taskbar can still be revealed.
  No resize bands.
- Fullscreen (the plugin's own or an external one that strips
  `WS_OVERLAPPEDWINDOW`): no frame, no bands, `SC_SIZE` blocked.
- Interactive resize: `WM_ERASEBKGND` is swallowed and `WM_NCCALCSIZE`
  returns `WVR_REDRAW` during a size-move so stale client pixels are not
  blitted to the new origin; activation changes do not repaint the
  non-client area.

Windows 11 *snap layouts* (the flyout on hovering a maximize button) are
not provided for a self-drawn maximize button.

### macOS

`fullSizeContentView` + transparent, hidden title bar while keeping the
`.titled` style bit (the window stays key/main-capable). The traffic
lights stay native; hide them with
`setTitleBarStyle(TitleBarStyle.hidden, windowButtonVisibility: false)`.
Edge resizing is native AppKit and already equal on all sides.

### Linux

GTK decorations are turned off (`gtk_window_set_decorated(false)`,
reversible). An undecorated GTK window has no resize border of its own, so
wrap your content in `DragToResizeArea` (or `VirtualWindowFrame`, which
does it for you): each edge calls `gtk_window_begin_resize_drag` with the
same `resizeEdgeSize` on all four sides.

- **X11**: everything works, including `setPosition` / `center`.
- **Wayland**: resizing and moving work through the compositor
  (`begin_resize_drag` / `begin_move_drag`), but clients cannot position
  themselves — `setPosition`, `setAlignment` and `center` are ignored by
  the compositor. Shadows and rounded corners are compositor policy.

## Resize diagnostics

`ResizeDiagnostics` (debug/profile builds; no-op in release) measures
interactive resizing instead of eyeballing it: for every frame rendered
during a resize it records the frame time and whether the rendered surface
size matched the native window size.

```dart
final diagnostics = ResizeDiagnostics(
  onReport: (report) => debugPrint(report.toString()),
)..start();
// drag a window edge; each finished gesture prints e.g.
// ResizeDiagnostics: 212 frames, 3 mismatched (1.4%),
//   frame time p50 4.1 ms / p95 9.8 ms / max 21.3 ms
diagnostics.stop();
```

A mismatch means a frame was presented at the previous window size
(stretched or clipped content). Frame timings arrive in engine batches, so
treat the ratio as a trend. Compare only with `customFrame` or a hidden
title bar (otherwise the surface is smaller than the window by design).
`ResizeDiagnosticsRecorder` exposes the same logic for custom sources.

## Coexistence with window_manager

| | window_manager 0.5.2 | window_frame_kit 0.3.0 |
|---|---|---|
| `windowManager` facade, `WindowListener`, 15 events, `WindowOptions` | ✓ | identical (verbatim port, CI-diffed) |
| `DragToMoveArea`, `DragToResizeArea`, `WindowCaption`, `VirtualWindowFrame` | ✓ | identical |
| `calcWindowPosition` | screen_retriever 0.2 | screen_retriever 0.3 (same results) |
| `TitleBarStyle.hidden` without `customFrame` | upstream behavior | unchanged |
| `customFrame` / `setCustomFrame` / `isCustomFrame` | — | ✓ |
| `ResizeDiagnostics`, `detectWindowManagerConflict` | — | ✓ |
| Method channel | `window_manager` | `window_frame_kit` |

Migrating is an import change:
`package:window_manager/window_manager.dart` →
`package:window_frame_kit/window_frame_kit.dart`.

**Do not register both.** They use different channels, so Dart calls do
not collide, but both plugins install a top-level window-procedure
delegate that answers `WM_NCCALCSIZE` / `WM_NCHITTEST` / `WM_NCACTIVATE`;
the result then depends on plugin registration order. `ensureInitialized()`
detects the other plugin once and logs a warning; call
`windowManager.detectWindowManagerConflict()` to check explicitly.

## Migrating from 0.2.x

- Raise your Flutter constraint to 3.47 (screen_retriever 0.3.0).
- Windows `customFrame`: the 0.2.x non-client side bands and 1px top strip
  are gone (no Win10 accent line any more; the client is now the full
  window). If you padded your UI to compensate for the 0.2.x bands, remove
  the padding. Code inspecting `GWL_STYLE` will see `WS_CAPTION` cleared
  while the frame is enabled.
- New names exported from the barrel (`ResizeDiagnostics`,
  `ResizeDiagnosticsReport`, `ResizeDiagnosticsRecorder`,
  `ResizeFrameSample`, `DisplayArea`, `selectDisplayArea`,
  `alignWindowInWorkArea`, `kWindowManagerConflictWarning`) may shadow
  identically named app symbols; use `hide` if needed.

## CI coverage

| Runner | analyze + tests | native build | native gtest | integration test |
|---|---|---|---|---|
| windows-latest (x64) | ✓ | ✓ | ✓ | ✓ |
| windows-11-arm | ✓ (x64 SDK, emulated) | — (Windows arm64 target experimental in Flutter) | — | — |
| macos-latest (arm64) | ✓ | ✓ | — | ✓ |
| macos-15-intel (x64) | ✓ | ✓ | — | ✓ |
| ubuntu-latest (x64) | ✓ | ✓ | — | ✓ (Xvfb + xfwm4) |
| ubuntu-24.04-arm | not run: Flutter stable ships no Linux arm64 host SDK | | | |
| Flutter beta (Windows) | early warning, non-blocking | | | |

What CI cannot see: visual results (accent line, corners, shadow,
tearing) — those need a real Windows 10 and Windows 11 desktop.

## Platform support matrix

Legend: ✓ supported · ✗ not supported (typed `UnimplementedError` /
channel-level `NotImplementedError` per platform) · ❓ ported from upstream
0.5.2 and CI-compiled, **not yet smoke-tested on a real machine** (the
CI integration test covers geometry, maximize/restore, fullscreen and the
custom frame on all three platforms).

| Method | Windows | Linux | macOS |
|---|---|---|---|
| `ensureInitialized` | ✓ | ❓ | ❓ |
| `getId` | ✓ | ✗ | ❓ |
| `setAsFrameless` | ✓ | ❓ | ❓ |
| `setCustomFrame` / `isCustomFrame` | ✓ | ✓ | ✓ |
| `destroy` / `close` | ✓ | ❓ | ❓ |
| `isPreventClose` / `setPreventClose` | ✓ | ❓ | ❓ |
| `focus` / `blur` / `isFocused` | ✓ | ❓ | ❓ |
| `show` / `hide` / `isVisible` | ✓ | ❓ | ❓ |
| `maximize` / `unmaximize` / `isMaximized` | ✓ | ❓ | ❓ |
| `minimize` / `restore` / `isMinimized` | ✓ | ❓ | ❓ |
| `isDockable` / `isDocked` / `dock` / `undock` | ✓ | ✗ | ✗ |
| `isFullScreen` / `setFullScreen` | ✓ | ❓ | ❓ |
| `setAspectRatio` | ✓ | ❓ | ❓ |
| `setBackgroundColor` | ✓ | ❓ | ❓ |
| `getBounds` / `setBounds` | ✓ | ❓ | ❓ |
| `setAlignment` / `center` | ✓ | ❓ | ❓ |
| `getPosition` / `setPosition` | ✓ | ❓ | ❓ |
| `getSize` / `setSize` | ✓ | ❓ | ❓ |
| `getMinimumSize` / `setMinimumSize` | ✓ | ❓ | ❓ |
| `getMaximumSize` / `setMaximumSize` | ✓ | ❓ | ❓ |
| `isResizable` / `setResizable` | ✓ | ❓ | ❓ |
| `isMovable` / `setMovable` | ✓ | ❓ | ❓ |
| `isMinimizable` / `setMinimizable` | ✓ | ❓ | ❓ |
| `isMaximizable` / `setMaximizable` | ✓ | ❓ | ❓ |
| `isClosable` / `setClosable` | ✓ | ❓ | ❓ |
| `isAlwaysOnTop` / `setAlwaysOnTop` | ✓ | ❓ | ❓ |
| `isAlwaysOnBottom` / `setAlwaysOnBottom` | ✓ | ❓ | ✗ |
| `getTitle` / `setTitle` | ✓ | ❓ | ❓ |
| `setTitleBarStyle` / `getTitleBarHeight` | ✓ | ❓ | ❓ |
| `isSkipTaskbar` / `setSkipTaskbar` | ✓ | ❓ | ✗ |
| `setProgressBar` | ✓ | ✗ | ❓ |
| `setIcon` | ✓ | ❓ | ✗ |
| `setBadgeLabel` | ✗ | ✗ | ❓ |
| `isVisibleOnAllWorkspaces` / `setVisibleOnAllWorkspaces` | ✗ | ✗ | ❓ |
| `hasShadow` / `setHasShadow` | ✓ | ❓ | ❓ |
| `getOpacity` / `setOpacity` | ✓ | ❓ | ❓ |
| `setBrightness` | ✓ | ❓ | ❓ |
| `setIgnoreMouseEvents` | ✓ | ❓ | ❓ |
| `popUpWindowMenu` | ✓ | ❓ | ✗ |
| `startDragging` | ✓ | ❓ | ❓ |
| `startResizing` | ✓ | ❓ | ✗ |
| `grabKeyboard` / `ungrabKeyboard` | ✗ | ❓ | ✗ |

Widgets (`DragToMoveArea`, `DragToResizeArea`, `WindowCaption`, …) are pure
Dart and platform-independent. `window_frame_kit` also exposes the upstream
15-window-event stream through `WindowListener`.

## Migrating from the bitsdojo_window + window_manager pair

| Before (two packages) | After (window_frame_kit) |
|---|---|
| `bitsdojo_window_configure(BDW_CUSTOM_FRAME)` in `main.cpp` | Delete it — `WindowOptions(customFrame: true)` / `setCustomFrame(true)` |
| `WindowBorder(color: ..., child: ...)` wrapper | Delete the wrapper; the native frame handles hit-testing |
| `doWhenWindowReady(() => appWindow.minSize = ...)` | `WindowOptions(minimumSize: ...)` or `windowManager.setMinimumSize(...)` — now truly enforced |
| `appWindow.maxSize = ...` | `windowManager.setMaximumSize(...)` |
| `windowManager.*` calls and `WindowListener` | Unchanged — same facade, same event stream |
| `windowManager.ensureInitialized()` | Unchanged |

The dependency tree drops both `bitsdojo_window` (and its locked `win32 5.x`)
and `window_manager` — one import, one plugin, one message handler.

## Relation to Flutter's official windowing APIs

Flutter's multi-window APIs (the `windowing` feature) target *multiple
window instances* per app. window_frame_kit targets *chrome takeover of the
single main window*: borderless drawing, native edge resize, min/max
constraints and a full state/event facade. The two compose — keep using the
official APIs for auxiliary windows and use window_frame_kit for the main
window's frame and lifecycle.

## Derivation

window_frame_kit is a fork-and-graft package, built in the open with full
attribution:

- **Base**: a verbatim port of
  [window_manager 0.5.2](https://github.com/leanflutter/window_manager)
  (MIT, © LiJianying) — the full capability API and event system. The
  `upstream` git remote points at this repository so upstream changes can
  be cherry-picked or diffed.
- **Frame graft**: the frame-takeover techniques (full-client
  `WM_NCCALCSIZE` with a 1px DWM margin, `WM_NCHITTEST` resize bands,
  child `HTTRANSPARENT` forwarding) are re-implemented from
  [bitsdojo_window 0.1.6](https://github.com/bitsdojo/bitsdojo_window)
  (MIT, © Bogdan Hobeanu). Grafted lines carry `// FRAME:` markers.
- **Intentional deviations** from upstream — e.g. not reproducing
  bitsdojo's unconditional `WM_GETMINMAXINFO` `return 0` (which silently
  broke `setMinimumSize`) — are tracked in
  [DEVIATIONS.md](DEVIATIONS.md). When merging upstream, every ledger
  entry is checked so a deviation is never silently overwritten.

## License

MIT — see [LICENSE](LICENSE). The license stacks the copyright notices of
both upstream projects (window_manager, bitsdojo_window) alongside this
package's own line.
