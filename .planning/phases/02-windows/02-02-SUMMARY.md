---
phase: 02-windows
plan: 02
subsystem: windows-native
tags: [window_manager, verbatim-port, cpp, c-api-bridge, cmake, gtest, tracer, checkpoint, ci]

# Dependency graph
requires:
  - phase: 02-windows
    plan: 01
    provides: Dart 全量面 verbatim（channel 锚 window_frame_kit）+ verbatim_diff.sh A 段 + example 验证载体 + CI 三 leg
  - phase: 01-package-skeleton-license-foundation
    provides: C-API 壳三件 + registrant 四锚 + CMake 承重墙 policy 行
provides:
  - window_manager 0.5.2 Windows C++ 全量面 verbatim 进驻（plugin.cpp 597 行 + window_manager.cpp 1129 行，残留 diff 精确封顶 22+2）
  - Pattern 1 注册链落码：registrant → plugin_c_api.cpp → 桥接头（14 行）→ WM 匿名命名空间（壳三件零改动）
  - tool/verbatim_diff.sh B 段（C++ 22/2 精确行数断言，CI ubuntu leg 持续门）
  - gtest 占位测试（保活 window_frame_kit_test.vcxproj + policy 补丁给 Phase 3 纯函数测试）
  - SC1 tracer 半边 + CAPB-01 native 接通：实机 4 项判定全过，dated sign-off 2026-09-05
affects: [02-03/04/05 实机对等 checkpoint, phase-3 frame graft（禁改区基线已锁）, phase-5 publish]

# Actuals (#2632)
actuals:
  tokens: 52000   # 近似值（harness 不暴露精确计数）；对标 estimate 55000
  tasks: 3
  commits: 2

# Tech tracking
tech-stack:
  added: []   # 零新依赖（对等移植）
  patterns: [C-API 壳+匿名命名空间桥接落码 (Pattern 1), 残留行数封顶=禁改区守护 (22/2), 可见控制台 tracer 拉起 (Start-Process powershell + Tee-Object), 证据驱动 checkpoint 驳回恢复 (mtime + native-only 事件行)]

key-files:
  created:
    - windows/window_manager.cpp（1129 行 verbatim，保留上游文件名，Open Q1 定案；仅 L1 include 适配）
  modified:
    - windows/window_frame_kit_plugin.cpp（骨架 59 行 → WM 597 行 verbatim + 3 适配 + 14 行桥接尾块 = 605 行）
    - windows/window_frame_kit_plugin.h（骨架类头 → 最小桥接头 14 行，Example 4 精确形态）
    - windows/CMakeLists.txt（PLUGIN_SOURCES 三源 + PLUGIN_NAME/TEST_RUNNER 双 codecvt silence define；承重墙行原样）
    - windows/test/window_frame_kit_plugin_test.cpp（模板 GetPlatformVersion 测试 → gtest 占位 + DEV 注释）
    - tool/verbatim_diff.sh（B 段：22/2 精确断言 + 输出串升级）

key-decisions:
  - "桥接头落 14 行（RESEARCH Example 4 精确形态）而非 frontmatter 估值 21 行——Example 4 是 probe 构建+启动双证的定案形态，验收门 ≤25 行 + 必含串全过；估值让位实证"
  - "tracer checkpoint 拉起形态：detached exe（orchestrator rule）+ Start-Process powershell 可见控制台 + Tee-Object 日志双写——首轮驳回证明纯 detached 无控制台使事件行对用户不可见（判定项 3 的观察面缺失）"
  - "用户指令（2026-09-05）：example 移除系统托盘功能 → 排期 02-05 执行（该计划拥有 CAPB-11 穿透 UAT，tray 原为其恢复路径）；已记 WINDOWS.md #7"

patterns-established:
  - "checkpoint 驳回恢复协议：先取硬证据（artifact mtime vs 源 mtime、native-only 事件行、Dart 侧首 invoke 反证）再定 stale/真 bug——本轮 mtime + 7002 行日志证据推翻 stale-exe 诊断，避免无谓 rebuild 循环"
  - "实机 tracer 拉起 = 可见控制台 + tee 日志：debugPrint 事件行同时面向用户目视与机器证据留存（example/build/tracer_stdout.log，UTF-16）"

requirements-completed: [CAPB-01, CAPB-04, CAPB-05, CAPB-06]   # shared-ID gate：四 ID 与 02-03/04/05 共同声明，REQUIREMENTS.md 勾选待兄弟计划落齐后由 ready-ids 放行（02-01 同款处理）

coverage:
  - id: D1
    description: "本机 example release 构建绿（A4：D:/ 真实路径复现 probe 结论）+ gtest 目标同构建绿"
    verification:
      - kind: other
        ref: "cd example && flutter build windows --release → ✓ Built window_frame_kit_example.exe（56.9s）；window_frame_kit_test.exe 产出；无 C4996/MSB3073"
        status: pass
    human_judgment: false
  - id: D2
    description: "C++ 残留 diff 精确封顶：plugin.cpp 恰 22 行（3 hunk：L1/L112/尾块）+ window_manager.cpp 恰 2 行（1 hunk：L1）= SC5 C++ 半边机器门"
    verification:
      - kind: other
        ref: "bash tool/verbatim_diff.sh <pub-cache-0.5.2> → 'VERBATIM PROOF OK (17 dart files zero-diff, cpp residue 22+2)'（本地 + CI ubuntu leg 双过）"
        status: pass
    human_judgment: false
  - id: D3
    description: "匿名命名空间内部标识符零改名（D-01 C++ 侧执行形态）：WindowManagerPlugin/WindowManager/window_manager 成员等含 window_manager 行全部原样"
    verification:
      - kind: other
        ref: "grep -c window_manager windows/window_frame_kit_plugin.cpp = 112 ≥ 100（粗证；精确面由 22 行 diff 封顶——差 1 行 = L112 channel 串合法适配点）"
        status: pass
    human_judgment: false
  - id: D4
    description: "tracer 实机成立：example 启动 → 窗口按 WindowOptions 出现 + WindowEventLogger 事件行 = ensureInitialized → waitUntilReadyToShow → native 事件回路全通（CAPB-01 tracer 证据）"
    verification:
      - kind: other
        ref: "round-1 日志 7002 行（example/build/tracer_round1_evidence.log，gitignored 本地留存）：show/focus/blur/move×N/resize/resized/close 全部 native 发射；round-2 可见控制台复证"
        status: pass
      - kind: manual
        ref: "用户实机 4 项判定全过，dated sign-off 2026-09-05：'去掉系统托盘的功能 approved — 4 项全过'"
        status: pass
    human_judgment: true
    rationale: "窗口观感（800×600/居中/hidden 标题栏）与事件行必须用户目视——宿主 UAT 证据标准；首轮驳回经证据诊断为观察面缺失（无控制台）+ preventClose 开关语义误读，非移植缺陷"
  - id: D5
    description: "注册链四锚不动：registrant → plugin_c_api.cpp → WindowFrameKitPlugin::RegisterWithRegistrar → WindowManagerPlugin::RegisterWithRegistrar（桥接落码，Pitfall 4 定案）"
    verification:
      - kind: other
        ref: "git diff --stat f055eeb 不含 plugin_c_api.cpp / include/**/c_api.h / pubspec.yaml；无'未解析的 WindowFrameKitPluginCApiRegisterWithRegistrar'链接错误；exe 启动即事件发射 = 链路通电"
        status: pass
    human_judgment: false
  - id: D6
    description: "禁改区零触碰 + 承重墙保留 + channel 红线 + FRAME: 零标记（prohibitions 6 条全 resolved）"
    verification:
      - kind: other
        ref: "grep -c 'CMAKE_POLICY_VERSION_MINIMUM 3.5' = 1；grep -rn 'FRAME:' lib windows example test tool = 0；'\"window_manager\"' 串在 plugin.cpp = 0；残留 22/2 封顶=quirk（IsWindows11OrGreater 恒真/物理 px/taskbar_ 无空判）全 verbatim；越界断言 git status 仅 windows/**+tool"
        status: pass
    human_judgment: false
  - id: D7
    description: "exe 启动存活断言 + 单原子 commit + push + CI 四 job 绿（windows leg 重点：native build 含 gtest 门开构建）"
    verification:
      - kind: other
        ref: "Start-Process → 6s ALIVE → Stop → CLEANED_UP（Task 2 探针）；CI run 33911514170 conclusion=success（checks×3 + pana）；windows leg 日志 C4996=0；ubuntu leg 输出含 'cpp residue 22+2'"
        status: pass
    human_judgment: false
  - id: D8
    description: "gtest 占位测试落位（新码区）：EXPECT_TRUE(true) 级 + DEV 注释说明 WM 类构造需活 registrar（上游因此无 native 测试）；保活机制给 Phase 3 纯函数测试"
    verification:
      - kind: other
        ref: "windows/test/window_frame_kit_plugin_test.cpp 落盘；window_frame_kit_test.vcxproj 构建绿（window_frame_kit_test.exe 产出）"
        status: pass
    human_judgment: false

# Metrics
duration: 55min（machine；wall ≈ 9h40m，含 ~8h 用户 checkpoint 等待与驳回恢复）
completed: 2026-09-05
status: complete
---

# Phase 2 Plan 02: Windows C++ 面 verbatim 移植 + 实机 tracer Summary

**window_manager 0.5.2 的 Windows C++ 全量面（597+1129 行）以"C-API 壳 + 匿名命名空间 verbatim 进驻 + 14 行桥接"落码，残留 diff 精确封顶 22+2 进 CI 持续门，example release 构建含 gtest 目标全绿；实机 tracer 经一轮证据驱动驳回恢复后 4 项判定全过（dated sign-off 2026-09-05），初始化协议 + 事件回路端到端成立，用户附带指令"移除系统托盘"已记账转 02-05**

## Performance

- **Duration:** 55 min machine（03:05–03:40 首段 + 12:05–12:20 恢复段 + 收口；wall 含用户等待）
- **Started:** 2026-09-05T03:05+08:00
- **Completed:** 2026-09-05（sign-off 后收口）
- **Tasks:** 3（T1 移植 / T2 存活+commit+CI / T3 tracer checkpoint）
- **Files modified:** 6（feat commit）+ 4（docs commit）

## Accomplishments

- **WM C++ 1726 行 verbatim 进驻：** plugin.cpp 597→605 行（3 适配：L1 include / L112 channel 串 / L592-597 extern-C 块→14 行桥接）；window_manager.cpp 1129 行仅 L1；匿名命名空间 112 行含 window_manager 标识符零改名；`#include "window_manager.cpp"`（plugin.cpp:15）原样 = 上游文件名保留理由成立
- **SC5 C++ 半边机器门落定：** verbatim_diff.sh B 段 22/2 精确断言，本地 + CI ubuntu leg（curl archive 对照）双绿；DEVIATIONS ②③④ 预记账行账实对齐（无需新增行）
- **Pattern 1 注册链通电：** 壳三件（plugin_c_api.cpp / c_api.h / pubspec pluginClass）零改动，桥接头 + 尾块转发 WindowManagerPlugin::RegisterWithRegistrar；无链接错误，exe 启动即事件发射
- **构建管线守护成立：** 双目标 codecvt silence define（Pitfall 5）+ 承重墙 policy 行 =1（grep 门）；windows leg C4996=0；gtest 占位保活 test.vcxproj
- **tracer 纵切收口（SC1 半边 + CAPB-01）：** 实机 4 项判定全过——窗口按 WindowOptions（800×600/居中/hidden 标题栏）出现、focus/blur/close 事件行、preventClose 开关 ON 后关闭弹对话框选 No 留存
- **CI 全绿：** run 33911514170 四 job success；commit f055eeb 已 push origin master

## Task Commits

1. **Task 1+2: C++ 面移植 + 存活断言** — `f055eeb` (feat) — 6 文件 +1742/−88（windows/ 五文件 + verbatim_diff.sh B 段，单原子 commit 按计划）
2. **Plan metadata** — 本 SUMMARY + STATE/ROADMAP/WINDOWS（docs commit，见 git log）

## Human Check（Task 3 tracer，dated sign-off）

**判定清单结果（round-2，2026-09-05）：4 项全 PASSED**

| # | 判定项 | 结果 |
|---|--------|------|
| 1 | 窗口出现且尺寸/位置符合 WindowOptions（800×600、居中） | ✅ PASS |
| 2 | 标题栏观感符合 titleBarStyle=hidden（无系统标题栏） | ✅ PASS |
| 3 | `[window_frame_kit] {时间戳} ...` 事件行：focus/blur 等 | ✅ PASS（可见控制台 + tee 日志双证） |
| 4 | 关闭流程：close 事件行 + preventClose 开关 ON 时弹确认对话框选 No 留存 | ✅ PASS（操作前提已修正：先拨 UI 开关） |

**用户签核原话（2026-09-05）：** "去掉系统托盘的功能 approved — 4 项全过"

**用户指令（verbatim 记录）：** "去掉系统托盘的功能" —— example 移除系统托盘。**执行指针：02-05**（该计划拥有 CAPB-11 穿透 UAT，tray 原为其恢复路径；需替代恢复路径候选：定时自动复位/快捷键；连带 ci.yml ubuntu leg libayatana-appindicator3-dev 依赖移除 + registrant 再生成同 commit）。已记 WINDOWS.md #7。本计划不实施（checkpoint 签核即收口，指令排期不扩散范围）。

## Decisions Made

- **桥接头 14 行（Example 4 精确形态）> frontmatter 估值 21 行**：probe 实证形态优先；验收门（≤25 行 + DEV + 声明串）全过
- **checkpoint 拉起形态升级**：detached exe → Start-Process powershell 可见控制台 + Tee-Object 日志——首轮驳回的核心教训是观察面缺失，不是代码缺陷
- **preventClose 语义修正**：example home.dart 的 `_isPreventClose` 默认 false（UI 开关，home.dart:40/:167-175），关闭对话框仅 ON 时弹（home.dart:1057-1083，逐字区=上游行为）；计划判定项 4 的"preventClose 上游默认路径"为规划期误读，按上游对等语义修正操作步骤后通过

## Deviations from Plan

**0 个代码偏差**——移植面/门禁/构建全部按计划与 RESEARCH 定案形态落码，无新增 DEVIATIONS.md 行（②③④ 预记账行账实对齐）。以下为过程偏差与恢复弧记录：

**1. [环境豁免] .planning 编排产物越界断言豁免**
- .planning/config.json(M)/state.json(??)/milestone.lock(??) 为 orchestrator 所有（rules 禁触）；T1 边界断言在计划的 `windows/|tool/` 之外追加排除 `\.planning/`；feat commit 未含任何 .planning 文件

**2. [估值差] 桥接头 14 行 vs frontmatter "21 行"；wm 标识符 112 vs 研究 "113 处"**
- 均以实证形态为准（Example 4 / 22 行 diff 封顶）；差值归因：21 为规划估值、113 含 L112 channel 串行本身（合法适配点）；验收门全过，无需记账

**3. [Rule 3 - Blocking] checkpoint 首轮驳回 → 证据驱动恢复（本计划最大过程弧）**
- **Found during:** Task 3 首轮 checkpoint（detached exe，无控制台）
- **Issue:** 用户判"全都没实现"（4 项全否）；orchestrator 初诊 exe stale
- **诊断（硬证据推翻 stale 论）:** ① exe/DLL mtime 03:24 > 源 mtime 03:15（构建在移植后、commit 前，顺序正确）；② rebuild 为纯 no-op（29.7s，mtime 不变，树 clean vs f055eeb）；③ round-1 日志 7002 行含 focus/blur/move×2306+/resize/close——全部仅由 WM native 处理器发射（plugin.cpp:206/226/230/319），模板 native 不可能产生；④ Dart 侧反证：waitUntilReadyToShow 首行 invokeMethod 对模板 native 必抛 MissingPluginException → show 事件不可能落日志，而它落了 = 全部 WindowOptions 调用 native 完成
- **症状归因:** 项 3 = 观察面缺失（detached exe 无控制台，事件行在日志里用户未见）；项 4 = preventClose 开关默认 OFF（上游对等行为，计划文本误读）；项 1/2 = 用户于 8h 后检查且曾拖动窗口 + DPI 物理像素误量嫌疑，round-2 新鲜窗口复判
- **Fix:** 拉起形态改可见控制台（Start-Process powershell -NoExit + Tee-Object）+ 判定项 4 操作步骤修正（先拨开关）+ 项 1/2 判读基准注明（逻辑像素/新鲜窗口）；零代码改动、零新 commit
- **Evidence:** round-1 日志留存 `example/build/tracer_round1_evidence.log`（gitignored 本地）；round-2 pid 16824 Responding=True + show 行 12:15:14
- **结果:** round-2 四项全过，approved

**4. [环境噪声] Bash 安全分类器间歇超时 ×4 + inline powershell $ 插值 parse error ×1**
- 重试即恢复（02-01 同款）；powershell 内联 `$p` 被 bash 吞 → 改 .ps1 文件执行（未启动任何孤儿进程）；无交付面影响

---

**Total deviations:** 代码 0 / 过程 4（1 环境豁免 + 1 估值差说明 + 1 blocking 恢复弧 + 1 环境噪声）
**Impact on plan:** 无范围蔓延；SC5 C++ 半边与 tracer 纵切零妥协交付。

## Issues Encountered

- checkpoint 观察面设计教训已固化为 pattern（可见控制台 + tee 日志），02-03/04/05 的实机 checkpoint 直接复用
- CI windows leg 的 googletest CMake deprecation 警告为 policy 补丁生效的预期噪声（非错误）

## User Setup Required

None — 用户指令（移除系统托盘）已记账转 02-05，无需外部配置。

## Next Phase Readiness

- **Ready for 02-03（批① UAT：初始化协议/几何/状态）：** native 事件回路已通电，窗口/事件基线在本 tracer 已立；Example 5 触发-事件对照表可直接驱动 UAT
- **02-05 携带用户指令：** 移除 example 系统托盘（WINDOWS.md #7），需先定 CAPB-11 穿透恢复路径替代（候选：定时自动复位/快捷键）
- **Phase 3 禁改区基线锁定：** 22/2 残留封顶 = NCCALCSIZE/GETMINMAXINFO/WM_NCHITTEST 任何嫁接触碰即红灯
- REQUIREMENTS.md CAPB-01/04/05/06 维持不勾（shared-ID gate，待 02-03/04/05 落齐由 ready-ids 放行）

## Self-Check: PASSED

- key-files 存在性：windows/ 五文件 + tool/verbatim_diff.sh 全部 `[ -f ]` 实测在盘；window_manager.cpp 1129 行、plugin.cpp 605 行（wc -l 复核）
- `git log --grep="02-02"` → f055eeb（feat）✓
- 全部 3 个 task 的 acceptance_criteria 复跑通过：T1（VERBATIM PROOF OK 22+2 / wm 计数 112≥100 / 桥接头 14≤25 含 DEV+声明 / PLUGIN_SOURCES 三源 / 壳三件+pubspec+linux+macos 零改动）、T2（存活断言 exit 0 + 清理 / commit 仅 6 文件 / CI windows leg C4996=0 / ubuntu leg 含 cpp residue 22+2）、T3（4 项判定全过 + dated sign-off + 指令记账）
- Plan 级 verification 复跑：analyze 0 + test +13 + verbatim_diff OK + example release 构建绿 + exe 存活 + CI 四 job success + tracer sign-off（本文件含 "approved" 字样，Task 3 automated gate 可过）

---
*Phase: 02-windows*
*Completed: 2026-09-05*
