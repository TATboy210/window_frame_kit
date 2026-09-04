---
phase: 01-package-skeleton-license-foundation
plan: 01
subsystem: infra
tags: [flutter-plugin, scaffold, pub-dev, method-channel, pubspec, desktop, cmake]

# Dependency graph
requires: []
provides:
  - 三平台 Flutter 插件骨架全量落盘（91 文件：lib 三件套 / windows / linux / macos / example 宿主 / test 双测）
  - MethodChannel 名 window_frame_kit 四处一致锚点（Dart lib/window_frame_kit_method_channel.dart:10、Windows plugin.cpp:23、Linux plugin.cc:69、macOS WindowFrameKitPlugin.swift:6）
  - pubspec 身份五组字段（D-01 homepage/repository/issue_tracker、D-03 description、D-04 topics×5）+ sdk ^3.13.2 带 Phase 4 复核注释
  - 依赖白名单（path ^1.9.1 / screen_retriever ^0.2.2 / plugin_platform_interface ^2.0.2）与零 win32 证据
  - dart pub publish --dry-run "Package has 0 warnings." 本机发布门
  - 骨架期质量门基线：analyze 0 issue / format exit 0 / 3 tests 全过（Plan 03/04 的 CI 与实机验证载体）
affects: [01-02-license-readme, 01-03-build-gates, 01-04-ci-first-run, phase-2-wm-port]

# Actuals (#2632) — pairs with the plan's `estimate` to calibrate future estimates.
actuals:
  tokens: 47846    # chars/4 over realized diff (191384 bytes across 92 files)
  tasks: 2
  commits: 2

# Tech tracking
tech-stack:
  added:
    - "flutter create --template=plugin --platforms=windows,linux,macos (Flutter 3.47.2 stable @ D:/flutter, Dart 3.13.2)"
    - "path ^1.9.1 (whitelist)"
    - "screen_retriever ^0.2.2 (whitelist; brings screen_retriever_windows/linux/macos into example registrants)"
    - "plugin_platform_interface ^2.0.2 (template platform interface layer, non-win32)"
    - "flutter_lints ^6.0.0 (template dev-dependency)"
  patterns:
    - "Channel 名由包名自动派生（研究 Pattern 1）——改包名即断四处，骨架期零手改"
    - "依赖白名单硬门：直接依赖仅三项 + `! grep -Eq 'win32' pubspec.lock` 留证据（XPLAT-02）"
    - "pubspec 身份五字段一次补齐 → dry-run 0 warnings（研究 Example 2 已验证形态）"
    - "Windows 插件 C-API 壳模式（WindowFrameKitPluginCApi 薄壳 + 内层 WindowFrameKitPlugin，Phase 2 移植结构约束）"

key-files:
  created:
    - "pubspec.yaml（模板 → 身份五字段 + 白名单，Task 2 定稿）"
    - "lib/window_frame_kit.dart / lib/window_frame_kit_platform_interface.dart / lib/window_frame_kit_method_channel.dart"
    - "test/window_frame_kit_test.dart / test/window_frame_kit_method_channel_test.dart"
    - "windows/（CMakeLists.txt + plugin.cpp/.h + plugin_c_api.cpp + include/ + test/）"
    - "linux/（CMakeLists.txt + plugin.cc + plugin_private.h + include/ + test/）"
    - "macos/（window_frame_kit.podspec（Task 2 清占位） + Package.swift + Sources/）"
    - "example/（三平台可运行宿主，publish_to 'none' + path 依赖 ../）"
    - ".gitignore（含 /pubspec.lock、.dart_tool/）/ .metadata / analysis_options.yaml"
    - "CHANGELOG.md / README.md / LICENSE（模板版——README/LICENSE 由 Plan 02 整体替换）"
  modified: []

key-decisions:
  - "依赖白名单按意图执行（研究 Open Question 2 裁量）：plugin_platform_interface 作为模板平台接口层计入白名单（非 win32），pubspec 内注释声明 Phase 2 移植后复核删除"
  - "environment.sdk 保留模板 ^3.13.2 保守值 + 'Phase 4 按依赖链复核放宽' 注释（研究 Pitfall 8——与验证环境一致、零虚报）"
  - "dry-run 0-warnings 门要求先 commit 再跑：pub 的 git-status 检查会把未提交的 pubspec/podspec/生成件计为 1 warning（研究中 dry-run 语义边界描述未覆盖此点，本机实测补全）"

patterns-established:
  - "scaffold 后立即 dart format .（Pitfall 5：模板 3 文件与 formatter 漂移，实测命中 3/8）"
  - "改依赖后 flutter pub get 会再生成 example 三平台 registrant（screen_retriever 注册），必须与 pubspec 同 commit 落盘，否则 dry-run 报 dirty-tree warning"
  - "白名单证据双 grep：pubspec.lock 零 win32 + pubspec.yaml 依赖键行零 win32"

requirements-completed: [PUB-01]  # 复制自 PLAN frontmatter（PUB-01 为本 phase 需求）；实际 LICENSE 交付在 Plan 02——shared-ID gate 阻止本计划先行勾选

# Coverage metadata (#1602)
coverage:
  - id: D1
    description: 三平台插件骨架完整落盘且 git 已跟踪（lib/windows/linux/macos/example/test 目录齐全）
    verification:
      - kind: other
        ref: "git ls-files: lib=3, windows=7, linux=5, macos=4, example=64, test=2"
        status: pass
    human_judgment: false
  - id: D2
    description: MethodChannel 名 window_frame_kit 在 Dart/Windows/Linux/macOS 四处一致
    verification:
      - kind: other
        ref: "grep -c 'window_frame_kit' lib/window_frame_kit_method_channel.dart=1, windows/window_frame_kit_plugin.cpp=4, linux/window_frame_kit_plugin.cc=14, macos/.../WindowFrameKitPlugin.swift=1"
        status: pass
    human_judgment: false
  - id: D3
    description: 骨架期质量门三门全绿（analyze 0 issue / format exit 0 / flutter test 全过）
    verification:
      - kind: unit
        ref: "D:/flutter/bin/flutter analyze → No issues found!"
        status: pass
      - kind: other
        ref: "D:/flutter/bin/dart format --set-exit-if-changed --output=none . → exit 0 (0 changed)"
        status: pass
      - kind: unit
        ref: "D:/flutter/bin/flutter test → All tests passed! (+3)"
        status: pass
    human_judgment: false
  - id: D4
    description: pubspec 身份五组字段就位（D-01 三 URL 字段 + D-03 description + D-04 topics 恰 5 值）
    verification:
      - kind: other
        ref: "grep -c 'TATboy210/window_frame_kit' pubspec.yaml = 4 (>=3)"
        status: pass
      - kind: other
        ref: "sed topics block → desktop/window/window-manager/frameless/custom-titlebar（D-04 原文顺序）"
        status: pass
    human_judgment: false
  - id: D5
    description: 依赖白名单成立——直接依赖仅 path/screen_retriever/plugin_platform_interface，零 win32
    verification:
      - kind: other
        ref: "! grep -Eq 'win32' pubspec.lock → PASS"
        status: pass
      - kind: other
        ref: "! grep -Eq '^\\s+win32' pubspec.yaml → PASS"
        status: pass
    human_judgment: false
  - id: D6
    description: dart pub publish --dry-run 输出 Package has 0 warnings.（SC5 本机半边）
    verification:
      - kind: other
        ref: "D:/flutter/bin/dart pub publish --dry-run → 'Package has 0 warnings.'"
        status: pass
    human_judgment: false
  - id: D7
    description: macOS podspec 占位符清零（summary/description/homepage/author 真实值）
    verification:
      - kind: other
        ref: "grep -E 'Your Company|example.com' macos/window_frame_kit.podspec → 无命中"
        status: pass
    human_judgment: false

# Metrics
duration: 22 min
completed: 2026-09-04
status: complete
---

# Phase 1 Plan 01: 三平台插件骨架落地 + 骨架期质量门全绿 Summary

**三平台 Flutter 插件骨架（91 文件）一次落盘，channel 名 window_frame_kit 四处自动派生一致，pubspec 身份五字段 + 三项依赖白名单使 `dart pub publish --dry-run` 直达 0 warnings，analyze/format/test 三门全绿。**

## Performance

- **Duration:** 22 min
- **Started:** 2026-09-04T09:34:13Z
- **Completed:** 2026-09-04T09:56:35Z
- **Tasks:** 2
- **Files modified:** 92（净新增；Task 1 92 文件 + Task 2 在其中 8 文件上定稿）

## Accomplishments

- Task 1（tracer）：`flutter create --template=plugin --platforms=windows,linux,macos --org com.example .` 在仓库根生成 91 文件全量骨架；随即 `dart format .`（实测 3 文件漂移被修复，与研究 Pitfall 5 预测完全一致）；`flutter pub get` 生成 pubspec.lock（根级 gitignored）。channel 名四处锚点逐一验证命中（行号与研究记录一致：Dart:10 / Win:23 / Linux:69 / macOS:6）。tracer 门全链复跑通过后进入扩展。
- Task 2：pubspec 一次补齐 description（D-03 研究验证措辞）/homepage/repository/issue_tracker（D-01 三字段同指 TATboy210/window_frame_kit）/topics×5（D-04 原文顺序）；依赖白名单三项落盘并注释声明白名单条款；sdk ^3.13.2 保守保留 + Phase 4 复核注释（Pitfall 8）；macOS podspec 占位符清零（真实 summary/homepage/author，git config 取值）；`dart pub publish --dry-run` 达成 **Package has 0 warnings.**（SC5 本机半边）。

## Task Commits

Each task was committed atomically:

1. **Task 1: 三平台插件骨架落地 + 骨架期质量门全绿** - `2d0d408` (feat)
2. **Task 2: pubspec 身份字段 + 依赖白名单 + dry-run 0 warnings** - `064fdb0` (feat；含 pub get 再生成的 example registrant 与 example/pubspec.lock)

## Files Created/Modified

- `pubspec.yaml` - 包身份五组字段 + 依赖白名单三项 + sdk 保守值注释（Task 2 定稿）
- `lib/window_frame_kit_method_channel.dart` - `MethodChannel('window_frame_kit')` 常量（channel 红线 Dart 侧锚点，:10）
- `lib/window_frame_kit.dart` / `lib/window_frame_kit_platform_interface.dart` - Dart facade 与平台接口层（模板三件套，Phase 2 由 WM facade 替换）
- `windows/window_frame_kit_plugin.cpp` - Windows 原生壳，channel 锚点 :23；C-API 壳模式（plugin_c_api.cpp → 内层类注册）
- `linux/window_frame_kit_plugin.cc` - Linux/GTK 原生壳，channel 锚点 :69
- `macos/window_frame_kit/Sources/window_frame_kit/WindowFrameKitPlugin.swift` - macOS 原生壳，channel 锚点 :6；SwiftPM 布局保留（pana 加分项）
- `macos/window_frame_kit.podspec` - summary/description/homepage/author 占位符全部替换为真实值
- `example/` - 三平台可运行宿主（Plan 03 实机启动验证与 CI native build 载体；registrant 三件由 pub get 再生成登记 screen_retriever）
- `test/` ×2 - 模板 Dart 测试（骨架期全绿基线）
- `.gitignore` / `.metadata` / `analysis_options.yaml` / `CHANGELOG.md` / `README.md` / `LICENSE` - 模板生成（README/LICENSE 待 Plan 02 整体替换）

## Decisions Made

- **plugin_platform_interface 计入白名单**（研究 Open Question 2 的裁量执行）：白名单意图是零 win32，模板平台接口层不算违规；pubspec 行内注释固化此判断与 Phase 2 复核点。
- **sdk floor 保留 ^3.13.2**：骨架期与验证环境一致、零虚报，Phase 4/XPLAT-01 按依赖链锁定最终 floor（pubspec 注释已标注）。
- **dry-run 前置 commit**：pub 的 git-status 检查把未提交修改计为 warning（见 Deviations），因此 Task 2 的验证顺序调整为"先 commit 后跑门"。

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] pub dry-run 的 git-status 检查在未提交状态下报 1 warning，阻断 0-warnings 门**
- **Found during:** Task 2（dry-run 首跑）
- **Issue:** 补齐四字段后 dry-run 仍报 "Package has 1 warning."——pub 检查到 pubspec.yaml、macos/window_frame_kit.podspec、example 生成件未提交（"Usually you want to publish from a clean git state"）。研究 §Code Examples 的 0-warnings 实测是在干净树上跑的，此前置条件未被写入计划。
- **Fix:** 调整验证顺序——先 commit Task 2 文件（连同 `flutter pub get` 因新增 screen_retriever 再生成的 5 个 example registrant 文件与 example/pubspec.lock，经 diff 检视均为合法的 screen_retriever 三平台注册），再跑 dry-run → 0 warnings。因同一任务逻辑变更，采用 amend 将生成件并入 Task 2 commit（无 remote，操作安全）。
- **Files modified:** example/linux/flutter/generated_plugin_registrant.cc、example/linux/flutter/generated_plugins.cmake、example/macos/Flutter/GeneratedPluginRegistrant.swift、example/windows/flutter/generated_plugin_registrant.cc、example/pubspec.lock（并入 `064fdb0`）
- **Verification:** dry-run 输出 "Package has 0 warnings."
- **Committed in:** 064fdb0（Task 2 commit，amend）

---

**Total deviations:** 1 auto-fixed（1 blocking）
**Impact on plan:** 验证顺序微调，无范围蠕变；研究的 0-warnings 形态本体完全成立。

## Issues Encountered

- `flutter pub get` 在新增 screen_retriever 后再生成 example 三平台 plugin registrant——属工具链预期行为，已并入 Task 2 commit（这也是后续 Phase 每次改依赖都要随 commit 带上生成件的惯例来源）。
- 根级 pubspec.lock 正确 gitignored（.gitignore:28 `/pubspec.lock`），example/pubspec.lock 按模板惯例提交（example 是 app）。

## Known Stubs

以下为计划内有意保留的模板占位（非缺陷），由后续计划解决：

| Stub | File | Reason | Resolved by |
|------|------|--------|-------------|
| README.md 为模板版（含 Android/iOS 错误措辞，Pitfall 7） | README.md | 计划明确"本计划生成但不编辑内容" | Plan 02 整体重写 |
| LICENSE 为模板 BSD-3 风格占位 | LICENSE | 同上；PUB-01 三行叠加 MIT 属 Plan 02 | Plan 02 整体替换（三段式叠加，研究 Example 1） |
| README/example 完整度 pana 分项骨架期不满分 | example/ | Phase 5 才强化 example（D-08：pana 设通过线而非满分级） | Plan 04 CI threshold / Phase 5 |

## User Setup Required

None - no external service configuration required.（D-01 GitHub 仓库创建与 upstream remote 属 Plan 02/03 范围，届时如需 gh 登录会出现 auth gate。）

## Next Phase Readiness

- **Ready for 01-02**（LICENSE 三行叠加 + README 重写）：骨架与 pubspec 身份已就位，LICENSE 替换后即满足"首个嫁接 commit 前 PITFALL-2 红线"。
- 01-03（构建门/实机）将消费本计划预埋的事实：VS 2026 捆绑 CMake 4.3.1 撞模板 gtest 1.11（Pitfall 1，`windows/CMakeLists.txt` 待补一行 policy）+ 失败 configure 须 `rm -rf example/build`（Pitfall 2）+ 预建 `example/build/native_assets/windows/`（Pitfall 3）。
- 无阻塞项；质量门基线（analyze 0 / format 0 / 3 tests）已是 Plan 04 CI 工作流的预期绿线。

## Self-Check: PASSED

- 10/10 key files exist on disk（pubspec、lib 三件套、test 双测、podspec、example 入口、Win/Linux 原生壳）
- 2/2 task commits exist in git history（2d0d408 scaffold、064fdb0 pubspec identity+whitelist）
- 计划级 <verification> 全项复跑通过：analyze 0 issue / format exit 0 / test 全过 / dry-run 0 warnings / channel 四处 grep 命中 / pubspec.lock 零 win32 / git ls-files 骨架可见

---
*Phase: 01-package-skeleton-license-foundation*
*Completed: 2026-09-04*
