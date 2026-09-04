# Roadmap: window_frame_kit

## Overview

window_frame_kit 用 6 个阶段把"双包并用的生态空白"变成 pub.dev 上站得住的单包：先立骨架与 MIT 归属基座（保证嫁接代码落盘前合规与上游合并路径已锁死），再逐字移植 window_manager 0.5.2 的完整能力面并在实机验证对等（嫁接必须落在已被证明与上游一致的地基上），然后把 bitsdojo 的 frame 接管代码作为一个不可拆分的单元嫁接进去（无边框 + 四边缩放 + min/max 生效 + C++-only 零 win32 依赖），同步补齐 Linux/macOS 最小可用分支，最后以 0.1.0 发布占名、以宿主项目 simple_player_flutter 双包替换完成 Validated 验收打开 1.0.0 路径。每个阶段都含实机验证项——无头 CI 无法验证视觉窗口行为，宿主项目的 UAT 证据标准全程适用（控制台无崩溃 ≠ UI 正确呈现）。

## Phases

**Phase Numbering:**

- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

- [ ] **Phase 1: 包骨架与许可基座** - 三平台插件骨架 + MIT 双上游归属 + 上游合并路径，嫁接前合规锁死
- [ ] **Phase 2: Windows 基座移植** - window_manager 0.5.2 逐字移植，实机行为对等验证（嫁接前置门）
- [ ] **Phase 3: Windows Frame 嫁接** - FrameController 单元嫁接：无边框 + 四边缩放 + 协作式 GETMINMAXINFO + 零 win32 依赖
- [ ] **Phase 4: Linux + macOS 移植** - 三平台同步 v1：GTK/AppKit 分支同名 API 全通 + 实机冒烟
- [ ] **Phase 5: Example 强化 + 发布 0.1.0** - example 成为三平台验证载体，0.1.0 上架 pub.dev
- [ ] **Phase 6: 宿主集成验收** - simple_player_flutter 双包替换，Validated 验收线，打开 1.0.0 路径

## Phase Details

### Phase 1: 包骨架与许可基座

**Goal**: 三平台插件骨架立起来，MIT 归属与上游合并路径就位——所有嫁接代码落盘之前，合规、可合并性与发布管线已经锁死。这是 Pitfall 1/2/10 的全部预防动作所在阶段。
**Mode:** mvp
**Depends on**: Nothing (first phase)
**Requirements**: PUB-01
**Success Criteria** (what must be TRUE):

  1. `flutter create --template=plugin --platforms=windows,linux,macos` 骨架完整落盘，example 在 Windows 实机启动并显示窗口（实机验证项）
  2. LICENSE 文件逐字保留 window_manager 与 bitsdojo_window 两行上游版权声明，并附本包修改版权行；README 含 Derivation 溯源节（pub.dev 不验证，须自查）
  3. `git remote -v` 显示 upstream remote（window_manager 仓库）；DEVIATIONS.md 存在且定义好偏差条目格式（上游 file:line 锚点 + 偏离理由）
  4. pubspec 依赖仅 path + screen_retriever（C++-only 零 win32 依赖决策从骨架期生效）；channel 名为 `window_frame_kit`（绝不复用 `window_manager`）
  5. pana 与 `dart pub publish --dry-run` 在骨架期即可跑通，并接入 CI（发布分数从第一天开始守护）

**Plans**: 4/4 plans executed

Plans:

- [x] 01-01-PLAN.md — 插件骨架落地 + 包身份与依赖白名单 + 发布门首绿（tracer 纵切）
- [x] 01-02-PLAN.md — MIT 双上游归属 LICENSE + README Derivation + DEVIATIONS.md 账本 + upstream remote（PUB-01）
- [x] 01-03-PLAN.md — Windows 构建门（CMake Pitfall-1 补丁 + release 构建）+ example 实机启动确认（SC1）
- [x] 01-04-PLAN.md — CI 三平台矩阵（D-05~08）+ GitHub 建仓推送（D-01）+ pana 基线钉死（SC5）

### Phase 2: Windows 基座移植

**Goal**: window_manager 0.5.2 的完整能力面（约 60 方法 + 15 事件名 + WindowListener）在 Windows 上逐字移植并通过实机行为对等验证。嫁接必须落在一块已被证明与上游一致的地基上——否则移植回归与嫁接回归无法区分。**退出门：对等验证通过之前不得开始任何嫁接。**
**Mode:** mvp
**Depends on**: Phase 1
**Requirements**: CAPB-01, CAPB-02, CAPB-03, CAPB-04, CAPB-05, CAPB-06, CAPB-07, CAPB-08, CAPB-09, CAPB-10, CAPB-11, CAPB-12
**Success Criteria** (what must be TRUE):

  1. example 实机：ensureInitialized + waitUntilReadyToShow(WindowOptions) 后窗口按参数出现；几何读写（getSize/setSize/setPosition/center 等）即时生效且逻辑像素换算正确（实机验证项）
  2. example 实机：maximize/unmaximize/minimize/restore/setFullScreen 状态操作与 isFullScreen/isMaximized/isMinimized/isFocused 等查询一一对应；setTitleBarStyle(hidden) 与 getTitleBarHeight 生效（实机验证项）
  3. 实机：11 个 WindowListener hook（close/focus/blur/maximize/unmaximize/minimize/restore/resize/move/enter-full-screen/leave-full-screen）在对应窗口操作时全部触发（实机验证项）
  4. 实机：setPreventClose(true) 后点关闭按钮窗口不消失，确认路径显式 destroy 成功；popUpWindowMenu 在 frameless 下可唤出系统菜单（实机验证项）
  5. 移植代码与上游 0.5.2 逐字 diff 可证一致；channel 契约测试覆盖方法参数/返回形状与 15 个事件名常量（与上游逐字同名）

**Plans**: TBD

### Phase 3: Windows Frame 嫁接

**Goal**: bitsdojo 的 frame 接管能力以独立 FrameController 形态嫁接进插件，作为**一个不可拆分的单元**落地：无边框 NCCALCSIZE 路线 + 四边原生缩放 + 协作式 GETMINMAXINFO（根治架空 setMinimumSize 的上游缺陷）+ 零 win32 Dart 依赖架构。Snap Layouts（HTMAXBUTTON）与可配置边缘宽度为 v2（DIFF-01/DIFF-02），不在本 roadmap。
**Mode:** mvp
**Depends on**: Phase 2（门：基座对等已验证）
**Requirements**: FRAME-01, FRAME-02, FRAME-03, FRAME-04, XPLAT-02, XPLAT-03
**Success Criteria** (what must be TRUE):

  1. 实机（Win11）：开启 custom frame 后冷启动首启即无边框——无系统主题色边框、无 Win11 8px 顶部 inset；hot restart 后同样干净（实机验证项）
  2. 实机：四边 + 四角拖拽缩放全部可用，光标在边缘正确变形，100% 与 150% DPI 下边缘命中一致（per-monitor DPI：GetSystemMetricsForDpi/GetDpiForWindow）（实机验证项）
  3. 实机：setMinimumSize 真正生效（拖拽低于最小值被挡住，不再被 GETMINMAXINFO 无条件 return 0 架空）；maximize 后内容不裁切不越界（rcWork/work-area clamp，多显示器不串屏）（实机验证项）
  4. 拖拽区域行为 widget 可用：GestureDetector 包裹区域拖动窗口，与 HTCAPTION 原生路径行为一致（含双击最大化）（实机验证项）
  5. pubspec 与 import 图零 win32 依赖（C++-only 架构落地，下游 win32 6.x/file_picker 12/secure_storage 11 解锁成立）；FrameController 为独立文件，所有嫁接行带 `// FRAME:` 标记并记入 DEVIATIONS.md

**Research**: 建议 plan 阶段先跑 phase-level research——Win32 消息级手术；子类化安装时序（ensureInitialized 时 FLUTTERVIEW 是否已存在，回退 CBT hook/WM_CREATE 监听）明确标注"实现时验证"
**Plans**: TBD

### Phase 4: Linux + macOS 移植

**Goal**: 三平台同步 v1 达成——Linux GTK 与 macOS AppKit 分支与 Windows 同名 API 全通并实机冒烟，SDK floor 按依赖链锁定，平台差异以 feature matrix 可见化而非假装对称。本阶段与 Phase 3 零代码依赖（不碰 Windows frame），可并行。
**Mode:** mvp
**Depends on**: Phase 2（移植 Dart facade + listener；可与 Phase 3 并行）
**Requirements**: XPLAT-01, XPLAT-04, XPLAT-05
**Success Criteria** (what must be TRUE):

  1. macOS 实机冒烟：styleMask.fullSizeContentView + titlebarAppearsTransparent 生效（标题栏隐藏观感），窗口可获焦可交互（canBecomeKeyWindow/canBecomeMainWindow 覆盖到位，borderless 不假死）（实机验证项）
  2. Linux 实机冒烟（X11 与 Wayland 各一次）：gtk_window_set_decorated(false) 路线无边框；可拖动可缩放，Wayland CSD 缺原生 resize 时 widget 兜底可用（实机验证项）
  3. 三平台 feature matrix 写入 README（每方法 × 每平台一格）；未实现/降级能力返回 typed unsupported 错误而非静默 true（silent-true sweep 通过）
  4. SDK floor 按实际依赖链确定并锁定进 pubspec，三平台分支在 Flutter 3.47 / Dart 3.13 下 `flutter analyze` 0 error、native 编译通过

**Research**: 建议 plan 阶段关注 Wayland CSD/SSD resize 缺口（合成器差异决定 widget 兜底范围）与 macOS key-window 子类细节
**Plans**: TBD

### Phase 5: Example 强化 + 发布 0.1.0

**Goal**: example 成为三平台验证载体 + 规范初始化序列的活文档，包以 0.1.0 发布上架 pub.dev——占住名字、启动社区反馈循环。1.0.0 等宿主验收（Phase 6）之后再发。
**Mode:** mvp
**Depends on**: Phase 3, Phase 4（三平台齐备才发布）
**Requirements**: PUB-02, PUB-03, PUB-04
**Success Criteria** (what must be TRUE):

  1. example 从全新 clone 零手动步骤在三平台构建运行；演示 frame + 事件 + min/max + fullscreen 完整矩阵，并展示规范初始化序列（冷启动首启即无边框）（实机验证项）
  2. README 含双包迁移映射表（appWindow.minSize → setMinimumSize 等，替代兼容层）+ 与官方 windowing API 分工说明
  3. pana 静态检查达标（≥140，争取 160），`dart pub publish --dry-run` 干净通过，platform tags 含 windows/linux/macos
  4. 0.1.0 成功发布到 pub.dev；CHANGELOG + 语义化版本就位（1.0.0 明确留在宿主验收之后）

**Plans**: TBD

### Phase 6: 宿主集成验收

**Goal**: simple_player_flutter 用 window_frame_kit 替换 bitsdojo_window + window_manager 双包——PROJECT.md 的 Validated 验收线。宿主替换同时充当真实世界的回归套件：media_kit 纹理密集场景下的顶边命中、resize × 播放场景都是本包的目标用户画像。
**Mode:** mvp
**Depends on**: Phase 5
**Requirements**: HOST-01
**Success Criteria** (what must be TRUE):

  1. 宿主 pubspec 中 bitsdojo_window 与 window_manager 双包移除、仅剩 window_frame_kit；依赖树不再含 win32 5.x；runner 中 `bitsdojo_window_configure` 调用删除（零 runner 改动路线成立）
  2. 宿主 `flutter analyze` 0 error + 全量测试通过（预存失败按鉴别法归类，不新增回归）
  3. 实机冒烟：resize（含顶边 texture 密集区）、全屏进出、关闭持久化全部正常；README 迁移映射表逐项核对无遗漏（实机验证项）
  4. 验证通过后能力在 PROJECT.md 标记 Validated；1.0.0 发布路径打开

**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6
（Phase 4 与 Phase 3 零代码依赖，可在 Phase 2 完成后并行）

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. 包骨架与许可基座 | 4/4 | In Progress|  |
| 2. Windows 基座移植 | 0/TBD | Not started | - |
| 3. Windows Frame 嫁接 | 0/TBD | Not started | - |
| 4. Linux + macOS 移植 | 0/TBD | Not started | - |
| 5. Example 强化 + 发布 0.1.0 | 0/TBD | Not started | - |
| 6. 宿主集成验收 | 0/TBD | Not started | - |

## Coverage Notes

- v1 requirements: **26 total**（REQUIREMENTS.md 原"22 total"为计数笔误，创建 roadmap 时已修正）
- 映射调整说明：PUB-01（LICENSE 双上游归属）映射到 Phase 1 而非 Phase 5——Pitfall 2 强制 LICENSE stack 在首个含嫁接代码的 commit 之前落盘，Phase 1 是首个能交付该要求的阶段；Phase 5 发布门会再次复核归属
- XPLAT-02（C++-only）映射到 Phase 3：pubspec 依赖约束从 Phase 1 骨架期生效，但该要求的完成验证（零 win32 依赖 + 下游解锁成立）随 Frame 嫁接单元一起落地
- 全屏零闪屏（EXPL-01）、Snap Layouts（DIFF-01）、可配置边缘宽度（DIFF-02）均为 v2/探索目标，不在本 roadmap（用户 2026-09-04 裁决）
