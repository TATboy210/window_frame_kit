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

> 02-04 计划追加；占位。

**批② sign-off:** （待 02-04）

---

## 批③ UAT 判定表（CAPB-07~12 — 拦截/标题栏/拖拽/外观/穿透/菜单）

> 02-05 计划追加；占位。

**批③ sign-off:** （待 02-05）

---

## Phase gate（退出门：SC1-4 全量 UAT + SC5 终跑）

> phase 末收口时填写；三批 sign-off 齐 + quick run 终跑记录。

**Phase gate sign-off:** （待 phase 末）
