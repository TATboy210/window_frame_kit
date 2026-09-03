# Project Research Summary

**Project:** window_frame_kit — Flutter desktop window management plugin (frame takeover + window capability API)
**Domain:** Flutter desktop plugin development (Win32/GTK/AppKit, pub.dev publishing)
**Researched:** 2026-09-04
**Confidence:** HIGH

## Executive Summary

window_frame_kit is a Flutter desktop plugin that combines two capabilities no single 2026 package provides: full window-manager capability API (the window_manager 0.5.2 surface) and native frame takeover (the bitsdojo_window capability — WM_NCCALCSIZE `return 0`, four-edge DPI-aware WM_NCHITTEST resize, working min/max size). Expert practice in this niche is fork-and-graft, not clean-room: window_manager (652k downloads, 160 pub points, actively maintained) is already a pure plugin that hooks window messages via `RegisterTopLevelWindowProcDelegate` — zero runner changes — and its ~3,300 lines of C++/GTK/Swift are the correct base. bitsdojo contributes only ~830 lines of frame code (the only mature NCHITTEST edge-dispatch implementation), grafted into a new `FrameController` module. The ecosystem gap is real and verified: bitsdojo is dead (locked to win32 5.x, unmaintained 4+ years), window_plus has 404'd from pub.dev and GitHub, and window_manager itself is drifting toward the nativeapi successor without frame capabilities.

The recommended approach is a single inline plugin (NOT federated) scaffolded with `flutter create --template=plugin --platforms=windows,linux,macos`, with a two-zone code layout: verbatim-ported window_manager files (mergeable against the upstream git remote) plus `// FRAME:`-marked new files for the graft. Windows is the real engineering center of gravity — Linux and macOS frame takeover are property flips window_manager already ships (GTK `set_decorated`, NSWindow `fullSizeContentView`), so those phases are ports plus widgets and can run parallel to the Windows graft. The core insight from host-project hard experience: frame hit-testing MUST live in the plugin layer (the engine's FLUTTERVIEW child window covers the top edge — runner-level NCHITTEST never fires there), and the bitsdojo GETMINMAXINFO unconditional `return 0` bug that silently kills `setMinimumSize` must be rewritten cooperatively.

Key risks, all with proven mitigations from the host project's dual-package migration: (1) restructuring while grafting destroys the upstream merge path — mitigated by verbatim ports, upstream git remote from day one, and a DEVIATIONS.md ledger; (2) MIT attribution errors — stack the LICENSE with both upstream © lines verbatim before the first graft commit; (3) win32 6.x extension-type traps — port in one mechanical pass using the host-validated checklist; (4) maximize overshoot and message-routing fights — keep style bits (NCCALCSIZE route, never WS_POPUP), one owner per window message, and clamp maximized rects to `rcWork` with per-DPI metrics; (5) headless CI false confidence — every visual claim (no white border, snap flyout, cursor morphing) requires a dated real-machine checklist sign-off. Fullscreen zero-flash is an exploration goal, never a v1 gate — two machine-verified mitigation cycles already failed.

## Key Findings

### Recommended Stack

Fork window_manager 0.5.2 (MIT) as the base — it is the exact architecture needed (single MethodChannel, WindowProc delegate, 17-callback listener) — and graft bitsdojo_window_windows 0.1.6's frame code as the differentiating capability.

**Core technologies:**
- **window_manager 0.5.2 source (MIT, fork)**: base — full ~60-method capability API + event stream, already plugin-layer (no runner changes), actively maintained upstream
- **bitsdojo_window 0.1.6 frame code (MIT, graft, ~830 lines)**: DPI-aware NCHITTEST 4-edge resize (`GetSystemMetricsForDpi`), full `return 0` NCCALCSIZE, `SC_MOVE|HTCAPTION` startDragging — the only mature implementation of these
- **Flutter SDK floor `>=3.38` / Dart `>=3.10.0`**: forced by win32 6.4.0's SDK floor — do NOT widen below this or pub resolution silently falls back to win32 5.x
- **screen_retriever ^0.2.2**: display info (inherited from WM; current latest)
- **flutter_lints ^6.0.0 + pana + dependency_validator**: analysis/pub-points tooling; target pana 160
- **`flutter create --template=plugin`**: scaffold; overlay forked sources keeping template CMake structure

**Critical divergence note (decision needed at planning):** STACK.md recommends carrying `win32: ^6.4.0` as a Dart dependency (the "unlock" — hosts get win32 6.x, freeing file_picker 12 / secure_storage 11), with bitsdojo's Dart geometry layer migrated to win32 6.x types. ARCHITECTURE.md's anti-pattern 3 argues the opposite: all frame logic in C++, pubspec deps only `path` + `screen_retriever`, because a win32 major-version coupling recreates the lock this package exists to escape. Resolution recommendation: do the C++-only architecture (ARCHITECTURE.md is the more rigorous position and matches the base package), and document the win32-6-compatibility selling point as "no win32 pin at all" — strictly better for downstream resolution than carrying ^6.4.0. The win32 6.x migration checklist remains essential reference material for any Dart-side geometry code.

### Expected Features

**Must have (table stakes, v1):**
- window_manager-parity capability API (~60 methods) + WindowListener event stream — anything less reads as broken next to the 652k-download leader
- Frame takeover: NCCALCSIZE `return 0` + plugin-layer WM_NCHITTEST four-edge resize (inseparable pair — one phase, never sold separately)
- Cooperative GETMINMAXINFO so `setMinimumSize`/`setMaximumSize` actually work (the bug that motivates the project)
- `setPreventClose` + `onWindowClose`, `startDragging` + drag-area behavior widget, `setTitleBarStyle(hidden)`, per-monitor DPI correctness
- Three-platform v1 (Windows reference, macOS NSWindow, Linux GTK), minimal but smoke-tested
- pub.dev publish-readiness: stacked MIT LICENSE + upstream attribution, README with dual-package migration map, example app, CHANGELOG

**Should have (differentiators):**
- #3 HTMAXBUTTON Snap Layouts flyout (Win11) — small cost, high visibility, neither competitor has it
- #4 Configurable resize border width (fixes bitsdojo hardcode)
- #5 Cooperative GETMINMAXINFO as a documented coexistence guarantee
- #6 Zero-runner-configuration frameless (v1.x; spike first — init ordering with `waitUntilReadyToShow` is entangled)

**Defer (v2+):**
- #8 Fullscreen zero-flash — exploration only; collect symptoms from the host's reverted Plans A/B before any re-attempt
- Window effects (mica/acrylic — flutter_acrylic owns this), tray/menus, multi-window (explicit anti-feature), bitsdojo/WM API compat shims (anti-feature — migration mapping table instead)
- Taskbar flash (`flash()`), VirtualWindowFrame widget — v1.x candidates

### Architecture Approach

Single inline plugin, two-zone code layout: verbatim window_manager ports (Dart facade, listener, per-platform plugins) plus new `// FRAME:`-marked modules. On Windows, frame logic lives in a `FrameController` (grafted bitsdojo code) invoked by the plugin's WindowProc delegate when `custom_frame_` is on, plus a `SetWindowSubclass` on the engine's FLUTTERVIEW child window returning `HTTRANSPARENT` at border zones so hits fall through to the top-level delegate. Install the subclass at `ensureInitialized` via `GetWindow(topLevel, GW_CHILD)` (class-name-independent), replacing bitsdojo's CBT hook; MUST `RemoveWindowSubclass` on teardown. Linux/macOS need no native frame graft — property flips window_manager already ships.

**Major components:**
1. **Dart facade + WindowListener** — static singleton, one bidirectional MethodChannel (`window_frame_kit` — never reuse `'window_manager'`), 15 event-name strings kept identical so hosts port by changing imports
2. **Windows FrameController + child subclass** — the graft: NCCALCSIZE custom-frame branch (with maximized work-area clamp), NCHITTEST edge dispatch, cooperative GETMINMAXINFO, configurable edge width
3. **Windows WindowProc delegate** — single owner of NCCALCSIZE/NCHITTEST/GETMINMAXINFO/DPICHANGED; returns a result only when definitively handled
4. **Linux (.cc GTK) + macOS (SwiftPM Swift) ports** — direct ports, parallel to Windows work
5. **Example app** — triple-duty: pana docs points, real-machine verification vehicle, canonical init-sequence documentation

### Critical Pitfalls

1. **Restructuring while grafting destroys the upstream merge path** — verbatim ports, upstream git remote + DEVIATIONS.md ledger from day one; intentional bug-fixes (GETMINMAXINFO) ledgered so merges don't re-import them
2. **MIT attribution wrong** — stack LICENSE with window_manager © (LiJianying) and bitsdojo © (Bogdan Hobeanu) lines verbatim before the first graft commit; pub.dev will not catch violations
3. **Style-bit stripping kills snap/cursor/flyout** — never WS_POPUP; NCCALCSIZE route with full `WS_OVERLAPPEDWINDOW` kept; implement the full HT contract including HTMAXBUTTON and HTCAPTION
4. **Message-routing fights / maximize overshoot** — one owner per message; delegate returns only on definite handling; clamp maximized rects via `GetSystemMetricsForDpi(SM_CXFRAME+SM_CXPADDEDBORDER)` or `rcWork`; handle `WM_DPICHANGED`
5. **Headless CI false confidence** — two-tier discipline: CI (analyze/unit/contract/compile/pana) + dated real-machine checklist sign-offs; "console no-crash ≠ UI correct"

## Implications for Roadmap

Based on research, suggested phase structure (mirrors ARCHITECTURE.md's validated build order):

### Phase 1: Package Skeleton + License Foundation
**Rationale:** Every downstream risk mitigation (Pitfalls 1, 2, 10) is an architecture decision that must precede any grafted code.
**Delivers:** Scaffolded 3-platform plugin, channel renamed `window_frame_kit`, stacked MIT LICENSE with both upstream © lines, upstream git remote, DEVIATIONS.md, pana/dry-run wired into CI, example app shell.
**Addresses:** Table Stakes C (publish readiness); Differentiator #9 (README positioning).
**Avoids:** Pitfalls 1, 2, 10.

### Phase 2: Windows Base Port (window_manager parity)
**Rationale:** The graft must land on a proven-identical-to-window_manager base, or port regressions and graft regressions are indistinguishable. Exit gate: behavioral parity of ~60 methods + events on the example app.
**Delivers:** Verbatim Dart facade + listener + Windows plugin port; channel contract tests; native build gate green.
**Uses:** window_manager 0.5.2 sources; flutter create template.
**Implements:** Components 1 and 3; Patterns 1, 5, 6.
**Avoids:** Pitfall 6 partially (delegate contract established here).

### Phase 3: Windows Frame Graft (the core differentiator)
**Rationale:** Depends on Phase 2's proven base; the largest and riskiest native work; must be one unit (NCCALCSIZE + NCHITTEST + GETMINMAXINFO land together).
**Delivers:** FrameController (child subclass via GW_CHILD, custom-frame NCCALCSIZE branch with maximized clamp, DPI-scaled 4-edge NCHITTEST, cooperative GETMINMAXINFO, configurable edge width), HTMAXBUTTON Snap Layouts.
**Implements:** Component 2; Patterns 2, 3, 4; Differentiators #1, #3, #4, #5.
**Avoids:** Pitfalls 3, 4, 5, 6, 7. Real-machine checklist mandatory (maximized multi-monitor, 100%/150% DPI, cold start, snap flyout, cursor morphing).
**Research flag:** Worth `--research-phase` during planning — Win32 message-level work, and the child-subclass install timing (`ensureInitialized` vs fallback CBT hook) is explicitly marked "verify at implementation time."

### Phase 4: Linux + macOS Ports (parallelizable, can split into two phases)
**Rationale:** Zero code dependency on the Windows graft — the roadmap's parallelization opportunity. macOS needs the non-optional `canBecomeKeyWindow` override budgeted up front.
**Delivers:** GTK + Swift ports of the base, drag/resize widgets, per-platform feature matrix in README, silent-true sweep (typed unsupported errors).
**Implements:** Component 4; Pattern 9 discipline (documented deltas, not fake parity).
**Avoids:** Pitfall 9.

### Phase 5: Example Hardening + Publish 0.1.0
**Rationale:** The example is the verification vehicle; publishing 0.1.0 early claims the name and starts the feedback loop; 1.0.0 waits for host validation.
**Delivers:** 3-platform example exercising frame + events + min/max + fullscreen; pana ≥140 verified; dry-run clean; 0.1.0 published.
**Avoids:** Pitfalls 10, 11.

### Phase 6: Host Integration (simple_player_flutter swap)
**Rationale:** PROJECT.md's "Validated" bar — the host swap doubles as the real-world regression suite and validates the API before 1.0.0.
**Delivers:** Dual-package removal (bitsdojo + window_manager), migration-map-verified swap, host quality gates (analyze 0 error, full test suite, real-machine resize/fullscreen smoke).
**Avoids:** Integration gotchas (delegate coexistence during migration, texture-heavy top-edge verification).

### Deferred / post-v1
- Zero-runner-configuration spike (#6), flash() taskbar, VirtualWindowFrame widget (v1.x)
- Fullscreen zero-flash exploration (v2+) — hard precondition: collect symptoms from the host's reverted Plans A/B first; never blocks release

### Phase Ordering Rationale

- Phases 2→3 ordering is the single most important sequencing decision: graft on a proven-parity base keeps regression sources distinguishable
- 4 parallels 2–4 (independent platforms); roadmap can run it as one phase or two
- License/skeleton work first because MIT attribution must exist before the first commit containing grafted code
- Fullscreen zero-flash is deliberately outside the critical path — re-litigating that demotion mid-roadmap is itself a documented pitfall

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 3 (Windows Frame Graft):** message-level Win32 surgery; child-subclass install timing and NCCALCSIZE interaction with WM's existing frameless branches need implementation-time verification
- **Phase 4 (Linux/macOS):** Linux CSD/SSD compositor split (Wayland resize edges) is the main unknown; macOS key-window subclass specifics

Phases with standard patterns (skip research-phase):
- **Phase 1 (Skeleton):** `flutter create` template + license stacking — fully documented
- **Phase 2 (Base Port):** verbatim copy from pub-cache sources, line references already mapped
- **Phase 5 (Publish):** pana/dry-run tooling is standard

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All versions/APIs verified from local pub-cache sources at exact pinned versions + live pub.dev + Context7 |
| Features | MEDIUM | API surfaces cross-checked (Context7 MEDIUM, pub pages LOW); ecosystem-status claims (window_plus 404, nativeapi direction) from web search — flagged LOW where unverifiable |
| Architecture | HIGH | Direct primary-source reading of both upstreams with file:line evidence; host-project experiments corroborate |
| Pitfalls | HIGH | Highest-risk items machine-verified in the host project's dual-package migration, archived in PRIOR-CONTEXT files |

**Overall confidence:** HIGH

### Gaps to Address

- **win32 dependency architecture (STACK vs ARCHITECTURE contradiction):** decide at planning — recommend C++-only (no win32 pub dep); the win32 6.x checklist stays as reference. This affects the "unlock" selling-point wording.
- **Child-subclass install timing:** `ensureInitialized` may run before FLUTTERVIEW exists; fallback (brief CBT hook or WM_CREATE watch) must be verified in Phase 3.
- **Linux Wayland resize:** CSD windows may lack native resize on some compositors — widget fallback (VirtualWindowFrame/DragToResizeArea) scope needs definition in Phase 4 planning.
- **Delegate-before-runner ordering with coinstalled legacy plugins:** migration-period behavior when hosts still carry window_manager/bitsdojo — document, test in Phase 6.
- **Flutter floor (`>=3.38`) exactness:** verify against template output at scaffold time; win32 6.x's Dart ≥3.10 floor is the binding constraint.

## Sources

### Primary (HIGH confidence)
- Pub cache, exact pinned versions: `window_manager-0.5.2` (plugin.cpp/window_manager.cpp/linux .cc/Swift, file:line mapped), `bitsdojo_window_windows-0.1.6` (frame handlers, line-mapped), `bitsdojo_window_macos-0.1.4`, `win32-6.4.0` pubspec/changelog
- PRIOR-CONTEXT-session-handoff.md / PRIOR-CONTEXT-window-layer.md — machine-verified host-project findings (win32 6 type checklist, GETMINMAXINFO bug, child-window hit-test dead zone, reverted fullscreen Plans A/B)
- Host memories: project_bitsdojo_dual_package_migration, bugfix_white_border_frameless, feedback_uat_evidence_standard, project_fullscreen_style_authority

### Secondary (MEDIUM confidence)
- Context7: `/leanflutter/window_manager`, `/halildurmus/win32` (5.x→6.x migration guide), `/bitsdojo/bitsdojo_window`
- Microsoft Learn (WM_NCHITTEST/HTMAXBUTTON, DWM custom frame), Apple NSWindow.StyleMask, Arch Wiki SSD/CSD
- pub.dev scoring help, dart.dev publishing/license guides

### Tertiary (LOW confidence)
- pub.dev page fetches (download counts, maintenance status), web-search ecosystem claims (window_plus 404, nativeapi 0.2.0, flutter_acrylic) — existence/status verified only via search

---
*Research completed: 2026-09-04*
*Ready for roadmap: yes*
