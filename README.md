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
| Linux    | Supported (GTK) |
| macOS    | Supported (AppKit) |

Desktop only — there are no mobile or web targets (windows on a phone are
not a thing; pretending otherwise just pollutes the API surface).

## Derivation

window_frame_kit is a fork-and-graft package, built in the open with full
attribution:

- **Base**: a verbatim port of
  [window_manager 0.5.2](https://github.com/leanflutter/window_manager)
  (MIT, © LiJianying) — the full capability API and event system. The
  `upstream` git remote points at this repository so upstream changes can
  be cherry-picked or diffed.
- **Frame graft**: the frame-takeover code (borderless via
  `WM_NCCALCSIZE`, native edge resize via `WM_NCHITTEST`) is grafted from
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
