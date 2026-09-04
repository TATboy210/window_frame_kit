---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 2
total_count: 3
last_updated: 2026-09-04T11:29:33.453Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 1 | stub | README.md |  | 模板版 README（含 Android/iOS 错误措辞）——Plan 02 整体重写 | fixed |  | 2026-09-04T10:03:37.042Z | 2026-09-04T11:29:31.446Z |
| 2 | 1 | stub | LICENSE |  | 模板 BSD-3 占位 LICENSE——Plan 02 替换为三行叠加 MIT（PUB-01） | fixed |  | 2026-09-04T10:03:37.738Z | 2026-09-04T11:29:32.605Z |
| 3 | 1 | stub | DEVIATIONS.md | 15 | 账本占位行（bitsdojo GETMINMAXINFO 协作式合并）——Phase 3 嫁接落码时替换为真实条目 | open |  | 2026-09-04T11:29:33.453Z |  |

````json
[
  {
    "id": 1,
    "kind": "stub",
    "phase": "1",
    "file": "README.md",
    "line": null,
    "description": "模板版 README（含 Android/iOS 错误措辞）——Plan 02 整体重写",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-09-04T10:03:37.042Z",
    "resolved_at": "2026-09-04T11:29:31.446Z"
  },
  {
    "id": 2,
    "kind": "stub",
    "phase": "1",
    "file": "LICENSE",
    "line": null,
    "description": "模板 BSD-3 占位 LICENSE——Plan 02 替换为三行叠加 MIT（PUB-01）",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-09-04T10:03:37.738Z",
    "resolved_at": "2026-09-04T11:29:32.605Z"
  },
  {
    "id": 3,
    "kind": "stub",
    "phase": "1",
    "file": "DEVIATIONS.md",
    "line": 15,
    "description": "账本占位行（bitsdojo GETMINMAXINFO 协作式合并）——Phase 3 嫁接落码时替换为真实条目",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-04T11:29:33.453Z",
    "resolved_at": null
  }
]
````
