# Phase 2: Windows 基座移植 - Research

**Researched:** 2026-09-05
**Domain:** window_manager 0.5.2 逐字移植（Dart facade + Windows C++ + example + test）→ window_frame_kit 骨架，C-API 壳整合，SC5 机器证明协议，契约测试，分批实机对等验证
**Confidence:** HIGH — 本研究**在本机 /tmp/port_probe 完整彩排了整个移植**（不触仓库）：Dart 面 12 文件 + C++ 面 2 文件 + example 3 文件 + test 2 文件全部落位后实跑 `flutter pub get`×2 / `flutter analyze`（0 issue，含 example）/ `dart format` 探针（发现 14/19 文件 tall-style 冲突）/ `flutter test`（契约测试 4/4 绿）/ `flutter build windows --release`（108.1s ✓ Built exe）/ exe 实机启动 6s Responding 后清理 / `dart pub publish --dry-run`（0 warnings）/ sed 归一化 diff 协议（16 个 Dart 文件全部零差）/ 空测试文件 exit 79 复现。所有 [VERIFIED: probe] 标注 = 本次彩排实测；所有 WM 源码论断带 pub-cache file:line。

<user_constraints>
## User Constraints (from 02-CONTEXT.md)

### Locked Decisions
- **D-01:** 公开 API 标识符**全保真**——`windowManager` 单例、`WindowManager`/`WindowListener`/`WindowOptions`/`ResizeEdge`/`TitleBarStyle` 等与上游逐字相同；包身份由包名 + channel 名承载，不改类名 — **Reversibility:** costly
- **D-02:** 上游 `test/window_manager_test.dart` 逐字移植（只改 import + channel 常量），叠加自有锚点断言：channel 名 + 15 事件常量逐字一致性（SC5 机器门）— **Reversibility:** reversible
- **D-03:** 共存纪律 = 互斥换 import（写 README 迁移说明属 Phase 5 PUB-02；本 phase 零代码）— **Reversibility:** reversible
- **D-04:** 实机验证载体 = 上游 example 三件逐字移植（只改 import + channel）；11 hook 用 `debugPrint` 打控制台事件日志（时间戳+事件名+关键参数）；**零新增 UI 代码**（屏上日志面板已否决）— **Reversibility:** reversible
- **D-05:** 节奏 = 分批实机 checkpoint（约 3 批）：①init+几何（CAPB-01/02/03）②状态+事件（CAPB-04/05/06）③拦截/标题栏/拖拽/外观/穿透/菜单（CAPB-07~12）；**退出门 = 末批全量 UAT（SC1-4）**；批边界规划期可微调 — **Reversibility:** reversible

### 事实核定（直接采信）
- **F-1:** WM 0.5.2 Dart 侧零 win32/零 dart:ffi（依赖仅 `path` + `screen_retriever`）——C++-only 约束与逐字移植无冲突 [VERIFIED: probe 双 lockfile grep win32 = 0，含 example 加 tray_manager 后]
- **F-2:** 零 @deprecated 成员——全量移植，无裁剪灰区
- **F-3:** 上游不用 plugin_platform_interface——移植后从 pubspec **移除**，白名单收敛为 `path` + `screen_retriever`
- **F-4:** Linux/macOS native 留模板态（Phase 4）；linux/macos 运行期能力缺失属预期，CI 只要求 native build 绿

### Claude's Discretion（本研究已给出推荐并彩排验证）
- 逐字 diff 证明协议 → **sed 归一化 + diff 零差脚本**（§Code Examples Example 2，[VERIFIED: probe 16 文件全过]）
- native C++ 命名 → **保留骨架 CApi 壳 + 桥接函数**，WM 匿名命名空间内部标识符全部原样（§Architecture Patterns Pattern 1，[VERIFIED: probe build+启动]）
- Dart lib/src 内部文件名 → **保留上游名**（window_manager.dart 等），barrel = `lib/window_frame_kit.dart`（内容与上游 barrel 字节一致）
- doc comment 语言（保留上游英文）、UAT 分组、批内任务切分、example config.dart 取舍（= 零改动字节拷贝 [VERIFIED: probe diff 零差]）、控制台日志格式

### Deferred (OUT OF SCOPE — 本 phase 绝对禁止)
- 任何 frame 嫁接码 / `// FRAME:` 落码（NCCALCSIZE/WM_NCHITTEST/GETMINMAXINFO 改动）——Phase 3；**退出门：对等验证通过前不得开始嫁接**
- 屏上事件日志面板（用户否决）；resize 右缘空白复测（WINDOWS.md #4，Phase 3）；Linux/macOS native 移植（Phase 4）；pana description 精简重钉（Phase 5，WINDOWS.md #5）
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CAPB-01 | ensureInitialized + waitUntilReadyToShow(WindowOptions) | Dart src/window_manager.dart:103-163（选项逐字段下发序列）；native CoCreateInstance 时序见 Pitfall 6；example main.dart:7-25 为规范用法；probe 实机启动 6s Responding = 协议全程走通 |
| CAPB-02 | 几何读写全套（逻辑像素换算正确） | GetBounds 除以 devicePixelRatio（window_manager.cpp:718-740）、SetBounds 乘（:742-776）；DPR 来源 = Dart `window.devicePixelRatio`（src/window_manager.dart:97-101，Flutter 3.47 仍存在 [VERIFIED: sky_engine/lib/ui/window.dart:1132]）；center/setAlignment 走 screen_retriever（calc_window_position.dart:8-20）；integration_test 有现成 640×480 精确断言可实机脚本化 |
| CAPB-03 | setMinimumSize/setMaximumSize/setAspectRatio | min/max 存逻辑 px + pixel_ratio_（window_manager.cpp:778-806），WM_GETMINMAXINFO 协作式条件覆盖（plugin.cpp:190-205，**Phase 3 禁改区**）；aspect 仅存值（cpp:657-660），实际约束在 WM_SIZING（plugin.cpp:230-290，物理 px 比值运算 DPI 无关） |
| CAPB-04 | 状态查询全套 | C++ 61 个 compare 分支中 Windows 缺 7 个 macOS/Linux-only 方法（isMovable/setMovable/isVisibleOnAllWorkspaces/setVisibleOnAllWorkspaces/setBadgeLabel/grabKeyboard/ungrabKeyboard）→ Windows 上走 `result->NotImplemented()`（plugin.cpp:585-587）= MissingPluginException，**属上游行为非回归**；example 对应条目在 Windows 点击会报——UAT 脚本须标注 |
| CAPB-05 | 状态操作全套 | maximize(vertically)/minimize/restore/setFullScreen/setResizable/setSkipTaskbar 等全部 verbatim；setFullScreen 用 media_kit 风格实现（cpp:568-655 注释自证）且 frameless 下正确（is_frameless_ 分支 :593）；setSkipTaskbar/setProgressBar 依赖 taskbar_ 初始化时序（Pitfall 6） |
| CAPB-06 | WindowListener 11 hook 全触发 | Dart 侧 15 常量 + funcMap 分发（src/window_manager.dart:15-30, 48-77）；Windows native 实际可发 13 个具名事件 + show/hide 两个裸事件（无具名 hook）；**docked/undocked 常量存在但 Windows native 从不发射**（plugin.cpp 全部 _EmitEvent 调用点枚举核实）——契约测试已头less证明 15 hook 分发全通 [VERIFIED: probe 4/4] |
| CAPB-07 | setPreventClose + isPreventClose，与 onWindowClose 成对 | WM_CLOSE → emit "close" → IsPreventClose 则 return -1（plugin.cpp:319-323）；destroy = PostQuitMessage(0)（cpp:232-234）**绕过 WM_CLOSE 不发 close 事件**；example onWindowClose 对话框 + destroy 为规范逃生门（home.dart:1056-1083） |
| CAPB-08 | setTitleBarStyle(normal/hidden, windowButtonVisibility) + getTitleBarHeight | SetTitleBarStyle 无条件重置 is_frameless_（cpp:914-929，Pitfall 7 时序）；hidden 分支 NCCALCSIZE 8px 调整（plugin.cpp:165-185，**Phase 3 禁改区**）；getTitleBarHeight 返回**物理 px**（cpp:931-943，无 DPR 除法——150% DPI 下 ≈47 属上游行为）；windowButtonVisibility 参数 Windows native 忽略（SetTitleBarStyle 只读 titleBarStyle 键） |
| CAPB-09 | startDragging + startResizing(ResizeEdge) | Dart 侧 fullscreen 守卫（src/window_manager.dart:719-748）；native SC_MOVE|HTCAPTION（cpp:1091-1095）与 8 向 HT* 码 PostMessage WM_NCLBUTTONDOWN（cpp:1097-1127）；均先 Undock+ReleaseCapture |
| CAPB-10 | setTitle/getTitle/setIcon/setHasShadow/setOpacity/setBackgroundColor/setBrightness/setAlwaysOnTop/setAlwaysOnBottom | 全部 verbatim；setIcon 的 Dart 侧路径拼接 = `path` 依赖唯一用点（src/window_manager.dart:607-617）；setBackgroundColor 走未文档化 SetWindowCompositionAttribute（cpp:662-716）；setBrightness 读注册表 + DWMWA_USE_IMMERSIVE_DARK_MODE（cpp:1040-1056） |
| CAPB-11 | setIgnoreMouseEvents(bool, {forward}) | Windows native 只读 "ignore" 键，**forward 参数被忽略**（cpp:1058-1069）= WS_EX_TRANSPARENT|WS_EX_LAYERED 切换；ignore=true 后窗口不可点击——**example 的 tray 菜单是唯一恢复路径**（home.dart:1043-1047），这是保留 tray_manager 依赖的硬理由 |
| CAPB-12 | popUpWindowMenu（frameless 下可唤出） | GetSystemMenu + TrackPopupMenu(TPM_RETURNCMD) at 光标位（cpp:1071-1089）；frameless（is_frameless_，WS_SYSMENU 保留）下可用——上游机制即 NCCALCSIZE 去框不剥样式位 |
</phase_requirements>

## Summary

本 phase 的全部工程风险已被一次**完整本机彩排**（/tmp/port_probe，仓库零接触）消解：WM 0.5.2 的 Dart 面（12 文件 1926 行）、Windows C++ 面（2 文件 1734 行）、example 面（3 文件 + integration_test + 4 张 tray 图标）、test 面（上游空壳测试 + 新契约测试）全部按"骨架结构 + 上游内容"落位后——analyze 0 issue（root analyze 覆盖 example，已用故意注错实证）、契约测试 4/4 绿、example release 构建 108s 通过、exe 启动 6 秒 Responding（ensureInitialized + waitUntilReadyToShow 全程走通的 tracer 证据）、dry-run 0 warnings、双 lockfile 零 win32。

三个彩排中**实弹发现**的坑改变了计划形态：① 本包 language version 3.13 触发 tall-style formatter，上游 19 个 Dart 文件中 **14 个**过不了 `dart format --set-exit-if-changed`——CI format 门必须改为"逐字区豁免 + 新码区正名单"双轨（豁免清单已实测锁定）；② 上游 `test/window_manager_test.dart` **零 test() 块**，单独存在时 `flutter test` exit 79（"No tests were found."）——D-02 的叠加断言不是可选项而是 CI 绿的必要条件，必须与模板测试删除同 commit；③ `package:window_manager/window_manager.dart` 这类 barrel import 需要**双重命名替换**（包名 + barrel 文件名），单 sed 会产出 404 import（probe 实弹命中后修正）。

C++ 整合方案已定型并构建+启动验证：**保留骨架 C-API 壳不动**（`window_frame_kit_plugin_c_api.cpp` + include 头 + pubspec pluginClass=WindowFrameKitPluginCApi + registrant 四锚全保），WM 的 597 行 plugin.cpp 以匿名命名空间原样进驻 `window_frame_kit_plugin.cpp`（内部标识符 `WindowManagerPlugin`/`WindowManager`/`window_manager` 成员**全部不改名**），仅 3 处适配：L1 include、L112 channel 串、尾部 extern "C" 导出块替换为 8 行桥接（`window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar` 转发）。实测残留 diff = plugin.cpp 恰 22 行、window_manager.cpp 恰 2 行——这两个数字就是计划 fails_when 的数值门。

SC5 逐字证明协议选定 **sed 归一化 + diff 零差**：`diff <(sed 's/window_frame_kit/window_manager/g' 本包文件) 上游文件` 对全部 16 个 Dart 文件（lib 12 + test 1 + example 3）实测**零差**——包名派生的 import/channel/title 串一次归一化全部消掉；C++ 两文件用精确行数断言（22/2）。建议脚本落 `tool/verbatim_diff.sh` 并作为 CI ubuntu leg 新步骤（curl pub.dev 0.5.2 archive 为对照源），把"编辑器 format-on-save 悄悄毁掉逐字性"这类事故变成即时红灯。

**Primary recommendation:** 按 5 计划推进——P01 Dart 全量面移植（lib+test+example Dart+pubspec×2+CI 三处改动，原子 commit 保 CI 绿）→ P02 Windows C++ 移植 + 实机 tracer checkpoint（窗口按 WindowOptions 出现 + 控制台事件日志首行）→ P03/P04/P05 = D-05 三批实机对等 checkpoint（CAPB-01~03 / 04~06 / 07~12），P05 末全量 UAT + SC5 终跑 = phase 退出门。example 依赖四件（bot_toast/preference_list/tray_manager/cupertino_icons）**全保留**——tray 是 CAPB-11 穿透后的唯一恢复路径；代价 = CI ubuntu leg 必须加 `libayatana-appindicator3-dev`（tray_manager linux CMakeLists 无它即 FATAL_ERROR，实源核实）。

## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| window_manager | 0.5.2（pub cache 钉死源） | 移植源全量 | D-01 底座；`C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/window_manager-0.5.2/` |
| Flutter SDK | 3.47.2 stable @ D:/flutter（CI 钉同版） | 构建/测试 | Phase 1 已钉；`window.devicePixelRatio` 弃用但**未移除** [VERIFIED: sky_engine/lib/ui/window.dart:1127-1132 @Deprecated + 声明仍在] |
| screen_retriever | ^0.2.2（骨架已在） | calc_window_position 屏幕查询 | 上游同款约束；federated（windows/linux/macos 实现自动注册）[VERIFIED: pub.dev API 查询] |
| path | ^1.9.1（骨架已在） | setIcon 路径拼接 | 上游 ^1.8.2 同 major；保留骨架较新值 |
| bot_toast | ^4.1.3（example 新增） | example toast | 上游 example 同钉；latest=4.1.3，零依赖，无 native [VERIFIED: pub.dev + probe 解析] |
| preference_list | ^0.0.2（example 新增） | example 设置列表 UI | 上游同钉；**已标 discontinued**（example-only，pana 只查根依赖，Phase 5 可评估替换）[VERIFIED: probe pub get 输出] |
| tray_manager | ^0.5.3（example 新增） | example tray（CAPB-11 恢复路径） | 上游同钉；Dart deps 仅 menu_base/path/shortid **零 win32**；三平台 native（Win C++/Linux GTK+appindicator/macOS SwiftPM Package.swift 在位）[VERIFIED: probe lockfile + cache 检视] |
| cupertino_icons | ^1.0.8（骨架 example 已有） | example 图标 | 保留骨架较新值（上游 ^1.0.2） |

### Supporting
| Tool | Purpose | When to Use |
|---------|---------|-------------|
| sed + diff（Git Bash 自带） | 移植机械化 + SC5 证明 | `s|package:window_manager/|package:window_frame_kit/|g` + barrel 二段替换 + channel 串替换；**禁用手工编辑逐字区** |
| tool/verbatim_diff.sh（新建） | SC5 机器门 | 本地对 pub cache / CI 对 curl archive；每 commit 可跑，秒级 |
| python（改写 C++ 尾块用） | 多行块替换 | sed 跨行块替换易碎；probe 用 python assert-in + replace 实弹成功 |
| flutter_test + TestDefaultBinaryMessenger | 契约测试 | setMockMethodCallHandler（Dart→native 捕获）+ handlePlatformMessage（native→Dart 注入）——两向机制均 probe 实证 |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| sed 归一化 diff（零差证明） | `git diff --no-index -w` / 格式双规范化后 diff | 归一化 sed 只在**包名派生串**上作用，证明强度=字节级；-w 会放过真实空白改动且 tall/short 换行重排无法用 -w 消 |
| 逐字区豁免 format 门（推荐） | 移植后全量 `dart format`（tall 化） | 格式化=14 文件偏离字节级 verbatim，cherry-pick 上游补丁需先重排，diff 噪音大；豁免方案下逐字区永远字节一致 [VERIFIED: probe 双方案实测] |
| 保留上游 example 四依赖（推荐） | 剔除 tray_manager（home.dart 手术 ~40 行） | 剔除破坏"example 逐字起点"（D-04）且砍掉 CAPB-11 UAT 恢复路径；保留的代价只是 CI ubuntu 一行 apt |
| 桥接函数进 C-API 壳结构（推荐） | 把 WM 类改名 WindowFrameKitPlugin 并搬进骨架头文件 | 改名方案触碰 113 处 `window_manager` 标识符行，diff 噪音 5 倍且 cherry-pick 全冲突；桥接方案 C++ 残留 diff = 22+2 行封顶 |
| 上游单类注册（改 pubspec pluginClass=WindowManagerPlugin） | 保留骨架 CApi 壳 | 改 pluginClass 会破坏 Phase 1 registrant 四锚与 example 生成件，且 3.47 模板惯例就是 CApi 壳——Discretion 已默认保壳 |

**Installation:**
```bash
# 无新工具。example 依赖在 example/pubspec.yaml 声明后：
cd example && flutter pub get   # registrant×3 + example/pubspec.lock 再生成，必须与 pubspec 同 commit（Phase 1 惯例）
```

**Version verification:** bot_toast 4.1.3 / preference_list 0.0.2 / tray_manager 0.5.3 / screen_retriever 0.2.2 / cupertino_icons 1.0.9 均为 pub.dev latest（2026-09-05 API 核实）；上游 example 钉的约束与 latest 一致（preference_list、tray_manager 无更新 major）。

## Package Legitimacy Audit

> 全部包 = 上游 WM 0.5.2 example 既有依赖或骨架已有，零 WebSearch 新发现。

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| bot_toast | pub.dev | 4.1.3（多年维护） | 高 | github.com/MrVolt21/bot_toast | OK | Approved（example-only） |
| preference_list | pub.dev | 0.0.2 | 中 | github.com/leanflutter | OK（**discontinued 标记**） | Approved（example-only；Phase 5 评估） |
| tray_manager | pub.dev | 0.5.3（leanflutter 活跃） | 高 | github.com/leanflutter/tray_manager | OK | Approved（example-only；CAPB-11 UAT 依赖） |
| screen_retriever / path | pub.dev | 既有 | 极高 | leanflutter / dart-lang | OK | Approved（Phase 1 已审） |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
**win32 零出现声明:** probe 实测 `grep -c win32 pubspec.lock example/pubspec.lock` = 0/0（tray_manager 0.5.3 native 走 C++/Swift，无 Dart win32）——XPLAT-02 生效条款在 example 扩依赖后仍成立，计划须复跑此双 grep 门。

## File-by-File Port Map

> 约定：**逐字区** = 字节级保上游（仅 sed 机械替换包名派生串），format 门豁免，SC5 证明对象；**新码区** = 本 phase 新写，须 tall-style format 干净。所有"改动行数"为 probe 实测值 = 计划 fails_when 数值门。

### A. lib/ —— Dart facade（逐字区；替换模板三件套）

| 上游文件（pub cache） | 行数 | 目标路径 | 改动行数 | 适配内容 |
|---|---|---|---|---|
| lib/window_manager.dart（barrel） | 11 | lib/window_frame_kit.dart | **0**（字节一致） | 仅文件名改（export 全相对路径；:10 `export 'src/window_manager.dart'` 因保留上游 src 名而无需动） |
| lib/src/window_manager.dart | 763 | lib/src/window_manager.dart | **6** | :9-13 五个 `package:window_manager/`→`package:window_frame_kit/`；:43 channel 串 `'window_manager'`→`'window_frame_kit'` |
| lib/src/window_listener.dart | 57 | 同名 | **0** | 字节拷贝 |
| lib/src/window_options.dart | 32 | 同名 | **1** | :3 import |
| lib/src/resize_edge.dart | 10 | 同名 | **0** | 字节拷贝 |
| lib/src/title_bar_style.dart | 4 | 同名 | **0** | 字节拷贝 |
| lib/src/utils/calc_window_position.dart | 93 | 同路径 | **0** | 字节拷贝（screen_retriever import 无包名派生串） |
| lib/src/widgets/drag_to_move_area.dart | 48 | 同路径 | **1** | :2 import |
| lib/src/widgets/drag_to_resize_area.dart | 158 | 同路径 | **2** | :4-5 import |
| lib/src/widgets/virtual_window_frame.dart | 146 | 同路径 | **4** | :5-8 import |
| lib/src/widgets/window_caption.dart | 139 | 同路径 | **4** | :3-6 import；**:19 doc comment 内 `'window_manager_example'` 字样保留不动**（文档示例文本，逐字区） |
| lib/src/widgets/window_caption_button.dart | 465 | 同路径 | **0** | 字节拷贝 |

**DELETE（模板三件套）:** `lib/window_frame_kit_platform_interface.dart`、`lib/window_frame_kit_method_channel.dart`（barrel `lib/window_frame_kit.dart` 被上游 barrel 内容覆盖）。
**pubspec.yaml:** 删 `plugin_platform_interface: ^2.0.2` 行及其注释行（F-3）；白名单注释"三项"改"两项"；`path`/`screen_retriever`/sdk floor/topics/身份五字段全不动。
**移植后 lib/ 内 `window_manager` 残留串（预期且必须保留）:** 5 处 `src/window_manager.dart` 文件名引用 + 1 处 window_caption doc comment = grep 白名单，fails_when 用 `grep -rn window_manager lib/ | grep -v "src/window_manager.dart" | grep -v "window_caption.dart:19"` = 空。

### B. windows/ —— C++（逐字区内容 + 骨架结构）

| 上游文件 | 行数 | 目标路径 | 残留 diff 行 | 适配内容 |
|---|---|---|---|---|
| windows/window_manager_plugin.cpp | 597 | windows/window_frame_kit_plugin.cpp | **22**（3 hunk，精确断言） | L1 include→`"window_frame_kit_plugin.h"`；L112 channel 串；L592-597 extern "C" 导出块→14 行桥接块（`// DEV:` 标记；probe 实测 diff=22 行整） |
| windows/window_manager.cpp | 1129 | windows/window_manager.cpp（**保留上游文件名**，推荐——见 Open Question 1） | **2**（1 hunk） | 仅 L1 include→`"window_frame_kit_plugin.h"` |
| windows/CMakeLists.txt | 27 | windows/CMakeLists.txt（骨架版为基底**合并**） | 结构合并 | ①PLUGIN_SOURCES 增 `"window_manager.cpp"`；②PLUGIN_NAME 增 `_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING`（上游 CMakeLists:17 同款）；③TEST_RUNNER 也增同 define（骨架 test 块直编 PLUGIN_SOURCES，上游无此目标故无此行——合并新增）；④**L81→86 `set(CMAKE_POLICY_VERSION_MINIMUM 3.5)` 承重墙保留**；⑤gtest 块/PLUGIN_NAME/include dirs/bundled_libraries 全保留骨架形态 |
| windows/include/window_manager/window_manager_plugin.h | 25 | **不移植** | — | 其职责（extern "C" 导出声明）由骨架 `include/window_frame_kit/window_frame_kit_plugin_c_api.h` 承担 |
| —（无上游对应） | 21 | windows/window_frame_kit_plugin.h | 新码区 | 骨架头（类+ctor+HandleMethodCall 声明）→ 替换为最小桥接头：namespace window_frame_kit 内仅 `static void RegisterWithRegistrar(flutter::PluginRegistrarWindows*)` 声明 |
| —（上游无 native 测试） | — | windows/test/window_frame_kit_plugin_test.cpp | 新码区 | 模板 GetPlatformVersion 测试（构造 plugin 实例调 HandleMethodCall——桥接方案下**不可编译**）→ 替换为 gtest 占位测试（保活 gtest 机制与 policy 补丁给 Phase 3 纯函数测试用；`// DEV:` 注释说明） |
| windows/window_frame_kit_plugin_c_api.cpp | 12 | **不动** | 0 | 薄壳原样：CApi 导出→`window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar` |
| windows/include/window_frame_kit/window_frame_kit_plugin_c_api.h | 27 | **不动** | 0 | registrant include 锚 |

**匿名命名空间内部标识符零改名（D-01 C++ 侧执行形态）:** `WindowManagerPlugin` 类、`WindowManager` 类、`window_manager` 成员指针、`IsWindows11OrGreater`、`adjustNCCALCSIZE` 等 113 处含 `window_manager` 的行**全部原样**——probe 实证编译+运行通过。命名 delta（文件名/桥接类名）记 DEVIATIONS.md。

### C. test/ —— 根测试（D-02）

| 上游文件 | 目标路径 | 改动行数 | 说明 |
|---|---|---|---|
| test/window_manager_test.dart | test/window_manager_test.dart（保留上游文件名） | **2**（sed 归一化后 diff 零差 [VERIFIED]） | :3 注释 import + :6 channel 串；**该文件零 test() 块——单独存在时 flutter test exit 79**（Pitfall 2） |
| —（新码区） | test/channel_contract_test.dart | 全新 ~110 行 | D-02 叠加锚点断言：channel 名捕获 / 15 事件常量 / handlePlatformMessage 注入驱动 15 具名 hook + onWindowEvent / getBounds 参数与返回形状——probe 4/4 绿 [VERIFIED] |

**DELETE:** `test/window_frame_kit_test.dart`、`test/window_frame_kit_method_channel_test.dart`（模板双测，引用被删的 platform_interface）。**同 commit 铁律:** 删除与 contract test 新增必须原子（否则 CI flutter test 红，Pitfall 2）。

### D. example/ —— 验证载体（D-04）

| 上游文件 | 目标路径 | 改动行数 | 适配内容 |
|---|---|---|---|
| example/lib/main.dart | 同名（覆盖模板版） | **3** | :3 barrel import（**双重替换**：包名+barrel 文件名，Pitfall 3）；:4-5 `window_manager_example`→`window_frame_kit_example` |
| example/lib/pages/home.dart | example/lib/pages/home.dart（新建目录） | **4** + 事件日志适配 | :10 barrel import（双重）；:11 example import；:703/:947 title 串；:1087 `print('[WindowManager] onWindowEvent: $eventName')` → debugPrint 日志（格式 `[window_frame_kit] {ts} {event}`；推荐独立 logger 文件方案见 Pattern 4，home.dart 零额外改） |
| example/lib/utils/config.dart | example/lib/utils/config.dart | **0**（字节一致 [VERIFIED]） | 纯 Flutter 代码零包名引用 |
| —（新码区，若选独立 logger） | example/lib/utils/window_event_logger.dart | 全新 ~55 行 | WindowListener 全 15 hook + onWindowEvent 各 debugPrint 一行（D-04 载体；main.dart +2 行注册）——**注意：新文件在 example/lib 下，须列进 format 门正名单** |
| example/integration_test/window_manager_test.dart | 同名（替换模板 plugin_integration_test.dart，DELETE 之） | **3**（sed+title 归一化后零差 [VERIFIED]） | :6 barrel import（双重）；:14/:118 title `'window_manager_test'`→`'window_frame_kit_test'`；**本地实机跑，不进 CI**（Pitfall 11 挂起风险） |
| example/images/{tray_icon,tray_icon_original}.{ico,png} | 同路径 ×4（~137KB） | 字节拷贝 | setIcon/tray 资源；example/pubspec 增 `assets: - images/` |
| example/pubspec.yaml | 同名 | +4 deps +assets | bot_toast ^4.1.3 / preference_list ^0.0.2 / tray_manager ^0.5.3（cupertino_icons 保留骨架 ^1.0.8） |
| example/test/widget_test.dart | **保留不动**（模板版） | 0 | 引用 MyApp+'Running on:'——main.dart 覆盖后运行必失败，但 **CI 从不跑 example/test**（root flutter test 不含嵌套包 [VERIFIED: probe 只跑 test/]）；上游同款文件同样坏——known stub 记账 |
| example/{windows,linux,macos}/flutter/generated_* | pub get 再生成 | 自动 | +TrayManagerPlugin 注册（probe 实证 windows registrant :16-17）；**与 example/pubspec 同 commit**（Phase 1 惯例）；example/pubspec.lock 变更一并提交 |
| example/windows|linux|macos 其余（runner/CMake/Podfile…） | **全部不动** | 0 | 裸模板 runner = Pattern 1 前提（HandleTopLevelWindowProc 转发链在位） |

### E. 不动区与 CI 改动清单

**不动:** `linux/`、`macos/`（模板壳 + channel 锚点 :69/:6 保持；F-4）、README.md、LICENSE、pubspec 身份字段、`.github/workflows/ci.yml` 的 pana 门（threshold 10）与触发器。
**ci.yml 三处改动（本 phase 唯一 CI delta）:**
1. **format 门双轨化**（Pitfall 1）：`dart format --set-exit-if-changed --output=none .` → 正名单形式（只查新码区：`test/channel_contract_test.dart example/lib/utils/window_event_logger.dart example/test` 等，逐字区豁免）
2. **ubuntu apt 增行**：`libayatana-appindicator3-dev`（tray_manager-0.5.3/linux/CMakeLists.txt 无 ayatana/appindicator 即 `FATAL_ERROR` [VERIFIED: 实源读取]；ubuntu-latest=noble 有该包，A1 首推验证）
3. **verbatim-diff 新步骤**（推荐，ubuntu leg）：curl `https://pub.dev/api/archives/window_manager-0.5.2.tar.gz` 解包 → `bash tool/verbatim_diff.sh <解包目录>`（纯 bash+sed+diff，秒级；把逐字性变成持续门）
**windows leg 的 `mkdir -p example/build/native_assets/windows` 预防步保留**（Phase 1 裁决：无法区分兜底生效与否，成本≈0）。

## Architecture Patterns

### Pattern 1: C-API 壳 + 匿名命名空间桥接（C++ 整合定案，[VERIFIED: probe build 108s + exe 启动 6s Responding]）

**What:** Flutter 3.47 模板注册链 = `generated_plugin_registrant.cc` → `WindowFrameKitPluginCApiRegisterWithRegistrar`（include/window_frame_kit/window_frame_kit_plugin_c_api.h）→ 薄壳 plugin_c_api.cpp → `window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar(PluginRegistrarWindows*)`。上游 WM 是单类直注册（pubspec pluginClass=WindowManagerPlugin，extern "C" 导出 `WindowManagerPluginRegisterWithRegistrar`，plugin.cpp:592-597）。整合 = **壳三件全保（registrant 锚不动），WM 597 行匿名命名空间原样进驻，尾部导出块换成 8 行桥**：

```
registrant (生成件,不动) → plugin_c_api.cpp (12行,不动) → plugin.h (新:桥接类声明,21行)
                                                        → plugin.cpp (WM 597行 verbatim
                                                           -L1 include -L112 channel
                                                           -尾块 +桥接14行)
                                                        → window_manager.cpp (WM 1129行
                                                           verbatim -L1 include)
```

**When to use:** 本 phase 唯一 C++ 结构决策；Phase 3 frame_controller.cpp 新文件将并列挂进同一 CMake（PLUGIN_SOURCES 追加）。
**Why not alternatives:** 改名方案（WM 类→WindowFrameKitPlugin 并拆进骨架头）触碰 113 处标识符行、cherry-pick 全灭；改 pubspec pluginClass 方案破坏 Phase 1 四锚与生成件。桥接方案残留 diff 封顶 22+2 行且全部可枚举。
**Evidence:** probe 产物 `windows/window_frame_kit_plugin.cpp` diff 上游 = 恰 3 hunk 22 行（L1/L112/尾块）；`window_manager.cpp` = 恰 1 hunk 2 行（L1）；构建含 gtest 目标（window_frame_kit_test.vcxproj）全绿。

### Pattern 2: 逐字区/新码区双轨纪律（format 门与 SC5 的结构基础）

**What:** 仓库 Dart 文件划两区——**逐字区**（lib/window_frame_kit.dart、lib/src/**、test/window_manager_test.dart、example/lib/main.dart、example/lib/pages/**、example/lib/utils/config.dart、example/integration_test/**）：字节级=上游（仅包名派生串 sed），format 门豁免，**任何手工编辑（含 IDE format-on-save）都会打破 SC5**；**新码区**（test/channel_contract_test.dart、example logger 新文件、tool/ 脚本）：tall-style format 干净，受 format 正名单门管。
**Why:** probe 实测 14/19 上游文件在 LV3.13 tall-style 下不 format-clean（清单见 Pitfall 1）；两全方案不存在——要么字节 verbatim（豁免），要么统一格式（放弃字节级）。D-01/D-02 的"逐字"字面义 + cherry-pick 零摩擦 ⇒ 豁免方案。
**Guard:** CI verbatim-diff 步（Pattern 3）使逐字区破坏即时红灯；README 或 CONTRIBUTING 可加一行"逐字区勿手动格式化"警示（裁量）。

### Pattern 3: sed 归一化 diff = SC5 机器证明（[VERIFIED: probe 16/16 Dart 文件零差 + C++ 精确行数]）

**What:** 单条归一化 `sed 's/window_frame_kit/window_manager/g'` 同时消掉 import 路径、channel 常量、example title 串、example 包名 import——因为它们全部由包名派生。归一化后：Dart 面 16 文件 diff 上游 = **零差**；C++ 面按精确 diff 行数断言（22/2，结构性偏离已在 DEVIATIONS.md 记账）。协议脚本化进 `tool/verbatim_diff.sh` + CI（对照源：本地=pub cache，CI=curl pub archive——与 cache 字节同源）。
**每处结构偏离 → DEVIATIONS.md 一行（六列格式已定）:** 预计 5 行——①包身份机械重命名（脚本证明，marker=n/a）②C-API 桥接块（anchor: window_manager_plugin.cpp:592-597，marker `// DEV:`）③window_manager.cpp:1 include 改径（marker 不加，行数记账 2）④windows/CMakeLists 合并三处（anchor: 上游 CMakeLists:10-17 + 骨架 test 块）⑤example/test 适配集（anchor: main.dart:3-5/home.dart:10-11,703,947/integration_test:6,14,118）+ 事件日志新增（D-04 授权）。

### Pattern 4: 控制台事件日志 = 独立 logger 文件（D-04 载体，推荐形态）

**What:** 新建 `example/lib/utils/window_event_logger.dart`（新码区，~55 行）：`class WindowEventLogger with WindowListener`，15 个具名 hook + `onWindowEvent(String)` 各 debugPrint 一行 `[window_frame_kit] {ISO时间戳} {hook名/事件名}`；main.dart 在 `windowManager.ensureInitialized()` 后 `windowManager.addListener(WindowEventLogger())`（+2 行）。
**Why 独立文件而非 home.dart 内联 11 个 override:** home.dart 保持在"4 行重命名"的最小适配面（逐字性可证）；logger 是新码区集中点；Phase 4 三平台复用同一文件（D-04 预期）。**注意:** logger 落在 example/lib/ 下 → format 门正名单须含它（Pattern 2 边界文件）。
**UAT 消费方式:** SC3 验证 = 用户按清单逐项操作窗口，控制台逐项出现对应 hook 行即打勾（宿主 UAT 证据标准：事件触发类验证控制台日志即为充分证据）。Windows native 可发射的具名事件 = 13（11 必验 + resized/moved），show/hide 走 onWindowEvent 裸串，docked/undocked Windows 不发射（见 CAPB-06 行）。

### Pattern 5: tracer-first 的 5 计划切分（D-05 三批 + 移植面原子性约束）

**硬约束（probe 实证）:** lib 面与 example Dart 面**不可分 commit**——模板 example 引用 `WindowFrameKit().getPlatformVersion()`，lib 换血后 example 即编译失败 → CI native build 三 leg 全红。故 Dart 全量面（lib+test+example Dart+pubspec×2+CI 改动）= P01 一个原子单元（文件多但全机械，probe 已跑通全程）。

| 计划 | 内容 | 门（自动化） | 实机 checkpoint |
|---|---|---|---|
| **P01 Dart 全量面移植** | §Port Map A+C+D(Dart 部分)+pubspec×2+ci.yml 三改+tool/verbatim_diff.sh+DEVIATIONS 5 行 | analyze 0（含 example）/ format 正名单 / test 全绿（contract ≥4 + 上游壳）/ diff 脚本 Dart 面零差 / dry-run 0 warnings / 双 lock 零 win32 / CI×3 leg 绿 | 无（头less） |
| **P02 Windows C++ 移植** | §Port Map B 全部 + registrant 再生成同 commit | 本机 build ✓ / gtest 占位编译 / diff 脚本 C++ 面 22+2 / CI windows leg 绿 | **tracer:** example 启动 → 窗口按 WindowOptions 出现（800×600 居中 hidden 标题栏）+ 控制台 focus/blur/close-attempt 事件行 = 初始化协议+事件回路全通（probe 已预演 6s Responding） |
| **P03 批①实机对等** | CAPB-01/02/03 UAT + 本地 integration_test 脚本化跑 | integration_test 本地绿（getSize=640×480 等精确断言） | **checkpoint ①:** SC1+SC2 UAT 清单（WindowOptions 逐字段、几何读写、100%+150% DPI 逻辑像素换算、min/max、aspect） |
| **P04 批②实机对等** | CAPB-04/05/06 UAT | —（查询断言并入 integration_test 可选） | **checkpoint ②:** SC3 UAT（11 hook 控制台逐项 + 查询/操作一一对应；Windows-only NotImplemented 7 方法按预期记录） |
| **P05 批③实机对等 + phase 门** | CAPB-07~12 UAT + SC5 终跑 | diff 脚本全量零差/精确值 + contract 全绿 + CI 绿 | **checkpoint ③ = 退出门:** SC4 全量 UAT（preventClose+destroy 对话框流、titleBarStyle 双态+getHeight、startDragging/8 向 startResizing、外观 9 项、穿透+tray 恢复、popUpWindowMenu frameless）+ SC1-4 复验 |

批边界微调余地：P02 tracer checkpoint 可并入 checkpoint ① 同场（用户偏好恰好 3 次时）；P03-P05 各含小修复循环（发现回归 → 修 → DEVIATIONS 记账 → 复验）。**任何 `// FRAME:` 落码 = phase 违规（退出门条款）。**

### Anti-Patterns to Avoid
- **手工编辑逐字区文件**（含"顺手修个 typo"/IDE format-on-save）——字节级证明即毁；一切改动走 sed 脚本或先记 DEVIATIONS
- **在本 phase"顺手修"上游 quirk**（IsWindows11OrGreater 恒真、getTitleBarHeight 物理 px、forward 参数忽略、example widget_test 坏）——全部 verbatim + 记账，修复属上游 issue 或 Phase 3+ 有意偏离
- **动 linux/macos 模板壳**（F-4 边界；channel 锚 :69/:6 保持）；**动 plugin_c_api.cpp / include c_api.h / pubspec pluginClass**（registrant 四锚）
- **删 windows/CMakeLists.txt 的 `CMAKE_POLICY_VERSION_MINIMUM 3.5`**（承重墙，01-03-SUMMARY 明令）或 gtest 块（policy 行的存在前提）
- **lib 与 example Dart 分 commit 移植**（CI 必红，Pattern 5 硬约束）
- **在 CI 跑 integration_test**（最小化/隐藏窗口类操作有挂起 runner 风险，Pitfall 11；本地实机跑）

## Code Examples

### Example 1: 机械移植脚本（P01 核心，probe 全程实证）

```bash
#!/usr/bin/env bash
# tool/port_from_upstream.sh 的一次性形态（执行期可内联，不必入库——入库的是 verbatim_diff.sh）
set -e
WM="C:/Users/35490/AppData/Local/Pub/Cache/hosted/pub.dev/window_manager-0.5.2"
# --- lib 逐字区 ---
rm lib/window_frame_kit_platform_interface.dart lib/window_frame_kit_method_channel.dart
cp "$WM/lib/window_manager.dart" lib/window_frame_kit.dart          # barrel 字节一致
mkdir -p lib/src/utils lib/src/widgets
cp "$WM"/lib/src/*.dart lib/src/
cp "$WM"/lib/src/utils/*.dart lib/src/utils/
cp "$WM"/lib/src/widgets/*.dart lib/src/widgets/
grep -rl 'package:window_manager/' lib | xargs sed -i 's|package:window_manager/|package:window_frame_kit/|g'
sed -i "s|MethodChannel('window_manager')|MethodChannel('window_frame_kit')|" lib/src/window_manager.dart
# --- pubspec: F-3 移除 plugin_platform_interface ---
sed -i '/模板 Dart 平台接口层/d; /plugin_platform_interface/d' pubspec.yaml
# --- test（D-02）---
rm test/window_frame_kit_test.dart test/window_frame_kit_method_channel_test.dart
cp "$WM/test/window_manager_test.dart" test/
sed -i "s|package:window_manager/window_manager.dart|package:window_frame_kit/window_frame_kit.dart|; \
        s|MethodChannel('window_manager')|MethodChannel('window_frame_kit')|" test/window_manager_test.dart
# --- example Dart（注意 barrel import 双重替换，Pitfall 3）---
cp "$WM/example/lib/main.dart" example/lib/main.dart
mkdir -p example/lib/pages example/lib/utils example/images
cp "$WM/example/lib/pages/home.dart" example/lib/pages/
cp "$WM/example/lib/utils/config.dart" example/lib/utils/
cp "$WM"/example/images/* example/images/
rm example/integration_test/plugin_integration_test.dart
cp "$WM/example/integration_test/window_manager_test.dart" example/integration_test/
grep -rl 'window_manager' example/lib example/integration_test | xargs sed -i \
  's|package:window_manager/window_manager.dart|package:window_frame_kit/window_frame_kit.dart|g; \
   s|window_manager_example|window_frame_kit_example|g; \
   s|window_manager_test|window_frame_kit_test|g'
# --- example pubspec: 上游四依赖 + assets ---
#（bot_toast ^4.1.3 / preference_list ^0.0.2 / tray_manager ^0.5.3；assets: - images/）
flutter pub get && (cd example && flutter pub get)   # registrant×3 再生成，同 commit！
```

### Example 2: tool/verbatim_diff.sh —— SC5 机器门（probe 16/16 零差实证）

```bash
#!/usr/bin/env bash
# SC5 逐字证明：对照源 $1 = 上游 window_manager-0.5.2 目录（本地 pub cache 或 CI curl 解包）。
# Dart 逐字区：sed 归一化后必须零差；C++：精确 diff 行数断言（结构偏离已记 DEVIATIONS.md）。
set -u
WM="${1:?usage: verbatim_diff.sh <window_manager-0.5.2 dir>}"
fail=0
norm() { sed 's/window_frame_kit/window_manager/g' "$1"; }   # 包名派生串一次归一
# A. Dart 逐字区（16 文件）——归一化后字节级零差
declare -A DART_PAIRS=(
  [lib/window_frame_kit.dart]=lib/window_manager.dart
  [lib/src/window_manager.dart]=lib/src/window_manager.dart
  [lib/src/window_listener.dart]=lib/src/window_listener.dart
  [lib/src/window_options.dart]=lib/src/window_options.dart
  [lib/src/resize_edge.dart]=lib/src/resize_edge.dart
  [lib/src/title_bar_style.dart]=lib/src/title_bar_style.dart
  [lib/src/utils/calc_window_position.dart]=lib/src/utils/calc_window_position.dart
  [lib/src/widgets/drag_to_move_area.dart]=lib/src/widgets/drag_to_move_area.dart
  [lib/src/widgets/drag_to_resize_area.dart]=lib/src/widgets/drag_to_resize_area.dart
  [lib/src/widgets/virtual_window_frame.dart]=lib/src/widgets/virtual_window_frame.dart
  [lib/src/widgets/window_caption.dart]=lib/src/widgets/window_caption.dart
  [lib/src/widgets/window_caption_button.dart]=lib/src/widgets/window_caption_button.dart
  [test/window_manager_test.dart]=test/window_manager_test.dart
  [example/lib/main.dart]=example/lib/main.dart
  [example/lib/pages/home.dart]=example/lib/pages/home.dart
  [example/lib/utils/config.dart]=example/lib/utils/config.dart
  [example/integration_test/window_manager_test.dart]=example/integration_test/window_manager_test.dart
)
for ours in "${!DART_PAIRS[@]}"; do
  if ! diff <(norm "$ours") "$WM/${DART_PAIRS[$ours]}" >/dev/null; then
    echo "VERBATIM FAIL: $ours"; fail=1
  fi
done
# B. C++ 面——精确残留行数（= DEVIATIONS.md 记账的结构偏离，改动即重审）
n1=$(diff windows/window_frame_kit_plugin.cpp "$WM/windows/window_manager_plugin.cpp" | grep -c '^[<>]')
n2=$(diff windows/window_manager.cpp "$WM/windows/window_manager.cpp" | grep -c '^[<>]')
[ "$n1" -eq 22 ] || { echo "plugin.cpp residue $n1 != 22"; fail=1; }
[ "$n2" -eq 2 ]  || { echo "window_manager.cpp residue $n2 != 2"; fail=1; }
# C. example home.dart 若采用内联 logger 适配，此处按选定方案增加对应豁免/断言（规划期定）
[ $fail -eq 0 ] && echo "VERBATIM PROOF OK (17 dart files zero-diff, cpp residue 22+2)"
exit $fail
# CI 步（ubuntu leg）：
#   curl -sL https://pub.dev/api/archives/window_manager-0.5.2.tar.gz | tar xz -C /tmp/wm052
#   bash tool/verbatim_diff.sh /tmp/wm052
```

注：home.dart 的事件日志若走**独立 logger 文件**（Pattern 4 推荐），home.dart 保持 4 行重命名 → A 段零差断言成立；若走内联 override，home.dart 须移入 C 段行数断言。二选一在 P01 落定。

### Example 3: 契约测试骨架（test/channel_contract_test.dart，probe 4/4 绿的原型）

```dart
// 新码区（D-02 叠加层）——SC5 的 channel 契约半边 + SC3 的 Dart 分发半边。
// 机制实证：setMockMethodCallHandler 捕获 Dart→native；handlePlatformMessage 注入 native→Dart。
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:window_frame_kit/window_frame_kit.dart';

class _Recorder extends WindowListener {
  final List<String> hooks = [];
  @override
  void onWindowClose() => hooks.add('close');
  // …15 个具名 hook 各一行（probe 版全文已验证）…
  @override
  void onWindowEvent(String eventName) => hooks.add('raw:$eventName');
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  const channel = MethodChannel('window_frame_kit');   // SC5 channel 名锚点断言的载体
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  test('channel name anchor: facade invokes on window_frame_kit', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return true;   // 若 facade 用了别的 channel 名 → MissingPluginException → 红
    });
    await windowManager.ensureInitialized();
    await windowManager.setPreventClose(true);
    expect(calls.map((c) => c.method), ['ensureInitialized', 'setPreventClose']);
    expect((calls[1].arguments as Map)['isPreventClose'], true);  // 参数形状
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('15 event-name constants verbatim', () {
    expect(kWindowEventClose, 'close');            // …15 条全列（值=上游数据串）
    expect(kWindowEventLeaveFullScreen, 'leave-full-screen');
    expect(kWindowEventUndocked, 'undocked');
  });

  test('native onEvent drives all 15 named hooks + raw onWindowEvent', () async {
    final recorder = _Recorder();
    windowManager.addListener(recorder);
    for (final name in [/* 15 个事件串 */]) {
      await messenger.handlePlatformMessage(
        channel.name,
        const StandardMethodCodec().encodeMethodCall(
          MethodCall('onEvent', {'eventName': name}),
        ),
        (data) {},
      );
    }
    // 15 具名 hook + raw 全部命中（probe 实证：SC3 的 Dart 分发半边头less 可证）
    windowManager.removeListener(recorder);
  });

  test('getBounds passes devicePixelRatio and parses x/y/width/height', () async {
    messenger.setMockMethodCallHandler(channel, (call) async {
      expect((call.arguments as Map).containsKey('devicePixelRatio'), true);
      return {'x': 1.0, 'y': 2.0, 'width': 800.0, 'height': 600.0};  // 返回形状
    });
    expect(await windowManager.getBounds(), const Rect.fromLTWH(1, 2, 800, 600));
    messenger.setMockMethodCallHandler(channel, null);
  });
  // 规划期可按方法组扩参数形状断言（maximize{vertically}/startResizing{resizeEdge+4bool}/
  // setBackgroundColor{ARGB 4键}/setTitleBarStyle{titleBarStyle,windowButtonVisibility}…）
}
```

### Example 4: C++ 桥接三件（P02，probe 构建+启动实证的精确形态）

```cpp
// windows/window_frame_kit_plugin.h（替换骨架版；新码区）
#ifndef FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
#define FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
#include <flutter/plugin_registrar_windows.h>
namespace window_frame_kit {
// DEV: registration bridge for the C-API shell
// (window_frame_kit_plugin_c_api.cpp). The ported window_manager 0.5.2
// implementation lives in window_frame_kit_plugin.cpp's anonymous namespace;
// this class only forwards RegisterWithRegistrar into it. See DEVIATIONS.md.
class WindowFrameKitPlugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);
};
}  // namespace window_frame_kit
#endif  // FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
```

```cpp
// windows/window_frame_kit_plugin.cpp 尾部（替换上游 L592-597 extern "C" 块）
namespace window_frame_kit {

// static
// DEV: C-API shell bridge. The Flutter 3.47 plugin template registers via
// WindowFrameKitPluginCApiRegisterWithRegistrar (window_frame_kit_plugin_c_api.cpp),
// which calls this class. Replaces upstream's extern "C" export
// WindowManagerPluginRegisterWithRegistrar (window_manager_plugin.cpp:592-597).
// See DEVIATIONS.md.
void WindowFrameKitPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  WindowManagerPlugin::RegisterWithRegistrar(registrar);
}

}  // namespace window_frame_kit
```

```cmake
# windows/CMakeLists.txt 合并增量（骨架基底上）
list(APPEND PLUGIN_SOURCES
  "window_manager.cpp"            # 上游第二源文件（保留上游名；plugin.cpp:15 #include 之）
  "window_frame_kit_plugin.cpp"
  "window_frame_kit_plugin.h"
)
# PLUGIN_NAME 与 TEST_RUNNER 两个目标都要（上游 CMakeLists:17 只有前者——上游无 native test 目标）：
target_compile_definitions(${PLUGIN_NAME} PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
target_compile_definitions(${TEST_RUNNER} PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
# set(CMAKE_POLICY_VERSION_MINIMUM 3.5) 承重墙行与 gtest 块原样保留（勿动）
```

### Example 5: 实机 UAT 触发-事件对照表（P03-P05 checkpoint 脚本骨架，native 发射点 file:line）

| UAT 操作 | 预期控制台日志（logger） | native 发射证据 |
|---|---|---|
| 点击窗口/点别处 | focus / blur | plugin.cpp:206-215（WM_NCACTIVATE） |
| 最大化/还原按钮 | maximize / unmaximize | plugin.cpp:302-304, 309-312（WM_SIZE 状态机） |
| 最小化/还原 | minimize / restore | plugin.cpp:305-308, 313-316 |
| 拖边框 resize 中/松手 | resize（连续）/ resized（一次） | plugin.cpp:230-232（WM_SIZING）/ 216-219（WM_EXITSIZEMOVE） |
| 拖动窗口中/松手 | move（连续）/ moved（一次） | plugin.cpp:226-229（WM_MOVING）/ 221-224 |
| setFullScreen(true/false) | enter-full-screen / leave-full-screen | plugin.cpp:292-300（WM_SIZE+last_state） |
| 点关闭（preventClose=on） | close（窗口不关，弹 example 对话框） | plugin.cpp:319-323（WM_CLOSE→emit→return -1） |
| show()/hide() | raw onWindowEvent: show/hide（**无具名 hook**，上游如此） | plugin.cpp:324-329 |
| dock()/undock() | **Windows 无 docked/undocked 发射**（常量仅 Dart 侧；is_docked_ 状态可查）——预期行为非缺陷 | plugin.cpp 无对应 _EmitEvent（枚举核实） |

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 移植机械替换 | 手工逐文件编辑 | Example 1 的 cp+sed 流水线 | probe 实证 16 文件零人为差错；手工编辑=逐字性不可证 |
| SC5 证明 | 人眼对读 5190 行 | verbatim_diff.sh（Example 2）+ CI 步 | 机器门每 push 复跑；行数断言即 fails_when 数值 |
| channel 契约 | 新发明 EventChannel/流式 API | 上游 onEvent-over-MethodChannel + 契约测试（Example 3） | ARCHITECTURE Pattern 5/6：照抄勿发明；EventChannel 重写 = post-v1 |
| 事件日志 UI | 屏上面板（用户已否决） | logger 类 debugPrint（Pattern 4） | D-04 零新增 UI；上游 home.dart:1086 本就有 print 钩子先例 |
| native 测试骨架 | 为 WM 类手写 registrar mock | gtest 占位 + Phase 3 纯函数测试 | WM 类构造需活 registrar（上游因此无 native 测试）；占位保活 policy 补丁与 gtest 管线 |
| 实机几何断言 | 手写截图比对 | 上游 integration_test 精确断言（getSize==640×480 等）本地跑 | 现成 17 个 testWidgets 覆盖 SC1/SC2 可脚本化半边 |
| DPI 换算 | 自写 GetDpiForWindow 包装（Phase 3 才做） | 上游 devicePixelRatio 传参链路 verbatim | 本 phase 是对等移植不是改进；换算点已全部定位（CAPB-02 行） |

**Key insight:** 本 phase 的"实现"几乎全部是**机械搬运 + 三处结构桥接**——真正的工程内容是证明体系（diff 脚本/契约测试/双轨 format 门）与验证编排（5 计划 3 checkpoint）。任何"改进冲动"（修 quirk、调格式、换机制）都直接侵蚀 SC5 与 Phase 3 的回归可分辨性。

## Common Pitfalls

> P1-P3 为本研究彩排**实弹命中**；P4-P12 为源码阅读 + probe 旁证定位。全部带 file:line。

### Pitfall 1: tall-style formatter 与逐字移植正面相撞（CI format 门必红）
**What goes wrong:** 本包 language version 3.13（pubspec `sdk: ^3.13.2`）触发 Dart tall-style formatter；上游代码在 short-style（sdk >=3.0.0）时代格式化。probe 实测 `dart format --output=none --show=changed .` 命中 **14/19 文件**：lib/src 9 文件（除 window_listener/window_options）、example home.dart/config.dart/integration_test、test 上游壳 + 未格式化的新契约测试。CI 现行 format 门（ci.yml:30）在移植 commit 后**必红**。
**Why it happens:** formatter 风格切换跟 language version 走；逐字移植与"全仓统一格式化"结构性不可兼得。
**How to avoid:** format 门双轨化（Pattern 2）：逐字区豁免、新码区正名单（`dart format --set-exit-if-changed --output=none test/channel_contract_test.dart example/lib/utils/window_event_logger.dart example/test/widget_test.dart`——probe 实证 OK）。**不可**反向操作（把上游文件 format 成 tall）——字节级 SC5 即毁。降 sdk floor 换 short-style 已被否（骨架生成件是 tall 格式，且推翻 Phase 1 Pitfall-8 裁决）。
**Warning signs:** 移植 commit 后 CI format job 红；或 IDE format-on-save 悄悄重写逐字区文件（CI verbatim-diff 步是唯一可靠防线）。

### Pitfall 2: 上游测试文件零 test() 块 → `flutter test` exit 79
**What goes wrong:** `test/window_manager_test.dart`（上游全文 27 行）只有 mock setUp/tearDown，**没有任何 test()/testWidgets()**。probe 隔离实证：仅含此文件的包 `flutter test` 输出 "No tests ran. / No tests were found."，**exit 79**（机制：test_core-0.6.20/lib/src/runner.dart:164 `(success ?? false) && (passed.isNotEmpty || skipped.isNotEmpty)` → 空集必 false）。若 P01 删模板双测而同 commit 未落契约测试 → CI test job 红。
**How to avoid:** D-02 叠加断言与模板测试删除**同 commit 原子落盘**（Pattern 5 P01 门已含）；fails_when 建议 `flutter test` 输出 `+[0-9]` 且 ≥4。
**Warning signs:** "No tests were found." 出现在任何 push 后。

### Pitfall 3: barrel import 需双重命名替换（单 sed 产出 404 import）
**What goes wrong:** `package:window_manager/window_manager.dart` 含两个待换名：包名**与 barrel 文件名**（上游 barrel=lib/window_manager.dart，本包=lib/window_frame_kit.dart）。只做 `s|package:window_manager/|package:window_frame_kit/|g` 会得到 `package:window_frame_kit/window_manager.dart`——文件不存在，analyze 直接红。probe 在 example/lib/main.dart:3、home.dart:10、integration_test:6 三处实弹命中。
**How to avoid:** 替换顺序 = 先长串 `package:window_manager/window_manager.dart` → `package:window_frame_kit/window_frame_kit.dart`，再短串 `package:window_manager/` → `package:window_frame_kit/`（Example 1 脚本已按此序）；随后 `grep -rn "window_frame_kit/window_manager.dart"` 必须为空。
**Warning signs:** analyze 报 "Error: Error when reading '...window_manager.dart'"。

### Pitfall 4: C-API 壳与上游单类注册的结构错配（照抄上游注册方式 = registrant 断链）
**What goes wrong:** Flutter 3.24+ 模板 Windows 注册链走 `WindowFrameKitPluginCApi`（pubspec.yaml:63 + registrant :18-19 + plugin_c_api.cpp:7-12）；上游 WM 是 pluginClass=WindowManagerPlugin 直导出 extern "C"（pub cache WM pubspec:47-48、plugin.cpp:592-597、include/window_manager/window_manager_plugin.h:18-19）。照抄上游（含其 include/ 头与 pubspec pluginClass）会破坏 Phase 1 registrant 四锚与 example 生成件；完全照抄骨架（内层类 ctor 无 registrar）又装不下 WM 的 `RegisterTopLevelWindowProcDelegate`（plugin.cpp:107-110，需要 PluginRegistrarWindows*）。
**How to avoid:** Pattern 1 桥接定案（probe 构建+启动双证）：壳三件不动，WM 匿名命名空间原样进驻，尾块换 8 行桥；DEVIATIONS 记账 anchor=plugin.cpp:592-597。
**Warning signs:** 链接错误（未解析的 WindowFrameKitPluginCApiRegisterWithRegistrar）或运行时 MissingPluginException（注册链断）。

### Pitfall 5: codecvt 弃用警告撞 /W4 /WX —— TEST_RUNNER 目标也要 silence define
**What goes wrong:** WM 代码用 `<codecvt>`/`std::wstring_convert`（window_manager.cpp:15, 902, 910, 993），C++17 弃用 → MSVC C4996；`apply_standard_settings` = `/W4 /WX /wd4100` + `_HAS_EXCEPTIONS=0`（example/windows/CMakeLists.txt:40-46 实读）→ 警告即错误。上游在 PLUGIN_NAME 上有 `_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING`（WM CMakeLists:17）但**上游没有 native test 目标**；骨架 test 块把 `${PLUGIN_SOURCES}` 直编进 TEST_RUNNER（骨架 windows/CMakeLists.txt:86-89）→ 只抄上游一处 define 会漏掉 test 目标，example 构建（gtest 门开，example/windows/CMakeLists.txt:56）在 test runner 编译时红。
**How to avoid:** 两个目标都加 define（Example 4 CMake 段）；probe 构建全绿实证。
**Warning signs:** `warning C4996: 'std::wstring_convert<...>': warning STL4017` + MSB 编译失败指向 window_frame_kit_test.vcxproj。

### Pitfall 6: taskbar_ 空指针时序 —— waitUntilReadyToShow 必须先于 setSkipTaskbar/setProgressBar
**What goes wrong:** `ITaskbarList3* taskbar_` 仅在 `WaitUntilReadyToShow()` 里 CoCreateInstance（window_manager.cpp:227-230，初值 nullptr :164）；`SetSkipTaskbar` 直接 `taskbar_->HrInit()`（:958）、`SetProgressBar` 直接 `taskbar_->SetProgressState`（:970）——**未先调 waitUntilReadyToShow 即空指针解引用崩溃**（native crash 非 Dart 异常）。
**How to avoid:** 规范序列（example main.dart:7-25 与 integration_test:10-19 都是先 ensureInitialized → waitUntilReadyToShow → 其余）写进 UAT 脚本；契约测试头less 不触发（mock 不进 native）。verbatim 移植**不加空判**（上游行为，加了=偏离）。
**Warning signs:** 调 setSkipTaskbar/setProgressBar 时进程直接消失（无 Dart 栈）。

### Pitfall 7: setTitleBarStyle 无条件重置 is_frameless_ —— setAsFrameless 调用顺序陷阱
**What goes wrong:** `SetTitleBarStyle` 第一动作 `is_frameless_ = false`（window_manager.cpp:919，上游注释明言"enables going from setAsFrameless() to normal/hidden"）。先 setAsFrameless 后 setTitleBarStyle → frameless 态被清（宿主 memory 同款教训："hidden 会重置 is_frameless_ 须在其后调"）。waitUntilReadyToShow(WindowOptions(titleBarStyle:...)) 内部第一步就是 setTitleBarStyle（src/window_manager.dart:129-134）→ **setAsFrameless 必须排在 waitUntilReadyToShow 完成之后**。
**How to avoid:** UAT 脚本固定顺序；example 是规范用法样板（其 main.dart 不混用两者）。Phase 3 frame 接管将引入 custom_frame_ 独立态——本 phase 禁改此函数（禁改区）。
**Warning signs:** frameless 生效但一次 setTitleBarStyle 后边框回来。

### Pitfall 8: IsWindows11OrGreater 恒真 quirk —— hidden 标题栏 top inset 行为与函数名相反
**What goes wrong:** `return dwBuild < 22000;`（plugin.cpp:31）——名为 "OrGreater" 实为 "LessThan"；且 `GetVersion()`（:25，弃用 API）在无 manifest 声明时**恒报 Win8（build 9200）**→ 函数恒真 → `sz->rgrc[0].top += IsWindows11OrGreater() ? 0 : 1`（:173）恒 +0。上游注释宣称的"Win10 白线 +1 补偿"实际从不生效。
**How to avoid:** **verbatim 移植，不修**（修=功能偏离，须走 DEVIATIONS + 对照上游 issue）；UAT 机为 Win11 26200，hidden 样式观察 top inset 行为时以此事实为基线（宿主白边框史：8px 内缩来自插件 NCCALCSIZE 后手，本函数链是同一战场——Phase 3 禁改区）。
**Warning signs:** 有人"顺手"把 `<` 改 `>=`——立即拦截。

### Pitfall 9: getTitleBarHeight 返回物理 px、setIgnoreMouseEvents 忽略 forward —— UAT 预期值要按上游行为定
**What goes wrong:** `GetTitleBarHeight` 用 TITLEBARINFOEX 原始差值，**无 DPR 除法**（window_manager.cpp:931-943）→ 150% DPI 机预期 ≈47 而非 ≈31；`SetIgnoreMouseEvents` 只读 "ignore" 键（:1058-1069），Dart 侧 `forward` 参数（src/window_manager.dart:704-710）在 Windows 被静默丢弃。两者都是**上游对等行为的组成部分**——SC2"一一对应"的基准是 WM 0.5.2 实机行为，不是理想行为。
**How to avoid:** UAT 清单把这两项的**预期值写成上游语义**（getHeight>0 且随 DPI 缩放为物理值；forward 无效果）；发现"不符"时先对照宿主的 window_manager 现行为再判回归。
**Warning signs:** UAT 记录把上游 quirk 判为本包缺陷 → 无谓修复循环。

### Pitfall 10: destroy 绕过 WM_CLOSE —— preventClose 拦截对 destroy 无效（设计如此）
**What goes wrong:** `Destroy()` = `PostQuitMessage(0)`（window_manager.cpp:232-234），不经 WM_CLOSE → 不触发 close 事件、不被 preventClose 拦截、直接退出消息循环。`Close()` = PostMessage SC_CLOSE（:236-239）才走可拦截路径。SC4 的"setPreventClose 拦截+destroy"正是要验证这对语义：拦截生效时 close 无效果 + 事件到，destroy 无条件终结。
**How to avoid:** UAT 脚本按 example 对话框流（home.dart:1056-1083：onWindowClose → 确认 → destroy）；宿主集成期文档化（Phase 6）。安全侧：prevent-close 必须有 destroy 逃生门（PITFALLS.md Security 表同款）。
**Warning signs:** 把"destroy 不触发 onWindowClose"报成 bug。

### Pitfall 11: headless CI 假信心 —— SC1-4 大头永不可 CI 验证（本 phase 的实机清单根据）
**What goes wrong:** 契约测试只证 Dart 侧形状与分发；native 发射、窗口视觉、DPI 实际换算、拖拽/菜单/穿透行为在 CI（mock + 无窗口会话）全部不可判。且 minimize/hide/close 类 integration_test 在 CI 有挂起 runner 风险（研究库 Pitfall 11；GH windows runner 虽有交互会话但焦点/定位不可靠）。
**How to avoid:** 三层验证架构（§Validation Architecture）：CI 头less 层 / 本地实机脚本层（integration_test + build + 启动存活）/ 人工目视层（D-05 三 checkpoint + 退出门）。integration_test 只本地跑，**CI 不挂**。
**Warning signs:** 任何"CI 绿 = SC 达成"的表述；无 dated sign-off 的实机项勾选。

### Pitfall 12: CI ubuntu leg 撞 tray_manager 的 appindicator 硬依赖
**What goes wrong:** tray_manager-0.5.3/linux/CMakeLists.txt：`pkg_check_modules(APPINDICATOR ... ayatana-appindicator3-0.1)` 失败→回退 appindicator3-0.1→再失败 = `FATAL_ERROR`（实源读取）。ci.yml 现行 apt 清单（clang cmake ninja-build pkg-config libgtk-3-dev，ci.yml:48）**不含它** → example 依赖加 tray_manager 后 ubuntu leg native build 必红。
**How to avoid:** apt 行加 `libayatana-appindicator3-dev`（noble universe 有此包；上游 WM 自家 build.yml 同款依赖清单佐证——01-RESEARCH 已录）。macOS leg 无此风险（tray_manager 有 macos/tray_manager/Package.swift，SwiftPM 原生支持 [VERIFIED: cache 检视]）。
**Warning signs:** ubuntu leg 报 "The `tray_manager` package requires ayatana-appindicator3-0.1"。

### Pitfall 13（Phase 1 继承，本 phase 复适用）: 依赖变更后生成件必须同 commit
`flutter pub get`（root+example）会再生成 example 三平台 registrant（+TrayManagerPlugin，probe 实证 windows registrant:16-17）与 example/pubspec.lock——全部并入同一 commit，否则 `dart pub publish --dry-run` 的 git-status 检查报 dirty warning（Phase 1 Plan 01 实测教训）。root pubspec.lock 为 gitignored 不涉及。

## Open Questions

1. **windows/window_manager.cpp 的文件名去留**
   - What we know: probe 用**保留上游名**方案全程验证通过（plugin.cpp:15 `#include "window_manager.cpp"` 行零改动）；ARCHITECTURE.md §Recommended Structure 草图曾画 `window_frame_kit.cpp`；Discretion 对 Dart src 明定保留上游名（零摩擦理由），C++ 第二文件无明定
   - Recommendation: **保留 `window_manager.cpp`**——与 Dart 侧同一裁量逻辑（cherry-pick 按路径零冲突），diff 残留少一行（include 行不用改）；DEVIATIONS 记"文件名保留上游"一行
2. **事件日志载体：独立 logger 文件 vs home.dart 内联 11 override**
   - What we know: 独立 logger（Pattern 4）保 home.dart 归一化零差、SC5 面最干净、+1 新文件；内联方案 home.dart 增 ~44 行（脱离零差断言，改行数断言），三件"逐字起点"少一件
   - Recommendation: **独立 logger**；main.dart +2 行注册（该行计入 main.dart 适配清单：3→5 行，sed 归一化需把 logger 注册行加入豁免或接受 home/main 行数断言——P01 落定细则）
3. **verbatim-diff 是否进 CI（vs 仅 phase 门本地跑）**
   - What we know: 脚本纯 bash 秒级；CI 化使 format-on-save 类事故即时红灯；成本 = ci.yml +4 行（curl+tar+bash）
   - Recommendation: **进 CI**（ubuntu leg）；Phase 3 嫁接期它还是"逐字区未被嫁接码污染"的守护
4. **example/test/widget_test.dart 处置**（模板版在 main.dart 覆盖后必失败-if-run，上游同款同样坏，CI 永不跑它）
   - Recommendation: **保留不动 + WINDOWS.md 记 known stub**（与上游行为对等）；Phase 5 example 强化时清理
5. **契约测试的参数形状断言铺多宽**（probe 4 用例已证机制；61 个 native 分支可逐一铺 mock 断言）
   - Recommendation: P01 按 CAPB 分组铺**每方法组 1-2 个代表**（几何/状态/外观/拦截/拖拽 5 组 ~12-15 用例），全量 61 分支铺设留给回归需要时增量——KISS，机制已证

## Assumptions

| # | Claim | Section | Risk if Wrong | Verification Method |
|---|-------|---------|---------------|---------------------|
| A1 | ubuntu-latest (noble) apt 有 `libayatana-appindicator3-dev`，tray_manager linux 编译通过 | Pitfall 12 / CI 改动 | ubuntu leg native build 红 | P01 首次 push CI 即验；红则回退 `appindicator3-dev`（tray CMake 的双回退链） |
| A2 | macos leg：tray_manager（SwiftPM Package.swift 在位）+ 现有 SwiftPM 工作流兼容，podspec 平台 :osx 10.11 无冲突 | Port Map D / CI | macos leg 红 | P01 首次 push CI 即验；红则按输出修（Flutter tool 自动 SwiftPM↔CocoaPods 回退） |
| A3 | pana 分数移植后不降（dartdoc 覆盖↑、依赖不变、description 未动）→ threshold 10 继续通过 | CI / Validation | pana job 红 | P01 首跑 CI pana job；若降分按 WINDOWS.md #5 流程重审 |
| A4 | probe 的构建/启动结论在 D:/window_frame_kit 真实路径复现（probe 在 C:\...\Temp，MSB8029 临时目录警告为 probe-only） | Pattern 1 / P02 | 本机 build 意外红 | P02 首个 task 即本机 `flutter build windows --release`（Phase 1 同环境已绿过，风险低） |
| A5 | integration_test 在实机 100%/150% DPI 下精确断言（getSize==640×480）通过（上游设计含 DPR 除法链路） | P03 / CAPB-02 | 批① 脚本半红 → 转人工判读+记录 | P03 实机首跑；红时先核 DPR 链路（src/window_manager.dart:97-101 + cpp:718-776）再判回归 |
| A6 | Windows 实机可触发 13 具名事件（11 SC3 必验 + resized/moved）；docked/undocked 不发射属上游对等 | CAPB-06 / Example 5 | SC3 某 hook 不触发 → 真回归需修 | P04 checkpoint 按 Example 5 表逐项；发射点 file:line 全列便于诊断 |
| A7 | example 四依赖在 Flutter 3.47 全平台解析（probe 仅 Windows 实证；bot_toast/preference_list 纯 Dart） | Port Map D | pub get 冲突 | P01 pub get ×2 + CI 三 leg；probe 已过 Windows 半边 |
| A8 | 编辑器 format-on-save 不会在 executor 会话破坏逐字区（或破坏即被 CI diff 步捕获） | Pattern 2/3 | 逐字性静默漂移 | CI verbatim-diff 步（Open Question 3 推荐）+ 每 plan 收口跑本地脚本 |

## Validation Architecture

> 三层：**CI-headless**（每 push 自动）/ **本地实机脚本**（executor 在 dev 机跑，可脚本化）/ **人工目视**（用户 checkpoint，dated sign-off）。Pitfall 11 铁律：控制台无崩溃 ≠ UI 正确呈现。

### Test Framework
| Property | Value |
|----------|-------|
| Framework | flutter_test（契约/单元，headless）+ integration_test（本地实机脚本）+ bash diff 脚本（SC5）+ 人工 UAT 清单 |
| Config file | analysis_options.yaml（format 门双轨后不变）+ ci.yml（三处改动）+ tool/verbatim_diff.sh（新） |
| Quick run command | `flutter analyze && flutter test && bash tool/verbatim_diff.sh <pub-cache-wm-0.5.2>`（~30s） |
| Full suite command | quick run + `dart format 正名单` + `dart pub publish --dry-run` + `cd example && flutter build windows --release` + `flutter test integration_test -d windows`（本地） + CI 三 leg + pana |

### SC 1-5 → 验证层映射
| SC | 内容 | CI-headless | 本地实机脚本 | 人工目视（判定标准） |
|----|------|------------|--------------|---------------------|
| SC1 | example 实机 ensureInitialized+waitUntilReadyToShow 窗口按参数出现、几何即时生效、逻辑像素换算正确 | 契约测试证调用序列/参数形状；build 绿 | integration_test：waitUntilReadyToShow(640×480,title) → getSize==640×480 / getTitle=='window_frame_kit_test' / getBounds isA\<Rect\>（17 用例现成） | 窗口出现=800×600 居中、hidden 标题栏（无系统标题栏目视）、透明背景；150% DPI 下 setSize(400,400) 后目视+getSize 回读 400 |
| SC2 | 状态操作与查询一一对应、setTitleBarStyle+getTitleBarHeight 生效 | 契约测试证 61 方法名/参数键存在 | integration_test 查询断言（isResizable/isClosable/isMaximized/isFullScreen/isPreventClose/getOpacity…上游 skip 表已按平台裁剪） | 每操作后对应查询回读一致（UAT 表驱动）；titleBarStyle normal↔hidden 切换目视 + getTitleBarHeight 控制台值>0（物理 px 语义按 Pitfall 9） |
| SC3 | 11 WindowListener hooks 全部触发 | **契约测试头less 证 Dart 分发半边**（15 hook+raw 全命中 [VERIFIED: probe]） | —（native 发射不可脚本安全触发——minimize/close 挂起风险，Pitfall 11） | 按 Example 5 触发-事件表逐项操作，控制台 logger 逐项出行为勾（13 可触发；show/hide 走 raw；docked/undocked 预期缺席） |
| SC4 | setPreventClose 拦截+destroy、popUpWindowMenu frameless 可唤出 | 契约测试证 setPreventClose 参数形状 | —（关闭/菜单行为不可 headless） | preventClose=on 点关闭→窗口不关+console close 行+对话框弹出→No 留存/Yes 进程退出；setAsFrameless 后 popUpWindowMenu 弹出系统菜单（移动/大小可见）；startDragging/DragToResizeArea 8 向实拖 |
| SC5 | 逐字 diff 可证一致 + channel 契约测试覆盖参数/返回形状与 15 事件常量 | **verbatim_diff.sh（Dart 17 文件零差 + C++ 22/2 精确）+ 契约测试全绿 = 全自动**（CI 步，Open Question 3） | 同脚本本地对 pub cache 复跑 | —（无需人工；脚本输出即证据，DEVIATIONS 行数与脚本豁免一一对应） |

### CAPB-01~12 → 验证层映射（批次归属 = Pattern 5）
| Req | CI-headless | 本地实机脚本 | 人工目视 | 批次 |
|-----|------------|--------------|----------|------|
| CAPB-01 初始化协议 | 契约：ensureInitialized/waitUntilReadyToShow 调用捕获 | integration_test waitUntilReadyToShow 前置段 | SC1 目视行 | P02 tracer + P03① |
| CAPB-02 几何读写 | 契约：getBounds/setBounds 参数形状+返回解析 [VERIFIED] | integration_test getBounds/getSize/getPosition/setBounds 精确断言 | DPI 双档目视+回读 | P03① |
| CAPB-03 min/max/aspect | 契约：setMinimumSize 三键（devicePixelRatio/width/height） | —（约束类行为需交互拖拽） | 拖至 min 以下被挡；aspect 16:9 拖边比例锁定；reset(0) 解锁 | P03① |
| CAPB-04 状态查询 | 契约：方法名存在性 | integration_test 12 查询断言（上游 skip 表） | 查询-操作交叉回读表 | P04② |
| CAPB-05 状态操作 | 契约：maximize{vertically}/setFullScreen{isFullScreen}/setResizable 等参数形状 | —（挂起风险，Pitfall 11） | 逐项操作目视 + 7 个 Windows-NotImplemented 方法点击报 MissingPluginException 记为预期 | P04② |
| CAPB-06 事件流 11 hook | **契约：15 hook 分发全命中 [VERIFIED: probe]** | — | Example 5 表逐项 console 打勾 | P04② |
| CAPB-07 关闭拦截 | 契约：setPreventClose/isPreventClose 形状 | — | SC4 对话框流全程（拦截→No→Yes→destroy 退出） | P05③ |
| CAPB-08 标题栏样式 | 契约：setTitleBarStyle 双键 | integration_test getTitleBarHeight isNonNegative | normal↔hidden 目视 + height 控制台值（物理 px 预期） | P05③ |
| CAPB-09 拖拽发起 | 契约：startResizing{resizeEdge+4 bool} 形状 | — | DragToMoveArea 实拖 + DragToResizeArea 8 向实拖（fullscreen 下 Dart 守卫拒发=预期） | P05③ |
| CAPB-10 外观置顶 | 契约：setBackgroundColor ARGB 4 键/setTitle/setOpacity 形状 | integration_test getTitle/getOpacity | 9 项逐项目视（icon 换图、阴影、透明度、背景色、暗色标题栏、top/bottom） | P05③ |
| CAPB-11 鼠标穿透 | 契约：setIgnoreMouseEvents{ignore,forward} 形状 | — | 穿透 on→点击落到下层窗口；**tray 菜单恢复路径**（home.dart:1043-1047）；forward 无效=预期（Pitfall 9） | P05③ |
| CAPB-12 系统菜单 | 契约：popUpWindowMenu 调用捕获 | — | frameless 下菜单弹出、移动/大小/最小化项可点 | P05③ |

### Sampling Rate
- **Per task commit:** quick run command（analyze + test + verbatim_diff）
- **Per plan 收口:** full suite 本地半边（format 正名单 + dry-run + example build；P02 起加启动存活断言）+ CI 三 leg 绿确认
- **Per checkpoint（P03/04/05）:** 对应批 UAT 清单 dated sign-off（宿主 UAT 证据标准）
- **Phase gate（P05 末）:** SC1-4 全量 UAT 复验 + SC5 终跑（diff 脚本全量 + 契约全绿 + DEVIATIONS 行数对账）+ CI 绿 + **零 `// FRAME:` grep 断言**（退出门：`! grep -rn "FRAME:" lib windows example test`）

### Wave 0 Gaps
- [ ] `tool/verbatim_diff.sh` — SC5 载体（Example 2 已给全文）
- [ ] `test/channel_contract_test.dart` — D-02 叠加层（Example 3 骨架，probe 版可扩展）
- [ ] `example/lib/utils/window_event_logger.dart` — D-04 载体（Pattern 4；若 Open Question 2 选内联则改为 home.dart 适配清单）
- [ ] ci.yml 三处改动（format 双轨 / apt+appindicator / verbatim-diff 步）
- [ ] DEVIATIONS.md 5 行新条目（Pattern 3 清单）
- [ ] windows/test/window_frame_kit_plugin_test.cpp 占位替换（Example 4 注释含 DEV 说明）

## Sources

### Primary (HIGH — 本会话实读/实跑)
- Pub cache 逐字全读：`window_manager-0.5.2/` — lib 全 12 文件、windows/window_manager_plugin.cpp（597 行全读）、windows/window_manager.cpp（1129 行全读）、windows/CMakeLists.txt、include 头、test/window_manager_test.dart、example/{pubspec,lib/main,pages/home,utils/config,integration_test,images}、pubspec.yaml
- **/tmp/port_probe 全量彩排**：移植脚本执行 → pub get ×2 → analyze 0（含 example 注错实证范围）→ format 探针（14/19 命中清单）→ flutter test 4/4 → build windows 108.1s ✓ → exe 启动 6s Responding+清理 → dry-run 0 warnings → 双 lock 零 win32 → diff 协议 16 文件零差 + C++ 22/2 → exit 79 隔离复现（/tmp/notests_probe）
- 骨架实读：D:/window_frame_kit 全部 windows/、lib/、test/、pubspec、ci.yml、analysis_options、example/{pubspec,registrant,CMakeLists}
- Flutter SDK：sky_engine/lib/ui/window.dart:1085-1132（`window` 弃用未移除）；test_core-0.6.20/lib/src/runner.dart:164（空测试 exit 逻辑）；example/windows/CMakeLists.txt:40-46（apply_standard_settings=/W4 /WX /wd4100 + _HAS_EXCEPTIONS=0）
- tray_manager-0.5.3 实源：linux/CMakeLists.txt（appindicator FATAL_ERROR）、macos/（Package.swift 在位）、pubspec（零 win32）
- pub.dev API：bot_toast/preference_list/tray_manager/screen_retriever/cupertino_icons latest+约束（2026-09-05）

### Secondary (MEDIUM — 项目档案)
- `.planning/research/ARCHITECTURE.md`（Pattern 1/5/6、Anti-Pattern 3/6、结构草图）、`PITFALLS.md`（Pitfall 1/6/11 本 phase 适用条）、01-RESEARCH/01-01~04-SUMMARY（C-API 壳模式、承重墙补丁、registrant 同 commit 惯例、pana 基线、CI 形态）、DEVIATIONS.md（六列格式）、WINDOWS.md（#3/#4/#5 开放项）

### Tertiary (LOW)
- 无（本 phase 全部论断有一手来源；A1-A8 均带验证方法）

## Metadata

**Confidence breakdown:**
- 移植面/改名面/残留计数：HIGH — probe 全量彩排 + 逐文件 grep/diff 实测
- C++ 整合与构建：HIGH — probe build+启动双证；唯 D:/ 真实路径复现留 A4
- CI 三 leg（ubuntu/macos 半边）：MEDIUM — Windows 半边实证，另两 leg 依赖 A1/A2/A3 首推验证
- 实机行为对等（SC1-4）：MEDIUM（设计置信）— 上游代码 verbatim + tracer 已通，但逐批 UAT 才是判定（本 phase 的存在意义）

**Research date:** 2026-09-05
**Valid until:** 2026-10-05（WM 0.5.2 钉死源不漂移；Flutter 3.47.2 钉死；唯一时效敏感 = pub.dev latest 版本与 CI runner 镜像）
