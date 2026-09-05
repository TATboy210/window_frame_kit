---
phase: 02-windows
plan: 03
subsystem: windows-native
tags: [integration_test, uat-evidence, dpi, geometry, min-max-aspect, window_manager, parity, deferred-uat]

# Dependency graph
requires:
  - phase: 02-windows
    plan: 02
    provides: C++ 全量面 verbatim 进驻 + tracer 实机 sign-off + 可见控制台拉起 pattern + 22/2 残留封顶门
provides:
  - 02-UAT-EVIDENCE.md 三批共用账本开卷（批①/批②/批③ + phase gate 四段骨架，02-04/02-05 追加）
  - integration_test 本地实机记录：+17 passed / ~3 skipped / −1 failed（isFocused 已诊断归类=环境，非移植回归；两次实跑确定性复现）
  - CAPB-02 换算链机器证据：getSize==640×480 / getTitle=='window_frame_kit_test' / getBounds isA<Rect> 精确断言实机通过（A5 假设 100% 档证实）
  - 批① 人工 7 项判定清单逐条落账（预期值按上游语义写死）——签核 DEFERRED 至 phase 末合并 UAT
affects: [02-04/02-05 账本追加, phase-end consolidated UAT session（批① 7 项待签核）, phase gate SC1-4 收口]

# Actuals (#2632)
actuals:
  tokens: 21000   # 近似值；对标 estimate 25000（machine half only，人工 checkpoint 未在本窗口执行）
  tasks: 1        # Task 1 (tracer/machine)；Task 2 (checkpoint/human) DEFERRED
  commits: 2      # test(02-03) 账本 + docs(02-03) 本 SUMMARY/state

# Tech tracking
tech-stack:
  added: []
  patterns: [红项 A5 诊断路径实跑（verbatim 对照排除移植回归 → 环境归类，不修断言不改码）, deferred-UAT 记账形态（判定表预填上游语义预期值 + DEFERRED 标注，批量会话直接续用）]

key-files:
  created:
    - .planning/phases/02-windows/02-UAT-EVIDENCE.md（三批共用账本：integration_test 实跑记录段 + 批① 判定表（DEFERRED）+ 批②/批③/phase gate 占位）
  modified: []

key-decisions:
  - "isFocused 红项归类=环境（Windows 前台激活限制），不修：IsFocused 实现与上游 pub cache 0.5.2 逐字一致（GetMainWindow()==GetForegroundWindow()）；重试一次确定性复现排除 flake；改断言=逐字区违规。批① 合并 UAT 时判定项 1（窗口目视出现）覆盖 focus 语义的人工面"
  - "用户指令（auto-run mode, 2026-09-05）：计划 Task 2 人工 checkpoint DEFERRED 至 phase 末合并 UAT 会话（orchestrator 主持）；判定清单 7 项 verbatim 保留在账本批① 段供批量会话直接执行"
  - "零代码改动承诺兑现：git status 全程仅 .planning/**（orchestrator 所有的 config.json/state.json/milestone.lock 未触碰）；quick run 三门绿（analyze 0 / test +13 / verbatim_diff 22+2）"

patterns-established:
  - "deferred-UAT 账本形态：判定表预填『项/预期上游语义/用户回答(deferred)/日期』四列 + DEFERRED 标注行，批量会话时用户回答与日期直接续写，sign-off 行等 'approved'"

requirements-completed: [CAPB-01, CAPB-02, CAPB-03]   # 机器半边证据落账；人工签核 DEFERRED 至 phase 末合并 UAT——shared-ID gate（与 02-04/05 共同声明），REQUIREMENTS.md 勾选待批量 sign-off 后由 ready-ids 放行

coverage:
  - id: D1
    description: "integration_test 17 用例本地实机绿记录落账（getSize==640×480 / getTitle / getBounds 精确断言通过；3 项上游平台 skip 照 skip；isFocused 红项诊断归类=环境）"
    requirement: "CAPB-02"
    verification:
      - kind: integration
        ref: "cd example && flutter test integration_test -d windows → +17 ~3 −1（isFocused）；两次实跑一致；账本 integration_test 记录段含命令/日期/计数/诊断"
        status: pass
      - kind: other
        ref: "isFocused 归类证据：windows/window_manager.cpp:272-274 与 pub cache window_manager-0.5.2 同实现逐字一致；verbatim_diff 'VERBATIM PROOF OK (17 dart files zero-diff, cpp residue 22+2)'"
        status: pass
    human_judgment: false
  - id: D2
    description: "UAT-EVIDENCE 账本开卷（三段 + phase gate 骨架；02-04/02-05 追加位就绪）"
    verification:
      - kind: other
        ref: ".planning/phases/02-windows/02-UAT-EVIDENCE.md 落盘并 commit ec6ca53；grep 'integration_test' + '批①' 命中"
        status: pass
    human_judgment: false
  - id: D3
    description: "批① 实机 UAT 7 项判定（初始化协议/几何读写/DPI 双档/center/min-max/aspectRatio/事件旁证）+ dated sign-off"
    requirement: "CAPB-01"
    verification: []
    human_judgment: true
    status: deferred
    rationale: "DEFERRED to phase-end consolidated UAT (orchestrator decision under user auto-run directive 2026-09-05)——窗口观感/拖拽约束/DPI 双档必须用户目视（宿主 UAT 证据标准，Pitfall 11）；判定清单已 verbatim 预填账本批① 段，批量会话直接续用"
  - id: D4
    description: "批① 异常项诊断纪律：任何红项走 A5 路径（DPR 链路先核）归类上游语义/环境/移植回归"
    requirement: "CAPB-03"
    verification:
      - kind: other
        ref: "Task 1 实跑已演练：isFocused 红项按纪律完成 verbatim 对照 + 环境归类 + 账本落账（未静默重跑到绿）"
        status: pass
    human_judgment: false

# Metrics
duration: 25min（machine half only；批① 人工 checkpoint 不计入——deferred）
completed: 2026-09-05
status: complete   # machine half 完成；Task 2 人工签核 DEFERRED 至 phase 末合并 UAT（orchestrator 指令，非 halt——02-04 不被阻塞）
---

# Phase 2 Plan 03: 批① 实机对等验证 — Machine Half Summary

**上游 integration_test 17 用例在本机 Windows 实机跑通精确断言（getSize==640×480 / getTitle / getBounds——CAPB-02 逻辑像素换算链机器证据成立），唯一红项 isFocused 经 A5 纪律诊断为环境（前台激活限制）非移植回归并落账；三批共用 UAT-EVIDENCE 账本开卷，批① 人工 7 项判定 DEFERRED 至 phase 末合并 UAT 会话**

## Performance

- **Duration:** ~25 min machine（integration_test 两次实跑 + 诊断 + 账本 + quick run）
- **Started:** 2026-09-05T13:05+08:00（约）
- **Completed:** 2026-09-05
- **Tasks:** 1 of 2（Task 1 machine half；Task 2 human checkpoint DEFERRED）
- **Files modified:** 1（账本，零代码改动）

## Accomplishments

- **integration_test 实机落账：** `+17 passed / ~3 skipped / −1 failed`；skip 3 项均为上游测试文件自带平台裁剪（isAlwaysOnBottom/isMovable/isSkipTaskbar on Windows）；按规则重试一次，结果确定性一致（非 flake）
- **SC1/SC2 几何半边机器证据成立：** waitUntilReadyToShow(640×480, title) → native GetBounds ÷DPR → Dart 逻辑像素精确断言全通（getSize==640×480、getTitle=='window_frame_kit_test'、getBounds isA<Rect> size==640×480）——A5 假设在本机 100% 档证实
- **isFocused 红项 A5 纪律诊断（不修不迁就）：** 实现逐字对照上游 pub cache 一致 → 归类环境（Windows 限制后台进程 SetForegroundWindow，runner 控制台持有前台）→ 账本落诊断全文；未改断言（逐字区违规）、未静默重跑
- **三批共用账本开卷：** 批①（判定表已预填 7 项上游语义预期值，DEFERRED 标注）/ 批② / 批③ / phase gate 四段骨架，02-04/02-05 直接追加
- **quick run 三门绿：** analyze 0 issues / test +13 all passed / verbatim_diff 'VERBATIM PROOF OK (22+2)'；`grep -rn 'FRAME:' lib windows example test` = 0；git status 仅 .planning/**（零代码改动承诺兑现）

## Task Commits

1. **Task 1: integration_test 实机跑 + 账本开卷** — `ec6ca53` (test) — .planning/phases/02-windows/02-UAT-EVIDENCE.md 新建（77 行）
2. **Plan metadata (machine half)** — 本 SUMMARY + STATE/ROADMAP（docs commit，见 git log）

**Task 2（批① 人工 checkpoint）:** DEFERRED — 无 commit；账本批① 段判定表已预填待批量会话续写。

## Human Check（Task 2 — DEFERRED）

DEFERRED to phase-end consolidated UAT (orchestrator decision under user auto-run directive 2026-09-05) — items listed verbatim for the batch session:

> **判定清单（逐项操作 → 用户目视/回读确认）：**
> 1. **初始化协议（CAPB-01/SC1）：** 冷启动后窗口按 main.dart WindowOptions 出现（尺寸/居中/标题栏样式对照源码读数）——02-02 tracer 已初证，此处复确认并记录
> 2. **几何读写（CAPB-02/SC1）：** 在 example UI 触发 setSize(400,400)/setPosition/getSize 回读——窗口即时变化且控制台/回读值 = 设定值（逻辑像素）
> 3. **DPI 双档（CAPB-02/SC1）：** 系统显示缩放切 100% 与 150% 各跑一次第 2 项——setSize(400,400) 后目视 + getSize 回读均为 400（物理尺寸随 DPI 变、逻辑值恒定）
> 4. **center/setAlignment（CAPB-02）：** 触发 center——窗口目视居中
> 5. **min/max（CAPB-03）：** setMinimumSize 后拖边框缩小——拖至 min 以下被挡；setMaximumSize 同理；reset（0,0）后解锁
> 6. **aspectRatio（CAPB-03）：** setAspectRatio(16/9) 后拖边——比例锁定；reset 解锁
> 7. **事件旁证：** 上述拖拽期间控制台连续 resize 行 + 松手一次 resized 行（Example 5 对照，native 发射点 plugin.cpp:230-232/216-219）
>
> **通过标准：** 7 项全过 → 批① dated sign-off 写入 UAT-EVIDENCE。
> **响应协议：** 用户回复 "approved"（或逐项"是"）→ 记账收口并 commit；任何一项异常 → 记录症状原文 + 当时的 DPI/显示器配置，按 Task 1 第 4 步诊断路径归类（上游语义/环境/移植回归），移植回归修复后**本批复验**再请用户签核；不得以"控制台无崩溃"替代目视判定（宿主 UAT 证据标准）。

**拉起形态（02-02 固化 pattern，批量会话复用）：** 可见控制台 + tee 日志（`cd /d/window_frame_kit/example && D:/flutter/bin/flutter run -d windows`，Start-Process powershell -NoExit + Tee-Object）。

## Decisions Made

- **isFocused = 环境归类，零改动收口**：verbatim 对照排除移植回归；前台激活限制与 RESEARCH Pitfall 11"GH runner 焦点不可靠"同源；批量 UAT 时用户手动激活窗口的目视面（判定项 1）覆盖该语义
- **DEFERRED 记账而非 halt**：plan frontmatter status: complete（machine half 交付完整）；02-04 depends_on 02-03 不被阻塞；人工签核债显式挂在账本批① 段 + 本 SUMMARY coverage D3（human_judgment: true, status: deferred），verify-work 分类器会路由到人

## Deviations from Plan

**0 个代码偏差。1 个执行形态偏差（orchestrator 指令）：**

**1. [指令偏差] Task 2 人工 checkpoint DEFERRED 至 phase 末合并 UAT**
- **Found during:** 计划派发（orchestrator objective）
- **Issue:** 用户 auto-run 指令（2026-09-05）：machine half only；人工 7 项判定推迟到 phase 末合并会话
- **Fix:** 账本批① 段判定表预填上游语义预期值 + DEFERRED 标注；本 SUMMARY Human Check 段 verbatim 保留清单；coverage D3 标记 deferred 供分类器路由
- **Files modified:** 仅 .planning/**
- **Verification:** 账本 grep '批①' + 'DEFERRED' 命中；7 项判定表行齐
- **Committed in:** ec6ca53 + 本 docs commit

**Task 1 fails_when 逐条核对：** 红项（isFocused）已先记账诊断归类再收口（未静默重跑到绿）✓；输出无 "No tests were found" ✓；账本含 integration_test 段 ✓。注：runner 退出码实际为 0（Flutter test 对 integration_test 失败用例仍返 0，以输出计数 +17 ~3 −1 为准）——verify 表达式 `rc -eq 0` 形式满足，实质判定以账本计数为准，此处如实记录。

---

**Total deviations:** 代码 0 / 执行形态 1（orchestrator 指令 defer）
**Impact on plan:** 机器半边零妥协交付；人工半边显式挂账（不丢失、可续用）。

## Issues Encountered

- integration_test runner 对失败用例返回退出码 0（`RC=0` 但输出 `Some tests failed.`）——判定依据必须看计数行而非 rc；已如实记账（见 Deviations 段注）
- isFocused 环境红项为可预期复现项：后续批②/批③ 若复用 integration_test 需预判同款前台激活限制

## User Setup Required

None — 批① 人工判定由 orchestrator 在 phase 末合并 UAT 会话主持；账本已备好判定表与拉起形态。

## Next Phase Readiness

- **Ready for 02-04（批②：状态操作/查询/事件流 CAPB-04/05/06）：** 账本批② 段占位就绪；Example 5 触发-事件对照表 + 02-02 可见控制台拉起 pattern 直接复用
- **Phase 末合并 UAT 债务：** 批① 7 项判定（本 SUMMARY Human Check 段 verbatim）+ 批②/批③ 各批清单一并签核；异常项按 A5 路径归类
- **REQUIREMENTS.md CAPB-01/02/03 维持不勾**（shared-ID gate + 人工签核 deferred；待批量 sign-off 后由 ready-ids 放行）
- **禁改区基线未漂移：** verbatim 22+2 / FRAME: 0 / CI 未触碰（git diff 不含 .github/workflows/ci.yml ✓）

## Self-Check: PASSED (machine-half scope)

- key-files 存在性：`.planning/phases/02-windows/02-UAT-EVIDENCE.md` `[ -f ]` 实测在盘（77+ 行，含 integration_test 记录段 + 批① 判定表）
- `git log --grep="02-03"` → ec6ca53（test）✓
- Task 1 acceptance_criteria 复跑：账本含命令/日期/计数/诊断 ✓；getSize/getTitle 精确断言在通过列表 ✓；git status 仅 .planning/**（无代码改动，无修复 commit → DEVIATIONS.md 无新行、verbatim_diff 绿）✓
- quick run 三门复跑：analyze 0 issues ✓ / flutter test +13 all passed ✓ / VERBATIM PROOF OK (22+2) ✓ / FRAME: 计数 0 ✓
- **NOT claimed:** Task 2 acceptance_criteria（批① 7 项判定 + "approved" sign-off）——DEFERRED，本 Self-Check 不覆盖、不冒充
- Plan 收口门（machine half）：integration_test 本地记录落账 ✓；批① sign-off DEFERRED（挂账）；quick run 绿 ✓；无 CI 新增门（本计划零代码改动，沿用上次绿 run）

---
*Phase: 02-windows*
*Completed: 2026-09-05 (machine half; batch-① human sign-off deferred to phase-end consolidated UAT)*
