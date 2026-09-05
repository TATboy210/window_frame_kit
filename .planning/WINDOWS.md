---
schema_version: 1
open_count: 4
waived_count: 0
fixed_count: 3
total_count: 7
last_updated: 2026-09-05T09:30:00.000Z
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
| 4 | 1 | todo | example/windows |  | 基线观察：快速拖拽左缘 resize 时右缘毫秒级瞬间空白（裸 Flutter 模板窗口路径，非 CMake configure 补丁；根因未诊断 render-pipeline/shader/windowing）复现：按住左边边缘→迅速左右往复拖拽→观察空白区右缘——Phase 3 frame 接管落地后复测（01-03-SUMMARY Baseline Observation） | open |  | 2026-09-04T12:05:00.643Z |  |
| 5 | 1 | todo | pubspec.yaml |  | pana 基线 140/160（Phase 2 02-01 重钉 threshold 20，CI run 33906880498）：①description 超 180 字符建议长度扣 0/10 ②逐字区 lib/src 不匹配 tall-style formatter 扣 40/50（SC5 字节级 verbatim 的结构性代价，pana format 检查无豁免机制）——description 已于 05-03 精简（~150 字符）；本地 pana 0.23.19 Windows sandbox bug（outputFolder 含冒号）无法实证新基线，ci.yml threshold 保持 20（安全下限）不冒进收紧；若后续 CI 实证 150 可收紧至 10 | open |  | 2026-09-04T13:18:21.677Z |  |
| 6 | 2 | stub | example/test/widget_test.dart |  | 模板版 widget_test（引用 MyApp+'Running on:'）在 main.dart 被上游版覆盖后 fail-if-run；CI 永不跑它（root flutter test 不含嵌套包）；上游 window_manager 0.5.2 同款文件同样坏（对等行为）——Phase 5 example 强化时清理（02-RESEARCH Open Q4 定案） | open |  | 2026-09-04T18:26:00.000Z |  |
| 7 | 2 | todo | example/lib/pages/home.dart |  | example 移除系统托盘（用户指令 2026-09-05）→ 02-05 执行（f192dd5）；CAPB-11 穿透恢复路径改为 5 秒自动复位开关 + ci.yml ubuntu leg 移除 appindicator + registrant/lock 再生成，同 commit 完成；用户 2026-09-05 三项实测通过（无托盘/穿透/5s 恢复） | fixed |  | 2026-09-05T04:39:05.252Z | 2026-09-05T09:30:00.000Z |

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
  },
  {
    "id": 4,
    "kind": "todo",
    "phase": "1",
    "file": "example/windows",
    "line": null,
    "description": "基线观察：快速拖拽左缘 resize 时右缘毫秒级瞬间空白（裸 Flutter 模板窗口路径，非 CMake configure 补丁；根因未诊断 render-pipeline/shader/windowing）复现：按住左边边缘→迅速左右往复拖拽→观察空白区右缘——Phase 3 frame 接管落地后复测（01-03-SUMMARY Baseline Observation）",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-04T12:05:00.643Z",
    "resolved_at": null
  },
  {
    "id": 5,
    "kind": "todo",
    "phase": "1",
    "file": "pubspec.yaml",
    "line": null,
    "description": "pana 基线 140/160（Phase 2 02-01 重钉 threshold 20，CI run 33906880498）：①description 超 180 字符建议长度扣 0/10 ②逐字区 lib/src 不匹配 tall-style formatter 扣 40/50（SC5 字节级 verbatim 的结构性代价，pana format 检查无豁免机制）——Phase 5 PUB-04 精简 description 后把 ci.yml 的 pana exit-code-threshold 重钉为 160 − 新基线",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-04T13:18:21.677Z",
    "resolved_at": null
  },
  {
    "id": 6,
    "kind": "stub",
    "phase": "2",
    "file": "example/test/widget_test.dart",
    "line": null,
    "description": "模板版 widget_test（引用 MyApp+'Running on:'）在 main.dart 被上游版覆盖后 fail-if-run；CI 永不跑它（root flutter test 不含嵌套包）；上游 window_manager 0.5.2 同款文件同样坏（对等行为）——Phase 5 example 强化时清理（02-RESEARCH Open Q4 定案）",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-04T18:26:00.000Z",
    "resolved_at": null
  },
  {
    "id": 7,
    "kind": "todo",
    "phase": "2",
    "file": "example/lib/pages/home.dart",
    "line": null,
    "description": "example 移除系统托盘（用户指令 2026-09-05）→ 02-05 执行（f192dd5）；CAPB-11 穿透恢复路径改为 5 秒自动复位开关 + ci.yml ubuntu leg 移除 appindicator + registrant/lock 再生成，同 commit 完成；用户 2026-09-05 三项实测通过（无托盘/穿透/5s 恢复）",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-09-05T04:39:05.252Z",
    "resolved_at": "2026-09-05T09:30:00.000Z"
  }
]
````
