---
phase: 01-package-skeleton-license-foundation
plan: 04
subsystem: infra
tags: [github-actions, ci, pana, gh-cli, windows, linux, macos, flutter-3.47.2]

# Dependency graph
requires:
  - phase: 01-package-skeleton-license-foundation (plans 01-03)
    provides: 插件骨架 + pubspec 身份字段（D-01 URL）+ LICENSE/README/DEVIATIONS + Pitfall-1 CMake policy 补丁（本次 CI windows leg 的实际救星）
provides:
  - 三平台 CI 矩阵 workflow（analyze/format/test/native build 每平台四项 + pana ubuntu 评分门）
  - 公开仓库 github.com/TATboy210/window_frame_kit（origin 已配、master 已推送）
  - pana 基线钉死：150/160 → threshold 10（D-08 闭环）
  - 研究假设 A2/A3/A5 首跑核对结论（Open Question 4 关闭）
affects: [phase-2 移植（每次 push/PR 被三平台 CI 守护）, phase-3 frame 嫁接, phase-5 发布管线（PUB-04 精简 description 后须重钉 pana threshold）]

# Actuals (#2632) — same estimateTokens scale (chars/4 over realized diff)
actuals:
  tokens: 900
  tasks: 2
  commits: 3

# Tech tracking
tech-stack:
  added:
    - GitHub Actions ci workflow（actions/checkout@v5 + subosito/flutter-action@v2，flutter-version 钉 "3.47.2"，cache true）
    - pana --exit-code-threshold 评分门（ubuntu-latest 专属 job，threshold=10 已按真实基线钉死）
    - GitHub 公开仓库 TATboy210/window_frame_kit（D-01 one-way 落盘）
  patterns:
    - native build 步骤 working-directory: example（插件根 windows/linux 是 library-only CMake 壳，编译宿主是 example/——上游 window_manager build.yml 同款）
    - Pitfall-3 CI 变体预防：windows build 前 mkdir -p example/build/native_assets/windows
    - 双 remote 仓库（origin+upstream）中 gh CLI 会解析到错误仓库——所有 gh 调用显式 -R TATboy210/window_frame_kit

key-files:
  created:
    - .github/workflows/ci.yml
  modified: []

key-decisions:
  - "pana threshold 钉死为 10（满分 160 − 首跑基线 150，D-08 公式；pana 语义 (max−granted)>threshold 才红，基线本身通过）"
  - "唯一扣分项 = pubspec description 超 pana 180 字符建议长度（0/10）；D-03 措辞为 Plan 01 已锁定交付物且超出本计划文件边界，骨架期保留不改，Phase 5 PUB-04 精简后须重钉 threshold"
  - "native build 在 example/ 下执行（working-directory），修正研究 Example 3 骨架的根目录简化写法——证据：上游 build.yml 实读 + flutter_tools build_windows.dart 直接用 project.windows + 插件根 CMake 为 library-only"
  - "A2 核对结论 = 原假设为假：windows-latest 现为 windows-2025-vs2026 镜像（VS 2026 捆绑 CMake 4.x），gtest 坑会在 CI 复现——Plan 03 的 CMAKE_POLICY_VERSION_MINIMUM 3.5 补丁是本次 windows leg 绿的实际保障（configure 仅出 <3.10 弃用警告带，无 <3.5 硬错误）"

patterns-established:
  - "CI 单文件三平台 matrix（fail-fast: false）+ pana 独立 ubuntu job——D-05~D-08 的落地形态"
  - "pana 基线以 YAML 注释元数据留痕（基线分数/满分/钉死日期/run id），threshold 变更必须同步更新注释"
  - "双 remote 仓库中 gh 一律带 -R 显式钉仓库"

requirements-completed: [PUB-01]

coverage:
  - id: D1
    description: "push 到 master 与所有 PR 触发三平台并行 CI：每平台 analyze+format / flutter test / native build，pana 仅 ubuntu 且带通过线阈值（D-05/D-06/D-07/D-08）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "grep gate: windows-latest|ubuntu-latest|macos-latest|exit-code-threshold|pull_request|flutter-version \"3.47.2\" .github/workflows/ci.yml → WORKFLOW_OK；python yaml.safe_load → jobs [checks, pana], matrix 3 OS, triggers [push(master), pull_request]"
        status: pass
      - kind: e2e
        ref: "GitHub Actions run 33873676570（push master 触发，D-06 证据）：checks(windows/ubuntu/macos)+pana 四 job 全 success"
        status: pass
    human_judgment: false
  - id: D2
    description: "github.com/TATboy210/window_frame_kit 存在且 master 已推送，origin remote 指向它（D-01）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "git remote get-url origin | grep github.com[:/]TATboy210/window_frame_kit && git branch --show-current = master && gh repo create 输出 https://github.com/TATboy210/window_frame_kit → CI_GREEN"
        status: pass
    human_judgment: false
  - id: D3
    description: "CI 首跑三 leg + pana 全绿；pana 基线分数已记录且 threshold 已按基线钉死（D-08；A2/A3 核对完成）"
    requirement: "PUB-01"
    verification:
      - kind: e2e
        ref: "run 33873676570 conclusion=success（首跑）+ run 33875181515 conclusion=success（threshold=10 钉死后复跑）；grep 'exit-code-threshold 10' ci.yml；pana log 'Points: 150/160.'"
        status: pass
    human_judgment: false

# Metrics
duration: 38 min
completed: 2026-09-04
status: complete
---

# Phase 1 Plan 04: CI 三平台矩阵 + 建仓推送 + pana 基线钉死 Summary

**三平台 GitHub Actions 矩阵（analyze/format/test/native build，Flutter 3.47.2 钉死）+ pana ubuntu 评分门按真实基线 150/160 钉死 threshold=10；公开仓 github.com/TATboy210/window_frame_kit 建成、master 已推、首跑与复跑均四 job 全绿（D-01/D-05~D-08 全闭环）。**

## Performance

- **Duration:** 38 min（其中 ~11 min 为两次 CI run 等待）
- **Started:** 2026-09-04T12:29:30Z
- **Completed:** 2026-09-04T13:07:00Z
- **Tasks:** 2
- **Files modified:** 1（生产文件 .github/workflows/ci.yml；另有本 close-out 的 planning 文件）

## Accomplishments

- `.github/workflows/ci.yml`：checks job 三 OS matrix（fail-fast: false）× 四项检查（pub get/analyze/format --set-exit-if-changed/test + 按平台条件 native build）；pana job 仅 ubuntu-latest，`--exit-code-threshold` 门；触发 = push[master] + pull_request 不限分支；零 secrets、零 publish 步骤、仅 checkout@v5 + flutter-action@v2
- D-01 one-way 落盘：`gh repo create TATboy210/window_frame_kit --public --source=. --remote=origin --push` 一条命令建成公开仓并推 master（URL 与 pubspec repository/homepage/issue_tracker 三字段逐字一致，推送前核对门通过）
- CI 首跑（run 33873676570）三 leg + pana 全绿，~5 分钟完成；pana 真实基线 **150/160**，threshold 由临时 60 钉死为 **10**（160−150），钉死后复跑（run 33875181515）再次四 job 全绿
- 研究假设核对（Open Question 4 关闭）：
  - **A2 = 假（重要）**：windows-latest 已是 `windows-2025-vs2026` 镜像（Version 20260824.214.3），VS 2026 捆绑 CMake 4.x——gtest 1.11 的 min<3.5 硬错误**会在 CI 复现**；Plan 03 已入库的 `CMAKE_POLICY_VERSION_MINIMUM 3.5` 补丁自动生效（configure 仅出 "<3.10 will be removed" 弃用警告带，`√ Built ...window_frame_kit_example.exe`）。补丁从"本机必需"升格为"CI 也必需"——不可删
  - **A3 = 真**：ubuntu apt 清单（clang cmake ninja-build pkg-config libgtk-3-dev）自足，`✓ Built build/linux/x64/release/bundle/window_frame_kit_example`；macos 模板一次通过，`✓ Built build/macos/Build/Products/Release/window_frame_kit_example.app (38.7MB)`
  - **A5 = 部分假（扣分项类别与预期不同）**：README/CHANGELOG/example/dartdoc/license 全满分，唯一扣分是 pubspec description 超长（0/10），非预期的 example/README 完整度分项
  - Pitfall-3 CI 变体：windows leg 无 native_assets INSTALL 错误（预防性 mkdir 已按 orchestrator 指令前置；无法区分"mkdir 兜底生效"还是"runner 环境不触发"，预防步保留，成本≈0）
- PUB-01 shared-ID 门关闭：四个计划均完成，REQUIREMENTS.md 的 PUB-01 标记 Complete（Phase 5 发布门按计划复核归属）

## Task Commits

1. **Task 1: CI workflow 落盘（D-05/D-06/D-07/D-08）** - `57bd97d` (ci)
2. **Task 2: 建仓推送 + CI 首跑全绿 + pana 基线钉死（D-01 + D-08 闭环）** - `4bb91a3` (ci, threshold pin；建仓推送本身无本地文件变更，远端仓库 + origin remote 为外部产物)

**Plan metadata:** 本 close-out commit（docs(01-04): complete CI launch plan）

## Files Created/Modified

- `.github/workflows/ci.yml`（新建，74 行）- 三平台 matrix checks job + pana ubuntu job；pana 步骤上方 YAML 注释记录基线 150/160、threshold=10、钉死日期 2026-09-04、run id、扣分项与 Phase 5 重钉提醒

## Decisions Made

1. **native build 步骤加 `working-directory: example`**（对研究 Example 3 骨架的修正，见 Deviations #1）——插件根的 windows/linux/ 是 library-only CMake 壳（设计为被 app 工程 add_subdirectory），macos/ 是 podspec；flutter_tools `build_windows.dart` 直接构建 `project.windows`（当前目录），根目录裸跑必红。证据：实读上游 window_manager build.yml（`working-directory: ./packages/window_manager/example`）+ 本机 Flutter 3.47.2 工具源码。计划中 Pitfall-3 mkdir 路径 `example/build/native_assets/windows` 亦印证构建树在 example 下
2. **threshold=10 而非"顺手修 description 冲 160"**：pubspec description 措辞是 D-03 在 Plan 01 的锁定交付物（dry-run 0 warnings 已验证），且本计划文件边界仅 ci.yml；pana 的长度扣分是评分启发式非发布阻断。处置：基线 150 如实钉死，YAML 注释 + WINDOWS.md 挂账 Phase 5（PUB-04）精简后重钉
3. **gh CLI 全部显式 `-R TATboy210/window_frame_kit`**：本仓有 origin+upstream 双 remote，gh 的仓库解析选中了 upstream（leanflutter/window_manager），`gh run list --workflow ci` 报"找不到 workflow"且列出了上游的 run——已按显式 -R 修正（见 Deviations #2）

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] native build 步骤从仓库根改为 working-directory: example**
- **Found during:** Task 1（写 ci.yml 前的证据核查）
- **Issue:** 研究 Example 3 骨架把 `flutter build windows/linux/macos --release` 写在仓库根；插件根无 app 工程，根目录裸跑三平台 build 必红（首跑即三 leg 全挂）
- **Fix:** 三个 native build 步骤加 `working-directory: example`；windows 的 Pitfall-3 mkdir 步保持在仓库根用全路径 `example/build/native_assets/windows`（与计划原文逐字一致）
- **Files modified:** .github/workflows/ci.yml
- **Verification:** 首跑三 leg 全绿：windows `√ Built build\windows\x64\runner\Release\window_frame_kit_example.exe`、ubuntu `✓ Built build/linux/x64/release/bundle/window_frame_kit_example`、macos `✓ Built build/macos/Build/Products/Release/window_frame_kit_example.app`
- **Committed in:** `57bd97d`

**2. [Rule 3 - Blocking] gh CLI 双 remote 仓库解析错误 → 显式 -R 钉仓库**
- **Found during:** Task 2（首跑观察，`gh run list --workflow ci` 报 could not find any workflows named ci）
- **Issue:** 仓库同时有 origin（新建）与 upstream（D-09，leanflutter/window_manager）两个 remote，gh 默认解析选中了 upstream，列出的 workflow/run 全是上游的
- **Fix:** 后续所有 gh 调用显式 `-R TATboy210/window_frame_kit`；纯流程修正，无仓库文件变更；已写入 patterns-established 供后续 phase 沿用
- **Verification:** `-R` 后 `gh run list` 立即列出本仓 ci run 33873676570（in_progress → success）
- **Committed in:** 不适用（流程偏差，无文件变更）

---

**Total deviations:** 2 auto-fixed（2 × Rule 3 - Blocking，0 Rule 1/2/4）
**Impact on plan:** 两处均为"首跑必红"的前置排雷与工具解析修正，零范围蔓延；计划的全部验收 grep/命令门原样通过。

## Issues Encountered

- 无红 leg：CI 首跑一次全绿（两次预防性修正——working-directory 与 mkdir——均在前置核查/Task 1 阶段落位，未消耗任何 fix-push-rerun 循环）
- Write/Edit 工具两次瞬态分类器超时（模型侧临时不可用），重试即成功，无产物影响

## Authentication Gates

- Task 2 前置门 `gh auth status`：TATboy210 已登录（keyring, https），scopes 含 repo + workflow——直接通过，未触发 human-action checkpoint
- **D-01 授权证据：** 用户本会话交互式确认「确认：建公开仓并推送」（orchestrator 记录），公开仓创建获用户明确授权后执行

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Phase 1 完结（4/4 计划）**：骨架 + 许可基座 + upstream 纪律账本 + CI 全落地；PUB-01 关闭
- Phase 2（WM 0.5.2 移植）起跑即受三平台 CI 守护：每次 push/PR 自动 analyze/format/test/native build + pana≥150 等效门
- **不可删项**：windows/CMakeLists.txt 的 `CMAKE_POLICY_VERSION_MINIMUM 3.5`（A2 核对证明 CI runner 也是 CMake 4.x）
- Phase 5 挂账（WINDOWS.md #5）：PUB-04 精简 pubspec description 至 ≤180 字符后，pana 预期回到 160/160，须把 threshold 重钉为 160 − 新基线
- 无阻断项

## Self-Check: PASSED

- `.github/workflows/ci.yml` 存在于磁盘且已入库（`git diff --stat 52f1d4e..HEAD`：74 insertions）
- Task commits 存在：`57bd97d`、`4bb91a3`（git log 核实）
- Task 1 verify 门复跑：WORKFLOW_OK（六 grep 全中）+ YAML_OK（jobs=[checks, pana]、matrix 三 OS、triggers=[push, pull_request]）+ fail-fast: false（L17）+ 负向 grep 无 secrets/publish/melos/旧版 action
- Task 2 verify 门复跑：CI_GREEN（origin URL 匹配、master、最新 run conclusion=success、`exit-code-threshold 10` 数值在位）
- 全部 acceptance_criteria（Task 1 六条 + Task 2 五条）逐条通过，见 Accomplishments/Decisions/coverage
- PUB-01 shared-ID gate：ready-ids 返回 1/1 ready，本 close-out 标记 Complete

---
*Phase: 01-package-skeleton-license-foundation*
*Completed: 2026-09-04*
