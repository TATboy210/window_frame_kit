# Simple Player — 完整会话上下文交接（2026-09-03）

> 本文档 = 2026-09-02 晚 ～ 2026-09-03 全部会话工作的完整交接快照，供新会话/换窗口续接。
> 来源仓库：D:\simple_player_flutter ｜ 分支 master ｜ HEAD：4a36aaad
> 在 window_frame_kit 中的用途：**上游经验输入**——win32 6 类型适配清单、bitsdojo 源码要点、双包架构分工、生态终局结论均直接复用于本包设计。
> 配套文档：`PRIOR-CONTEXT-window-layer.md`（窗口层细节）

---

## ① 当前工作区状态（2026-09-03 更新：魔改已撤回，工作区全干净）

魔改（vendor bitsdojo + win32 6 解锁 + file_picker 12）已按用户指令**全部撤回**：third_party/ 已删、pubspec/两调用点回 HEAD（file_picker 11.0.3 + secure_storage 9.2.4 + bitsdojo 走 pub）、analyze 0 error、标准版构建 ✓。

**若将来重启魔改**，断点记录：
- PATCH 1（已验证可行）：vendor `bitsdojo_window_windows-0.1.6` → `third_party/`，pubspec `win32: ^5.1.1` → `">=5.1.1 <7.0.0"`，根 pubspec `dependency_overrides` 指路径；pub 解析通过（win32 6.4.0 + file_picker 12.2.0）
- PATCH 2（未做，**卡点**）：`lib/src/window.dart` 需 win32 6 类型适配，编译错集中行 24/26/58/90/91/101/204——win32 6.4 类型：`extension type const HWND(Pointer _)`（int 构造= `HWND(Pointer.fromAddress(n))`）、`extension type const SYSTEM_METRICS_INDEX(int _)`（可直构）、`GetSystemMetricsForDpi → Win32Result<int>`（取 `.value`）
- 12.x 调用点适配代码可从 simple_player_flutter commit `9c321793` 取回

> window_frame_kit 注（2026-09-04）：此魔改路线已被本包项目取代——不再 vendor 改 bitsdojo，而是以 window_manager 0.5.2 为底座新建单包。PATCH 1/2 的知识仍然有效（win32 6 迁移经验直接复用）。

---

## ② 今日工作时间线

1. **/gsd-progress**：v1.1 milestone（Phases 6-11）现状清点 → Phase 6 执行中断（06-01 停在 D-04 RED）
2. **/gsd-debug 清理**：4 个过时会话结案（fvp×2 过时、白边框被 C1 取代、windows-exe 的 CMP0175 已修）→ commit `ed239808`。剩 g03-1/g03-3（diagnosed）
3. **windows/ 模板恢复事件**：用户把 windows/ 恢复成 "test" 占位模板 → 存档 3 提交（`2f569100` windows 修复 / `9c321793` file_picker 12 / `d2d3c7ee` 06-01 checkpoint）→ 决策"模板+只补构建修复"（CMP0175 pin + 项目身份 + /utf-8）
4. **file_picker 12 编译修复**：静态 pickFiles API 变更（非空 List，取消=空列表）→ 2 处调用点适配
5. **runner NCHITTEST 恢复实验**：从 HEAD 恢复 HitTestWindowEdge → 用户实测"没用"（50px 也一样）→ 撤回 → **根因**：Flutter 子窗口盖住顶边，WM_NCHITTEST 到不了 runner（左右下能缩是因为插件留了真非客户区，Win11 顶部 inset=0）
6. **frameless 实验撤回**：setAsFrameless + SmartDragToResizeArea → 用户"撤回"
7. **架构调查/讲解**：4 层窗口架构、双包关系、分诊法（lib/ui=内容、window_bridge=操作、FFI=系统外观属性、runner=消息裁决）
8. **bitsdojo 迁移（BB 同款双包）**：侦察 BlueBubbles 源码（子代理）→ 用户裁决双包 → 实施（详见配套文档）→ 实机验证 1/2/3/4/6 过（#5 全屏未回报）→ 规范化（依赖倒置收回/双包边界文档/陈旧注释）→ minSize 双通道修复（854×480 真正生效）→ 取舍收窄（startDragging 归还 window_manager）
9. **抖动调查**：4 会话仪器基线入库 → 排除双包抢权力/纹理/rect → debug 会话 `resize-jank-axtree-flood` → ExcludeSemantics 修复（commit `b4336d27`，**待实机验证**）
10. **包生态终局**：native_api/webview/multi_window/desktop_window/window_plus/windowing 全 triage → 无更优单包，window_plus 唯一备胎
11. **官方 windowing 实验**：stable 门禁三层锁 → SDK 一行 patch 解锁 → 实测"对话框弹独立系统窗口"✓ → 用户裁决收起（config off，patch 留 D:\flutter 备用）→ 期间 build/ 被外部清理（用户 flutter clean？）
12. **魔改启动（半成品）**：见 ①

---

## ③ 架构定稿（勿回退）

- **双包分工**：bitsdojo 只做 frame 视觉与命中（接触面 3 处）；window_manager 做其余一切（含 startDragging——按用户裁决从 bitsdojo 收回）
- **架构文档**：`lib/kernel/window_bridge/window_bridge.dart` doc comment（双包边界 + UI 层纪律）
- **minSize 双通道**：bitsdojo 的 GETMINMAXINFO hook 无条件 return 0 会架空 setMinimumSize → 必须两侧同值 854×480
- **候选替换位**：window_plus（bitsdojo 单点替换，仍需配 window_manager）
- **魔改纪律**：`third_party/bitsdojo_window_windows/PATCHES.md` 每处改动必登记

> window_frame_kit 注：双包分工的知识对本项目核心——window_frame_kit 的存在意义就是把这两半合成一个包。

---

## ④ 未结事项（simple_player_flutter 侧，按优先级）

1. **魔改 PATCH 2**——已被 window_frame_kit 项目取代
2. **ExcludeSemantics 实机验证**：播放中快速拉伸 → AXTree 洪流消失？jank60 从 55-73% 降？→ 决定 debug 次轮（MediaQuery rebuild + BackdropFilter）——debug 会话 `resize-jank-axtree-flood` status: awaiting_human_verify
3. **#5 全屏验证**：bitsdojo 迁移验证清单唯一未回报项（BDW_CUSTOM_FRAME × media_kit 全屏）
4. **Phase 6**：06-01 停在 D-04 RED（DwmCapabilities 启动报 attribute 34/35/36 错误待查）；06-02 GATE 1 钉的 C1 结构已随模板化消失，**计划需重议**
5. **Windows ledger 6 open** + g03-1/g03-3 debug 会话（diagnosed）

---

## ⑤ 关键技术事实速查（window_frame_kit 直接复用）

- **win32 6.4.0 类型**：`extension type const HWND(Pointer _)`（从 int 构造= `HWND(Pointer.fromAddress(n))`）；`extension type const SYSTEM_METRICS_INDEX(int _)`（可直接构造）；`GetSystemMetricsForDpi(SYSTEM_METRICS_INDEX, int) → Win32Result<int>`（取 `.value`）；`GetSystemMetrics(SYSTEM_METRICS_INDEX) → int`；`GetDpiForWindow(HWND) → int`；`MonitorFromWindow(HWND, MONITOR_FROM_FLAGS) → HMONITOR`；`GetMonitorInfo(HMONITOR, Pointer<MONITORINFO>) → bool`
- **bitsdojo 源码要点**：GETMINMAXINFO 无条件 return 0（bitsdojo_window.cpp:550-568）；`bypass_wm_size` 恒 FALSE（adjustChildWindowSize 死代码）；hidden 分支 Win11 顶部 inset=0（window_manager_plugin.cpp:170 附近）
- **win32 5.x 锁**：bitsdojo_window_windows pubspec `win32: ^5.1.1`（已在 vendor 副本放宽）
- **windowing stable 解锁**：SDK features.dart 加 stable setting + 强刷 tool snapshot（rm bin/cache/flutter_tools.stamp snapshot）+ config enable-windowing；dart-define 注入被工具封死
- **宿主机 Flutter**：3.47.0 stable git checkout @ D:\flutter（SDK patch 已在 features.dart，upgrade 会冲掉）

---

## ⑥ 关键路径

- 原仓库 debug 会话：`.planning/debug/resize-jank-axtree-flood.md`（awaiting_human_verify）；g03-1/g03-3（diagnosed）
- memory（simple_player_flutter 项目）：`reference_official_windowing_api_stable_unlock` / `project_bitsdojo_dual_package_migration` / `project_window_frame_kit`
- 提交链（simple_player_flutter master）：ed239808 → 2f569100 → 9c321793 → d2d3c7ee → 9128920e → e256a019 → b4336d27 → 4a36aaad
