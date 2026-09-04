# window_frame_kit

## What This Is

Flutter 桌面窗口管理单包：一个包同时提供 **frame 接管**（无边框/免系统主题色 + 四边拖拽缩放，源自 bitsdojo_window）与 **完整窗口能力**（事件流、几何管理、置顶、startDragging、setPreventClose 等，源自 window_manager）。三平台（Windows/Linux/macOS）同步支持，紧跟 win32 6.x 与最新 Flutter/Dart。填补生态空白：截至 2026-09 的包生态扫描结论是"无任何单包提供 frame 接管 + 事件流组合"，开发者只能双包并用。目标发布到 pub.dev。

## Core Value

一个包同时提供 frame 接管 + 完整事件流能力——用户不再需要在"无边框窗口"和"可监听窗口"之间二选一，也不再被上游锁死的 win32 5.x 拖累整个依赖树。

## Business Context

- **Customer**: Flutter 桌面开发者（首要用户是作者本人的媒体播放器项目 simple_player_flutter）
- **Revenue model**: 免费开源（MIT），无直接收入
- **Success metric**: pub.dev 发布后被宿主项目实际替换双包架构，且 pub.dev likes/下载量证明社区需求真实存在
- **Strategy notes**: 上游经验全部来自 simple_player_flutter 的双包架构实战（2026-09-03 实机验证），交接文档见 `.planning/research/PRIOR-CONTEXT-*.md`

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] 无边框 frame 接管：NCCALCSIZE return 0 路线（免系统主题色边框、免 Win11 顶部 inset）
- [ ] 四边等宽拖拽缩放：WM_NCHITTEST 命中分发（源自 bitsdojo，边缘宽度可配置而非硬编码）
- [ ] 完整窗口能力 API：事件流（maximize/resize/move/close/focus）、setPreventClose、几何读写、置顶、最小化、startDragging（以 window_manager API 为基）
- [ ] win32 6.x 支持（extension type 适配，解锁 file_picker 12 / flutter_secure_storage 11 等下游依赖）
- [ ] 适配最新 Flutter 3.47 / Dart 3.13
- [ ] 修复 bitsdojo 已知缺陷：GETMINMAXINFO 无条件 return 0 架空 setMinimumSize 的问题（改为协作式）
- [ ] 三平台同步 v1：Windows/Linux/macOS 均有插件分支并实机验证
- [ ] pub.dev 发布就绪：LICENSE（含上游 MIT 归属）/README/example/CHANGELOG（Phase 1 已交付：LICENSE 三行叠加 + README Derivation 溯源 + CI pana 门；example 强化/CHANGELOG/API 文档 → Phase 5）

### Out of Scope

- 多开窗口管理（独立多窗口创建/管理）—— 官方 windowing API 赛道，与本包单窗口 chrome 定位不冲突
- bitsdojo API 兼容层 —— 新包新 API，不背历史包袱
- 修改 window_manager/bitsdojo 上游包 —— 自建包，不再 vendor 魔改

## Context

- **技术路线（已裁决）**：以 **window_manager 0.5.2 为底座**（活跃维护、API 全套、事件流完备），嫁接 bitsdojo_window 0.1.6 的 frame 代码（NCCALCSIZE + WM_NCHITTEST，量小）。不混合新写、不以 bitsdojo 为底
- **上游硬事实**（来自 simple_player_flutter 2026-09-03 实战，详见 `.planning/research/PRIOR-CONTEXT-*.md`）：
  - win32 6.4.0 类型迁移清单已验证可用（HWND extension type 构造、GetSystemMetricsForDpi 返回 Win32Result 取 `.value` 等）
  - bitsdojo GETMINMAXINFO hook 无条件 return 0 会架空 window_manager 的 setMinimumSize（双包时代靠双通道同值绕过，本包要根治）
  - Flutter 子窗口盖顶边导致 runner 的 WM_NCHITTEST 收不到——frame 命中必须由插件层（非 runner 模板）接管
  - 生态终局：window_manager 赛道第一（652k 下载），window_plus 是 bitsdojo 唯一备胎，无单包组合
- **宿主试验场**：simple_player_flutter（D:\simple_player_flutter，双包架构已实机验证）是本包首个集成目标，成熟后替换其 bitsdojo + window_manager 双包
- **全屏闪屏探索**：全屏切换闪老 Windows 窗口样式问题（历史方案 A/B 均实机不理想已撤回，档案见 PRIOR-CONTEXT-window-layer.md §六）——设为探索目标，成了是招牌特性，不阻塞发布

## Constraints

- **Tech stack**: Flutter plugin (Dart + C++/Win32, GTK, macOS AppKit)；底座 window_manager 0.5.2 源码（MIT）；frame 代码参考 bitsdojo_window_windows 0.1.6（MIT）
- **Dependencies**: win32 ^6.x（不得锁 5.x——这正是立项动机）；三平台插件结构（Windows/Linux/macOS）
- **Compatibility**: 最低支持到哪个 Flutter 版本由研究阶段决定；开发环境 Flutter 3.47.0 stable @ D:\flutter
- **License**: MIT，必须保留 window_manager 与 bitsdojo 上游版权声明（具体条款研究阶段核实）
- **Quality**: flutter analyze 0 error；宿主项目集成验证通过才算能力 Validated

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 全新单包（而非 vendor 魔改 bitsdojo） | 生态空白真实存在；bitsdojo 4 年未维护锁 win32 5.x，魔改是死路；单包根治依赖树问题 | — Pending |
| window_manager 0.5.2 为底座 | API 全套 + 事件流完备 + 活跃维护；frame 代码量小适合嫁接，反向工程量大得多 | — Pending |
| 三平台同步 v1 | 用户明确要求跨平台（与"适配全新 Flutter"一致）；避免 Windows-only 包的生态印象 | — Pending |
| 全屏零闪屏 = 探索目标非硬指标 | 历史方案 A/B 均实机不理想；硬指标会无限期阻塞发布 | — Pending |
| 包名 window_frame_kit | pub.dev 2026-09-04 核实未占用；语义直指核心价值 | ✓ 落定（Phase 1：公开仓 TATboy210/window_frame_kit + pubspec 三字段 + channel 名四处一致） |
| Pitfall-1 CMake 补丁为承重墙 | windows-latest 已是 windows-2025-vs2026 镜像（VS2026 捆绑 CMake 4.x），gtest min<3.5 硬错误本机与 CI 双侧复现 | ✓ `CMAKE_POLICY_VERSION_MINIMUM 3.5` 一行补丁不可删（Phase 1 A2 核对） |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-09-04 after Phase 1（包骨架与许可基座：4/4 计划、验证 5/5 SC 通过、PUB-01 收口、公开仓上线 + 三平台 CI 守护）*
