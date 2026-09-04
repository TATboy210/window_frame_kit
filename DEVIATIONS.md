# DEVIATIONS

Ledger of intentional deviations from upstream sources. Every entry
corresponds to a `// FRAME:` or `// DEV:` marker in the code. When merging
upstream (`git merge upstream/main`), check each entry so a deviation is
never silently overwritten by an upstream change.

Upstream anchors refer to the pinned pub-cache versions:
`window_manager-0.5.2` and `bitsdojo_window_windows-0.1.6`.

## Ledger

| # | Upstream anchor (file:line) | This package | Deviation | Rationale | Marker |
|---|-----------------------------|--------------|-----------|-----------|--------|
| 1 | bitsdojo_window_windows-0.1.6/windows/bitsdojo_window.cpp:550-568 | TBD (Phase 3) | GETMINMAXINFO unconditional `return 0` → cooperative merge | Upstream bug silently defeats `setMinimumSize` (host-project verified); single package = single min/max owner | `// FRAME:` (to be attached when grafted in Phase 3) |

*(Row above is a placeholder to be replaced with the real entry when the
Phase 3 graft lands. New rows append below — one row per functional block,
never per line.)*

## Recording rules

1. Every `// FRAME:` (grafted code) or `// DEV:` (deviation from ported
   upstream code) marker in the source MUST have exactly one ledger row.
2. Upstream anchors use the pinned pub-cache version's `file:line`.
3. Entries are recorded per functional block, not per line.
