# Phase 1: 包骨架与许可基座 - Context

**Gathered:** 2026-09-04
**Status:** Ready for planning

<domain>
## Phase Boundary

三平台（windows/linux/macos）Flutter 插件骨架完整落盘 + MIT 双上游归属 LICENSE + upstream remote 与 DEVIATIONS.md 嫁接纪律账本 + pubspec（channel 名 `window_frame_kit`、依赖仅 path + screen_retriever）+ pana/dry-run 骨架期跑通并接入 CI。嫁接代码落盘之前，合规、可合并性与发布管线锁死。**不含**：window_manager 源码移植（Phase 2）、任何 frame 嫁接代码（Phase 3）、example 强化（Phase 5）。

</domain>

<decisions>
## Implementation Decisions

### 包身份与仓库
- **D-01:** GitHub 仓库放个人账号 `github.com/TATboy210/window_frame_kit`——pubspec 的 repository/homepage/issues 字段全部指向它；个人号直接发 pub.dev 完全成立 — **Reversibility:** one-way — pub.dev 包页与已发布版本会永久链接首次上传的 repository URL，改仓库名=断链历史
- **D-02:** LICENSE 三段式叠加：window_manager 版权行（逐字）+ bitsdojo_window 版权行（逐字）+ 本包版权行 `Copyright (c) 2026 TATboy210`；MIT 正文一份共享，三行版权声明并排
- **D-03:** pubspec description 用英文（pub.dev 搜索主语种），覆盖关键词：frameless / frame takeover / window management / events / Windows/Linux/macOS——具体措辞规划期定稿
- **D-04:** pubspec topics 全量 5 个：`desktop, window, window-manager, frameless, custom-titlebar`（pub.dev 上限 5 个，从中选组合）

### CI 矩阵
- **D-05:** GitHub Actions 从骨架期即上**三平台全量矩阵**（windows / ubuntu / macos runner 并行）——三平台同步 v1 是硬需求，平台问题越早暴露越好；公共仓库三平台 runner 全免费，无额度顾虑
- **D-06:** 触发时机 = push 到主分支 + 所有 PR（反馈最及时）
- **D-07:** 每平台跑 4 项检查：① `flutter analyze` + `dart format --set-exit-if-changed` ② `flutter test`（Dart 单测；channel/API 契约测试可无头跑，视觉窗口行为留给实机清单）③ pana 静态评分（发布分数从第一天守护）④ `flutter build windows/linux/macos` native 编译验证（抓模板/CMake 问题）
- **D-08:** 注意：骨架期 Linux/macOS 的 build 在纯 GitHub runner 上可行（模板自足），但 pana 分数里 example/README 完整度分项在 Phase 5 才满分——CI 里 pana 设下限（通过线）而非满分级

### 上游同步策略（讨论中被跳过，按推荐默认执行）
- **D-09:** upstream remote 直接指 `https://github.com/leanflutter/window_manager`（用户 GitHub fork 不是必须——remote 指上游即可 cherry-pick/对 diff）；DEVIATIONS.md 条目格式 = 上游 file:line 锚点 + 偏离理由，按功能块记录（每处 `// FRAME:` 标记对应一条）— **Reversibility:** costly — remote/账本结构后期改动要重写全部偏差记录

### Claude's Discretion
- README Derivation 节的具体排版
- DEVIATIONS.md 的 frontmatter/表格样式（只要含 file:line 锚点 + 理由即可）
- CI workflow 文件拆分（单文件 matrix vs 多文件）
- example app 的最小占位内容（Phase 5 才强化）
- pubspec description 的最终英文措辞（含 D-03 关键词即可）

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 项目规划
- `.planning/PROJECT.md` — 包定位、Core Value、约束（C++-only 零 win32 依赖、三平台 v1、全屏闪屏=探索目标）
- `.planning/REQUIREMENTS.md` — 26 条 v1 需求（本 phase 覆盖 PUB-01）
- `.planning/ROADMAP.md` §Phase 1 — 目标与 5 条成功标准
- `.planning/config.json` — 工作流配置

### 研究输入
- `.planning/research/STACK.md` — 插件结构/脚手架命令/pub 发布清单/CI 矩阵建议（骨架期直接消费）
- `.planning/research/PITFALLS.md` — Pitfall 1（嫁接纪律两区布局+upstream remote+DEVIATIONS.md）与 Pitfall 2（MIT 归属，pub.dev 不验证须自查）是本 phase 的全部动机；10（pana 从第一天守护）
- `.planning/research/SUMMARY.md` — 综合结论（two-zone 布局、非联邦单插件、channel 命名）

### 上游硬事实（宿主实战）
- `.planning/research/PRIOR-CONTEXT-session-handoff.md` §⑤ — win32 6.4 类型清单、bitsdojo 源码要点（Phase 2/3 消费，Phase 1 只需了解存在）
- `.planning/research/PRIOR-CONTEXT-window-layer.md` §三 — 依赖链硬事实表

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- 无既有代码——全新仓库（`D:\window_frame_kit`，git 已 init，master 分支，6 个 planning commits）
- 上游源码在 pub cache：`window_manager-0.5.2` 与 `bitsdojo_window_windows-0.1.6`（Phase 2/3 的移植来源；Phase 1 只建骨架不搬码）

### Established Patterns
- 宿主 simple_player_flutter 的 UAT 证据标准（控制台无崩溃 ≠ UI 正确呈现）——本 phase 成功标准 #1 的实机项适用
- 嫁接纪律模式：verbatim port 区 + `// FRAME:` 标记 + DEVIATIONS.md 账本（研究 PITFALLS Pitfall 1）

### Integration Points
- `.claude/CLAUDE.md`（已生成，`da409a8`）——项目指南，骨架代码须遵守
- GitHub 仓库 `TATboy210/window_frame_kit`（D-01，骨架期创建并 push）

</code_context>

<specifics>
## Specific Ideas

- LICENSE 必须在**首个含嫁接代码的 commit 之前**落盘（Pitfall 2 强制；ROADMAP 把 PUB-01 映射进本 phase 正是为此）
- channel 名 `window_frame_kit`——绝不复用 `window_manager`（避免与原包共存时的 channel 冲突）
- pubspec 依赖白名单：`path` + `screen_retriever`，任何 win32 出现都视为违规（C++-only 架构的骨架期生效条款）

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 1-包骨架与许可基座*
*Context gathered: 2026-09-04*
