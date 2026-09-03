# Simple Player — 窗口层魔改与调试上下文（2026-09-03 交接文档）

> 来源仓库：D:\simple_player_flutter ｜ 分支 master ｜ 本文档对应的 git HEAD：4a36aaad
> 在 window_frame_kit 中的用途：**上游经验输入**——双包分工边界、依赖链硬事实、抖动根因排除法、全屏闪屏历史方案，均直接复用于本包设计。

---

## 一、当前架构（已提交，实机验证过）

### 双包分工（BB/BlueBubbles 同款，2026-09-03 用户裁决）

| 包 | 只负责 | 代码接触面 |
|----|--------|-----------|
| **bitsdojo_window 0.1.6** | ① 无边框/免系统主题色（NCCALCSIZE return 0）② 四边等宽拖拽缩放（WM_NCHITTEST）| `windows/runner/main.cpp` 一行 `bitsdojo_window_configure(BDW_CUSTOM_FRAME)` + `WindowBorder(transparent, width:0)` 容器 + minSize 同步一行 |
| **window_manager 0.5.2** | 其余一切：事件流（maximize/resize/move/close/focus）、setPreventClose、几何持久化、置顶、最小化、startDragging（窗口移动）| `lib/kernel/window_bridge/window_manager_service.dart` |

- 架构文档已写进 `lib/kernel/window_bridge/window_bridge.dart` doc comment
- runner 保持 Flutter 上游模板，**零自写窗口消息代码**
- UI 层纪律：widget 级视觉件可用 bitsdojo；**能力调用必须走 WindowBridge**，禁止持有 appWindow/windowManager 单例

> window_frame_kit 注（2026-09-04）：本包 = 上表两行合一。API 面以 window_manager 为基，frame 能力从 bitsdojo 移植。

### 当日已提交（simple_player_flutter master，5 个核心）
- `e256a019` feat(window): BB 双包迁移
- `b4336d27` fix(player): resize 期间 ExcludeSemantics 控制栏语义（**待实机验证**）
- `4a36aaad` docs(debug): resize-jank-axtree-flood 会话基线
- `9128920e` / `2f569100` / `9c321793` / `d2d3c7ee` planning 存档 / windows 模板修复 / 依赖适配 / 06-01 checkpoint

---

## 二、关键排除法结论（调试会话 resize-jank-axtree-flood）

**抖动根因链**（快速拉伸窗口时）：
1. ~~双包抢权力~~ 已排除：bitsdojo `adjustChildWindowSize` 在死分支（`bypass_wm_size` 恒 FALSE），搬子窗口只有模板 `WM_SIZE → MoveWindow` 一处
2. ~~纹理重建~~ 已排除：textureIdChanges=0（4 会话 + 历史 13 会话一致）
3. ~~media_kit rect 驱动~~ 已排除：rectChanges=0，rect 冻结 1280×720
4. **主因（已修，待验证）**：控制栏子树 visible 期间每帧重发 ~34 个语义节点 → accessibility_bridge 每帧 AXTree 同步失败（"Nodes left pending by the update: 34"）——修复 = `player_video_controls.dart:926` ValueListenableBuilder(resizing) → ExcludeSemantics
5. **次轮嫌疑（AND-gate：语义修复后 jank 仍高则打）**：MediaQuery 每帧全树 rebuild（build 侧）+ BackdropFilter blur 每帧重算（raster 侧）

**仪器基线**（播放中 drag+settle，4 会话）：totalP50 18.6-32.1ms（预算 16.6ms）、jank60 55-73%、buildP50 6.7-11.5ms（空态 0.2ms）。

**待验证判据**：标准版播放视频快速拉伸 → AXTree 洪流消失？jank60 从 55-73% 显著降？→ 不达标则开次轮。

> window_frame_kit 注：#4/#5 是宿主 app 层问题，非窗口包问题；但排除法本身（子窗口搬运/纹理/rect 的仪器判据）对本包的原生层验收有参考价值。

---

## 三、依赖链硬事实（魔改的根据 → window_frame_kit 的立项动机）

| 事实 | 后果 |
|------|------|
| bitsdojo_window_windows 0.1.6 锁 `win32: ^5.1.1` | file_picker 12（需 win32 6）装不上 → 退回 11.0.3 |
| 同链波及 flutter_secure_storage | 11.0.0 → 退回 9.2.4（lib/ 本来就不用它）|
| bitsdojo GETMINMAXINFO hook **无条件 return 0** | window_manager 的 setMinimumSize 被架空 → minSize 必须双通道同值（`doWhenWindowReady(() => appWindow.minSize = minimumWindowSize)`）|
| bitsdojo `bypass_wm_size` 恒 FALSE | `adjustChildWindowSize` 是死代码，无子窗口搬运冲突 |
| window_manager 0.5.2 | 活跃（60 天前更新，652k 下载）——赛道第一，无需动 |

**生态终局**（扫过 native_api / desktop_webview_window / desktop_multi_window / desktop_window / window_plus / flutter_native_window / 官方 windowing）：无任何单包提供"frame 接管+事件流"组合。window_plus 是 bitsdojo 唯一备胎。双包并用是行业正解（BB、MixinNetwork 均 fork/组合）。

> window_frame_kit 注：这正是本包要填补的空白。bitsdojo 的 GETMINMAXINFO 缺陷在本包中必须修复（协作式而非无条件 return 0）。

---

## 四、原魔改计划（已撤回，被 window_frame_kit 取代）

**原目标**：vendor bitsdojo 双包到 `third_party/` + 解锁 win32 6.x + file_picker 回 12.x + secure_storage 回 11.x

断点：PATCH 1（pubspec 放宽 win32 约束）已验证可行；PATCH 2（window.dart win32 6 类型适配）卡在 extension type 构造（完整类型清单见 `PRIOR-CONTEXT-session-handoff.md` §⑤）。

**后续可选魔改**（未排期，window_frame_kit 可吸收）：resize 边缘宽度可配置（现在硬编码）、裁 Linux/macOS 代码、GETMINMAXINFO 行为调整。

---

## 五、官方 windowing API 实验档案（已收起，2026-09-03 裁决）

- **stable 解锁法**：SDK patch（`D:\flutter\packages\flutter_tools\lib\src\features.dart` 给 windowingFeature 加 stable/beta setting）+ 强刷 tool snapshot（`rm bin/cache/flutter_tools.stamp bin/cache/flutter_tools.snapshot`）+ `flutter config --enable-windowing`。`flutter upgrade` 会冲掉 patch。
- **实测**：showDialog → 独立原生系统窗口 ✓（用户确认）；AXTree 错误变体切换 + 洪流；exit 127（已知引擎签名）
- **现状**：config off（惰性），SDK patch 留在原地备用。定位=多开窗口管理，非 chrome 替代。

> window_frame_kit 注：官方 windowing 是"多开窗口管理"赛道，与本包（单窗口 chrome/能力）不冲突，故列为 Out of Scope 而非竞品。

---

## 六、全屏闪屏问题档案（window_frame_kit 探索目标的上游输入）

历史方案（simple_player_flutter 侧，均实机效果不理想已 revert：aad3ba36 / 36883b77）：
- **方案 A（FFI 桥）**：DWM 层窗口样式切换控制
- **方案 B（DWM 禁用）**：DwmSetWindowAttribute 禁用过渡
- 全屏回 media_kit 原状；技术事实仍有效：route+utils.cc 双层链路 / Vulkan 不可行 / wm.setFullScreen frameless 缺陷
- 相关记忆：标题栏闪现用户暂缓（全局 DWMNCRP 已撤回）
- **重启攻坚前先采集撤回时具体症状**——window_frame_kit 的探索目标须以此为前置

其他挂账：Phase 6 06-01 DwmCapabilities 探测（main.dart 已接线）启动报 attribute 34/35/36 错误（`hr=0x-7ff8ffa9`，Win11 26200 应可用）——D-04 待修，另案。
