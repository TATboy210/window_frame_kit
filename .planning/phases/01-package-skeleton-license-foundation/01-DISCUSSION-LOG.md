# Phase 1: 包骨架与许可基座 - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-09-04
**Phase:** 1-包骨架与许可基座
**Areas discussed:** 包身份与仓库, CI 矩阵范围

---

## 灰色地带选择

| Option | Description | Selected |
|--------|-------------|----------|
| 包身份与仓库 | GitHub 仓库位置/pubspec 链接/发布者身份/版权行归属 | ✓ |
| 上游同步策略 | upstream remote 直指 leanflutter vs 先 fork；DEVIATIONS.md 粒度 | |
| CI 矩阵范围 | 三平台 runner vs Windows 先行 | ✓ |
| 全部讨论 | 聊以上所有 | |

**User's choice:** 包身份与仓库 + CI 矩阵范围
**Notes:** 上游同步策略未选——CONTEXT.md 按推荐默认补记（D-09）

---

## 包身份与仓库

| Option | Description | Selected |
|--------|-------------|----------|
| 个人账号 | github.com/TATboy210/window_frame_kit，个人号直接发 pub.dev 完全成立 | ✓ |
| 新建组织 | 先建 GitHub organization，适合系列发包 | |
| 暂不上 GitHub | pubspec repository 留占位，代价是 pub.dev 扣分 | |

**User's choice:** 个人账号（推荐）

| Option | Description | Selected |
|--------|-------------|----------|
| 个人名 | LICENSE 第三行写 TATboy210 + 2026 | ✓ |
| 笔名/工作室 | 用名号统一品牌 | |

**User's choice:** 个人名（推荐）

| Option | Description | Selected |
|--------|-------------|----------|
| 英文描述 | 关键词全覆盖（frameless/frame takeover/window management/三平台），pub.dev 搜索友好 | ✓ |
| 中文先草拟 | 发布前再润色成英文 | |

**User's choice:** 英文描述（推荐）

| Option | Description | Selected |
|--------|-------------|----------|
| 全量 topics | desktop, window, window-manager, frameless, custom-titlebar | ✓ |
| 精简 topics | 三个核心词 | |
| 你来定 | 按 pub.dev 惯例选 | |

**User's choice:** 全量 topics（推荐）

---

## CI 矩阵范围

| Option | Description | Selected |
|--------|-------------|----------|
| 三平台全量 | 骨架期就上 windows/ubuntu/macos 矩阵；三平台同步 v1 是硬需求，公共仓库免费 | ✓ |
| Windows 先行 | 骨架期只 Windows，Phase 4 再补——安静但晚发现 | |
| 全量+降噪 | analyze/test 全平台，pana 只在 PR 主分支跑 | |

**User's choice:** 三平台全量（推荐）

| Option | Description | Selected |
|--------|-------------|----------|
| push+PR | 主分支 push 与所有 PR 都触发，反馈最及时 | ✓ |
| 仅 PR+主分支 | 减少重复跑 | |

**User's choice:** push+PR（推荐）

| Option | Description | Selected |
|--------|-------------|----------|
| analyze+format | flutter analyze + dart format --set-exit-if-changed | ✓ |
| dart 单测 | flutter test（channel/API 契约可无头跑） | ✓ |
| pana 评分 | 发布分数从第一天守护（ROADMAP 成功标准 #5） | ✓ |
| native 编译 | flutter build 三平台，抓模板/CMake 问题 | ✓ |

**User's choice:** 全部四项（multiSelect）
**Notes:** pana 设通过线下限而非满分级（example/README 完整度分项到 Phase 5 才满分）——记入 D-08

---

## Claude's Discretion

- README Derivation 节排版
- DEVIATIONS.md frontmatter/表格样式
- CI workflow 文件拆分（单文件 matrix vs 多文件）
- example app 最小占位内容
- pubspec description 最终英文措辞（含关键词即可）

## Deferred Ideas

None — discussion stayed within phase scope
