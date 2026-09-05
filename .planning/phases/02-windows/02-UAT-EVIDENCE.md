# 02-UAT-EVIDENCE — 三批实机对等 UAT 证据账本

> 账本约定（02-03-PLAN Task 1 开卷）：每批 = 判定表（项/预期上游语义/用户回答/日期）+ sign-off 行。
> 预期值一律按上游 window_manager 0.5.2 语义写（Pitfall 9 纪律，引用 02-RESEARCH Example 5 / CAPB 行），不按"理想值"。
> 任何红项先走 A5 诊断路径（DPR 链路 src/window_manager.dart:97-101 → window_manager.cpp:718-776）归类：上游语义 / 环境（DPI/多显示器）/ 移植回归；不得静默重跑到绿。
> 02-04 / 02-05 在批② / 批③ 段追加；phase gate 段收 SC1-4 全量。

---

## integration_test 本地实机记录（批① 自动化半边）

- **命令:** `cd /d/window_frame_kit/example && D:/flutter/bin/flutter test integration_test -d windows`
- **日期:** 2026-09-05（两次实跑：首跑 + 按规则重试一次，结果一致）
- **结果:** **+17 passed / ~3 skipped / −1 failed（isFocused）**，输出末行 `00:00 +17 ~3 -1: Some tests failed.`（runner 退出码 0）

### 通过用例（精确断言）

getBounds（isA<Rect> 且 size==640×480）、isAlwaysOnTop、isClosable、isFullScreen、hasShadow、isMaximizable、isMaximized、isMinimizable、isMinimized、getOpacity==1.0、getPosition isA<Offset>、isPreventClose、isResizable、**getSize==640×480**、**getTitle=='window_frame_kit_test'**、getTitleBarHeight isNonNegative、isVisible

> SC1/SC2 几何半边的机器证据成立：waitUntilReadyToShow(640×480, title) → native GetBounds ÷ devicePixelRatio → Dart 逻辑像素精确断言全通（CAPB-02 换算链，A5 假设在本机 100% 档证实）。

### Skip 用例（上游按平台裁剪，预期内）

isAlwaysOnBottom（skip: macOS||Windows）、isMovable（skip: Linux||Windows）、isSkipTaskbar（skip: Windows）——3 项均为上游测试文件自带平台 skip，照 skip。

### 失败用例与诊断

**isFocused**（`expect(await windowManager.isFocused(), isTrue)` 失败）

- **重试:** 按规则重试一次，结果确定性复现（同 +17/~3/−1，唯一红项仍 isFocused）→ 非 flake。
- **A5 诊断路径:** 非几何/DPR 项，转环境 vs 移植回归鉴别。
- **移植回归排除:** `windows/window_manager.cpp:272-274 IsFocused() { return GetMainWindow() == GetForegroundWindow(); }` 与上游 pub cache window_manager-0.5.2 同文件同实现逐字一致（verbatim_diff 22+2 封顶同步复核 OK，`IsFocused` 不在残留 diff 内）。
- **归类: 环境（Windows 前台激活限制）**——测试由本机 terminal/runner 进程拉起，前台窗口归 runner 控制台所有；Windows 限制后台进程 `SetForegroundWindow`，故 `waitUntilReadyToShow` 前置段的 `focus()` 未能夺取前台，`GetForegroundWindow()` ≠ 测试窗口 → isFocused=false。上游同款测试在 GH runner 上同样面临前台不可靠问题（RESEARCH Pitfall 11 "GH windows runner 虽有交互会话但焦点/定位不可靠"同源）。
- **判定:** 预期环境行为，非移植回归，不修代码不改断言（改断言 = 逐字区违规）。批① 合并 UAT 会话时，用户手动激活 example 窗口后该断言的语义（focus() 生效）已由判定项 1（窗口按 WindowOptions 出现+可见）覆盖目视面。

---

## 批① UAT 判定表（CAPB-01/02/03 — SC1 + 几何/约束目视）

> 状态：DEFERRED to phase-end consolidated UAT (orchestrator decision under user auto-run directive 2026-09-05)
> 判定清单逐项列于 02-03-PLAN.md Task 2 <human_check>，7 项预期值按上游语义；批量会话时由用户操作 + dated sign-off。

| # | 判定项 | 预期（上游语义） | 用户回答 | 日期 |
|---|--------|------------------|----------|------|
| 1 | 初始化协议（CAPB-01/SC1）：冷启动窗口按 main.dart WindowOptions 出现 | 800×600、居中、titleBarStyle=hidden 无系统标题栏（02-02 tracer 已初证，此处复确认） | （deferred） | — |
| 2 | 几何读写（CAPB-02/SC1）：setSize(400,400)/setPosition/getSize 回读 | 窗口即时变化且回读值 = 设定值（逻辑像素） | （deferred） | — |
| 3 | DPI 双档（CAPB-02/SC1）：系统缩放 100% 与 150% 各跑一次第 2 项 | setSize(400,400) 后目视 + getSize 回读均为 400（物理尺寸随 DPI 变、逻辑值恒定；GetBounds ÷DPR cpp:718-740） | （deferred） | — |
| 4 | center/setAlignment（CAPB-02）：触发 center | 窗口目视居中（screen_retriever calc_window_position.dart:8-20） | （deferred） | — |
| 5 | min/max（CAPB-03）：setMinimumSize 后拖边框缩小；setMaximumSize 同理；reset(0,0) | 拖至 min 以下被挡 / max 以上被挡；reset 后解锁（WM_GETMINMAXINFO plugin.cpp:190-205 禁改区 verbatim） | （deferred） | — |
| 6 | aspectRatio（CAPB-03）：setAspectRatio(16/9) 后拖边；reset | 比例锁定；reset 解锁（WM_SIZING plugin.cpp:230-290 物理 px 比值 DPI 无关） | （deferred） | — |
| 7 | 事件旁证：拖拽期间控制台 | 连续 resize 行 + 松手一次 resized 行（Example 5；native 发射点 plugin.cpp:230-232/216-219） | （deferred） | — |

**批① sign-off:** DEFERRED — 待 phase 末合并 UAT 会话，7 项全过后用户回复 "approved" + 日期落账。

---

## 批② UAT 判定表（CAPB-04/05/06 — 状态操作/查询/事件流）

> 02-04 准备半边：源码预期已核实；以下均为待验预期，不是实际通过记录。人工项合并至 phase 末。
> 行号基准：windows/window_frame_kit_plugin.cpp（移植版）。

| 操作 | 预期事件/行为 | native 发射点 | 用户回答 | 日期 |
|---|---|---|---|---|
| 激活/失焦 | focus / blur | 208 / 210 | deferred | — |
| 最大化/还原 | maximize / unmaximize | 303 / 311 | deferred | — |
| 最小化/恢复 | minimize / restore | 306 / 314 | deferred | — |
| 持续拖边/松手 | resize / resized | 232 / 218 | deferred | — |
| 持续移动/松手 | move / moved | 228 / 222 | deferred | — |
| 进入/退出全屏 | enter-full-screen / leave-full-screen | 294 / 299 | deferred | — |
| 关闭（先开 preventClose） | close；确认对话框，取消后仍可操作 | 320 | deferred | — |
| show / hide | raw onWindowEvent，无具名 hook；hide 后需可恢复 | 326 / 328 | deferred | — |
| 禁用/恢复 resize | isResizable=false 时拖边无效，恢复后可缩放 | WM_NCHITTEST 分支及 SetResizable | deferred | — |

### 三类上游预期（勿误判回归）

| 类型 | 预期 | 用户回答 | 日期 |
|---|---|---|---|
| show/hide | raw onWindowEvent；无 onWindowShow/onWindowHide 具名回调 | deferred | — |
| docked/undocked | expected-absent：Windows native 无发射点 | deferred | — |
| isMovable / setMovable | MissingPluginException；plugin.cpp:586 NotImplemented | deferred | — |
| isVisibleOnAllWorkspaces / setVisibleOnAllWorkspaces | MissingPluginException；同上 | deferred | — |
| setBadgeLabel / grabKeyboard / ungrabKeyboard | MissingPluginException；同上 | deferred | — |

### 状态交叉回读

| 操作 | 预期查询 | 用户回答 | 日期 |
|---|---|---|---|
| maximize / unmaximize | isMaximized true / false | deferred | — |
| minimize / restore（由外部恢复后查） | isMinimized true / false；不可用失焦后点击查询代替最小化瞬间记录 | deferred | — |
| setFullScreen true / false | isFullScreen true / false | deferred | — |
| 激活 / 失焦 | isFocused true / false；读取不能抢回焦点 | deferred | — |
| setResizable false / true | isResizable false / true | deferred | — |
| setSkipTaskbar true / false | 任务栏图标消失 / 恢复（启动完成后操作） | deferred | — |
| setAlwaysOnTop true / false | 与批③置顶验证共用目视证据 | deferred | — |

**批② sign-off:** DEFERRED — 11 必验 hook、resized/moved、查询回读及特殊预期需实际操作后签核。

---

## 批③ UAT 判定表（CAPB-07~12 — 拦截/标题栏/拖拽/外观/穿透/菜单）

> 02-05：按用户要求移除托盘，CAPB-11 恢复改为 5 秒自动复位。以下全部待真实操作，不以单测代替目视。

| 组 | 操作与预期 | 用户回答 | 日期 |
|---|---|---|---|
| CAPB-07 关闭拦截 | 打开 preventClose → 点关闭出现确认框且窗口保留；No 留存；Yes 显式 destroy 退出 | deferred | — |
| CAPB-08 标题栏 | hidden/normal 往返；getTitleBarHeight 物理像素值>0，不能按逻辑像素误判 | deferred | — |
| CAPB-09 拖动 | DragToMoveArea 移动；红色 DragToResizeArea 八方向缩放；全屏拖动被 Dart 守卫拒绝 | deferred | — |
| CAPB-10 外观 | title/getTitle、窗口图标、阴影、透明度、背景色、brightness、置顶、置底逐项观察（Windows上游不支持项不冒充有效） | deferred | — |
| CAPB-11 穿透 | 开启 setIgnoreMouseEvents 后点击落到下层；5 秒后恢复可点击、开关自动归 false；通知区无托盘图标；forward 在 Windows 无效=上游语义 | deferred | — |
| CAPB-12 系统菜单 | 初始化之后 setAsFrameless，再 popUpWindowMenu，菜单显示且移动/大小/最小化可用 | deferred | — |

**批③ sign-off:** DEFERRED — 六组需实测；单测仅证明恢复调度，不证明原生点击穿透。

---

## Phase gate（退出门：SC1-4 全量 UAT + SC5 终跑）

> phase 末收口时填写；三批 sign-off 齐 + quick run 终跑记录。

**Phase gate sign-off:** （待 phase 末）
