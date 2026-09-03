# Pitfalls Research

**Domain:** Flutter desktop window-management plugin (window_frame_kit — window_manager 0.5.2 base graft + bitsdojo frame takeover, win32 6.x, Windows/Linux/macOS, pub.dev publish)
**Researched:** 2026-09-04
**Confidence:** HIGH — the highest-risk items (win32 6 type traps, WndProc routing, NCCALCSIZE behavior, GETMINMAXINFO conflict, fullscreen-flash dead ends) are corroborated by first-hand machine-verified findings from the simple_player_flutter dual-package migration (2026-09-03, archived in `PRIOR-CONTEXT-*.md`), cross-checked against ecosystem sources. Marked per-item below.

## Critical Pitfalls

### Pitfall 1: Restructuring while grafting destroys the upstream merge path

**What goes wrong:**
window_manager is actively maintained (0.5.x line, ~652k downloads, updated ~60 days before the 2026-09 scan — prior-context fact). If the new package renames files, reformats, or inlines upstream code into new structures while copying it, `git merge` against upstream becomes a conflict on every file. Every future upstream release (bug fixes, new Windows APIs, community patches) must then be hand-ported forever — or silently missed. This is exactly the death spiral bitsdojo entered: 4 years unmaintained, locked to win32 ^5.1.1, ecosystem abandoned it.

**Why it happens:**
A "new package with its own API" feels like a clean-room rewrite, so devs copy-and-modify freely. The merge cost only shows up months later when upstream ships 0.6.0 with a fix you need.

**How to avoid:**
- Two-zone layout: grafted upstream files stay in their own zone, kept as byte-identical as feasible (same file names, same function bodies); all new code (frame takeover, coordinators, new API surface) lives in *new* files that call into the upstream zone.
- Add upstream as a `git remote` from day one; schedule a merge review on every upstream release tag. Merging (not rebasing) preserves shared history and keeps future merges small.
- Maintain a `DEVIATIONS.md` ledger: every place the package intentionally diverges from upstream, anchored to upstream `file:line`, with the reason. This is the same discipline that made the vendor-PATCHES.md workable in the host project (`PRIOR-CONTEXT-session-handoff.md` §③).
- Known inherited-bug fixes (e.g. bitsdojo's GETMINMAXINFO unconditional `return 0`, `bypass_wm_size` dead branch) are the *intentional* deviations — ledger them so a future upstream merge doesn't silently re-import the bug.

**Warning signs:**
Diff against the upstream snapshot shows edits scattered *inside* upstream function bodies rather than isolated in new files; upstream release notes mention a bug you recognize from your own codebase.

**Phase to address:** Package skeleton / graft phase (architecture decision that gates everything downstream).

---

### Pitfall 2: MIT attribution done wrong — pub.dev will not catch it

**What goes wrong:**
MIT requires the original copyright notice and permission notice to be preserved in "all copies or substantial portions" of the software. Deriving from window_manager (MIT) and grafting bitsdojo_window_windows code (MIT) means both upstream copyright lines must survive verbatim. pub.dev explicitly does *not* verify that license text is where it should be — dart.dev states the publisher is responsible for complying with dependency license requirements. The classic violation: replacing the upstream LICENSE with your own MIT text and your own copyright, dropping theirs.

**Why it happens:**
MIT is famously permissive and devolvable; the only real obligation (notice preservation) is one sentence that everyone assumes applies to "someone else's fork".

**How to avoid:**
- Stack the LICENSE: keep the original MIT text with each upstream copyright line kept verbatim (window_manager's © line, bitsdojo's © line — copy the exact strings from the upstream LICENSE files at graft time, do not retype from memory), then append your own copyright line for modifications.
- Preserve copyright headers in copied source files where upstream has them; add a header comment on heavily-derived files pointing at the LICENSE entries.
- Add a "Derivation" section to README: what came from window_manager, what from bitsdojo, what is net-new. This costs 10 lines and inoculates against the most common community complaint on fork-packages.
- Adding your own copyright for your modifications is permitted and does not violate MIT.

**Warning signs:**
`pana` license check passes (it only checks that *a* LICENSE exists) but the upstream © lines are absent; grafted files carry rewritten headers.

**Phase to address:** Package skeleton phase — set the LICENSE stack before the first commit containing grafted code.

---

### Pitfall 3: win32 6.x extension types — mechanical port traps that hide semantic bugs

**What goes wrong:**
win32 6.0.0 is a full API redesign. All of the 5.x-era code (all of bitsdojo, all pre-6 window_manager snippets, every Stack Overflow answer) fails to compile against it, and the failure modes are not uniform:
- `HWND` is `extension type const HWND(Pointer _)` — constructing from a raw int must be `HWND(Pointer.fromAddress(n))` (verified hands-on, `PRIOR-CONTEXT-session-handoff.md` §⑤).
- `GetSystemMetricsForDpi` returns `Win32Result<int>` — the value is `.value`; the result atomically captures the last-error state. Forgetting `.value` is a compile error (good), but naive mechanical ports have routed the error-code side through as if it were the metric.
- All enums are extension types over int (`SYSTEM_METRICS_INDEX` is directly constructible from int) — code doing raw `int` math on them compiles but loses type safety.
- Handle null-checks are `.isNull`, not `== 0`; `BOOL` maps uniformly to `bool`; `Finalizer` auto-cleanup is gone; `TEXT()`/`BSTR`/`Guid` helpers are removed.

Additionally, win32 6.x raises the floor to **Dart SDK ≥ 3.10** — the package's declared minimum Flutter/Dart compatibility must account for this (dev env Flutter 3.47 / Dart 3.13 is fine; the *minimum supported version* decision must not silently exclude win32 6's floor).

**Why it happens:**
Extension types are ABI-compatible with their representation, so half-ported code can compile and run while carrying wrong types through FFI boundaries; 5.x-era muscle memory (`CreateWindowEx(...) == 0`) produces compile errors one at a time, and devs "fix" each call site ad hoc rather than porting systematically.

**How to avoid:**
- Port in one mechanical pass using the verified type checklist (already hands-on validated: `HWND(Pointer.fromAddress(n))`, `Win32Result.value`, direct enum construction, `GetDpiForWindow`, `MonitorFromWindow`, `GetMonitorInfo` — session-handoff §⑤).
- Centralize handle construction/comparison in small helpers instead of scattering `Pointer.fromAddress` calls.
- Decide and document the minimum Flutter version explicitly, including win32 6.x's Dart ≥3.10 floor; run `pub outdated` before publishing.

**Warning signs:**
Compile errors clustered at handle construction lines (24/26/58/90/91/101/204 in the vendor test were exactly this pattern); a metrics function returning absurd values (error code passed as metric).

**Phase to address:** win32 6.x base-port phase.

---

### Pitfall 4: Maximize overshoot (the classic +pad bug) and the GETMINMAXINFO fight

**What goes wrong:**
Two compounding failures on the NCCALCSIZE-frameless route:
1. With `WS_THICKFRAME` kept (required for snap/shadow — see Pitfall 5), Windows maximizes the window beyond the monitor bounds by the frame padding `SM_CXFRAME + SM_CXPADDEDBORDER`. Content edges get cropped, and on multi-monitor the window bleeds into adjacent monitors. Per-monitor DPI makes it worse: `GetSystemMetrics` returns system-DPI values, so the padding is wrong on mixed-DPI setups (need `GetSystemMetricsForDpi`).
2. bitsdojo's inherited `WM_GETMINMAXINFO` hook **unconditionally returns 0**, which silently arbores window_manager's `setMinimumSize` — proven in the host project, where minSize only worked via a "double-channel same-value" workaround (prior-context §③). Grafting that hook as-is re-imports the bug this package explicitly promises to fix (PROJECT.md Active requirement).

**Why it happens:**
The NCCALCSIZE `return 0` recipe is copied from tutorials that never test maximized state; the GETMINMAXINFO bug is invisible in single-monitor dev setups with default sizes.

**How to avoid:**
- In `WM_NCCALCSIZE` (`wParam == TRUE`), when `IsZoomed(hwnd)`, shrink the proposed rect by the frame padding — computed via `GetSystemMetricsForDpi(SM_CXFRAME, GetDpiForWindow(hwnd)) + SM_CXPADDEDBORDER` — or clamp to `MONITORINFO.rcWork` via `MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)` + `GetMonitorInfo` (use `rcMonitor` only if covering the taskbar is intended).
- Watch the documented edge cases: autohide taskbar loses hover-reveal when maximized to `rcMonitor`; Windows sends an *interim* NCCALCSIZE during maximize transitions (Chromium intercepts it to avoid stale-border glitches).
- Rewrite GETMINMAXINFO cooperatively: compute min/max constraints and merge with DefWindowProc's handling rather than `return 0`; expose minSize through the plugin API so hosts don't need the double-channel workaround.
- Handle `WM_DPICHANGED` around maximize/restore — the known Windows bug where a maximized window restores to the wrong monitor's work area.

**Warning signs:**
Maximized window crops content edges or bleeds into the second monitor; `setMinimumSize` silently does nothing; content mis-centering only when maximized; min/max limits work at 100% DPI but not 150%.

**Phase to address:** Windows frame-takeover phase (NCCALCSIZE + NCHITTEST + GETMINMAXINFO land together and must be verified as a unit: normal / maximized / multi-monitor / per-monitor-DPI).

---

### Pitfall 5: Style-bit stripping kills snap, snap layouts, and cursor feedback

**What goes wrong:**
The tempting "frameless" implementation — strip `WS_OVERLAPPEDWINDOW` down to `WS_POPUP` — silently loses: Aero Snap (Win+Arrow), the Windows 11 Snap Layouts flyout, DWM resize borders, and minimize/restore animations. Snap Layouts specifically require the window to be top-level, have `WS_MAXIMIZEBOX`/`WS_THICKFRAME`/`WS_SYSMENU`, *and* have `WM_NCHITTEST` return `HTMAXBUTTON` when the cursor is over your custom maximize button — without which the flyout never appears and there is no error, just a missing feature.

**Why it happens:**
`WS_POPUP` looks correct in screenshots (that's what the tutorials show), and the loss is all invisible-until-used behavior: keyboard snap, flyout, cursor morphing at edges.

**How to avoid:**
- Keep the full style set and remove the frame *visually* via `WM_NCCALCSIZE return 0` — the approach this package already mandates (PROJECT.md: NCCALCSIZE return 0 route).
- Implement the full `WM_NCHITTEST` contract: edge returns (`HTLEFT`…`HTBOTTOMRIGHT`) for four-edge resize — with edge width **configurable**, fixing bitsdojo's hardcoded value (host-project improvement candidate, prior-context §④) — plus `HTCAPTION` on the drag region and `HTMAXBUTTON` on the maximize button.
- Cursor feedback falls out of correct HT returns; if you implement resize by *manual* mouse tracking instead of NCHITTEST, you must also hand-implement cursor shapes — a known rabbit hole (bitsdojo's `SmartDragToResizeArea` fallback existed precisely because the frameless route dropped system resize).

**Warning signs:**
Win+Arrow does nothing; no flyout on maximize-button hover; resize works but the cursor stays a default arrow at edges; window can't be dragged by the custom title bar.

**Phase to address:** Windows frame-takeover phase; add snap/cursor items to the real-machine verification checklist.

---

### Pitfall 6: WndProc ownership — the plugin must own the message seam, and order decides everything

**What goes wrong:**
Three proven failure modes from the host project, all message-routing:
1. The Flutter **child window covers the top edge**, so the runner's `WM_NCHITTEST` never fires there — top-edge resize was dead while left/right/bottom worked (the plugin leaves real non-client area on those sides). Any plan that puts frame hit-testing in the runner template is structurally broken.
2. A plugin's **top-level delegate returned early**, preempting the runner's NCCALCSIZE `return 0` and leaving the Win11 8px top inset (the white-border root cause).
3. Ecosystem confirmation: `RegisterTopLevelWindowProcDelegate` delegates run **before** the runner's window delegate; returning a value short-circuits the chain. Raw `SetWindowLongPtr(GWLP_WNDPROC)` subclassing inserts at the chain head and must forward via `CallWindowProc` or runner handlers silently die.

For this package the pitfall is architectural: window_manager 0.5.2's frame handling was written assuming *it* decides NCCALCSIZE behavior; grafting bitsdojo's frame code on top creates **two** components editing the same message outcomes. Conflicting returns produce intermittent, host-runner-dependent bugs (white border appearing on some machines/templates, not others).

**Why it happens:**
Message ordering is invisible; each layer looks right in isolation, and the bug only manifests as an interaction.

**How to avoid:**
- One owner per message: the plugin's top-level proc delegate exclusively owns `WM_NCCALCSIZE` / `WM_NCHITTEST` / `WM_GETMINMAXINFO` / `WM_DPICHANGED`; the runner template stays stock (PROJECT.md constraint — zero self-written window message code in the host).
- Delegate contract: return a result *only* when this package definitively handles the message; otherwise let the chain continue. Never assume a later handler's return value.
- If low-level subclassing is ever needed, save the original proc and always `CallWindowProc`-forward.
- Design for coexistence: document behavior when another plugin also registers a top-level delegate (ordering is registration order — deterministic but host-controlled), since hosts migrating from the dual-package setup may still carry window_manager or bitsdojo.
- Add a contract test with a stock Flutter runner template (CI can compile-check; the behavior itself is real-machine — see Pitfall 10).

**Warning signs:**
Top edge not resizable while other edges are; an 8px white/light strip at the window top on Win11; frame behavior differs between two hosts with different runner templates; drag works but resize doesn't (or vice versa).

**Phase to address:** Windows frame-takeover phase — establish the single-owner message architecture *before* porting NCCALCSIZE/NCHITTEST code.

---

### Pitfall 7: Frame-takeover init order — hidden/show sequencing resets the frameless state

**What goes wrong:**
The host project's white-border root cause had a second half: `window_manager`'s *hidden* branch leaves the Win11 top inset, and calling `hidden` **after** frameless configuration resets `is_frameless_` — the working sequence was `waitUntilReadyToShow` callback → `setAsFrameless` (prior-context memory: "hidden 会重置 is_frameless_ 须在其后调"). A plugin that exposes frame takeover without encoding this ordering produces the classic heisenbug: frameless works on hot restart but not cold start, or the 8px inset appears only on first launch.

**Why it happens:**
Plugin APIs take calls in any order; nothing enforces "after the window is ready" at the API level, and the wrong order silently succeeds.

**How to avoid:**
- Encode the canonical init sequence in the package: expose a single setup entry point (or assert/error when frame takeover is requested before window-ready), and make the plugin internally idempotent against re-issued style changes.
- Ship the exact sequence in the example app — the example *is* the documentation for this (Pitfall 9).
- Verify cold start explicitly in the checklist (Pitfall 10).

**Warning signs:**
8px top strip only on first launch; differences between cold start / hot restart / hot reload; hosts asking "why do I need to call X before Y".

**Phase to address:** Windows frame-takeover phase; enforced by example app + verification checklist.

---

### Pitfall 8: Fullscreen flash chased with per-symptom hacks — both prior routes already failed

**What goes wrong:**
Fullscreen toggling on a frameless window flashes the legacy frame. The host project tried and **reverted** both ecosystem-standard mitigations: Plan A (FFI bridge controlling DWM-level style switching) and Plan B (`DwmSetWindowAttribute` disabling transitions) — both "实机不理想" (prior-context §六). The ecosystem facts explain why: `DWMWA_TRANSITIONS_DISABLED` (attribute 3) only kills DWM *animation*, not the one-frame legacy-frame paint from the style-change redraw itself; style toggling (`WS_OVERLAPPEDWINDOW` add/remove + `SWP_FRAMECHANGED`) repaints the non-client area by design. Known additional mitigations exist (single `SetWindowPos` combining style+geometry, `WM_SETREDRAW`, `DWMNCRP_DISABLED`, `DWMWA_CLOAK` during transition), but the structural fix is to **never toggle styles**: keep `WS_OVERLAPPEDWINDOW` permanently and make fullscreen a pure bounds change — which is exactly what the NCCALCSIZE route provides. Also relevant: `wm.setFullScreen` on frameless windows has a known defect (prior-context fact), and DWM attribute probes on Win11 26200 already produced errors in the host (attribute 34/35/36, `hr=0x-7ff8ffa9`) — so capability probing must be advisory, never a startup gate.

**Why it happens:**
The symptom (a single frame) invites micro-fixes (attributes, redraw suppression), each of which passes a screenshot test but fails on real hardware — the project already spent two machine-verified cycles proving this.

**How to avoid:**
- Architectural commitment from day one: frameless = NCCALCSIZE route with styles kept, fullscreen = bounds change. This makes the flash structurally smaller even if not fully eliminated.
- Honor the PROJECT.md demotion: zero-flash is an *exploration target*, not a v1 gate — do not let it block the release path (this decision was made deliberately; re-litigating it mid-roadmap is itself a pitfall).
- Before re-attempting flash elimination, follow the archive instruction: collect the exact symptoms observed when Plans A/B were reverted (PRIOR-CONTEXT §六 requirement) — otherwise round three repeats round two.

**Warning signs:**
Implementation accretes DWM attribute toggles around every transition; flash appears only on some machines/compositor states (a sign the fix is fighting DWM timing, not addressing structure).

**Phase to address:** Structure decision in the Windows frame-takeover phase; flash elimination deferred to a post-v1 exploration phase.

---

### Pitfall 9: Cross-platform "symmetry" illusion — one API, three incompatible decoration models

**What goes wrong:**
The three platforms do not share a decoration model: Windows = style bits + NCCALCSIZE; Linux/GTK = SSD (WM-drawn, `xdg-decoration` negotiation) vs CSD (`GtkHeaderBar`) split — `gtk_window_set_decorated(false)` can lose resize edges and shadow on some compositors/Wayland; macOS = `NSWindow.styleMask`, where `borderless` windows **cannot become key/main** unless `canBecomeKeyWindow`/`canBecomeMainWindow` are overridden (window_manager ships a custom `FlutterWindow` subclass exactly for this), and the hidden-titlebar look requires the `titled` + `fullSizeContentView` + `titlebarAppearsTransparent` combination instead. A Windows-mentality API mapped onto the other two produces silently degraded behavior: no shadow, dead resize, windows that won't take focus.

**Why it happens:**
The package's value proposition is "one API", which psychologically pressures implementers into pretending the platforms are symmetric. Flutter's own desktop_embedding discussions (issue on frameless/transparent windows) confirm the models are not 1:1 mappable.

**How to avoid:**
- Publish a per-platform feature matrix in the README (v1 ships three platforms *with documented deltas*, not three platforms pretending to be identical). This is the feature-matrix discipline the question asks for: every API method gets a per-platform support cell.
- Unimplemented or degraded capabilities must be *visible*: return a typed unsupported error (or documented no-op + a capability-query getter) — never a silent `true`. The silent-true pattern is how bitsdojo accumulated "works on Windows, mysterious on Linux" bug reports.
- Per-platform real-machine smoke tests (GNOME/X11, GNOME/Wayland, macOS Sonoma+) — headless CI cannot cover any of this (Pitfall 10).
- macOS-specific: the key-window subclass is not optional; budget for it in the macOS phase rather than discovering it in UAT.

**Warning signs:**
Platform channels returning `true` for calls with no macOS/Linux implementation; README claiming parity without a caveat table; issue reports of "no shadow" / "can't focus" from Linux/macOS users.

**Phase to address:** Linux and macOS phases — write the feature matrix *before* implementing, use it as the phase's acceptance surface.

---

### Pitfall 10: pub.dev publishing self-inflicted wounds (score, tags, versioning, name)

**What goes wrong:**
pub.dev awards points across six categories; a window plugin loses points in predictable ways:
- **Documentation:** requires an example per the package layout convention *and* ≥20% of public API members documented. A plugin with a large API surface (window_manager's is big) easily falls under 20%.
- **Platform support:** tags are detected from the plugin structure and import graph — a malformed `pubspec.yaml` plugin declaration (wrong `pluginClass` / `ffiPlugin` / platform keys) drops windows/linux/macos tags; iOS/macOS plugins additionally need Swift Package Manager support for full marks.
- **Conventions:** CHANGELOG heading/version formatting must render on pub.dev; invalid or non-https URLs in pubspec lose points.
- **Analysis/dependencies:** zero analyzer issues and up-to-date dependencies required — the win32 ^6.x base is an *asset* here (it's the whole reason the package exists), but any stale transitive pin re-creates the disease being cured.
- **Versioning:** 0.x signals instability (breaking changes allowed at minor bumps). Publishing 1.0.0 before the host integration validates the API means every API fix costs a major version; sitting in 0.x forever reads as abandoned (bitsdojo's fate at 0.1.6).
- **Name:** `window_frame_kit` was verified unoccupied 2026-09-04 (PROJECT.md). Unpublished packages don't hold the name — squatting is real on pub.dev.

**Why it happens:**
Publishing is done once, at the end, under time pressure; scoring rules are only read when the score is already bad.

**How to avoid:**
- Run `dart pub publish --dry-run` and `pana` locally from the first skeleton phase onward, not just at release; wire it into CI.
- Example app is triple-duty: pana documentation points, the manual-verification vehicle (Pitfall 11), and the derivation/usage documentation (Pitfall 7). Build it early, keep it runnable on all three platforms.
- Ship 0.1.0 once dry-run is clean (claims the name, starts the feedback loop); go 1.0.0 only after the simple_player_flutter integration validates the API (PROJECT.md success metric).
- Use a verified publisher if available (recommended, not mandatory).

**Warning signs:**
`pana` report below par on documentation/platforms; example folder absent or Windows-only; `pub outdated` showing stale pins; version hesitancy delaying first publish.

**Phase to address:** Skeleton phase (structure + CI wiring) and release phase (final gates).

---

### Pitfall 11: Headless CI false confidence — visual window behavior cannot be verified in CI

**What goes wrong:**
`flutter test` exercises only mocked channels — it verifies Dart-side logic, never OS behavior. GitHub Actions `windows-latest` has an interactive session, so integration tests *run*, but focus, DPI, and positioning are unreliable there, and tests that minimize/close/hide the app window can hang the CI job (the app under test is the process driving the run). Golden/screenshot comparisons are flaky between software and GPU rendering. Everything this package actually sells — no white border, no 8px inset, snap layouts flyout, cursor feedback, maximize bounds, fullscreen flash — is invisible to headless CI. The host project's UAT evidence standard applies verbatim: **console-no-crash ≠ UI correct presentation; window visibility must be human-confirmed** (memory: feedback_uat_evidence_standard).

**Why it happens:**
Green CI feels like proof. The gap between "tests pass" and "window renders correctly on a real desktop" is invisible until a real user opens the app.

**How to avoid:**
- Two-tier testing discipline, formalized in the repo:
  - **CI tier:** `flutter analyze` (0 issues), unit tests, mocked-channel contract tests (arguments, event-stream shape), native compile check, `pana`/dry-run. All headless-safe.
  - **Real-machine tier:** a scripted manual verification checklist executed per release on real hardware (Windows 11 dev box, a Linux GNOME machine/VM incl. Wayland, a macOS machine). Item list in the "Looks Done But Isn't" checklist below.
- Make the checklist a tracked file with dated sign-offs (the host project's UAT flow) so "verified" always has a who/when/evidence.
- Prefer contract assertions over on-screen assertions in automated tests; reserve pixel/window judgments for humans.

**Warning signs:**
Roadmap phases whose only verification is `flutter test`; "done" claims for frame/snap/fullscreen features without a dated manual-checklist entry; goldens added for window visuals.

**Phase to address:** Every phase's verification section; formalized as a release gate in the release phase.

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Vendor-copy upstream files without adding upstream as a git remote | Fast skeleton | Every upstream release becomes a hand-port; fixes silently missed | Never — the remote costs one command |
| Keeping grafted dead code (`bypass_wm_size` / `adjustChildWindowSize` dead branch) "for later" | Zero effort now | False confidence, audit noise, merges carry code that never runs | Only with a DEVIATIONS.md entry marked for deletion in v1 |
| Hardcoded resize edge width (bitsdojo's hardcoded value) | Skips an API decision | Users can't match their design; issues asking for configurability | Never — PROJECT.md already requires configurable |
| Silent `return true` for unimplemented platform calls | Fastest cross-platform compile | Undetectable behavior gaps; trust-destroying bug reports | Never — unsupported must be typed and visible |
| Scattered `GetLastError()`-style handling instead of `Win32Result` | Porting speed | Corrupted last-error state across FFI boundaries; wrong error surfaced | Never under win32 6.x |
| Ad-hoc "fix each compile error" win32 6 port | Feels incremental | Half-ported semantics (error codes as metrics, int-typed handles cached) | Never — one mechanical pass against the validated checklist |
| Deferring the LICENSE stack "until publishing" | Skip a boring step now | Grafted commits accumulate with no attribution; retrofitting is error-prone | Never — costs minutes in the skeleton phase |
| Blocking v1 on fullscreen-flash elimination | Purist completeness | Infinite deferral — two machine-verified cycles already failed | Never — already demoted to exploration target by PROJECT.md |

## Integration Gotchas

Common mistakes when connecting this package to host apps and the ecosystem.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Hosts migrating from dual-package (window_manager + bitsdojo) | Running both the new package and the old ones simultaneously — two plugins register top-level proc delegates on the same HWND; ordering becomes host-registration dependent | Publish a migration guide: remove bitsdojo *and* window_manager, map old API → new API; document delegate coexistence behavior for the transition window |
| media_kit / video-texture apps | Assuming runner-level NCHITTEST works — the Flutter child window covers the top edge, so it never fires there (host-proven) | Frame hit-testing lives in the plugin's top-level delegate; test specifically with a texture-heavy child window |
| Geometry persistence (host uses shared_preferences) | Plugin and host both writing window geometry on different events, racing on restore | Package exposes events (move/resize/close); persistence ownership stays host-side; document the serialization hook |
| win32 6.x unlock of downstream (file_picker 12, flutter_secure_storage 11) | Assuming the unlock is automatic — the host's constraint is solved only if *no other dependency* still pins win32 5.x | Document the win32 6 requirement prominently; it is a selling point, state it in README |
| Per-monitor DPI changes mid-session | Computing frame padding once at startup and caching it | Recompute via `GetSystemMetricsForDpi` / `GetDpiForWindow` per NCCALCSIZE; handle `WM_DPICHANGED` |
| Event streams at mouse-move frequency | Forwarding every `WM_MOVE`/`WM_SIZE` through the channel unthrottled; hosts doing `setState` per event cause rebuild storms (host jank sessions showed resize-path rebuild sensitivity) | Batch/coalesce move/resize events on the Dart side; document the event frequency contract |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Expensive work inside `WM_NCHITTEST` | Laggy cursor tracking, resize stutter | NCHITTEST fires on every mouse move — pure integer hit-test math only, no allocations, no channel calls | Immediately on any nontrivial handler body |
| Per-resize Dart-channel round-trips | Build-side jank during drag-resize | Keep native frame work in the native layer; emit coalesced events only | Visible on texture-heavy windows (host baseline: build P50 6.7–11.5ms during resize vs 0.2ms idle) |
| Unthrottled event streams | Host rebuild storms during window move | Coalesce on the Dart side; document frequency | Any host UI listening to move events |
| BackdropFilter/glass UI in hosts *combined with* plugin-caused extra relayouts | resize jank60 spikes | Plugin must not force extra layout passes (e.g. re-issuing SetWindowPos redundantly) | Host apps with blur chrome (the primary target user) |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| `setPreventClose` without an escape hatch pattern | Users trapped in a hung app that cannot be closed | Document the intended pattern (prevent-close + confirm dialog + explicit destroy); test the force-kill path |
| Accepting arbitrary geometry from persisted host state without clamping | Off-screen or zero-size windows after monitor topology changes | Clamp/validate geometry against `MonitorFromWindow` bounds in the API layer (host project already learned this in its persistence bugfix cycle) |
| Logging raw file paths / window titles in plugin-level logs | Privacy leakage in host logs | Redact in any plugin-side logging; hosts control their own logs |
| Raw pointer/int handle storage across async gaps | Use-after-free / stale HWNDs after window destruction | Keep handles as extension types, never cache raw addresses; validate `isNull` before native calls |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Snap Layouts flyout missing on custom maximize button | Windows 11 users lose the headline window-management feature and blame the app | NCHITTEST `HTMAXBUTTON` support as a first-class package feature (Pitfall 5) |
| No cursor morphing at edges | Resize feels broken even when drag-resize technically works | Correct HT edge returns; configurable edge width so hit zones match visual borders |
| Maximized window covers autohide taskbar | Taskbar unreachable until unmaximize | Default to `rcWork` clamping; make taskbar-covering an explicit opt-in |
| Double-click on drag area doesn't maximize | Feels broken vs native windows | `HTCAPTION` route gives double-click behavior for free; custom drag code loses it — prefer HTCAPTION over manual move tracking where possible |
| Alt+Space system menu lost (WS_POPUP route) | Keyboard users lose window menu | Keep style bits (NCCALCSIZE route preserves it); HTSYSMENU on the app icon region |
| macOS borderless window can't take focus | App appears frozen/unresponsive on macOS | `canBecomeKeyWindow`/`canBecomeMainWindow` overrides — non-optional (Pitfall 9) |

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces. Use as the real-machine verification checklist (Pitfall 11).

- [ ] **Frame takeover:** cold start (first launch), hot restart, and after a DPI change all render frameless with no 8px top inset — verify on Windows 11 specifically (Win11 inset ≠ Win10)
- [ ] **Four-edge resize:** every edge and corner resizes, cursor morphs correctly, edge width honored at 100%/150% DPI
- [ ] **Maximized bounds:** no content cropping, no bleed into second monitor, taskbar not covered (default), content centered
- [ ] **minSize actually enforced:** attempt to resize below minimum — the inherited bitsdojo bug makes "code exists" ≠ "works"
- [ ] **Snap:** Win+Arrow snapping works; Snap Layouts flyout appears on maximize-button hover
- [ ] **Fullscreen:** enter/exit 10× consecutively — count flashes, watch for stuck states or input loss (frameless `setFullScreen` defect)
- [ ] **Events:** move/resize/maximize/close/focus all fire on Windows, Linux (X11 *and* Wayland), macOS
- [ ] **setPreventClose:** close button refuses, confirmation path completes, force-kill still possible
- [ ] **Example app:** runs on all three platforms from a fresh clone with zero manual steps
- [ ] **Publishing:** `dart pub publish --dry-run` clean; pana report reviewed; LICENSE shows both upstream © lines + own; platform tags show windows/linux/macos
- [ ] **Migration:** host project (simple_player_flutter) integrates and passes its own verification suite — the PROJECT.md "Validated" bar

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Upstream merge path destroyed (Pitfall 1) | HIGH | Re-establish upstream remote; regenerate a pristine snapshot of current upstream; three-way diff package-vs-snapshot to re-extract deviations into the ledger; future merges resume from the new baseline |
| MIT attribution missing (Pitfall 2) | LOW | Add stacked LICENSE entries immediately (copyright lines are in upstream repos); add derivation README section; republish patch version |
| win32 6 semantic port bug (Pitfall 3) | MEDIUM | Audit every `Win32Result` call site for `.value` extraction; grep for cached raw-int handles; add unit tests around metric/padding computation |
| Maximize overshoot / minSize ignored (Pitfall 4) | MEDIUM | IsZoomed-aware rect adjustment + cooperative GETMINMAXINFO; regression-test with a multi-monitor/DPI-mixed matrix |
| Snap/cursor losses (Pitfall 5) | MEDIUM | Restore style bits, re-run NCCALCSIZE route; add HTMAXBUTTON; re-verify cursor per edge |
| Message-routing regression (Pitfall 6) | HIGH (intermittent, host-dependent) | Reproduce on a stock runner template first (eliminates host variables); bisect delegate return values; restore single-owner contract |
| Platform silent-true debt (Pitfall 9) | MEDIUM | Sweep all platform channel handlers; convert silent returns to typed unsupported; publish feature matrix |
| Published version with broken API (1.0.0 too early) | MEDIUM | If within 7 days: retract and republish; otherwise ship breaking fix at next minor while in 0.x — never retrace to 1.0.0 hastily |

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls. (Phase names descriptive — roadmap creation maps them to numbers.)

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| 1. Upstream merge path | Package skeleton / graft | `git remote -v` shows upstream; DEVIATIONS.md exists with anchors; dry-run merge from upstream tag produces bounded conflicts |
| 2. MIT attribution | Package skeleton / graft | LICENSE contains both upstream © lines verbatim + own line; README derivation section |
| 3. win32 6 port | win32 6.x base port | Compiles clean under win32 ^6.x; metric/padding unit tests pass; `pub outdated` clean |
| 4. Maximize overshoot + GETMINMAXINFO | Windows frame takeover | Real-machine: maximized on primary + secondary monitor at 100%/150% DPI; minSize enforced test |
| 5. Style bits / snap / cursor | Windows frame takeover | Real-machine checklist: Win+Arrow, flyout, per-edge cursor |
| 6. WndProc ownership | Windows frame takeover (architecture step before porting) | Contract test vs stock runner template + real-machine top-edge resize with texture-heavy child |
| 7. Init ordering | Windows frame takeover + example app | Cold-start-first-launch verification; example shows canonical sequence |
| 8. Fullscreen flash | Structure decision in Windows frame phase; exploration post-v1 | Structure check (no style toggling in code review); flash re-attempt only after symptom collection per archive instruction |
| 9. Platform symmetry | Linux/macOS phases | Feature matrix in README before implementation; silent-true sweep (grep channel handlers); per-platform smoke on real GNOME Wayland + macOS |
| 10. Publishing | Skeleton (CI wiring) + release phase | `pana`/dry-run in CI from phase 1; release gates: tags, docs %, example, 0.1.0 publish |
| 11. Headless CI gap | Every phase + release gate | Every phase's verification section names its real-machine items; dated checklist sign-offs in repo |

## Sources

- PRIOR-CONTEXT files (hands-on, machine-verified, HIGH confidence): `D:/window_frame_kit/.planning/research/PRIOR-CONTEXT-session-handoff.md` (win32 6.4 type checklist §⑤, bitsdojo GETMINMAXINFO/dead-code facts §③, message-routing findings §②), `PRIOR-CONTEXT-window-layer.md` (reverted fullscreen Plans A/B §六, white-border root cause, dual-package boundary §一/§三)
- [Dart publishing guide — pre-publish checks / license responsibility](https://dart.dev/tools/pub/publishing#about-pre-publish-checks) (MEDIUM)
- [dart.dev — pub licenses](https://dart.dev/tools/pub/licenses) (MEDIUM)
- [MIT license text — opensource.org](https://opensource.org/license/MIT) (HIGH, canonical text)
- [Stack Overflow — MIT derivative copyright preservation](https://softwareengineering.stackexchange.com/questions/10623197), [relicensing under MIT](https://stackoverflow.com/questions/54845301) (MEDIUM)
- [win32 6.0.0 changelog — breaking changes, Dart ≥3.10 floor](https://pub.dev/packages/win32/changelog) and win32 6 migration guide (`win32.pub/docs/migration/5xx-to-6xx`, via Context7 `/halildurmus/win32`) (MEDIUM, plus hands-on verification)
- [Microsoft Learn — Custom Window Frame Using DWM](https://learn.microsoft.com/windows/win32/dwm/customframe); [WM_NCHITTEST reference (HTMAXBUTTON)](https://learn.microsoft.com/windows/win32/inputdev/wm-nchittest); [DwmSetWindowAttribute / DWMWA_TRANSITIONS_DISABLED](https://learn.microsoft.com/windows/win32/api/dwmapi/nf-dwmapi-dwmsetwindowattribute) (MEDIUM)
- [melak47/BorderlessWindow](https://github.com/melak47/BorderlessWindow) — minimal NCCALCSIZE reference; [microsoft/terminal NonClientIslandWindow](https://github.com/microsoft/terminal) — canonical HTMAXBUTTON/snap-layouts example (MEDIUM)
- [Flutter engine — RegisterTopLevelWindowProcDelegate](https://api.flutter.dev/flutter/windows/PluginRegistrarWindows/RegisterTopLevelWindowProcDelegate.html) — delegate-before-runner ordering (MEDIUM)
- [Arch Wiki — Window decorations (SSD vs CSD)](https://wiki.archlinux.org/title/Window_decorations) (MEDIUM)
- [Apple — NSWindow.StyleMask (borderless / fullSizeContentView)](https://developer.apple.com/documentation/appkit/nswindow/stylemask) (MEDIUM)
- [pub.dev scoring help](https://pub.dev/help/scoring) — six categories, documentation ≥20%, platform detection (MEDIUM)
- Flutter desktop testing limitations: community/maintainer discussions on window-dependent integration tests in CI; Flutter repo's physical-device lab for Windows desktop tests (LOW–MEDIUM)
- Host project memories (HIGH, machine-verified): feedback_uat_evidence_standard ("console no-crash ≠ UI correct"), project_bitsdojo_dual_package_migration, bugfix_white_border_frameless, project_fullscreen_style_authority

---
*Pitfalls research for: window_frame_kit — Flutter desktop window management plugin*
*Researched: 2026-09-04*
