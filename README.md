# window_frame_kit

A Flutter plugin that combines **frame takeover** (frameless window + native
four-edge drag resize) with a **full window management API** (event streams,
geometry, always-on-top, close prevention, and more) in a single package —
for Windows, Linux, and macOS desktop apps.

One package, both halves: until now, Flutter desktop apps that wanted a
borderless window *and* a listenable window had to combine two packages
(frame plugins and window-manager plugins fight over the same Win32
messages). window_frame_kit merges them — and ships with a **C++-only,
zero-`win32`-package dependency tree**, so your app is never locked to an
outdated `win32` major version.

## Platforms

| Platform | Status |
|----------|--------|
| Windows  | Reference implementation |
| Linux    | Supported (GTK); CI-compiled, real-machine smoke pending |
| macOS    | Supported (AppKit); CI-compiled, real-machine smoke pending |

Desktop only — there are no mobile or web targets.

## Getting started

```yaml
dependencies:
  window_frame_kit: ^0.1.0
```

The single import exposes the same `windowManager` facade you already know:

```dart
import 'package:window_frame_kit/window_frame_kit.dart';
```

### Enabling the custom frame

Pass `customFrame: true` in `WindowOptions` — it is applied **after**
`titleBarStyle` and **before** any fullscreen/maximized state restore, so the
window starts borderless on first paint (no themed border, no Win11 8px top
inset):

```dart
Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();
  const options = WindowOptions(
    size: Size(1280, 720),
    minimumSize: Size(854, 480),   // enforced natively, see below
    titleBarStyle: TitleBarStyle.hidden,
    customFrame: true,             // <-- frame takeover
  );
  windowManager.waitUntilReadyToShow(options, () async {
    await windowManager.show();
    await windowManager.focus();
  });
  runApp(const MyApp());
}
```

When enabled, the native `FrameController` owns the window frame:

- **Borderless** `WM_NCCALCSIZE` — no system themed border, no Win11 8px top
  inset; maximized windows clamp to the monitor work area (no taskbar
  overlap, no cropped content).
- **Four-edge / four-corner resize** via `WM_NCHITTEST` with per-monitor-DPI
  hit bands — including the top edge, which plain `WM_NCCALCSIZE` route
  cannot reach because the Flutter view covers the whole client area.
- **Cooperative `WM_GETMINMAXINFO`** — `setMinimumSize` / `setMaximumSize`
  are enforced natively (this fixes bitsdojo_window's unconditional
  `return 0`, which silently defeated them).
- **Fullscreen is never resizable** — both the plugin's own fullscreen state
  and an external fullscreen path that strips `WS_OVERLAPPEDWINDOW` (e.g. a
  video player's native fullscreen route) make every edge inert.

Prefer a runtime toggle? Call `windowManager.setCustomFrame(true / false)`
any time — the decoration state is snapshotted and restored on both
platforms, so the toggle is reversible.

## Platform support matrix

Legend: ✓ supported · ✗ not supported (typed `UnimplementedError` /
channel-level `NotImplementedError` per platform) · ❓ ported from upstream
0.5.2 and CI-compiled, **not yet smoke-tested on a real machine**.

| Method | Windows | Linux | macOS |
|---|---|---|---|
| `ensureInitialized` | ✓ | ❓ | ❓ |
| `getId` | ✓ | ✗ | ❓ |
| `setAsFrameless` | ✓ | ❓ | ❓ |
| `setCustomFrame` | ✓ | ❓ | ❓ |
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
- **Frame graft**: the frame-takeover concepts (borderless via
  `WM_NCCALCSIZE`, native edge resize via `WM_NCHITTEST`) are grafted from
  [bitsdojo_window 0.1.6](https://github.com/bitsdojo/bitsdojo_window)
  (MIT, © Bogdan Hobeanu). Grafted lines carry `// FRAME:` markers.
- **Intentional deviations** from upstream — including the cooperative
  `GETMINMAXINFO` rewrite that fixes bitsdojo's unconditional `return 0`
  (which silently broke `setMinimumSize`) — are tracked in
  [DEVIATIONS.md](DEVIATIONS.md). When merging upstream, every ledger
  entry is checked so a deviation is never silently overwritten.

## License

MIT — see [LICENSE](LICENSE). The license stacks the copyright notices of
both upstream projects (window_manager, bitsdojo_window) alongside this
package's own line.
