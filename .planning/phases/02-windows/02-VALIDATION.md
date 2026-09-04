---
phase: "2"
slug: "windows"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-09-05"
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> 三层验证架构（源：02-RESEARCH.md §Validation Architecture）：**CI-headless**（每 push 自动）/ **本地实机脚本**（executor 可脚本化）/ **人工目视**（用户 checkpoint，dated sign-off）。Pitfall 11 铁律：控制台无崩溃 ≠ UI 正确呈现。

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | flutter_test（契约/单元，headless）+ integration_test（本地实机脚本）+ bash diff 脚本（SC5）+ 人工 UAT 清单 |
| **Config file** | analysis_options.yaml + .github/workflows/ci.yml（三处改动）+ tool/verbatim_diff.sh（Wave 0 新建） |
| **Quick run command** | `D:/flutter/bin/flutter analyze && D:/flutter/bin/flutter test && bash tool/verbatim_diff.sh <pub-cache-wm-0.5.2>` |
| **Full suite command** | quick run + `dart format 正名单` + `dart pub publish --dry-run` + `cd example && flutter build windows --release` + integration_test（本地）+ CI 三 leg + pana |
| **Estimated runtime** | quick ~30s；full 本地 ~8 min（example build 108s 实测）；CI ~10 min |

---

## Sampling Rate

- **After every task commit:** quick run command（analyze + test + verbatim_diff）
- **After every plan 收口:** full suite 本地半边（format 正名单 + dry-run + example build；P02 起加 exe 启动存活断言）+ CI 三 leg 绿确认
- **Per checkpoint（P03/04/05 批末）:** 对应批 UAT 清单 dated sign-off（宿主 UAT 证据标准）
- **Phase gate（P05 末）:** SC1-4 全量 UAT 复验 + SC5 终跑（diff 脚本全量 + 契约全绿 + DEVIATIONS 行数对账）+ CI 绿 + 零 `// FRAME:` grep 断言（退出门：`! grep -rn "FRAME:" lib windows example test`）
- **Max feedback latency:** 30s（quick）

---

## Per-Task Verification Map

> 种子行 = 研究提出的 5 计划批次（P01 Dart 全量面 → P02 C++ + tracer 实机 → P03/04/05 = D-05 三批）。planner 落 PLAN.md 后按真实 Task ID 细化；逐 CAPB 的三层映射见 02-RESEARCH.md §Validation Architecture 表（本文件的权威源）。

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-* | P01 | 1 | CAPB-01~12 Dart 面 | — | N/A | contract+diff | quick run（契约 4/4 + 16 文件零差） | ❌ W0 | ⬜ pending |
| 02-02-* | P02 | 2 | CAPB-01（tracer） | — | N/A | build+launch | example build + exe 存活 + tracer 实机 checkpoint | ❌ W0 | ⬜ pending |
| 02-03-* | P03 | 3 | CAPB-01/02/03 | — | N/A | integration+UAT | integration_test 几何断言 + 批① UAT | ❌ W0 | ⬜ pending |
| 02-04-* | P04 | 3 | CAPB-04/05/06 | — | N/A | integration+UAT | 查询断言 + 11 hook console 打勾 + 批② UAT | ❌ W0 | ⬜ pending |
| 02-05-* | P05 | 3 | CAPB-07~12 | — | N/A | UAT+phase gate | 批③ UAT + SC5 终跑 + 零 FRAME grep | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tool/verbatim_diff.sh` — SC5 载体（RESEARCH Example 2 已给全文）
- [ ] `test/channel_contract_test.dart` — D-02 叠加层（RESEARCH Example 3 骨架；上游 test 零 test() 块 → flutter test 裸跑 exit 79，本文件与模板测试删除必须同 commit）
- [ ] `example/lib/utils/window_event_logger.dart` — D-04 载体（11 hook debugPrint 日志，example 侧不入库 lib）
- [ ] ci.yml 三处改动（format 双轨 / ubuntu leg 加 libayatana-appindicator3-dev / verbatim-diff 步）
- [ ] DEVIATIONS.md 5 行新条目（RESEARCH Pattern 3 清单）
- [ ] `windows/test/window_frame_kit_plugin_test.cpp` 占位替换（RESEARCH Example 4）

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| 窗口按 WindowOptions 出现（尺寸/居中/hidden 标题栏/透明背景） | CAPB-01 / SC1 | 视觉呈现不可 headless（Pitfall 11） | P02 tracer checkpoint：启动 example → 目视窗口 800×600 居中、无系统标题栏、控制台首批事件行 |
| 几何即时生效 + 150% DPI 逻辑像素换算 | CAPB-02 / SC1 | 物理屏幕 DPI 依赖 | setSize(400,400) 目视 + getSize 回读 400（DPI 双档） |
| min/max/aspectRatio 拖拽约束 | CAPB-03 | 需真实拖拽手势 | 拖至 min 以下被挡；16:9 比例锁定；reset(0) 解锁 |
| 状态操作↔查询一一对应；titleBarStyle 切换 + getTitleBarHeight | CAPB-04/05/08 / SC2 | 窗口状态视觉判定 | UAT 表驱动逐项操作+回读；normal↔hidden 目视；height 控制台值>0（物理 px 预期） |
| 11 WindowListener hooks 全触发 | CAPB-06 / SC3 | native 发射不可脚本安全触发（minimize/close 挂起风险） | RESEARCH Example 5 触发-事件表逐项操作，console logger 逐行打勾（13 可触发；show/hide 走 raw；docked/undocked 预期缺席=parity） |
| preventClose 拦截→destroy；popUpWindowMenu frameless 唤出；startDragging/8 向 resize 实拖 | CAPB-07/09/12 / SC4 | 关闭/菜单/拖拽行为不可 headless | SC4 对话框流全程；frameless 下系统菜单弹出可点；DragToMoveArea + DragToResizeArea 8 向 |
| 外观 9 项（icon/阴影/透明度/背景色/暗色标题栏/top/bottom）+ 鼠标穿透 + tray 恢复路径 | CAPB-10/11 | 视觉/输入穿透判定 | 逐项目视；穿透 on→点击落底层窗口；tray 菜单恢复（home.dart:1043-1047）；forward 无效=预期 |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s（quick run）
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
