---
phase: 01-package-skeleton-license-foundation
plan: 03
subsystem: infra
tags: [cmake, windows-build, vs2026, cmake-4.x, googletest, release-build, example, uat, human-check, sc1]

# Dependency graph
requires:
  - phase: 01-package-skeleton-license-foundation (plan 01)
    provides: 三平台插件骨架与 example 工程（2d0d408/064fdb0）——本计划的构建对象
provides:
  - CMake Pitfall-1 一行补丁：windows/CMakeLists.txt 在 FetchContent_MakeAvailable(googletest) 前 set(CMAKE_POLICY_VERSION_MINIMUM 3.5)（含英文注释），使 VS2026 捆绑 CMake 4.3.1 可 configure gtest 1.11.0 块
  - example Windows release 构建产物 window_frame_kit_example.exe（本机首跑一次通过，90.3s；Pitfall 2 毒化缓存未命中；Pitfall 3 native_assets 目录预建规避）
  - SC1 人工半边确认：用户实机目视三项 human-check 全过（窗口出现可见 / 示例 UI 可交互 / 关闭按钮正常退出）
  - Phase 3 基线观察记录：裸模板 resize 路径的右缘毫秒级空白现象（见 Baseline Observation 节 + WINDOWS.md #4）
affects: [01-04-ci-first-run, phase-2-wm-port, phase-3-frame-graft]

# Actuals (#2632) — pairs with the plan's `estimate` (40000 tokens) to calibrate future estimates.
actuals:
  tokens: 349    # chars/4 over realized diff (1394 bytes, windows/CMakeLists.txt +3 行)
  tasks: 2
  commits: 1

# Tech tracking
tech-stack:
  added: []    # 零新依赖——仅一行 CMake 兼容性变量
  patterns:
    - "CMake 4.x 兼容补丁置于被拒块（googletest FetchContent）之前、附英文注释说明根因（gtest 1.11.0 cmake_minimum_required < 3.5 被 CMake 4.x 直接拒绝）"
    - "构建失败处置纪律（Pitfall 2）：INSTALL/MSB3073 类异常 = 毒化缓存信号 → rm -rf example/build 全清重建，禁止在脏缓存上重试（本次未命中，纪律留痕）"
    - "人工半边证据标准：release exe 直接启动（不挂调试器）+ 用户目视三项确认，控制台无崩溃 ≠ UI 正确呈现"

key-files:
  created: []
  modified:
    - "windows/CMakeLists.txt（+3 行：Pitfall-1 补丁 set(CMAKE_POLICY_VERSION_MINIMUM 3.5) + 两行英文注释，位于 googletest FetchContent 之前）"

key-decisions:
  - "补丁落在插件根 windows/CMakeLists.txt（example 构建时引用插件 CMake）——不改 PLUGIN_NAME/目标名/PLUGIN_SOURCES（模板 tooling 约束）"
  - "构建首跑一次通过（90.3s，fresh cache + Pitfall 3 目录预建），未触发 Pitfall 2 处置流程"
  - "用户实机观察到的'快速拖拽左缘 resize 时右缘瞬间空白'按用户裁决记为 Phase 3 基线观察（非本计划缺陷）：归因裸 Flutter 模板窗口路径，本计划补丁只触 configure"

patterns-established:
  - "SC1 双边门：构建半边自动化（grep 补丁行 + exe 存在断言）+ 人工半边 human-check 三项（出现可见/可交互/正常退出）——后续实机验证项沿用此双边结构"

requirements-completed: [PUB-01]  # 复制自 PLAN frontmatter；shared-ID gate：01-01/02/03/04 均声明 PUB-01，REQUIREMENTS.md 勾选待最后一个声明计划（01-04）完成后由 ready-ids 放行

# Coverage metadata (#1602) — 三条 must_haves.truths 逐条对应
coverage:
  - id: D1
    description: "example 在 Windows 本机以 release 配置编译产出 window_frame_kit_example.exe（SC1 可自动化半边；D-07 检查④本机执行）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "grep -c 'CMAKE_POLICY_VERSION_MINIMUM' windows/CMakeLists.txt = 1（line 81）; test -f example/build/windows/x64/runner/Release/window_frame_kit_example.exe → 存在; flutter build windows --release 首跑一次通过 90.3s（收口时复验两门均 PASS）"
        status: pass
    human_judgment: false
  - id: D2
    description: "example 实机启动并显示窗口（SC1 人工半边——宿主 UAT 证据标准：控制台无崩溃不等于 UI 正确呈现）"
    requirement: "PUB-01"
    verification:
      - kind: manual_procedural
        ref: "release exe 后台启动（PID 31236）→ 5 秒后 Get-Process 存活断言 pass → 用户目视三项 human-check（窗口出现可见/示例 UI 可交互/关闭正常退出）approved；收口时进程已清理（Stop-Process → STOPPED，无残留）"
        status: pass
    human_judgment: true
    rationale: "视觉窗口行为无头不可判定——用户已实机 approved（2026-09-04），此条目为 UAT 证据留痕；verify-work 阶段可按用户既有批准复核"
  - id: D3
    description: "VS2026 捆绑 CMake 4.x 环境下 example 可正常 configure（研究 Pitfall 1 的一行补丁生效）"
    requirement: "PUB-01"
    verification:
      - kind: other
        ref: "本机 VS2026（generator 'Visual Studio 18 2026'）+ CMake 4.3.1-msvc1：fresh cache configure 通过、无 gtest cmake_minimum_required 拒绝错误、构建直达 exe 产出"
        status: pass
    human_judgment: false

# Metrics
duration: 30min
completed: 2026-09-04
status: complete
---

# Phase 1 Plan 03: Windows 构建门（CMake Pitfall-1 补丁）+ example 实机启动确认 Summary

**VS2026/CMake 4.3.1 环境下 example Windows release 构建一次打通（Pitfall-1 一行补丁 + Pitfall-3 目录预建，90.3s 产出 window_frame_kit_example.exe），用户实机三项 human-check 全过（窗口出现/可交互/正常退出）——SC1 构建半边与人工半边全量交付；用户同时留下一条 Phase 3 基线观察（裸模板快速 resize 右缘毫秒级空白）。**

## Performance

- **Duration:** ≈30 min wall clock（Task 1 补丁+构建 ≈11:35–11:42Z；Task 2 启动+human-checkpoint 等待用户确认；收口会话 ≈12:00–12:05Z——含 checkpoint 等待，活跃执行时间约 15 min）
- **Started:** 2026-09-04T11:35Z（约值，以 Task 1 执行窗口为锚）
- **Completed:** 2026-09-04T12:05Z
- **Tasks:** 2
- **Files modified:** 1（windows/CMakeLists.txt +3 行；exe 为 gitignored 构建产物不入库）

## Accomplishments

- **Task 1：** windows/CMakeLists.txt 在 `FetchContent_MakeAvailable(googletest)` 之前插入 `set(CMAKE_POLICY_VERSION_MINIMUM 3.5)` + 两行英文根因注释（gtest 1.11.0 声明的 cmake_minimum_required < 3.5 被 VS2026 捆绑 CMake 4.x 直接拒绝）；PLUGIN_NAME/目标名/PLUGIN_SOURCES 保持模板原值。预建 example/build/native_assets/windows（Pitfall 3 规避）后 `flutter build windows --release` 首跑一次通过（90.3s，fresh cache），产出 example/build/windows/x64/runner/Release/window_frame_kit_example.exe；Pitfall 2 毒化缓存症状未命中（MSB3073/"Program Files" INSTALL 零出现）。
- **Task 2：** release exe 直接后台启动（不 flutter run、不挂调试器——最接近用户真实场景），5 秒后 Get-Process 存活断言通过（PID 31236）；用户实机目视三项 human-check 全部 approved：① 窗口出现且完整可见 ② 默认插件示例 UI 可见可交互 ③ 关闭按钮正常退出。收口时进程已 Stop-Process 清理并复核无残留（STOPPED）。

## Human-Check Result（SC1 人工半边，用户批准）

三项判定 **PASSED**（用户 approved，2026-09-04）。用户批准时附带一条观察记录（原话）：

> 按住窗口左边边缘然后迅速左右往复拖拽快速改变窗口大小时，窗口空白区域的右边有几毫秒的瞬间没渲染上。

用户裁决：记录为 Phase 3 基线观察，收口 01-03——三项 human-check 判定不受影响，观察项入本 SUMMARY + WINDOWS.md（含复现步骤），标注 Phase 3 frame 接管后复测。详见下节。

## Baseline Observation（Phase 3 基线观察 — 必录项）

**现象：** 快速拖拽左缘 resize 时右缘瞬间空白（毫秒级未渲染）。

- **复现步骤：** 按住窗口左边边缘 → 迅速左右往复拖拽改变窗口大小 → 观察窗口空白区域的右缘（有几毫秒的瞬间没渲染上）。
- **Phase-1 归因（orchestrator 分析，标注为归因注记）：** 本阶段的 example 是**裸 Flutter 模板应用**——frame 接管（NCCALCSIZE/GETMINMAXINFO 嫁接）要到 Phase 3 才落地，因此该现象属于 Flutter Windows 模板的 resize 路径（raster/resizing 同步），**不属于本计划的 CMake configure 补丁**（补丁只触 configure 阶段，不触运行时渲染）。
- **开放问题：** 根因未诊断——render pipeline / shader / windowing 依赖行为三者尚不能区分；该诊断是 Phase 3 的输入。
- **处置：** **Phase 3 frame 接管落地后复测**（记入 Phase 3 backlog/input；WINDOWS.md #4 同步挂账，status open）。window_frame_kit 的目标正是合并 window_manager（完整事件流）+ bitsdojo_window（frame 接管）两家之长，该现象属于届时正面处理的范围。

## Task Commits

Each task was committed atomically:

1. **Task 1: CMake Pitfall-1 补丁 + example Windows release 构建** - `ee91a58` (fix)
2. **Task 2: 启动 example 供实机确认** - 无文件修改（exe 启动 + human-check 收集；产物 gitignored）

**Plan metadata:** 本 SUMMARY + STATE/ROADMAP/WINDOWS 更新随 `docs(01-03): complete windows build gate plan (SUMMARY + state)` 落盘。

## Files Created/Modified

- `windows/CMakeLists.txt` - +3 行：`set(CMAKE_POLICY_VERSION_MINIMUM 3.5)` + 两行英文根因注释，位于 googletest FetchContent 之前（line 81）
- `example/build/windows/x64/runner/Release/window_frame_kit_example.exe` - 构建产物（gitignored，不入库）

## Decisions Made

- **补丁位置与最小面**：只加一行 CMake 变量到插件根 windows/CMakeLists.txt（example 构建引用插件 CMake，研究 Pattern 4 的 include_window_frame_kit_tests 门控在位）——不改 PLUGIN_NAME/目标名/PLUGIN_SOURCES（模板注释明言 tooling 依赖这些名字），不引入任何 C++ 代码（/W4 /WX 零新告警风险）。
- **基线观察不入本计划缺陷**：用户实机观察到的右缘空白按用户裁决记为骨架期基线现象（裸模板窗口路径），不阻塞 SC1 判定；归因诊断留作 Phase 3 输入。
- **CI 保留条款**：若 Plan 04 CI 首跑在 GitHub runner 上不复现 gtest 坑，补丁仍保留——研究 Pitfall 1 明言该行对本机 VS2026/CMake4 环境必需（acceptance_criteria 原样满足）。

## Deviations from Plan

None - plan executed exactly as written（Task 1 首跑一次通过未触发 Pitfall 2 处置流程；Task 2 human-check 三项全过，观察项按用户裁决作为基线记录而非缺陷处置）。

## Issues Encountered

None（构建/启动/确认全程无阻断；用户在 checkpoint 留下的右缘空白观察项按裁决转入 Baseline Observation 节与 WINDOWS.md #4，非本计划执行问题）。

## Known Stubs

无新增占位。WINDOWS.md 联动：新增 #4 基线观察条目（kind: todo，status: open，Phase 3 frame 接管后复测）；既有 #3（DEVIATIONS.md 占位行）保持 open 待 Phase 3。

## User Setup Required

None - no external service configuration required.

## Threat Model Review

| Threat | Disposition | 落地证据 |
|--------|-------------|----------|
| T-01-09 Tampering（CMake 构建缓存毒化） | mitigated | Pitfall 2 处置纪律写入任务 action 并留痕；本次 fresh cache 首跑一次通过，MSB3073/"Program Files" INSTALL 零出现——无脏缓存产物 |
| T-01-10 DoS（构建脚本副作用） | accepted | 实际变更仅一行 CMake 变量 + 注释，零代码零网络新面；构建产物在 gitignored build/ 内未入库 |

无新增威胁面（构建门计划，无运行时攻击面变化）。

## Next Phase Readiness

- **Ready for 01-04**（CI 三平台矩阵 + GitHub 建仓推送 + pana 基线）：本机构建门已通过，CMake 补丁随仓库进入 CI；D-07 检查④的本机半边完成，CI 半边由 01-04 交付。PUB-01 shared-ID gate 的最终勾选待 01-04。
- **Phase 3 输入**：基线观察（右缘毫秒级空白）已挂账 WINDOWS.md #4——frame 接管（NCCALCSIZE/GETMINMAXINFO）落地后复测并诊断根因（render pipeline / shader / windowing 行为三选一）。
- 无阻塞项。

## Self-Check: PASSED

- 1/1 key file 补丁在位：`grep -c CMAKE_POLICY_VERSION_MINIMUM windows/CMakeLists.txt` = 1（line 81，收口时复验）
- exe 产物在盘：example/build/windows/x64/runner/Release/window_frame_kit_example.exe（收口时 test -f 复验）
- 1/1 task commit 在 git 历史：ee91a58（git show 核验，未动、未 amend）
- 进程清理复核：Get-Process window_frame_kit_example → 空（Stop-Process 后 STOPPED），无残留后台实例
- must_haves.truths 三条 ↔ coverage D1–D3：D1/D3 自动化证据 pass（human_judgment: false）；D2 human_judgment: true 且用户实机 approved（证据留痕）
- 基线观察双落盘：本 SUMMARY Baseline Observation 节 + WINDOWS.md #4（含复现步骤与 Phase 3 处置）

---
*Phase: 01-package-skeleton-license-foundation*
*Completed: 2026-09-04*
