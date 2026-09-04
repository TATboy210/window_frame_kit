# Phase 2: Windows 基座移植 - Context

**Gathered:** 2026-09-04
**Status:** Ready for planning

<domain>
## Phase Boundary

window_manager 0.5.2 的完整能力面（约 60 方法 + 15 事件名常量 + WindowListener 11 hook；Dart 侧 763 行主文件 + listener/options/enums/widgets/utils，Windows C++ 侧 597 行）在 Windows 上**逐字移植**进本包，并通过**实机行为对等验证**（SC1-4）与**逐字 diff 机器证明 + channel 契约测试**（SC5）。**退出门：对等验证通过之前不得开始任何嫁接**——移植回归与嫁接回归必须可区分（Phase 3 的前提）。

**不含**：任何 frame 嫁接代码 / `// FRAME:` 标记落码（Phase 3）；Linux/macOS native 移植（Phase 4——本 phase 两者留模板态，仅要求 CI native build 保持绿）；example 强化与发布件（Phase 5）；宿主 simple_player_flutter 集成（Phase 6）。

</domain>

<decisions>
## Implementation Decisions

### Dart API 面保真度
- **D-01:** 公开 API 标识符**全保真**——`windowManager` 单例、`WindowManager`/`WindowListener`/`WindowOptions`/`ResizeEdge`/`TitleBarStyle` 等全部与上游逐字相同。包身份由包名 `window_frame_kit` + channel 名承载，不改类名。收益：宿主 Phase 6 迁移≈只换 import 行；SC5 逐字 diff 噪音最小（只剩 channel 常量与 import 路径两类必要偏离）；上游未来修复可直接 cherry-pick。 — **Reversibility:** costly — 后期改名要动全部宿主调用点、推翻 diff 证明基线并重写 DEVIATIONS.md 全部对应条目
- **D-02:** 上游 `test/window_manager_test.dart` **逐字移植**（只改 import + channel 常量），叠加本包自有锚点断言：channel 名 `window_frame_kit` 与 15 个事件名常量逐字一致性（SC5 机器门）。 — **Reversibility:** reversible
- **D-03 informational:** 共存纪律 = **互斥换 import**：宿主迁移期依赖树可同时含两包，但同一 Dart 文件绝不同时 import `window_frame_kit` 与 `window_manager`——同名类冲突结构性不存在。写进 README 迁移说明，零额外代码；**不做**前缀别名兼容层（YAGNI）。 — **Reversibility:** reversible [informational] Phase 2 无代码面：其唯一落点"README 迁移说明"属 Phase 5 PUB-02 范围（见 `<deferred>` 与 01-02 计划边界）；本 phase 各执行器仅需知晓纪律，无交付物，故不计入计划覆盖追踪

### 实机对等验证载体与节奏
- **D-04:** 载体 = **上游 example 白拿**：example 三件（main.dart 71 行 / pages/home.dart 1089 行·122 处 API 调用 / utils/config.dart）逐字移植为起点（只改 import + channel）；11 个 WindowListener hook 用 `debugPrint` 打**控制台事件日志**（时间戳 + 事件名 + 关键参数）。**零新增 UI 代码**——用户明确否决屏上日志面板（"只是做一个 flutter 依赖，没必要这么复杂"）。事件验证看控制台、窗口行为看目视（宿主 UAT 证据标准）。Phase 4 三平台直接复用同一载体。 — **Reversibility:** reversible
- **D-05:** 节奏 = **分批实机 checkpoint（约 3 批）**：建议切分 ①初始化协议 + 几何读写（CAPB-01/02/03）②状态操作 + 查询 + 事件流（CAPB-04/05/06）③拦截/标题栏/拖拽/外观/穿透/菜单（CAPB-07~12）；每批完成后用户做一次小规模实机确认，**Phase 2 退出门 = 末批全量 UAT**（SC1-4 四组）。批边界规划期可微调。 — **Reversibility:** reversible

### 事实核定（讨论期查证，非决策——下游直接采信）
- **F-1:** WM 0.5.2 Dart 侧**零 win32/零 dart:ffi**（依赖仅 `path` + `screen_retriever`）——XPLAT-02 "C++-only 零 win32" 与逐字移植天然无冲突；魔改时代的 win32 6 类型适配清单（PRIOR-CONTEXT §⑤）本 phase **用不上**，仅作参考存档
- **F-2:** WM 0.5.2 **零 @deprecated 成员**——"裁剪弃用面"灰区不存在，全量移植即可
- **F-3:** 上游 WM **不用 plugin_platform_interface**——Phase 1 挂账的复核有了确定答案：移植替换模板 lib 层后**移除该依赖**，白名单收敛为 `path` + `screen_retriever`（与上游 pubspec 完全一致）
- **F-4:** Linux/macOS native 留模板态（Phase 4 才移植）；Dart facade 三平台共享（上游 Dart 层无平台分支），linux/macos 上运行期能力缺失属预期，CI 只要求 native build 绿

### Claude's Discretion
- **逐字 diff 证明协议**（SC5 的机器化形态）：推荐 diff 脚本对 pub cache `window_manager-0.5.2`，排除项白名单 = channel 常量 / import 路径 / 包名派生标识（plugin 类名、文件名）；每处必要偏离记 DEVIATIONS.md（`// DEV:` 标记规则沿用 Phase 1 账本）
- **native C++ 命名**：默认保留 Phase 1 骨架的 `WindowFrameKitPlugin` 文件名/类名（channel 派生 + registrant 已锚定），把 WM 的 597 行实现移植进该结构；命名 delta 记账
- **Dart lib/src 内部文件名**：默认保留上游名（window_manager.dart 等），diff/cherry-pick 零摩擦；barrel = 本包 `lib/window_frame_kit.dart`
- doc comment 语言（默认保留上游英文）、UAT 条目分组细节、批次内任务切分、example 移植中 config.dart 的取舍
- 控制台事件日志的具体格式（含时间戳 + 事件名即可）

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 移植源（逐字对照基准）
- `C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/window_manager-0.5.2/` — 移植源全量：`lib/`（763 行主文件 + window_listener/window_options/resize_edge/title_bar_style/widgets/utils）、`windows/window_manager_plugin.cpp`（597 行）、`test/window_manager_test.dart`、`example/lib/`（三件 1160 行）

### 项目规划
- `.planning/PROJECT.md` — 包定位、Core Value、约束（C++-only、三平台 v1）；Phase 1 后已演化
- `.planning/REQUIREMENTS.md` — CAPB-01~12（本 phase 全部 12 条）
- `.planning/ROADMAP.md` §Phase 2 — 目标 + 5 条成功标准 + **退出门条款**
- `.planning/WINDOWS.md` — 开放账本（#4 resize 基线观察属 Phase 3，本 phase 勿动）

### 研究输入
- `.planning/research/ARCHITECTURE.md` — Pattern 1（WindowProc delegation，WM 原生机制）/ Pattern 5（单双向 channel）/ Pattern 6（Dart singleton + listener mixin，照抄勿发明）；Anti-Pattern 3（Dart 侧 FFI 禁令）；§Recommended Project Structure（lib/src 布局）
- `.planning/research/PITFALLS.md` — Pitfall 3（win32 6 extension types——本 phase 因 F-1 不涉及，Phase 3+ 才相关）、Pitfall 6（WndProc 所有权与顺序）、Pitfall 11（headless CI 假信心——视觉窗口行为不可 CI 验证，实机项的根据）
- `.planning/research/PRIOR-CONTEXT-window-layer.md` §一/§三 — 双包分工表（本包 = 两行合一的 API 面基准）、依赖链硬事实
- `.planning/research/PRIOR-CONTEXT-session-handoff.md` — 宿主魔改断点存档（仅参考；本 phase 用不上 win32 适配清单）

### 前序 phase 锁定项
- `.planning/phases/01-package-skeleton-license-foundation/01-CONTEXT.md` — D-01~D-09（channel 名红线、依赖白名单、UAT 证据标准）
- `.planning/phases/01-package-skeleton-license-foundation/01-01-SUMMARY.md` — channel 四处锚点行号（Dart:10 / Win:23 / Linux:69 / macOS:6）；plugin_platform_interface "Phase 2 复核删除" 挂账（F-3 已答）
- `.planning/phases/01-package-skeleton-license-foundation/01-03-SUMMARY.md` — CMake Pitfall-1 补丁承重墙事实（CI windows leg 必需，移植改 windows/CMakeLists.txt 时**不得删除**）
- `DEVIATIONS.md` — 偏离账本（本 phase 新增条目按 `// DEV:` 规则记账）

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 1 三平台骨架：`lib/` 模板三件套（被 WM 移植**替换**的对象）、`windows/window_frame_kit_plugin.cpp`（C-API 壳模式：外层 `WindowFrameKitPluginCApi` 薄壳 + 内层类——WM 实现移植进内层）、`example/`（被上游 example 三件替换）、`test/` 模板双测（被上游测试 + 锚点断言替换）
- CI 三平台矩阵已守护（analyze/format/test/native build + pana threshold 10）——移植期间每 push 自动回归
- `windows/CMakeLists.txt` 含承重墙补丁行 `set(CMAKE_POLICY_VERSION_MINIMUM 3.5)`（gtest FetchContent 前，勿删）

### Established Patterns
- 逐字移植纪律：verbatim port 区 + 偏离必记 DEVIATIONS.md（Phase 1 建账，格式已定：六列表 + file:line 锚点）
- 宿主 UAT 证据标准：控制台无崩溃 ≠ UI 正确呈现——窗口行为须用户目视；事件触发类验证控制台日志即为充分证据
- Phase 1 惯例：改依赖后 `flutter pub get` 再生成的 example registrant 必须与 pubspec 同 commit 落盘

### Integration Points
- channel 名 `window_frame_kit` 四处锚点：移植后 Dart 侧 channel 常量、Windows plugin.cpp、Linux plugin.cc、macOS Swift 四处必须保持同名一致（Linux/macOS native 留模板态但 channel 锚点不动）
- pubspec：移除 `plugin_platform_interface`（F-3）；`screen_retriever` 保留（WM Dart 侧真实依赖，用于 center/对齐的屏幕尺寸查询）

</code_context>

<specifics>
## Specific Ideas

- 控制台事件日志：11 个 hook 各自 `debugPrint` 一行 `[window_frame_kit] {timestamp} {eventName} {关键参数}`——实机验证 SC3 时用户对照 UAT 清单逐项操作、看控制台逐项打勾
- 分批 checkpoint 建议切分见 D-05（规划期可按移植单元微调，但**每批必须含该批能力的实机确认项**）
- SC5 的"逐字 diff 可证一致"：证明对象 = lib/ + windows/ 移植面；example 与 test 的适配改动（import/channel）不在逐字契约面内但改动必须最小且可列举

</specifics>

<deferred>
## Deferred Ideas

- 屏上事件日志面板（UI 内实时滚动）——用户本 phase 否决（复杂度质疑）；Phase 5 example 强化时如有需要可重提
- GETMINMAXINFO 协作式合并、NCCALCSIZE/WM_NCHITTEST 嫁接——Phase 3（本 phase 绝对禁止落任何嫁接码，退出门条款）
- resize 右缘瞬间空白基线观察的复测——Phase 3（WINDOWS.md #4）
- Linux/macOS native 移植与三平台实机——Phase 4
- pana description 精简 + threshold 重钉——Phase 5（WINDOWS.md #5）

</deferred>

---

*Phase: 2-Windows 基座移植*
*Context gathered: 2026-09-04*
