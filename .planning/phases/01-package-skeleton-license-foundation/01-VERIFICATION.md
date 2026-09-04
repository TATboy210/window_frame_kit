---
phase: 01-package-skeleton-license-foundation
verified: 2026-09-04T13:46:18Z
status: passed
score: 5/5
mode: mvp
requirements: [PUB-01]
head: 1a56bb8
human_verification: none   # SC1 人工半边已于 01-03 checkpoint 由用户实机三项 approved（01-03-SUMMARY §Human-Check Result），无需新增人工项
---

# Phase 1 Verification: 包骨架与许可基座

**Goal**: 三平台插件骨架立起来，MIT 归属与上游合并路径就位——所有嫁接代码落盘之前，合规、可合并性与发布管线已经锁死。这是 Pitfall 1/2/10 的全部预防动作所在阶段。

**Verdict: PASSED — 5/5 success criteria verified against the real codebase**（本机全量重跑自动化门 + GitHub 远端 CI 状态实查；人工半边沿用 01-03 checkpoint 用户实机批准证据）。

验证环境：Windows 本机 @ HEAD `1a56bb8`（= origin/master），Flutter 3.47.2 / Dart 3.13.2 @ D:/flutter。所有命令于 2026-09-04T13:3x–13:46Z 新鲜复跑，非仅采信 SUMMARY 声明。

---

## SC1 — 骨架完整落盘 + example Windows 实机启动显示窗口 ✅ TRUE

**可自动化半边（本次全部新鲜复跑）：**

| 检查 | 命令 | 结果 |
|------|------|------|
| 三平台骨架 git 跟踪 | `git ls-files <dir> \| wc -l` | lib=3, windows=7, linux=5, macos=4, example=64, test=2（与 01-01-SUMMARY D1 一致） |
| 静态分析 | `D:/flutter/bin/flutter analyze` | `No issues found! (ran in 17.9s)` |
| 格式门 | `D:/flutter/bin/dart format --set-exit-if-changed --output=none .` | `Formatted 8 files (0 changed)`, exit 0 |
| 测试门 | `D:/flutter/bin/flutter test` | `All tests passed!` (+3) |
| Pitfall-1 补丁 | `grep -c "CMAKE_POLICY_VERSION_MINIMUM 3.5" windows/CMakeLists.txt` | 1（line 81，恰位于 `FetchContent_MakeAvailable(googletest)` 之前，含英文根因注释 :79-80） |
| release 构建产物 | `test -f example/build/windows/x64/runner/Release/window_frame_kit_example.exe` | 存在（gitignored 构建产物） |

**人工半边（01-03 checkpoint 已收集，证据在案）：** 用户 2026-09-04 实机目视三项 human-check 全部 approved——① 窗口出现且完整可见 ② 默认插件示例 UI 可见可交互 ③ 关闭按钮正常退出（01-03-SUMMARY §Human-Check Result，D2 coverage: manual_procedural pass, human_judgment: true）。宿主 UAT 证据标准（控制台无崩溃 ≠ UI 正确呈现）已满足。

**基线观察（非 gap）：** 用户批准时附带"快速拖拽左缘 resize 时右缘毫秒级瞬间空白"观察——归因为裸 Flutter 模板窗口路径（frame 接管 Phase 3 才落地），按用户裁决记为 Phase 3 基线观察，WINDOWS.md #4 挂账 open，Phase 3 复测。SC1 只要求模板 example 实机启动显示窗口，该观察不构成 Phase 1 缺口。

## SC2 — LICENSE 三行叠加 MIT + README Derivation 溯源节 ✅ TRUE

| 检查 | 命令 | 结果 |
|------|------|------|
| window_manager © 行逐字 | `grep -cx "Copyright (c) 2022-present LiJianying <lijy91@foxmail.com>" LICENSE` | 1（整行精确） |
| bitsdojo © 行逐字 | `grep -cx "Copyright (c) 2020-2021 Bogdan Hobeanu" LICENSE` | 1 |
| 本包 © 行第三 | `grep -cx "Copyright (c) 2026 TATboy210" LICENSE` | 1（D-02 行序） |
| MIT 正文 | `grep -c "Permission is hereby granted, free of charge" LICENSE` | 1 |
| 负债条款 | `tr '\n' ' ' < LICENSE \| grep -c "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE"` | 1（原文跨行折叠，归一化后命中——与 01-02-SUMMARY 记录一致） |
| 模板占位清零 | `! grep -Eq 'BSD' LICENSE` | PASS |
| 正文逐字性 | `diff <(tail -n +5 pub-cache/window_manager-0.5.2/LICENSE) <(tail -n +7 LICENSE)` | 唯一差异 = 上游缺 EOF 尾换行（POSIX 惯例，无法律/语义影响；01-02-SUMMARY 已如实记录） |
| Derivation 节 | `grep -c "## Derivation" README.md` | 1（恰好一次） |
| 两上游血统 | `grep -c leanflutter/window_manager` / `grep -c bitsdojo` README.md | 1 / 3（各含 MIT 归属：© LiJianying、© Bogdan Hobeanu） |
| 账本指路 | `grep -c "DEVIATIONS.md" README.md` | 1 |
| 三平台声明 | `grep -c Windows/Linux/macOS README.md` | 2/2/2 + "Desktop only — no mobile or web targets"（模板 Android/iOS 错误措辞已清除，WINDOWS.md #1 fixed） |

**Pitfall 2 时间条款：** LICENSE stack（ecd8113, 2026-09-04）先于任何嫁接代码落盘——git 历史中本 phase 全部 commit 均为模板骨架/文档/CI，零上游移植码。合规基座在 Phase 2/3 嫁接前锁死 ✅。

## SC3 — upstream remote + DEVIATIONS.md 偏差账本 ✅ TRUE

| 检查 | 命令 | 结果 |
|------|------|------|
| upstream remote | `git remote get-url upstream` | `https://github.com/leanflutter/window_manager`（D-09，window_manager 上游仓库） |
| 账本存在 + 格式 | `grep -c "file:line" DEVIATIONS.md` | 2（六列表格含 "Upstream anchor (file:line)" 列 + 记账规则第 2 条） |
| 偏离理由列 | 目视核对 DEVIATIONS.md:13 | 表头六列齐备：# / Upstream anchor (file:line) / This package / Deviation / Rationale / Marker |
| // FRAME: 标记对应 | `grep -c "FRAME" DEVIATIONS.md` | 3（记账规则第 1 条：每标记恰一行账目） |
| 记账规则 + 示例行 | 目视核对 | 三条规则在位；一条 Phase-3 待回填示例行（bitsdojo_window.cpp:550-568 GETMINMAXINFO 协作式合并）——计划明确要求的占位（WINDOWS.md #3 open，Phase 3 嫁接落码时替换，非缺陷） |

## SC4 — 依赖白名单 + channel 名 window_frame_kit ✅ TRUE

| 检查 | 命令 | 结果 |
|------|------|------|
| 直接依赖白名单 | 读 pubspec.yaml:27-34 | 恰三项：`path ^1.9.1`、`screen_retriever ^0.2.2`、`plugin_platform_interface ^2.0.2`（+ flutter sdk 框架引用）。plugin_platform_interface 裁量已记录：pubspec 行内注释"模板 Dart 平台接口层（非 win32）；Phase 2 移植后复核删除"（研究 Open Question 2 的裁量执行） |
| 零 win32（锁文件） | `! grep -Eq 'win32' pubspec.lock` | PASS |
| 零 win32（pubspec 依赖键） | `! grep -Eq '^\s+win32' pubspec.yaml` | PASS |
| channel Dart 侧 | `grep -c "MethodChannel('window_frame_kit')" lib/window_frame_kit_method_channel.dart` | 1 |
| channel 三原生侧 | `grep -c window_frame_kit` windows/.cpp / linux/.cc / macos/.swift | 4 / 14 / 1（四处一致，包名自动派生） |
| channel 红线（绝不复用 window_manager） | `! grep -rn "MethodChannel('window_manager')" lib windows linux macos` | PASS（零命中） |
| D-01 身份三字段 | `grep -c "TATboy210/window_frame_kit" pubspec.yaml` | 4（≥3：homepage/repository/issue_tracker 同指） |
| D-04 topics | 读 pubspec.yaml:15-20 | 恰 5 值，D-04 原文顺序：desktop/window/window-manager/frameless/custom-titlebar |
| podspec 占位清零 | `! grep -Eiq 'Your Company\|example\.com' macos/window_frame_kit.podspec` | PASS（真实 summary/homepage/author） |

## SC5 — pana + dry-run 骨架期跑通并接入 CI ✅ TRUE

**本机半边（新鲜复跑）：**

| 检查 | 命令 | 结果 |
|------|------|------|
| 发布门 | `D:/flutter/bin/dart pub publish --dry-run` | **`Package has 0 warnings.`**（2026-09-04T13:4xZ 复跑；树中 .planning 编排器工件不影响包内容判定） |

**CI 半边（ci.yml 结构 + GitHub 远端实查，gh 显式 `-R TATboy210/window_frame_kit`）：**

| 检查 | 命令 | 结果 |
|------|------|------|
| 三平台矩阵 | plan-04 六模式 grep 门（windows/ubuntu/macos-latest、exit-code-threshold、pull_request、flutter-version "3.47.2"） | `WORKFLOW_OK`（全命中；fail-fast: false @ L17） |
| 每平台四项检查 | 读 ci.yml | pub get → analyze → format --set-exit-if-changed → test → 按平台条件 native build（working-directory: example，上游 WM build.yml 同款） |
| pana 门 | `grep -Eo 'exit-code-threshold [0-9]+' ci.yml` | `exit-code-threshold 10` = 满分 160 − 首跑基线 150（D-08 公式钉死，YAML 注释记录基线/日期/run id/Phase 5 重钉提醒 @ L69-73） |
| 触发条件 | 读 ci.yml:9-12 | push[master] + pull_request 不限分支（D-06） |
| 无 secrets/publish/melos | 精确负向 grep（排除注释行） | PASS（唯一命中为 L6 的"明确不含"声明注释本身） |
| CI 实跑绿 | `gh run list -R TATboy210/window_frame_kit --workflow ci --limit 5 --json conclusion,headSha` | 3 runs 全 `success`；最新 run 33877681806 headSha=`1a56bb8` == 本地 HEAD == origin/master |
| D-01 仓库在线 | `git remote get-url origin` + `git rev-parse origin/master` | `https://github.com/TATboy210/window_frame_kit.git`；origin/master = `1a56bb8`（master 已推送且与本地同步） |

pana 基线 150/160 的唯一扣分项 = pubspec description 超 180 字符建议长度（0/10）——D-03 锁定措辞骨架期保留，WINDOWS.md #5 挂账 Phase 5（PUB-04 精简后重钉 threshold）。这是计划内裁量，非 gap。

---

## Requirement Traceability: PUB-01 ✅ Complete

- REQUIREMENTS.md:42 → `- [x] **PUB-01**`；Traceability 表 → `PUB-01 | Phase 1 | Complete`
- 四个 PLAN frontmatter `requirements:` 全部声明 PUB-01（01-01/02/03/04），shared-ID gate 按规程在最后一个声明计划（01-04）完成后放行勾选——四个 SUMMARY 的 `requirements-completed: [PUB-01]` 与之一致
- 映射说明成立：PUB-01 → Phase 1（Pitfall 2 时间条款，先于首个嫁接 commit）；Phase 5 发布门按计划复核归属
- 无孤儿需求：Phase 1 仅映射 PUB-01，无遗漏 ID

## Plan-Level Must-Haves Cross-Reference

| Plan | must_haves.truths | 验证结果 |
|------|-------------------|----------|
| 01-01 | 骨架四门全绿 / channel 四处一致 / dry-run 0 warnings / 白名单零 win32 | 4/4 ✅（本次全部新鲜复跑） |
| 01-02 | LICENSE 三行逐字 / README Derivation / upstream remote / DEVIATIONS.md 格式 | 4/4 ✅（含正文 diff 逐字实证） |
| 01-03 | release exe 产出 / example 实机显示窗口 / CMake 4.x configure 通过 | 3/3 ✅（exe 在盘 + 用户三项 approved + 补丁 line 81 在位；CI windows leg 同镜像复证 A2） |
| 01-04 | 三平台 CI 触发 / D-01 仓库在线 / 首跑全绿 + threshold 钉死 | 3/3 ✅（3 runs success @ HEAD + threshold 10 + origin==HEAD） |

Task commits 全部在 git 历史核验：`2d0d408` `064fdb0` `ecd8113` `eb99e1c` `a0bead7` `ee91a58` `57bd97d` `4bb91a3` `1a56bb8`（与四份 SUMMARY 声明逐一对上，无 amend/回退痕迹）。

## Deferred Items（计划内后置，非 Phase 1 gap）

| 项 | 载体 | 处置阶段 |
|----|------|----------|
| DEVIATIONS.md 示例行占位（"TBD (Phase 3)"——唯一的债务标记，账本化管理） | WINDOWS.md #3 (open) | Phase 3 嫁接落码时替换为真实条目 |
| resize 右缘毫秒级空白基线观察（裸模板现象，根因未诊断） | WINDOWS.md #4 (open) + 01-03-SUMMARY §Baseline Observation | Phase 3 frame 接管后复测 |
| pana 150/160（description 超长扣分）→ threshold 重钉 | WINDOWS.md #5 (open) | Phase 5 PUB-04 |
| README 完整 API 文档 + 双包迁移映射表 | 01-02 计划边界明示 | Phase 5 (PUB-02) |
| plugin_platform_interface 白名单复核删除 | pubspec 行内注释 | Phase 2 |
| environment.sdk floor 最终锁定（现 ^3.13.2 保守值 + 注释） | pubspec 注释 | Phase 4 (XPLAT-01) |
| CMake 补丁不可删条款（A2 已证 CI runner 同为 CMake 4.x） | 01-04-SUMMARY key-decisions | 长期约束 |

## Anti-Patterns / Notes

- 生产文件中零未挂账债务标记：唯一 TBD 位于 DEVIATIONS.md 账本占位行，WINDOWS.md #3 正式跟踪（引用了跟进阶段）——合规。
- 01-01-SUMMARY 记录的 dry-run 顺序偏差（先 commit 后跑门）为工具语义发现（pub 的 git-status 检查），已如实记录且本次复跑独立证实 0 warnings。
- 01-04 两处 Rule 3 blocking 偏差（native build working-directory、gh 双 remote 解析）均为执行期排雷，产物验证全绿，patterns-established 已固化 `-R` 纪律。

## Conclusion

**STATUS: PASSED — 5/5 success criteria, 14/14 plan-level truths, PUB-01 Complete。**

Pitfall 1（CMake/gtest 阻断——补丁在位且本机+CI 双证）、Pitfall 2（LICENSE 时间条款——合规基座先于一切嫁接代码）、Pitfall 10（发布分数失守——pana threshold 钉死 + dry-run 0 warnings + CI 三 run 全绿）的全部预防动作已在本阶段锁死。Phase 2（WM 0.5.2 移植）起跑即受三平台 CI 守护，无阻断项。

---
*Verified: 2026-09-04T13:46:18Z @ HEAD 1a56bb8*
*Verifier: independent re-run of all automated gates + GitHub remote state query (gh -R pinned)*
