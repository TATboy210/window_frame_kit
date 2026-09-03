# Stack Research

**Domain:** Flutter desktop window management plugin (frame takeover + window capability API), Windows/Linux/macOS, pub.dev publish
**Researched:** 2026-09-04
**Confidence:** HIGH (upstream sources verified from actual published packages in pub cache + live pub.dev + Context7)

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Flutter SDK | 3.47.0 stable (dev env) / floor `flutter >=3.38` | Plugin framework | The dev machine runs 3.47.0 (Dart 3.13.0). The floor is forced by win32 6.4.0's `sdk: ^3.10.0` (Dart 3.10 ≈ Flutter 3.38) — verify exact Flutter floor against template output at scaffold time.window_manager 0.5.2 only requires `flutter >=3.3.0`, so the win32 6.x dependency is the binding constraint |
| Dart | `sdk: ">=3.10.0 <4.0.0"` | Language floor | Lowest version win32 6.4.0 accepts. Do NOT widen below this — pub resolution would fall back to win32 5.x and recreate the lock this package exists to break |
| window_manager 0.5.2 source (MIT) | 0.5.2 (pub 2026-07, verified) | The base to fork: full window API + event stream | Track leader (652k dl, 1.12k likes, 160 pub points, verified publisher, active 2026). Crucially it is **already a pure plugin** — hooks the Flutter engine's top-level window proc via `registrar->RegisterTopLevelWindowProcDelegate` (plugin.cpp:107), zero runner changes. Already handles WM_NCCALCSIZE (frameless/hidden branches), WM_NCHITTEST (minimal: `!is_resizable_` → HTNOWHERE), and WM_GETMINMAXINFO (cooperative min/max — the exact pattern bitsdojo breaks). Fork keeps this scaffolding; graft upgrades the frame branches |
| bitsdojo_window_windows 0.1.6 frame code (MIT) | 0.1.6 | The graft: real border hit-testing | Its `bitsdojo_window.cpp` (614 lines) is the only mature implementation of DPI-aware WM_NCHITTEST 4-edge resize (`GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)` at line 155 → HTLEFT/HTRIGHT/HTTOP/HTBOTTOM dispatch at ~206), Win11-top-inset-0 NCCALCSIZE (full `return 0` vs window_manager's `right-=8/bottom-=8/left-=-8` hack for issue #483), and `startDragging` via `SendMessage(WM_SYSCOMMAND, SC_MOVE \| HTCAPTION)` (line 606). Small enough to graft wholesale (~830 lines C++ incl. plugin/api/common headers) |
| win32 (Dart FFI) | ^6.4.0 (2026-08-05, verified latest) | Dart-side frame geometry (border width, DPI metrics, NCHITTEST coordinate math) | Declaring `win32: ^6.4.0` makes this package carry win32 6.x into every host dependency tree — the stated unlock value (file_picker 12 / flutter_secure_storage 11). Migration knowledge is already validated in the host project (see §win32 6.x below). BSD-3-Clause, Flutter Favorite, 7.86M downloads |
| screen_retriever | ^0.2.2 (inherited from WM 0.5.2) | Display/screen info | Verified latest on pub.dev (2026-07, leanflutter.dev, 982k dl) — the constraint is current, no bump needed. Note: it carries the same "migrating to libnativeapi" notice as window_manager |

### Plugin Package Structure

Single inline plugin (NOT federated), exactly like the base:

```bash
flutter create --template=plugin --platforms=windows,linux,macos \
  --org <your-reversed-domain> window_frame_kit
```

```
window_frame_kit/
├── lib/
│   ├── window_frame_kit.dart            # barrel: singleton + exports (WM pattern)
│   └── src/
│       ├── window_frame_kit.dart        # core API, forked from WM window_manager.dart (763 lines)
│       ├── window_listener.dart         # 17 callbacks, kept 1:1 with WM (57 lines)
│       ├── window_options.dart          # WindowOptions (32 lines)
│       ├── frame_config.dart            # NEW: configurable resize border width (fixes bitsdojo hardcode)
│       └── widgets/                     # WindowCaption, VirtualWindowFrame (from WM)
├── windows/
│   ├── CMakeLists.txt
│   ├── include/window_frame_kit/
│   ├── window_frame_kit_plugin.cpp      # fork of WM window_manager_plugin.cpp (597 lines):
│   │                                    #   RegisterTopLevelWindowProcDelegate + _EmitEvent channel loop
│   ├── window_manager.cpp|.h            # fork of WM window_manager.cpp (1129 lines): HWND/geometry/fullscreen
│   └── frame_custom.cpp|.h              # GRAFT of bitsdojo_window.cpp (614 lines): NCHITTEST edges,
│                                        #   NCCALCSIZE return-0, GETMINMAXINFO (rewritten cooperative)
├── linux/
│   ├── CMakeLists.txt
│   └── window_frame_kit_plugin.cc       # fork of WM linux plugin (1136 lines) + bitsdojo gtk api_impl.h graft
├── macos/
│   └── window_frame_kit/Package.swift   # KEEP SwiftPM layout (pana platform bonus) + Sources/*.swift
│                                        #   (WM WindowManager.swift) + 52-line BitsdojoWindow.swift graft
├── example/                             # mandatory for pana docs points
├── test/                                # unit tests, channel-level fakes
├── integration_test/                    # real-window smoke tests per platform
└── .github/workflows/{ci.yml,publish.yml}
```

pubspec plugin declaration (shape verified against both upstreams):

```yaml
flutter:
  plugin:
    platforms:
      windows:
        pluginClass: WindowFrameKitPlugin      # C++ class
      linux:
        pluginClass: WindowFrameKitPlugin      # C class
      macos:
        pluginClass: WindowFrameKitPlugin      # Swift class, sharedDarwinSource not needed
```

Plus `topics: [window, desktop, frameless-window, custom-titlebar, window-manager]`, `repository:`, `issue_tracker:`, `documentation:` (all present in WM 0.5.2's pubspec — copy the convention).

### Base Architecture — window_manager 0.5.2 (verified from source, what you inherit)

| Mechanism | Fact (file:line in pub cache source) | Action for fork |
|-----------|--------------------------------------|-----------------|
| Channel | Single `MethodChannel('window_manager')`; ~90 methods invoke 1:1 native calls | Rename channel to `window_frame_kit`; keep 1:1 mapping |
| Events | Native `_EmitEvent(std::string)` → `channel->InvokeMethod("onEvent", ...)` (plugin.cpp:125-131). NOT an EventChannel | Keep mechanism in v1 (faithful fork); EventChannel rewrite is optional post-v1 polish |
| Listener | `WindowListener` abstract, 17 callbacks incl. `onWindowResize/Resized/Move/Moved/Maximize/EnterFullScreen/Docked` + raw `onWindowEvent(String)` | Keep verbatim |
| Frame hook | `HandleWindowProc` via `RegisterTopLevelWindowProcDelegate` (plugin.cpp:107) — plugin layer, **no runner changes** | This is the integration point for the graft |
| WM_NCCALCSIZE | Handles `is_frameless_` and `TitleBarStyle.hidden` branches; hidden branch does `top += IsWindows11OrGreater() ? 0 : 1; right -= 8; bottom -= 8; left -= -8` (the issue-#483 8px hack) | Replace hidden-branch hack with bitsdojo full `return 0` + own NCHITTEST dispatch |
| WM_NCHITTEST | Only `!is_resizable_ → HTNOWHERE` — **no border hit-testing at all** | Graft bitsdojo's DPI-aware edge dispatch here |
| WM_GETMINMAXINFO | Cooperative: applies `minimum_size_/maximum_size_` only when set (pixel-ratio scaled) — the correct pattern | PRESERVE. Fix bitsdojo's unconditional `return 0` (bitsdojo_window.cpp:550-568) to this cooperative form — this is the documented host-project bug fix |
| Fullscreen/maximize | Full state machine in window_manager.cpp (1129 lines) incl. `setAsFrameless()` | Keep |

### Graft Inventory — bitsdojo_window_windows 0.1.6 (exact, from pub cache)

| Upstream file | Lines | What to take | What to fix on intake |
|---------------|-------|--------------|----------------------|
| `windows/bitsdojo_window.cpp` | 614 | NCHITTEST edge dispatch (~334, ~438, ~206), `GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)` border width (155), NCCALCSIZE with `DefSubclassProc` fallback (288), `SendMessage(WM_SYSCOMMAND, SC_MOVE\|HTCAPTION)` startDragging (606) | Drop the runner-hook static-initializer machinery (`bitsdojo_window_configure`, line 612 — not needed, WM's delegate already receives these messages); GETMINMAXINFO (550) → cooperative; border width → configurable constant |
| `windows/bitsdojo_window_plugin.cpp` + `api/common/util.h` | ~175 | Subclassing helpers if needed | Merge into plugin.cpp; rename symbols |
| `lib/src/window.dart` | 298 | Dart-side frame geometry (DPI metrics via win32 package) | **This is the win32 5.x→6.x hotspot** (host-validated migration list: `HWND(Pointer.fromAddress(n))`, `SYSTEM_METRICS_INDEX(int)` direct, `GetSystemMetricsForDpi → Win32Result<int>` take `.value`); errors concentrated at lines 24/26/58/90/91/101/204 |
| `lib/src/app_window.dart` (30) + `window_util.dart` (40) | 70 | API surface concepts only | Re-express on WM API, no bitsdojo platform_interface dep |
| macOS `Classes/BitsdojoWindow.swift` + plugin | 74 | `setCustomFrame` flag handling | Trivial; fold into WM's Swift plugin |
| Linux `api.h/api_impl.h/common.h` | ~small | GTK deco removal + edge resize | Fold into WM's single .cc |

Runner impact: **zero** for the new package (bitsdojo required `bitsdojo_window_configure(BDW_CUSTOM_FRAME)` in `main.cpp`; window_manager's delegate path makes that obsolete). This is a headline improvement to document in the README — but verify startup-timing edge cases (the host's white-border history shows hidden-before-show ordering matters) before advertising.

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| ffi | ^2.2.0 | Dart FFI primitives (required transitively by win32 6.4.0 which wants ^2.1.4) | Only if Dart-side frame code does raw FFI beyond win32 package |
| flutter_lints | ^6.0.0 (latest, verified) | Base lint set | dev_dependencies; satisfies pana analysis scoring with 0 findings |
| mostly_reasonable_lints | ^0.1.2 | WM 0.5.2's own dev lint choice | Optional — mirror the base's dev-deps for lower diff noise when pulling upstream fixes |
| dependency_validator | ^3.0.0 | Catches unused/missing deps before publish | dev_dependencies, from WM's pubspec |
| melos | — | Monorepo tooling | NOT needed: single inline package, no federated subpackages |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| `flutter create --template=plugin` | Scaffold | Generates windows/linux/macos branches, pubspec plugin block, example/ baseline; then overlay the forked sources |
| `pana` (run locally: `dart pub global activate pana && pana`) | Pre-publish score check | Run before every publish; target 160/160 (both win32 and window_manager display 160 pub points — that's the current ceiling) |
| `flutter pub publish --dry-run` | Publish validation | Gate in CI publish workflow |
| GitHub Actions matrix | CI on windows-latest / ubuntu-latest / macos-latest | See CI section |
| CMake 3.14+ / VS C++ toolchain / GTK dev / Xcode | Native builds | Standard desktop toolchain requirements; CI runners have them (Linux needs `apt-get install clang cmake ninja-build pkg-config libgtk-3-dev`) |

## Installation

```bash
# Scaffold
flutter create --template=plugin --platforms=windows,linux,macos --org <domain> window_frame_kit

# Runtime deps (package's own pubspec)
#   win32: ^6.4.0        (the unlock carrier)
#   screen_retriever: ^0.2.2
#   path: ^1.9.0

# Dev deps
#   flutter_lints: ^6.0.0
#   dependency_validator: ^3.0.0

# Pre-publish
dart pub global activate pana
pana                              # target 160
flutter pub publish --dry-run
```

## Publishing Requirements (pub.dev, verified 2026-09)

Per pub.dev/help/scoring, points come in six categories; practical checklist for a 160-point plugin:

| Requirement | Detail | Consequence if missed |
|-------------|--------|----------------------|
| `flutter.plugin.platforms` declared | windows/linux/macos blocks with correct `pluginClass` names | Publishing rejected outright |
| LICENSE | MIT. Must preserve upstream notices verbatim — MIT requires "the above copyright notice and this permission notice shall be included in all copies or substantial portions". Add three lines: own copyright + `Copyright (c) 2022-present LiJianying <lijy91@foxmail.com>` (window_manager) + `Copyright (c) 2020-2021 Bogdan Hobeanu` (bitsdojo) — both verified from pub-cache LICENSE files. Best practice: also a `THIRD-PARTY-NOTICES.md` + per-file header comments on grafted files. Note: win32 dep is BSD-3-Clause but a *dependency* is not a *copy* — no license-file change needed for it | License violation; pana detects license mismatch |
| CHANGELOG.md | Valid semver header per release (pana ≥0.22.10 withholds docs points for invalid changelogs) | Lose docs points |
| README.md + example/ | Pana wants an *illustrative example* (example/ app) | Lose docs points |
| dartdoc coverage | ≥20% of public API members documented | Lose docs points |
| Platform tags | 3 desktop platforms auto-detected; **SwiftPM support on macOS earns bonus** — WM 0.5.2's `macos/window_frame_kit/Package.swift` layout already has it, keep it | Lose platform points |
| Static analysis | 0 errors/warnings/infos under `flutter analyze` | Lose analysis points |
| Up-to-date deps | Must resolve against latest stable SDK + latest dep versions | Lose dependency points |
| Verified publisher | Optional but recommended; WM uses leanflutter.dev | Trust signal only |
| SDK constraint | `sdk: ">=3.10.0 <4.0.0"`, `flutter: ">=3.38"` (win32-driven; verify Flutter floor at scaffold) | Too low → resolution picks win32 5.x and the unlock silently dies |

## CI / Testing

| Layer | Tool | Where it runs | Notes |
|-------|------|---------------|-------|
| Static analysis | `flutter analyze` (strict casts/inference/raw-types) | All 3 runners | 0-issue gate = pana analysis points |
| Unit tests | `flutter test` | All 3 runners, headless | Channel-level fakes for engine/window; no native code needed |
| Native build gate | `flutter build windows\|linux\|macos` on example/ | Respective runner | The REAL gate for C++/GTK/Swift grafts — a plugin that compiles nowhere publishes nowhere. macOS: `CODE_SIGNING_ALLOWED=NO` |
| Integration tests | `integration_test` package: `flutter test integration_test -d windows\|linux\|macos` | windows-latest / macos-latest / ubuntu-latest | Real-window smoke: frameless set, NCHITTEST edge resize simulation, min/max enforcement, event stream delivery. Linux needs `xvfb-run -a` + libgtk-3-dev |
| Workflow shape | `subosito/flutter-action@v2`, matrix `fail-fast: false`, pub cache enabled | — | Upstream window_manager itself runs build.yml/lint.yml/test.yml (names verified on GitHub; contents not inspected) |
| Score check | `pana` step on Linux | — | Publish-blocking |

Practical: Windows integration runs are slow (full app build ~10-20 min) — make them a separate workflow job that isn't required for unit-test red/green, but IS required before publish.

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| window_manager 0.5.2 as base, single inline package | bitsdojo_window as base | Never — unmaintained 4+ years (`sdk >=2.17.0`), locks win32 ^5.1.1, requires runner surgery, no event stream. Reverse graft (frame base + API graft) would mean porting ~2,600 lines instead of ~830 |
| Single inline plugin (windows/ linux/ macos/ in one package) | Federated structure (bitsdojo's 5-package layout) | Only if third parties need to implement custom platforms. One maintainer + 3 platforms = federation is pure overhead |
| Adopt win32 ^6.4.0 in Dart layer | Move all frame metrics to C++ and depend on NO win32 package | The no-win32 variant is architecturally cleaner but forfeits the package's stated unlock value (hosts get win32 6.x from this dep) and wastes the host-validated migration knowledge. Keep win32 ^6.4.0 |
| Keep WM's onEvent-over-MethodChannel mechanism | Migrate events to EventChannel / package:web-style streams | Only post-v1 if event delivery proves lossy; v1 must stay a faithful fork to keep upstream-diffing cheap |
| Plugin-layer `RegisterTopLevelWindowProcDelegate` (WM path) | Keep bitsdojo's runner-hook (`bitsdojo_window_configure` in main.cpp) | Fallback only if a startup-timing edge case defeats the delegate path (bitsdojo's hook exists because it races window creation; WM's delegate demonstrably receives NCCALCSIZE/NCHITTEST, so the plugin path should hold) |
| Wait-and-watch libnativeapi | Base the package on nativeapi-flutter | Not yet — it's a README-level migration notice, not a stable published replacement. Revisit if it ships; its direction (unified C++ core) may eventually supersede this package |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| bitsdojo_window as runtime dependency | Locks `win32: ^5.1.1` (verified in its pubspec) — this is the exact dependency-tree lock the package exists to break; unmaintained since ~2021 | Graft its ~830 lines of frame C++ into the fork |
| Vendor-modifying bitsdojo in the host app (the abandoned PATCH 1/2 route) | Already tried and retracted in simple_player_flutter 2026-09-03; per-fork maintenance is a dead end | This standalone package |
| win32 5.x anywhere in the tree | 5.x semantics (`int` handles, plain ints for enums) would force re-migration later and defeat the unlock | win32 ^6.4.0 from day one; apply the host-validated extension-type migration list |
| Unconditional `return 0` in WM_GETMINMAXINFO (bitsdojo's bug, line 550-568) | Silently defeats `setMinimumSize`/`setMaximumSize` — the host project needed a two-channel same-value workaround | WM's cooperative pattern: apply ptMinTrackSize/ptMaxTrackSize only when set, fall through otherwise |
| Runner-template window message code | Any NCCALCSIZE/NCHITTEST logic in `windows/runner/` makes every host app patch its runner (bitsdojo's biggest adoption tax) | All message handling inside the plugin via `RegisterTopLevelWindowProcDelegate` |
| New API-compat shims for bitsdojo (`doWhenWindowReady`, `appWindow`, `MoveWindow`) | Out of scope per PROJECT.md; drags in bitsdojo's widget-coupled design | WM-style API (`windowFrameKit.waitUntilReadyToShow`, singleton) + frame config object |
| EventChannel rewrite in v1 | Pure deviation risk with zero user-visible gain at v1 | Keep `_EmitEvent` → `onEvent` loop |

## Stack Patterns by Variant

**If the plugin-layer delegate receives all needed messages (expected):**
- Zero runner changes; frame takeover is `setFrameTakeover(true)` (or `WindowOptions(titleBarStyle: TitleBarStyle.hidden)`) from Dart only
- Because `RegisterTopLevelWindowProcDelegate` demonstrably intercepts WM_NCCALCSIZE/WM_NCHITTEST/WM_GETMINMAXINFO at plugin layer in WM 0.5.2

**If startup timing defeats the delegate (windows flash white / inset glitch before Flutter draws):**
- Fall back to a minimal optional runner macro (bitsdojo-style one-liner), documented as opt-in
- Because the host project's white-border history proves the hidden-before-show ordering has real edge cases

**If libnativeapi stabilizes during development:**
- Keep this package's API stable, consider its C++ core underneath in v2
- Because upstream WM + screen_retriever both announce migration there; being API-stable makes that an implementation swap

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| win32 6.4.0 | Dart `^3.10.0`, ffi `^2.1.4` | Verified from its pubspec in cache. Hosts on Dart <3.10 cannot use this package's win32-carrier value |
| window_frame_kit (planned) | Flutter `>=3.38` (derived), dev-tested on 3.47.0 | Flutter floor = Dart 3.10's Flutter counterpart; confirm against template default at scaffold |
| window_manager 0.5.2 | screen_retriever `^0.2.2` (still latest), path `^1.8.2` | No dependency bumps needed; constraints are current as of 2026-09 |
| bitsdojo 0.1.x frame code | Needs its Dart side migrated to win32 6.x types (host-validated, ~7 call sites) | C++ side is win32-package-agnostic (pure Win32 API calls) — grafts cleanly |
| `flutter create` plugin template | Generates `*_c_api.h`-style include skeleton on Windows | Overlay fork files over template; keep template CMake structure so hosts need no build changes |

## Sources

- Pub cache (primary, HIGH): `window_manager-0.5.2` — pubspec, plugin.cpp HandleWindowProc (147/186/190), RegisterTopLevelWindowProcDelegate (107), _EmitEvent (125), listener surface, LICENSE copyright line
- Pub cache (primary, HIGH): `bitsdojo_window_windows-0.1.6` — bitsdojo_window.cpp frame handlers (155/206/288/334/438/446/550/606/612), lib/src inventory, `win32: ^5.1.1` lock, LICENSE copyright line
- Pub cache (primary, HIGH): `win32-6.4.0` — pubspec `sdk: ^3.10.0`, CHANGELOG 6.2.0-6.4.0 (GET_X/Y_LPARAM macros, native-hook removal), BSD-3 license
- Context7 `/halildurmus/win32` (HIGH) — official 5.x→6.x migration guide: extension-type handles, enum extension types, `Win32Result<T>`, BOOL→bool
- pub.dev live pages (HIGH): window_manager 0.5.2 (652k dl, 160 pts, deps list), win32 6.4.0 (Flutter Favorite), screen_retriever 0.2.2 latest, flutter_lints 6.0.0 latest
- pub.dev/help/scoring (MEDIUM for point weights — page no longer publishes per-category weights; 160 ceiling verified empirically on live package pages)
- docs.flutter.dev developing-packages (HIGH) — `flutter create --template=plugin --platforms=` flags and generated structure
- github.com/leanflutter/window_manager, github.com/bitsdojo/bitsdojo_window (HIGH for structure/names) — repo trees, macOS SwiftPM layout, migration notice to libnativeapi
- MIT license text (HIGH) — notice-preservation clause for derivative attribution
- Host-project handoff docs `.planning/research/PRIOR-CONTEXT-*.md` (HIGH, machine-verified 2026-09-03) — win32 6.4 type migration call-site list, GETMINMAXINFO bug, Flutter-child-covers-top-edge fact

---
*Stack research for: window_frame_kit — Flutter desktop window management plugin*
*Researched: 2026-09-04*
