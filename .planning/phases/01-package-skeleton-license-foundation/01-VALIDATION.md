---
phase: "1"
slug: "package-skeleton-license-foundation"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-09-04"
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | flutter_test (SDK, plugin template ships with example widget test) |
| **Config file** | none — template self-contained |
| **Quick run command** | `flutter test`（example 目录） |
| **Full suite command** | `flutter analyze && flutter test`（example 目录）+ CI 三平台矩阵 |
| **Estimated runtime** | ~30 秒 |

---

## Sampling Rate

- **After every task commit:** Run `flutter analyze`（包根）——秒级静态门
- **After every plan wave:** Run CI 全量（analyze+format / test / pana / native build 三平台）
- **Before `/gsd-verify-work`:** CI 全绿 + 实机清单全过
- **Max feedback latency:** 本地 <10s（analyze），CI <15min

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-XX | 01 | 1 | PUB-01 | — | N/A | CLI | `grep -c "LiJianying" LICENSE` | 按任务定 | ⬜ pending |
| 01-01-XX | 01 | 1 | PUB-01 | — | N/A | CLI | `dart pub publish --dry-run`（0 warnings） | 按任务定 | ⬜ pending |
| 01-01-XX | 01 | 1 | PUB-01 | — | N/A | CLI | `grep -E "^\s+win32" pubspec.yaml || exit 1`（白名单 gate） | 按任务定 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*注：Per-task 表在 PLAN.md 定稿后由 validate-phase/audit 补全具体 Task ID。5 个验证锚点（研究 RESEARCH.md Validation Architecture 节）：① example Windows 实机启动显示窗口 ② LICENSE 上游两行逐字 grep ③ 依赖白名单 grep（零 win32）④ dry-run 0 warnings ⑤ CI 三平台全绿。*

---

## Wave 0 Requirements

- 模板自带 `example/test/widget_test.dart`（plugin template 默认）——覆盖"骨架可构建可测试"的最低反馈要求

*Existing infrastructure covers all phase requirements — 本 phase 无新测试基建，验证靠 CLI 断言 + CI + 实机清单。*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| example 实机启动显示窗口 | PUB-01/SC1 | 无头环境无法验证视觉窗口行为（宿主 UAT 证据标准：控制台无崩溃 ≠ UI 正确呈现） | `cd example && flutter run -d windows`，确认窗口出现、默认插件示例 UI 可交互、关闭正常退出 |
| LICENSE/README 视觉审阅 | PUB-01 | 格式排版是发布门面 | 人工过一遍 LICENSE 三段叠加与 README Derivation 节的呈现 |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
