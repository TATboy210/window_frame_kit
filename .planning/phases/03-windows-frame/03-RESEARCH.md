# 03-RESEARCH — Windows Frame 嫁接接入点研究(2026-09-05)

> 本会话三个只读探索代理实查产出,替代独立 research 代理轮。
> 来源:①`D:\window_frame_kit` 基座 f192dd5 仓库现状;②pub cache `window_manager-0.5.2` Windows 源码;③宿主 `D:\simple_player_flutter` 窗口层。

## 1. 接入点事实(基座仓库,行号为 f192dd5)

| 接入点 | 位置 | 现状 |
|---|---|---|
| 顶层消息 delegate | `windows/window_frame_kit_plugin.cpp:107-110` 注册,`HandleWindowProc` 135-340 | `RegisterTopLevelWindowProcDelegate` 返回 `std::optional<LRESULT>`;**只覆盖顶层窗口** |
| WM_NCCALCSIZE | plugin.cpp:147-185 | 上游三分支:全屏+非normal / `is_frameless_` / `title_bar_style_=="hidden"`;`adjustNCCALCSIZE`(65-92)用 `MonitorFromRect` + work area 内边距 8px |
| WM_NCHITTEST | plugin.cpp:186-189 | 仅 `!is_resizable_ → HTNOWHERE`;**无四边命中分发 = 干净嫁接点** |
| WM_GETMINMAXINFO | plugin.cpp:190-205 | 上游已是协作式:读 `minimum_size_`/`maximum_size_`(逻辑像素,wm.cpp:80-81)× `pixel_ratio_` 写 ptMinTrackSize/ptMaxTrackSize,return 0 |
| HWND 获取 | plugin.cpp:347-350(`ensureInitialized` 分支) | `GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT)`;**此时 view 已存在**——子类化时序无障碍,无需 CBT hook 回退 |
| 子窗口 HWND | wm.cpp:198-212 `ForceChildRefresh` 用 `GetWindow(root, GW_CHILD)`;wm.cpp:628 用 `FindWindowEx(root, L"FLUTTERVIEW")`(常量 wm.cpp:158) | 两种取法并存;FLUTTERVIEW 盖满 client area,顶边 WM_NCHITTEST 先到子窗口 |
| 全屏状态 | wm.cpp:159 `g_is_window_fullscreen`(private 成员,非 static);`IsFullScreen()` wm.cpp:564-566(public 方法) | WindowManager 类在匿名 namespace 不导出,但 FrameController 可从 plugin.cpp(同翻译单元)经实例调用 public 方法 |
| 原生全屏样式 | wm.cpp:604-606 进入摘 `WS_THICKFRAME|WS_MAXIMIZEBOX`(保留 WS_OVERLAPPEDWINDOW 其余位);615-654 退出恢复 | 样式检查会漏掉此路径 → 必须双判定 |
| SetWindowSubclass | 全仓库零使用 | FRAME-01 全新落地;宿主先例 `fullscreen_resize_guard.h`(51 行)验证可行:root+child 双 subclass、child 失败回滚 root、WM_NCDESTROY 自卸载 |

## 2. FrameController 设计要点

1. **新文件** `windows/frame_controller.h/.cpp`,独立单元,所有行带 `// FRAME:` 记账(每 marker 对应 DEVIATIONS 恰一行)
2. **纯函数抽出**(可 gtest,不触真实 HWND):
   - `FrameHitTest(x, y, rect, dpi, resizable, fullscreen)` → HT 代码。四边 `HTLEFT/HTRIGHT/HTTOP/HTBOTTOM` + 四角;边缘带宽 = `GetSystemMetricsForDpi(SM_CXSIZEFRAME,dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi)`(测试注入等价常量)
   - `FrameAdjustMinMaxInfo(info, minSize, maxSize, dpi, workRect)` → track size × DPI + **最大化时 ptMaxPosition/ptMaxSize = work area clamp**(无边框窗口系统默认按 monitor rect 最大化,致裁切/盖任务栏;协作式修复,不 return 0 架空 min/max)
   - `IsExternalFullscreen(root)` = `(GetWindowLongPtr(root, GWL_STYLE) & WS_OVERLAPPEDWINDOW) == 0`(media_kit 路径)
3. **全屏双路径禁缩放**(CONTEXT 边界,不重演 bitsdojo 无视全屏 bug):
   - `window_manager->IsFullScreen()`(插件内状态,捕获 wm.cpp:604 路径)
   - `IsExternalFullscreen(root)`(捕获 media_kit 摘样式路径)
   - 两者任一为真 → NCHITTEST 返回 HTCLIENT、SC_SIZE 吞掉
4. **消息处理顺序**(HandleWindowProc 入口,`customFrame` 开启时优先,未启用全部 fall through):
   - `WM_NCCALCSIZE` → return 0(无主题色边框/无 Win11 8px inset);IsZoomed 时按 work area 调整 rgrc
   - `WM_NCHITTEST` → 四边四角分发(子窗口侧由 subclass 处理透传)
   - `WM_GETMINMAXINFO` → 协作式完全接管(track×DPI + work-area clamp,return 0)
   - `WM_SYSCOMMAND`+SC_SIZE → 全屏时吞
5. **子窗口 subclass**(FrameController 内):`WM_NCHITTEST` 全屏/resizable-off → HTCLIENT;边缘带 → 四边四角码(子窗口坐标即父 client 坐标);`WM_NCDESTROY` 自卸载;`WM_NCCALCSIZE` 不在子窗口处理(顶层已管)
6. **Dart 面**:`WindowOptions.customFrame`(默认 null=不发,上游兼容)+ `setCustomFrame(bool)` 通道方法;`waitUntilReadyToShow` 在 titleBarStyle 之后应用
7. **宿主映射**(Phase 6 预告):宿主 media_kit 是全屏真实执行者(摘 WS_OVERLAPPEDWINDOW),不经 window_manager;guard 条件与本设计 IsExternalFullscreen 同式

## 3. verbatim gate 影响(Phase 3 起合法偏差)

- `lib/src/window_options.dart` + `lib/src/window_manager.dart` 零差门 → 精确偏差块(预期 diff 快照)
- `windows/window_frame_kit_plugin.cpp` 残留 22 行门 → 新精确值 + 委托接线块
- `windows/window_manager.cpp` 残留 2 行门 → **不动**(嫁接不碰该文件)
- 新文件 frame_controller.h/.cpp / 测试 → 不在比对集
- `DEVIATIONS.md`:#1 占位回填(GETMINMAXINFO cooperative merge);新增 WindowOptions/window_manager.dart/plugin.cpp 接线条目

## 4. 残余不确定性(实现时验证)

- hot restart 后 FLUTTERVIEW 重建,subclass 随旧 HWND 销毁(WM_NCDESTROY 自卸载);重启后 Dart 需重调 setCustomFrame——行为是否可接受实机验证
- `GetSystemMetricsForDpi` 在 Win10 1607+ 可用,项目 floor Windows 10+,无兼容问题
- 双击最大化:拖拽区由 verbatim 区 `drag_to_move_area.dart`(GestureDetector→startDragging,含 onDoubleTap 最大化)承担,不在 FrameController 范围
