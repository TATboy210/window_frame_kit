---
phase: 01-package-skeleton-license-foundation
plan: 02
subsystem: infra
tags: [license, mit, attribution, readme, derivation, deviations-ledger, upstream-remote, pub-dev, compliance]

# Dependency graph
requires:
  - phase: 01-package-skeleton-license-foundation (plan 01)
    provides: 三平台骨架与 git 跟踪仓库（2d0d408/064fdb0）+ 模板版 LICENSE/README 占位（本计划整体替换对象）
provides:
  - LICENSE 三行叠加 MIT：window_manager © 行 + bitsdojo © 行逐字（grep -cx 字节级命中）+ 本包行 Copyright (c) 2026 TATboy210 第三，MIT 正文一份共享（diff 证明与 window_manager-0.5.2 正文逐行一致）
  - README.md 重写四要素：包定位（frame 接管 + 完整事件流单包，C++-only 零 win32 依赖）/ 三平台声明（desktop-only）/ ## Derivation 溯源节（两上游血统 + DEVIATIONS.md 指路 + upstream 合并路径）/ License 段
  - DEVIATIONS.md 偏离账本：六列表格（# / 上游锚点 file:line / 本包位置 / 偏离内容 / 理由 / 标记）+ 三条记账规则 + Phase-3 待回填示例行（bitsdojo GETMINMAXINFO 协作式合并）
  - git remote upstream → https://github.com/leanflutter/window_manager（D-09，非文件配置）
affects: [01-03-build-gates, 01-04-ci-first-run, phase-2-wm-port, phase-3-frame-graft, phase-5-publish]

# Actuals (#2632) — pairs with the plan's `estimate` (35000 tokens) to calibrate future estimates.
actuals:
  tokens: 1175    # chars/4 over realized diff (4699 bytes across LICENSE/README.md/DEVIATIONS.md)
  tasks: 3
  commits: 3

# Tech tracking
tech-stack:
  added: []    # 纯文档/合规计划，零新依赖
  patterns:
    - "MIT 三行版权叠加：每持有人一行、两上游逐字保留、本包行第三（D-02 锁定行序）、正文一份共享（研究核实两上游正文逐字相同）"
    - "嫁接纪律三件套先于嫁接落盘：// FRAME: 标记 ↔ DEVIATIONS.md 行目 ↔ upstream remote diff/cherry-pick（Pitfall 1 / D-09）"
    - "账本锚点钉死 pub cache 版本（window_manager-0.5.2 / bitsdojo_window_windows-0.1.6）的 file:line，按功能块记录不逐行拆分"

key-files:
  created:
    - "DEVIATIONS.md（偏离账本：表头 + 三条记账规则 + 待回填示例行）"
  modified:
    - "LICENSE（模板 BSD-3 占位 → 三行叠加 MIT，整体替换）"
    - "README.md（模板版含 Android/iOS 错误措辞 → 四要素整体重写，Pitfall 7 占位清零）"

key-decisions:
  - "DEVIATIONS.md 按前会话已落盘版本原样提交（英文表格）：D-09/Claude's Discretion 只约束 file:line 锚点列 + 理由列 + 规则，逐条核对验收标准全过，不重写"
  - "LICENSE 逐字性用 diff 对 pub cache 上游正文实证（强于计划的 grep 门）：正文全行一致，唯一差异为上游无 EOF 尾换行（本包按 POSIX 保留，无法律/语义影响）"
  - "账本示例行编号用 # 序号（1）而非研究 Example 4 的 D-1——同属 D-09 表格样式裁量区，语义等价"

patterns-established:
  - "合规自查机器门：grep -cx 整行精确断言三条版权行——pub.dev 不验证归属（Pitfall 6），grep 即自查；后续任何 LICENSE 改动都过此门"
  - "PUB-01 时间条款守护：合规基座（LICENSE/README/账本/remote）全部先于首个含嫁接代码的 commit 落盘——本计划三个 commit 均零嫁接代码"

requirements-completed: [PUB-01]  # 复制自 PLAN frontmatter；shared-ID gate：01-01/03/04 亦声明 PUB-01，REQUIREMENTS.md 勾选待最后一个声明计划（01-04）完成后由 ready-ids 放行

# Coverage metadata (#1602) — 四条 must_haves.truths（SC2/SC3 的 D 级真相）逐条对应
coverage:
  - id: D1
    description: "LICENSE 逐字保留两条上游版权行并附本包版权行——三行各恰好一次、MIT 正文完整、模板占位清零（SC2 truth 1 / T-01-05）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "grep -cx 'Copyright (c) 2022-present LiJianying <lijy91@foxmail.com>'=1; grep -cx 'Copyright (c) 2020-2021 Bogdan Hobeanu'=1; grep -cx 'Copyright (c) 2026 TATboy210'=1; grep -c 'Permission is hereby granted, free of charge'=1; ! grep -Eq 'BSD' → PASS"
        status: pass
      - kind: other
        ref: "diff <(tail -n +5 pub-cache/window_manager-0.5.2/LICENSE) <(tail -n +7 LICENSE) → 仅 EOF 换行差异（正文逐行一致）; 负债条款跨行折叠经 tr 归一后 grep 命中=1"
        status: pass
    human_judgment: false
  - id: D2
    description: "README 含 ## Derivation 溯源节，指明 window_manager 0.5.2 与 bitsdojo_window 0.1.6 血统（各含 MIT 归属）+ DEVIATIONS.md 指路 + 三平台声明（SC2 truth 2 / T-01-06）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "grep -c: '## Derivation'=1（恰好一次）, 'leanflutter/window_manager'=1, 'bitsdojo'=3, 'DEVIATIONS.md'=1, 'Windows'=2, 'Linux'=2, 'macOS'=2"
        status: pass
    human_judgment: false
  - id: D3
    description: "git remote -v 显示 upstream → leanflutter/window_manager（SC3 truth 3 / D-09）"
    verification:
      - kind: other
        ref: "git remote get-url upstream → 'https://github.com/leanflutter/window_manager'（精确字符串相等断言）; git remote -v | grep -c upstream → 2（fetch+push）"
        status: pass
    human_judgment: false
  - id: D4
    description: "DEVIATIONS.md 存在且定义好偏差条目格式：上游 file:line 锚点 + 偏离理由 + // FRAME: 标记对应（SC3 truth 4 / T-01-07）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "grep -c 'file:line'=2, grep -c 'FRAME'=3; 六列表头 + 3 条 Recording rules + '(Phase 3 backfill)' 待回填占位行经目视核对在位"
        status: pass
    human_judgment: false

# Metrics
duration: 40 min
completed: 2026-09-04
status: complete
---

# Phase 1 Plan 02: LICENSE 三行叠加 + README Derivation 溯源 + DEVIATIONS.md 账本/upstream remote Summary

**PUB-01 合规基座全量落盘：MIT 三行版权叠加（window_manager + bitsdojo 两上游 © 行逐字、正文经 diff 证明与上游逐行一致、本包行第三），README 四要素整体重写含 ## Derivation 溯源节，DEVIATIONS.md 六列偏离账本 + 三条记账规则就位，upstream remote 直指 leanflutter/window_manager（D-09）——SC2/SC3 全量交付，且全部先于任何嫁接代码（Pitfall 2 时间条款满足）。**

## Performance

- **Duration:** ≈40 min wall clock（分裂执行：前会话 Tasks 1–2 ≈10:35–10:45Z 后被打断；续会话 Task 3 + SUMMARY ≈11:07–11:22Z；含中断间隔，活跃执行时间约 25 min）
- **Started:** 2026-09-04T10:41Z（以前会话 Task 1 commit 时间为锚——被打断会话的确切开始时间不可考）
- **Completed:** 2026-09-04T11:22Z
- **Tasks:** 3
- **Files modified:** 3（LICENSE、README.md 整体重写；DEVIATIONS.md 新建）+ upstream remote 配置（非文件，不入 commit）

## Accomplishments

- **Task 1（前会话）：** LICENSE 整体替换为三行叠加 MIT（D-02）——两条上游版权行从 pub cache 逐字复制（grep -cx 三连字节级命中）、本包行 `Copyright (c) 2026 TATboy210` 第三、MIT 正文一份共享；模板 BSD-3 占位清零。续会话追加更强实证：与 window_manager-0.5.2/LICENSE 正文 diff 逐行一致（仅 EOF 尾换行差异）。
- **Task 2（前会话）：** README.md 整体重写（非模板打补丁，Pitfall 7 占位清除）——包定位（frame 接管 + 完整窗口能力单包、C++-only 零 win32 依赖树）、三平台表（desktop-only，明确无移动/web 目标）、`## Derivation` 溯源节四要素（底座 window_manager 0.5.2 verbatim port / frame 嫁接 bitsdojo_window 0.1.6 含 `// FRAME:` 标记约定 / DEVIATIONS.md 账本指路 / upstream remote 合并路径）、License 段说明三行叠加。
- **Task 3（续会话收口）：** DEVIATIONS.md 按前会话已落盘版本核验后原样提交——六列账本表格（# / Upstream anchor file:line / This package / Deviation / Rationale / Marker）、三条记账规则（FRAME 标记一一对应 / pub cache 版本锚点 / 按功能块记录）、一条 Phase-3 待回填示例行（bitsdojo_window.cpp:550-568 GETMINMAXINFO 无条件 return 0 → 协作式合并）；`git remote add upstream https://github.com/leanflutter/window_manager` 配置并精确验证（D-09：指上游即可 cherry-pick/对 diff，无需个人 fork）。

## Task Commits

Each task was committed atomically:

1. **Task 1: LICENSE 三行叠加 MIT（D-02 / PUB-01 核心）** - `ecd8113` (docs，前会话）
2. **Task 2: README 重写 + Derivation 溯源节（SC2）** - `eb99e1c` (docs，前会话）
3. **Task 3: DEVIATIONS.md 偏离账本 + upstream remote（D-09 / SC3）** - `a0bead7` (docs，续会话；remote 配置非文件，commit 仅覆盖 DEVIATIONS.md）

**Plan metadata:** 本 SUMMARY + STATE/ROADMAP/REQUIREMENTS/WINDOWS 更新随 `docs(01-02): complete license/readme/deviations plan (SUMMARY + state)` 落盘。

## Files Created/Modified

- `LICENSE` - 模板占位 → 三行叠加 MIT（两上游 © 行逐字 + 本包行 + 共享正文；PUB-01 核心交付）
- `README.md` - 模板版 → 四要素重写（定位 / Platforms 表 / ## Derivation / License）
- `DEVIATIONS.md` - 新建偏离账本（表头 + 三条记账规则 + Phase-3 待回填示例行）
- `.git/config`（非工作树文件）- 新增 `remote "upstream"` → leanflutter/window_manager

## Decisions Made

- **DEVIATIONS.md 原样提交不重写**：续会话按 Task 3 验收标准逐条核验前会话落盘版本（六列含 file:line 锚点/理由/标记列、三规则、占位行、双 grep 门非零）全过后直接 commit——遵守 safe-resume 收口指令与 D-09 裁量区边界。
- **逐字性验证升级**：在计划的 grep -cx 门之外追加与 pub cache 上游 LICENSE 的正文 diff（结果：逐行一致，唯一差异为上游文件缺 EOF 尾换行）——为 T-01-05 留下比 grep 更强的证据链。
- **英文账本**：DEVIATIONS.md 用英文书写（与 README/pub.dev 门面同语境）；D-09 锁定的是锚点+理由格式而非语言，验收标准全部满足。

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] 前会话中断导致计划分裂执行（Tasks 1–2 与 Task 3/SUMMARY 分属两个会话）**
- **Found during:** 续会话 resume（orchestrator safe-resume 上下文）
- **Issue:** 前会话在 commit Tasks 1–2（ecd8113/eb99e1c）并把 DEVIATIONS.md 写盘后被打断——文件未提交、upstream remote 未配置、SUMMARY 未写，构成 execute-plan.md 定义的非法半态（生产 commit 已存在而 SUMMARY 缺失）。
- **Fix:** 续会话按收口规程执行：复验 Tasks 1/2 全部 grep 门（PASS，零改动）→ 核验 DEVIATIONS.md 对 Task 3 验收标准（全过，原样提交不重写）→ 配置并精确验证 upstream remote → 原子 commit Task 3 → 复跑计划级 <verification> 四门 → 写本 SUMMARY。未 amend、未回退、未重做任何前会话 commit。
- **Files modified:** DEVIATIONS.md（提交既有落盘版本）
- **Verification:** 计划级四门全 PASS（见 Self-Check）
- **Committed in:** a0bead7

---

**Total deviations:** 1 auto-fixed（1 blocking——纯执行流程层面，内容零偏差）
**Impact on plan:** 无内容影响；三个 task 的交付物与验收标准逐条对上，分裂执行如实记录于本节与 Issues。

## Issues Encountered

- **STATE.md 位置字段被 dispatch 期改写造成 off-by-one**：orchestrator 派发本续会话时把工作树的 Current Position 从入库版 "Plan: 2 of 4 / Ready to execute" 改写为 "Plan: 1 of 4 / Executing Phase 01"，导致 `state.advance-plan`（语义 = Plan 字段 +1）落在 "2 of 4" 而非期望的 "3 of 4"。收口时手工校正为 "Plan: 3 of 4"（与 Plan 01 归档时确立的"下一待执行计划"语义一致）。
- **LICENSE EOF 尾换行与上游不一致**（上游无、本包有）：属 POSIX 文本文件惯例，正文逐行一致，无法律/语义影响——如实记录，不视为偏离。

## Known Stubs

以下为计划内有意保留的占位（非缺陷），账本同步至 .planning/WINDOWS.md：

| Stub | File | Reason | Resolved by |
|------|------|--------|-------------|
| 账本示例行为占位（This package = "TBD (Phase 3)"，标记待嫁接时附挂） | DEVIATIONS.md:15 | 计划明确要求"一行待回填示例"——真实条目只能在 Phase 3 嫁接落码后存在 | Phase 3 frame 嫁接 |
| README 无完整 API 文档与双包迁移映射表 | README.md | 计划边界明示：属 Phase 5（PUB-02）范围，本阶段不虚构尚不存在能力的 usage/API | Phase 5 |

WINDOWS.md 联动：既有 #1（模板 README）/#2（占位 LICENSE）两条 open 项由本计划解决 → 标记 fixed；新增 DEVIATIONS.md 占位行条目（Phase 3 解决）。

## User Setup Required

None - no external service configuration required.（upstream remote 为只读公开 URL，无需凭据；D-01 GitHub 仓库创建属 Plan 03/04 范围。）

## Threat Model Review

| Threat | Disposition | 落地证据 |
|--------|-------------|----------|
| T-01-05 Tampering/Repudiation（上游 © 行） | mitigated | grep -cx 三连整行精确断言（各 =1）+ 上游正文 diff 逐行一致 |
| T-01-06 Spoofing（溯源缺失） | mitigated | `## Derivation` 固定标题恰一次 + 两上游仓库 URL/归属 grep 命中，fork 血统包页可见 |
| T-01-07 Repudiation（嫁接偏离无账本） | mitigated | DEVIATIONS.md 六列账本 + 三规则 + upstream remote——嫁接纪律两件套在嫁接发生前就位 |
| T-01-08 Information Disclosure | accepted | 仅落盘三个公开文档，零密钥/凭据/内部信息 |

无新增威胁面（纯文档/配置计划，无运行时攻击面变化）。

## Next Phase Readiness

- **Ready for 01-03**（构建门/实机验证）：合规基座已锁死，Pitfall 1/2 的纪律载体（账本 + remote + LICENSE 时间条款）全部先于嫁接代码就位。
- **Phase 2/3 直接消费**：DEVIATIONS.md 记账规则（每处 `// FRAME:` 一行、锚点以 pub cache 版本 file:line 为准、按功能块）；upstream remote 可用于 WM 0.5.2 的 cherry-pick/对 diff。
- **Phase 5 发布门**：README Derivation + LICENSE 三行叠加即 pub.dev 包页的归属呈现，发布时复核 grep 门即可。
- 无阻塞项。

## Self-Check: PASSED

- 3/3 key files 在盘（LICENSE / README.md / DEVIATIONS.md，wc -c 实体确认）
- 3/3 task commits 在 git 历史（ecd8113 / eb99e1c / a0bead7，git show 时间戳核验，无 amend/回退）
- 计划级 <verification> 四门复跑全过：① LICENSE 三行 grep -cx =1/1/1 + 正文 grep=1 + BSD 零命中 ② README 五 grep 全非零（Derivation=1/leanflutter=1/bitsdojo=3/DEVIATIONS=1/Windows=2）③ `git remote get-url upstream` 精确等于 https://github.com/leanflutter/window_manager ④ DEVIATIONS grep file:line=2 / FRAME=3
- 全部 task acceptance_criteria 逐条复核通过（含 Task 2 的"## Derivation 恰一次"与 Task 3 的"待回填示例行"）
- must_haves.truths 四条 ↔ coverage D1–D4 全 pass（human_judgment: false 均有非空 pass 证据）

---
*Phase: 01-package-skeleton-license-foundation*
*Completed: 2026-09-04*
