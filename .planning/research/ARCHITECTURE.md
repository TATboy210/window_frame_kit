# Architecture Research

**Domain:** Flutter desktop window-management plugin (frame takeover + full window API, 3 platforms)
**Researched:** 2026-09-04
**Confidence:** HIGH (all findings from direct primary-source reading of the exact pinned versions in local pub cache: `window_manager-0.5.2`, `bitsdojo_window_windows-0.1.6`, `bitsdojo_window_macos-0.1.4`, `bitsdojo_window_linux-0.1.4`, plus the Flutter runner template in the host project; bitsdojo live behaviors additionally confirmed by host-project experiments documented in PRIOR-CONTEXT-*.md)

---

## Standard Architecture

### System Overview

Every published Flutter desktop window plugin (window_manager, window_plus, bitsdojo) shares one shape: a Dart facade → one bidirectional MethodChannel → per-platform plugin classes that own the native window handle. window_manager 0.5.2 is the canonical instance and the project's chosen base:

```
┌──────────────────────────── Host App (Flutter/Dart) ────────────────────────────┐
│                                                                                 │
│  ┌──────────────────┐  ┌───────────────────┐  ┌───────────────────────────┐    │
│  │ windowFrameKit   │  │ WindowListener    │  │ Widgets: DragToMoveArea,  │    │
│  │ (static singleton│  │ (abstract mixin,  │  │ DragToResizeArea,         │    │
│  │  + ~60 methods)  │  │  15 no-op hooks)  │  │ VirtualWindowFrame        │    │
│  └────────┬─────────┘  └────────▲────────────┘  └────────────┬──────────────┘    │
│           │  invokeMethod       │ addListener/removeListener │ (pure Flutter)    │
├───────────┴─────────────────────┼────────────────────────────┴───────────────────┤
│                     MethodChannel 'window_frame_kit'  (bidirectional)           │
│        Dart→native: command calls        native→Dart: InvokeMethod("onEvent")   │
├─────────────────────────────────┼───────────────────────────────────────────────┤
│                                 ▼        per-platform plugin branches          │
│  ┌──────────────────────────────────────────┐                                   │
│  │ WINDOWS (C++/Win32) — the real work      │   ┌────────────────────────────┐ │
│  │  ┌────────────────────────────────────┐  │   │ LINUX (GTK/C)              │ │
│  │  │ WindowProc delegate                │  │   │  gtk_window_* API          │ │
│  │  │ (RegisterTopLevelWindowProcDelegate│  │   │  set_decorated / headerbar │ │
│  │  │  → NCCALCSIZE/NCHITTEST/           │  │   │  begin_move/resize_drag    │ │
│  │  │   GETMINMAXINFO/SIZE/CLOSE…)       │  │   │  g_signal_connect → onEvent│ │
│  │  └────────────────────────────────────┘  │   └────────────────────────────┘ │
│  │  ┌────────────────────────────────────┐  │   ┌────────────────────────────┐ │
│  │  │ ChildWindowSubclass (NEW, grafted) │  │   │ macOS (Swift/AppKit)       │ │
│  │  │ SetWindowSubclass(FLUTTERVIEW)     │  │   │  NSWindow.styleMask        │ │
│  │  │ WM_NCHITTEST → HTTRANSPARENT       │  │   │  fullSizeContentView etc.  │ │
│  │  │ at border zones                    │  │   │  NSWindowDelegate → onEvent│ │
│  │  └────────────────────────────────────┘  │   └────────────────────────────┘ │
│  │  ┌────────────────────────────────────┐  │                                  │
│  │  │ WindowState (hwnd, min/max size,   │  │                                  │
│  │  │ title_bar_style_, is_frameless_,   │  │                                  │
│  │  │ last_state, custom_frame_, …)      │  │                                  │
│  │  └────────────────────────────────────┘  │                                  │
│  └──────────────────────────────────────────┘                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│  Native window system: top-level window (FLUTTER_RUNNER_WIN32_WINDOW)           │
│    └── FLUTTERVIEW child window (engine-owned, covers client area)              │
│  Runner template: Win32Window → FlutterWindow::MessageHandler                   │
│    → flutter_controller_->HandleTopLevelWindowProc → plugin delegates           │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | How it's built (source of truth: window_manager 0.5.2) |
|-----------|----------------|--------------------------------------------------------|
| Dart `WindowFrameKit` | Static singleton facade; ~60 async methods; one channel; listener registry | `WindowManager.instance` + global `windowManager`; `ObserverList<WindowListener>`; deps only `path` + `screen_retriever` — **no win32 package** |
| `WindowListener` mixin | 15 no-op event hooks + `onWindowEvent(String)` catch-all | `abstract mixin class`, fans out from a `Map<String, Function>` dispatch in `_methodCallHandler` |
| Event constants | 15 `kWindowEvent*` strings (close/focus/blur/maximize/unmaximize/minimize/restore/resize/resized/move/moved/enter-full-screen/leave-full-screen/docked/undocked) | Top-level consts in the Dart lib — keep names identical so host listener code ports by changing imports only |
| Windows `WindowManagerPlugin` | Channel handler (~60 `compare()` branches) + **WindowProc delegate registration** + `_EmitEvent` | `registrar->RegisterTopLevelWindowProcDelegate(...)`; delegate returns `std::optional<LRESULT>` (nullopt = pass through); `HandleTopLevelWindowProc` returns the delegate's LRESULT verbatim (current runner template) |
| Windows `WindowState` (WindowManager class) | hwnd, pixel_ratio_, title_bar_style_, is_frameless_, minimum/maximum_size_, aspect_ratio_, is_resizable_, last_state machine, is_prevent_close_ | Plain C++ class in `window_manager.cpp` (1129 lines), no Win32 subclassing of its own |
| **Windows `FrameController` (new — the graft)** | Owns everything bitsdojo did: custom-frame NCCALCSIZE, four-edge NCHITTEST zones, maximized clamp, configurable resize margin, `HTTRANSPARENT` child trick, min/max GETMINMAXINFO | Ported from `bitsdojo_window.cpp` (615 lines total, of which the frame-relevant core is ~200 lines) into the plugin's delegate + one child-window subclass |
| **Windows child subclass (new)** | Subclasses the engine's `FLUTTERVIEW` child window; on `WM_NCHITTEST` returns `HTTRANSPARENT` at border zones so the hit falls through to the top-level window | `SetWindowSubclass` (comctl32 v6) installed at `ensureInitialized` — child reachable via `GetWindow(topLevel, GW_CHILD)` (window_manager already uses exactly this in `ForceChildRefresh`) |
| Linux plugin | GTK: `gtk_window_set_decorated(false)` / hide headerbar for frame; `gtk_window_begin_move_drag` / `begin_resize_drag`; events via `g_signal_connect` (delete-event, focus-in/out, show/hide, check-resize, configure-event, window-state-event, event-after) → `onEvent` | Direct port of `window_manager_plugin.cc` (1136 lines); no frame graft needed natively |
| macOS plugin | NSWindow at runtime: `styleMask.insert(.fullSizeContentView)`, `titlebarAppearsTransparent`, `titleVisibility = .hidden`, styleMask bits for resizable/miniaturizable/closable; `NSWindowDelegate` → `onEvent` closure → `FlutterMethodChannel.invokeMethod("onEvent")` | Direct port of `WindowManager.swift` (605) + `WindowManagerPlugin.swift` (278); no runner NSWindow subclass needed |

## Recommended Project Structure

Single package (NOT federated) — window_manager 0.5.2 is exactly this shape and it is the shape pub.dev users of this domain expect:

```
window_frame_kit/
├── lib/
│   ├── window_frame_kit.dart        # barrel: exports + kWindowEvent* constants
│   └── src/
│       ├── window_frame_kit.dart    # singleton facade (port of window_manager.dart, renamed)
│       ├── window_listener.dart     # mixin (port verbatim)
│       ├── window_options.dart      # WindowOptions for waitUntilReadyToShow
│       ├── resize_edge.dart         # ResizeEdge enum (startResizing)
│       ├── title_bar_style.dart     # normal/hidden
│       └── widgets/                 # cross-platform widget layer (port)
│           ├── drag_to_move_area.dart
│           ├── drag_to_resize_area.dart
│           ├── virtual_window_frame.dart   # Linux/Wayland resize fallback
│           └── window_caption.dart
├── windows/
│   ├── CMakeLists.txt               # SHARED lib; link Dwmapi + Comctl32 (bitsdojo needs both)
│   ├── include/window_frame_kit/window_frame_kit_plugin.h
│   ├── window_frame_kit_plugin.cpp  # channel handler + WindowProc delegate (port of window_manager_plugin.cpp)
│   ├── window_frame_kit.cpp         # WindowState + window ops (port of window_manager.cpp)
│   └── frame_controller.cpp/.h      # NEW: grafted frame logic (child subclass + NCCALCSIZE + NCHITTEST)
├── linux/
│   ├── CMakeLists.txt
│   ├── include/window_frame_kit/...
│   └── window_frame_kit_plugin.cc   # port of window_manager_plugin.cc
├── macos/window_frame_kit/
│   ├── Package.swift
│   └── Sources/window_frame_kit/
│       ├── WindowManager.swift      # port (rename class)
│       └── WindowManagerPlugin.swift
├── example/                         # demo exercising frame + events + min-size on all 3 platforms
├── pubspec.yaml                     # deps: flutter sdk, path, screen_retriever. NO win32.
├── CHANGELOG.md / README.md / LICENSE
└── analysis_options.yaml
```

### Structure Rationale

- **Single package, three platform dirs:** bitsdojo's federated-style split (`bitsdojo_window` + `bitsdojo_window_windows` + …) exists only to gate per-platform pubspecs; window_manager proves one package with `windows/`+`linux/`+`macos/` folders works and keeps the solve simpler. Fewer packages = one version story.
- **`frame_controller.cpp` as a separate file:** the graft must stay *structurally separable* from the ported window_manager code. Every grafted line sits behind `custom_frame_` checks, and every deviation from verbatim-ported window_manager code gets a `// FRAME:` comment. Reason: window_manager is actively maintained upstream (60 days old at research time); a thin, marked graft lets future upstream fixes be cherry-picked without a three-way merge.
- **No `third_party/`, no runner code:** bitsdojo needed a STATIC lib + runner `main.cpp` call because `bitsdojo_window_configure(BDW_CUSTOM_FRAME)` must be invoked before any window exists from outside the plugin. Replacing that flag with a Dart method call (`setCustomFrame`) makes the plugin self-contained — standard SHARED plugin, standard runner template, zero runner edits.

## Architectural Patterns

### Pattern 1: WindowProc delegation for top-level messages (adopt from window_manager)

**What:** Flutter's official mechanism — `registrar->RegisterTopLevelWindowProcDelegate()` registers a `std::optional<LRESULT>`-returning delegate. The runner template's `FlutterWindow::MessageHandler` forwards every top-level message to `flutter_controller_->HandleTopLevelWindowProc(...)`, which walks all delegates and returns the first non-nullopt LRESULT verbatim.

**When to use:** always, for every top-level message: WM_NCCALCSIZE, WM_NCHITTEST, WM_GETMINMAXINFO, WM_SIZE, WM_SIZING, WM_MOVING, WM_EXITSIZEMOVE, WM_NCACTIVATE, WM_CLOSE, WM_SHOWWINDOW, WM_WINDOWPOSCHANGED, WM_DPICHANGED.

**Trade-offs:** requires the standard runner template (the `HandleTopLevelWindowProc` call in `flutter_window.cpp`) — a near-universal convention; a customized runner without it breaks the plugin. Documented as a requirement in the README. No comctl32 dependency for the top-level path.

**Evidence (verbatim, window_manager-0.5.2 windows/window_manager_plugin.cpp):**
```cpp
window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
    [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
      return HandleWindowProc(hWnd, message, wParam, lParam);
    });
// HandleWindowProc returns std::optional<LRESULT>; nullopt = not handled.
```

### Pattern 2: Child-window subclass for border hit-testing (the one thing that MUST be grafted from bitsdojo)

**What:** The engine's `FLUTTERVIEW` child window covers the whole client area, so its `WM_NCHITTEST` goes to the engine-owned child wndproc and **never reaches top-level delegates** (proven live in the host project: runner-level NCHITTEST on the top edge never fired even at 50px insets). bitsdojo solves this by `SetWindowSubclass`-ing the FLUTTERVIEW window and returning `HTTRANSPARENT` on border zones — the system then re-queries the parent, where the plugin's delegate returns the real `HTTOPLEFT`/`HTLEFT`/… codes and DefWindowProc enters the native sizing loop on the top-level window.

**When to use:** only in custom-frame mode, only for the child window. The top-level NCHITTEST (with real `HT*` return codes) stays in the existing delegate.

**Trade-offs:** requires comctl32 v6 (`SetWindowSubclass`) — linked via CMake; Flutter runner manifests enable comctl32 v6 by default. Must `RemoveWindowSubclass` on plugin teardown or the dangling subclass crashes at engine destruction. Must NOT return resize codes directly from the child (a child's HTBORDER result would start the sizing loop on the child, resizing the wrong window) — `HTTRANSPARENT` passthrough is the correct and battle-tested route.

**Evidence (verbatim, bitsdojo_window_windows-0.1.6 windows/bitsdojo_window.cpp):**
```cpp
// child side: pass border zones through to the parent
LRESULT CALLBACK child_window_proc(...) {
  case WM_NCHITTEST: {
    LRESULT result = handle_nchittest(window, wparam, lparam);
    if (result != HTCLIENT) return HTTRANSPARENT;
    break;
  }
}
// parent side: real codes, DPI-scaled margin
int getResizeMargin(HWND window) {
  int resizeBorder  = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
  int borderPadding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
  return IsZoomed(window) ? borderPadding : resizeBorder + borderPadding;
}
```

**Design improvement over bitsdojo:** install the child subclass at `ensureInitialized` (first Dart call) using `GetWindow(native_window, GW_CHILD)` instead of bitsdojo's `WH_CBT` thread hook. The CBT hook exists only because bitsdojo's configure() runs from runner `main.cpp` before windows exist. A plugin's `ensureInitialized` runs after the view exists (window_manager already resolves `GetAncestor(GetView()->GetNativeWindow(), GA_ROOT)` there). Dropping the CBT hook removes global hook surface and a class of startup ordering bugs. If the child is somehow not yet created at `ensureInitialized`, fall back to a brief `WM_CREATE`-watching CBT hook — verify at implementation time.

### Pattern 3: Single-source-of-truth min/max size (fixes the GETMINMAXINFO conflict by construction)

**What:** The dual-package conflict was structural: bitsdojo's subclass proc on the top-level window runs **before** the runner wndproc chain, and its `WM_GETMINMAXINFO` case returns 0 **unconditionally** (bitsdojo_window.cpp:550-568) without calling `DefSubclassProc` — so window_manager's delegate never saw the message at all and `setMinimumSize` was silently dead; the host had to set the same value through both packages' channels. In one package there is exactly one min/max state and one handler, so the "cooperative" path is: store min/max in logical px in `WindowState`; on `WM_GETMINMAXINFO`, override `ptMinTrackSize`/`ptMaxTrackSize` **only when non-default** (min ≠ 0, max ≠ -1), scale by per-window DPI, return 0.

**When to use:** always — this handler must run in both normal and custom-frame mode. Native border drags (NCHITTEST → sizing loop) and widget-driven resizes (`startResizing`) are both constrained by the same MINMAXINFO, so no dual-channel sync exists to get wrong.

**Trade-offs:** returning 0 without calling DefWindowProc skips the default proc's maximized-size initialization — same behavior as both upstreams, harmless for track-size purposes since the system pre-fills MINMAXINFO before sending. Keep the conditional-override guard (window_manager already has it at window_manager_plugin.cpp:190-205) rather than bitsdojo's unconditional form.

**Improvement to adopt:** bitsdojo scales with `GetDpiForWindow` + `GetSystemMetricsForDpi` (per-monitor correct); window_manager scales min/max with a `pixel_ratio_` that is only updated on `WM_DPICHANGED`. Use `GetDpiForWindow` for the scaling factor in the merged handler.

### Pattern 4: Custom-frame NCCALCSIZE branch (graft bitsdojo's, keep window_manager's others)

**What:** window_manager's delegate already handles WM_NCCALCSIZE in three branches (fullscreen; `is_frameless_` return 0; `title_bar_style_ == "hidden"` with 8px right/bottom/left client-rect adjustment — that adjustment is why left/right/bottom resize works in the dual-package setup, and why the top edge doesn't: Win11 top inset = 0). Add a fourth branch for `custom_frame_` ported from bitsdojo's `handle_nccalcsize`: call `DefSubclassProc` first, clamp `rgrc` to the monitor work area when maximized (`adjustMaximizedRects`/`adjustMaximizedSize` — without this a return-0 window overscans the screen when maximized), `top -= 1` when normal (kills the Win10 white line), return 0.

**When to use:** only when `custom_frame_` is on; the three existing branches keep their semantics so `setTitleBarStyle`/`setAsFrameless` behave identically to window_manager.

**Supporting graft details (all from bitsdojo, all needed):** `WM_ERASEBKGND → return 1` (anti-flicker); `WM_NCCREATE → add WS_CLIPCHILDREN`; `WM_CREATE → DwmExtendFrameIntoClientArea(margins {0,0,1,0})` + re-`SetWindowPos(SWP_FRAMECHANGED)` to force NCCALCSIZE re-evaluation (this is also how window_manager applies its frame state late — `SetAsFrameless()` does exactly that, so late application of the custom-frame flag is a solved pattern).

### Pattern 5: One bidirectional MethodChannel for both RPC and events (adopt from window_manager)

**What:** Same channel name for `invokeMethod` (Dart→native commands) and native `InvokeMethod("onEvent", {eventName})` (native→Dart events). Dart fans out to registered listeners. No EventChannel.

**When to use:** always. It matches upstream exactly, keeps event dispatch a plain function map, and avoids a second channel to keep alive.

**Trade-offs:** none material. (window_plus uses EventChannels; not worth diverging from the base we're porting.)

### Pattern 6: Dart singleton + listener mixin API (adopt; do not invent)

**What:** `WindowFrameKit.instance` static singleton + top-level `windowFrameKit` convenience; `WindowListener` abstract mixin with no-op hooks; `addListener/removeListener`. Events keep the exact 15 window_manager names.

**When to use:** always. The window is a process-global singleton — an instance-based API adds nothing. Familiarity is a stated product goal (host app and the whole window_manager user base port by changing imports and `windowManager.` → `windowFrameKit.`).

**Trade-offs:** listener mixin lacks stream composability; if demanded later, add `Stream<String> get onEvent` as a thin wrapper — a superset, zero breakage. Do NOT build it in v1 (Unix: small is beautiful; no speculative API).

## Data Flow

### Command flow (Dart → native)

```
Dart: windowFrameKit.minimize()
  → MethodChannel('window_frame_kit').invokeMethod('minimize')
    → [Windows] HandleMethodCall → WindowState::Minimize() → ShowWindow/PostMessage
    → [Linux]   handle_method_call → gtk_window_iconify()
    → [macOS]   WindowManager.miniaturize() → NSWindow.miniaturize
  → result->Success(...) → Future completes
```

### Event flow (native → Dart)

```
[Windows] top-level message (e.g. WM_SIZE w/ SIZE_MAXIMIZED)
  → runner WndProc → FlutterWindow::MessageHandler
  → HandleTopLevelWindowProc → plugin delegate HandleWindowProc
  → WindowState.last_state machine classifies (STATE_NORMAL/MAXIMIZED/MINIMIZED/FULLSCREEN)
  → _EmitEvent("maximize") → channel.InvokeMethod("onEvent", {eventName})
  → Dart _methodCallHandler → for each WindowListener: onWindowEvent + funcMap[eventName]

[Linux]   g_signal_connect(window, "window-state-event", …) → onEvent → same
[macOS]   NSWindowDelegate.windowDidExitFullScreen → onEvent closure → channel → same
```

### Hit-test / resize flow (the grafted path, Windows custom-frame mode)

```
Mouse over border zone of FLUTTERVIEW client area
  → child subclass WM_NCHITTEST → handle_nchittest() → zone ≠ HTCLIENT
  → return HTTRANSPARENT  (system re-queries the window beneath = top-level)
  → top-level WM_NCHITTEST → delegate handle_nchittest() → HTTOPLEFT etc.
  → DefWindowProc enters modal sizing loop on the top-level window
  → WM_SIZING (per-frame: emit "resize" event + aspect-ratio adjust)
  → WM_GETMINMAXINFO (delegate clamps to min/max — single source of truth)
  → WM_EXITSIZEMOVE → emit "resized" event
```

### Key data flows

1. **ensureInitialized:** resolves `native_window = GetAncestor(view hwnd, GA_ROOT)`; NEW: also `SetWindowSubclass(GetWindow(native_window, GW_CHILD))` for the frame controller; returns window id.
2. **setCustomFrame(true):** sets `custom_frame_` + applies WS_CLIPCHILDREN/DwmExtendFrame + `SetWindowPos(SWP_FRAMECHANGED)` to re-run NCCALCSIZE — no restart needed (bitsdojo needed the pre-creation runner call; we don't).
3. **setMinimumSize:** writes `WindowState.minimum_size_` (logical px) — the same struct WM_GETMINMAXINFO reads. One write, two enforcement points.

## Platform Frame Matrix (what "frame takeover" means per platform)

| Platform | Frame takeover mechanism | Already in window_manager 0.5.2? | Graft work needed |
|----------|--------------------------|-----------------------------------|-------------------|
| Windows | NCCALCSIZE return 0 (client = whole window) + child HTTRANSPARENT → parent NCHITTEST four-edge resize + maximized work-area clamp | Partially: `setAsFrameless` returns 0 but has **no** child hit-test passthrough and no four-edge zones (hidden-titlebar branch only frees left/right/bottom via −8px insets; top edge dead on Win11) | Yes — the core graft (FrameController + child subclass + NCHITTEST zones + custom-frame NCCALCSIZE branch) |
| Linux GTK | `gtk_window_set_decorated(false)` / hide headerbar; move/resize via `gtk_window_begin_move_drag`/`begin_resize_drag`; on Wayland CSD there is no native resize area → widget-driven edges | Yes — `setTitleBarStyle(.hidden)`, `startDragging`, `startResizing`, plus `VirtualWindowFrame`/`DragToResizeArea` widgets | None natively. Port as-is; value = unified API + widgets |
| macOS | `styleMask.fullSizeContentView` + `titlebarAppearsTransparent` + `titleVisibility = .hidden`; `.titled` stays so **native resize borders keep working automatically** | Yes — `setTitleBarStyle(.hidden)` does exactly this at runtime; bitsdojo's runner-level `MainFlutterWindow: BitsdojoWindow` subclass is unnecessary | None required. Optional: `isOpaque = false`, `isMovable`/`movableByWindowBackground` toggle |

Key asymmetry: Windows is the only platform where frame takeover requires message-level surgery; Linux/macOS frame takeover is a property flip window_manager already ships. The plugin's engineering center of gravity is the Windows branch; Linux/macOS are ports plus widgets. This should shape phase sizing.

## Scaling Considerations

(Adapted for a plugin: the growth axes are platforms, API surface, and upstream drift — not users.)

| Scale | Architecture adjustments |
|-------|--------------------------|
| v0.1 Windows-first | Dart skeleton + Windows port + frame graft. Gate: example app + host-project integration. |
| v0.2 three platforms | Linux/macOS ports land (parallelizable — independent of Windows graft). Same channel contract, platform-gated methods marked `@platforms` in doc comments (window_manager convention). |
| v0.3+ maintenance | Upstream window_manager releases: cherry-pick fixes into the marked port layers. API additions only via append (never rename — same discipline as the host project's error-code registry). |

### Scaling Priorities

1. **First risk:** upstream drift — window_manager is actively maintained; our ported 1700+ lines of Windows/Linux code will diverge. Mitigation: verbatim ports with `// FRAME:`-marked deviations only, no opportunistic refactors of ported code.
2. **Second risk:** Flutter engine changes to the child-window class name (`FLUTTERVIEW`) or runner delegate chain. Mitigation: resolve the child via `GetWindow(topLevel, GW_CHILD)` (class-name-independent) rather than bitsdojo's class-name string match where possible.

## Anti-Patterns

### Anti-Pattern 1: Runner-level frame code
**What people do:** put NCHITTEST/NCCALCSIZE handling in `windows/runner/main.cpp` / `win32_window.cpp` (bitsdojo's legacy contract).
**Why it's wrong:** proven live in the host project — the FLUTTERVIEW child covers the top edge; runner-level WM_NCHITTEST for it never fires. Also forces every user to hand-patch their runner template.
**Do this instead:** all message handling inside the plugin (delegate for top-level, child subclass for FLUTTERVIEW). Zero runner edits.

### Anti-Pattern 2: Unconditional `WM_GETMINMAXINFO → return 0`
**What people do:** bitsdojo's form — return 0 regardless of whether min/max are set, skipping `DefSubclassProc`.
**Why it's wrong:** any other handler in the chain (e.g., a co-installed plugin) loses the message; with own min/max unset the constraint silently vanishes — the exact bug that forced dual-channel same-value minSize in the host.
**Do this instead:** conditional override + return 0 (window_manager's form), single source of truth.

### Anti-Pattern 3: Dart-side FFI for frame logic
**What people do:** port bitsdojo's Dart layer (`window.dart` via the `win32` package) instead of doing it in C++.
**Why it's wrong:** couples the package to a `win32` major version — the precise dependency-tree lock this project exists to escape (bitsdojo's `win32: ^5.1.1` blocked file_picker 12 / secure_storage 11 in the host). Also splits frame state across two languages.
**Do this instead:** all native logic in the plugin's C++; public Dart API passes plain ints (e.g., `getId()` returns the hwnd as int). Keep `pubspec` deps to `path` + `screen_retriever`, matching window_manager.

### Anti-Pattern 4: Subclass without teardown
**What people do:** `SetWindowSubclass(FLUTTERVIEW…)` with no `RemoveWindowSubclass` in the plugin destructor.
**Why it's wrong:** dangling subclass proc → crash at engine/window teardown (window_manager carefully calls `UnregisterTopLevelWindowProcDelegate(window_proc_id)` in its destructor; mirror that for the child subclass).
**Do this instead:** RAII-style or destructor-time `RemoveWindowSubclass` with the same subclass id used at install.

### Anti-Pattern 5: NCCALCSIZE return 0 without the maximized clamp
**What people do:** treat "return 0" as the whole frameless story.
**Why it's wrong:** a maximized return-0 window extends past the work area by the removed frame metrics on every side — content cut off.
**Do this instead:** bitsdojo's `adjustMaximizedRects`/`adjustMaximizedSize` (clamp `rgrc`/`WINDOWPOS` to monitor work area when maximized), which is why bitsdojo's maximized frameless windows behave.

### Anti-Pattern 6: Reusing the `window_manager` channel name or type names
**What people do:** keep channel `'window_manager'` "for compatibility".
**Why it's wrong:** two plugins registered on one channel name = undefined dispatch during any migration period where both are installed.
**Do this instead:** channel `'window_frame_kit'`; keep the *event name strings* identical (they're data, not identifiers) so listener code ports unchanged.

## Integration Points

### Host project swap map (simple_player_flutter, the first integration target and validation gate)

| Dual-package today | window_frame_kit tomorrow |
|--------------------|---------------------------|
| `bitsdojo_window_configure(BDW_CUSTOM_FRAME)` in runner main.cpp | `windowFrameKit.setCustomFrame()` (Dart, no runner edit) |
| `WindowBorder(transparent: true, width: 0)` wrapper | deleted — native NCHITTEST covers it |
| `doWhenWindowReady(() => appWindow.minSize = …)` (dual-channel workaround) | `setMinimumSize(...)` — works, single channel |
| `windowManager.*` calls in `WindowService` | `windowFrameKit.*` (same method names) |
| `WindowListener` impl | same mixin shape, new import |
| `win32 5.x` in the dep tree (from bitsdojo) | gone — package has no win32 dep → file_picker 12 / secure_storage 11 unlock for the host |

### Internal boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Dart facade ↔ native plugins | one MethodChannel, standard codec | ~60 methods; NotImplemented for gaps (same as upstream) |
| Windows delegate ↔ FrameController | delegate calls into FrameController for NCCALCSIZE/NCHITTEST/GETMINMAXINFO when `custom_frame_` is on | keeps ported window_manager code untouched |
| FrameController ↔ child subclass | same C++ module; subclass installed/removed by FrameController | child only ever returns HTTRANSPARENT or DefSubclassProc |
| Event emitter ↔ Dart listeners | `onEvent` method call on the channel | same 15 event names on all platforms (dock events Windows-only, matching upstream) |

## Suggested Build Order (component dependencies)

```
1. Scaffold ──────────────► 2. Windows base port ──► 3. Frame graft ──► 4. Min-size fix
   (Dart skeleton:                        (verbatim,        (child subclass,    (GETMINMAXINFO
    singleton/listener/                    prove 60          NCCALCSIZE,         cooperative +
    constants/channel rename;              methods +         NCHITTEST zones,    per-window DPI)
    flutter create plugin                  events on         maximized clamp,
    template, 3 platforms)                 example app)      configurable margin)
        │
        ├─────────────────► 5. Linux port ──────────────► (parallel with 2-4;
        │                   (GTK cc port + widgets)        independent of Windows)
        │
        └─────────────────► 6. macOS port ──────────────► (parallel; styleMask only)
                            (Swift port)

7. Example app hardening (3 platforms: frame + events + min/max + fullscreen matrix)
8. Publish readiness (LICENSE MIT + window_manager/bitsdojo attribution, README,
   dartdoc coverage, pana 140+, CI analyze/test on 3 OSes, 0.1.0)
9. Host integration (swap dual-package in simple_player_flutter — final validation)
```

**Ordering rationale:**
- 2 before 3: the graft must land on a *proven-identical-to-window_manager* base, or regressions in the port and the graft are indistinguishable. Phase 2's exit gate is behavioral parity on the example app.
- 4 after 3 (or merged with it): the min-size fix is trivial code but its *validation* requires custom-frame mode active (the conflict only exists there).
- 5/6 parallel to 2-4: zero code dependency — Linux/macOS ports don't touch the Windows frame. This is the roadmap's parallelization opportunity.
- 9 last and gated: PROJECT.md says a capability is only Validated after host integration; the host swap doubles as the real-world regression suite.
- Fullscreen zero-flash exploration (PROJECT.md exploration goal) deliberately outside the critical path; the frame architecture above doesn't preclude it (the `last_state` machine and `SWP_FRAMECHANGED` re-application are the natural extension points).

## Sources

- Primary source (local pub cache, exact pinned versions):
  - `C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/window_manager-0.5.2/` — `lib/src/window_manager.dart` (763 ln), `lib/src/window_listener.dart`, `windows/window_manager_plugin.cpp` (597 ln), `windows/window_manager.cpp` (1129 ln), `linux/window_manager_plugin.cc` (1136 ln), `macos/.../WindowManager.swift` (605 ln) + `WindowManagerPlugin.swift`, `windows/CMakeLists.txt`, `pubspec.yaml`
  - `C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/bitsdojo_window_windows-0.1.6/` — `windows/bitsdojo_window.cpp` (615 ln), `bitsdojo_window_plugin.cpp`, `CMakeLists.txt`, `pubspec.yaml`
  - `C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/bitsdojo_window_macos-0.1.4/` — `macos/Classes/BitsdojoWindow.swift`
- Host project runner template: `D:/simple_player_flutter/windows/runner/flutter_window.cpp` (HandleTopLevelWindowProc forwarding)
- Live-verified prior findings: `D:/window_frame_kit/.planning/research/PRIOR-CONTEXT-session-handoff.md`, `PRIOR-CONTEXT-window-layer.md` (child-window hit-test dead zone, GETMINMAXINFO line refs, dual-package division, win32 6 type facts)
- Context7 `/leanflutter/window_manager` cross-check (v0.5.0 indexed)

---
*Architecture research for: window_frame_kit — Flutter desktop window management plugin*
*Researched: 2026-09-04*
