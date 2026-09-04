# Requirements: window_frame_kit

**Defined:** 2026-09-04
**Core Value:** 一个包同时提供 frame 接管 + 完整事件流能力——用户不再需要在"无边框窗口"和"可监听窗口"之间二选一，也不再被上游锁死的 win32 5.x 拖累整个依赖树。

## v1 Requirements

v1 = "能替换宿主项目双包架构" + "pub.dev 上站得住"。技术路线：window_manager 0.5.2 fork 为底座（verbatim port 保上游 merge 路径），嫁接 bitsdojo frame 代码（~830 行，`// FRAME:` 标记）。

### Frame 接管（Windows 参考实现）

- [ ] **FRAME-01**: Windows 无边框 frame 接管——WM_NCCALCSIZE return 0（免系统主题色边框、免 Win11 顶部 inset），由插件层实现（RegisterTopLevelWindowProcDelegate + 子窗口 SetWindowSubclass），目标零 runner 改动；若 spike 验证子类化时序失败则降级为 runner ≤1 行配置并文档化
- [ ] **FRAME-02**: Windows 四边等宽原生拖拽缩放——插件层 WM_NCHITTEST 命中分发（HTLEFT/HTRIGHT/HTTOP/HTBOTTOM + 四角），FLUTTERVIEW 子窗口顶边命中可达（HTTRANSPARENT 透传 → 父窗口分区码）
- [ ] **FRAME-03**: 协作式 GETMINMAXINFO——setMinimumSize/setMaximumSize 在 frame 模式下真正生效，不再被无条件 return 0 架空（根治 bitsdojo 缺陷），升级 per-window DPI（GetDpiForWindow）
- [ ] **FRAME-04**: 拖拽区域行为 widget——MoveWindow 等价物（GestureDetector→startDragging），纯行为非视觉组件；窗口按钮/标题栏视觉件不在范围（让位宿主设计系统）

### 能力 API（window_manager parity）

- [ ] **CAPB-01**: 初始化协议——ensureInitialized() + waitUntilReadyToShow(WindowOptions)（size/center/minimumSize/titleBarStyle/backgroundColor/title/skipTaskbar）
- [ ] **CAPB-02**: 几何读写全套——getSize/setSize/getPosition/setPosition/getBounds/setBounds/center/setAlignment（逻辑像素换算正确）
- [ ] **CAPB-03**: min/max 尺寸与宽高比——setMinimumSize/setMaximumSize/setAspectRatio
- [ ] **CAPB-04**: 状态查询全套——isFullScreen/isMaximized/isMinimized/isFocused/isVisible/isAlwaysOnTop/isResizable/isPreventClose 等
- [ ] **CAPB-05**: 状态操作全套——maximize/unmaximize/minimize/restore/setFullScreen/setResizable/setMovable/setMaximizable/setMinimizable/setClosable/setSkipTaskbar（setFullScreen 在 frameless 下正确）
- [ ] **CAPB-06**: WindowListener 事件流——onWindowClose/Focus/Blur/Maximize/Unmaximize/Minimize/Restore/Resize/Move/EnterFullScreen/LeaveFullScreen 全 11 hook
- [ ] **CAPB-07**: 关闭拦截——setPreventClose(bool) + isPreventClose()，与 onWindowClose 成对
- [ ] **CAPB-08**: 标题栏样式——setTitleBarStyle(normal/hidden, windowButtonVisibility) + getTitleBarHeight()
- [ ] **CAPB-09**: 拖拽发起——startDragging() + startResizing(ResizeEdge)
- [ ] **CAPB-10**: 外观与置顶——setTitle/getTitle/setIcon/setHasShadow/setOpacity/setBackgroundColor/setBrightness/setAlwaysOnTop/setAlwaysOnBottom
- [ ] **CAPB-11**: 鼠标穿透——setIgnoreMouseEvents(bool, {forward})
- [ ] **CAPB-12**: 系统菜单——popUpWindowMenu()（frameless 下用户找回移动/大小/最小化的路径）

### 跨平台与依赖树

- [ ] **XPLAT-01**: 适配最新 Flutter 3.47 / Dart 3.13（SDK floor 在规划期按实际依赖链确定并锁定）
- [ ] **XPLAT-02**: 零 win32 Dart 依赖（C++-only 架构）——pubspec 仅 path + screen_retriever；下游依赖树因此解锁 win32 6.x / file_picker 12 / flutter_secure_storage 11（2026-09-04 裁决）
- [ ] **XPLAT-03**: Per-monitor DPI 正确——边缘几何/minSize/几何换算全部走 DPI 缩放（GetSystemMetricsForDpi / GetDpiForWindow）
- [ ] **XPLAT-04**: macOS 分支最小可用——NSWindow styleMask.fullSizeContentView + titlebarAppearsTransparent，同名 API 全通，实机冒烟过
- [ ] **XPLAT-05**: Linux 分支最小可用——GTK gtk_window_set_decorated(false) 路线，同名 API 全通，实机冒烟过

### 发布就绪

- [x] **PUB-01**: LICENSE 叠加双上游 MIT 归属——window_manager 与 bitsdojo 版权声明逐字保留（pub.dev 不验证，须自查）
- [ ] **PUB-02**: README——含双包迁移映射表（appWindow.minSize → controller.setMinimumSize 等，替代兼容层）+ 与官方 windowing API 分工说明
- [ ] **PUB-03**: example app——演示全 API 面，兼作集成测试宿主
- [ ] **PUB-04**: 发布管线——CHANGELOG + 语义化版本 + pana 静态检查通过 + `flutter pub publish --dry-run` 通过

### 宿主验收

- [ ] **HOST-01**: simple_player_flutter 双包替换——bitsdojo_window + window_manager → window_frame_kit，flutter analyze 0 error + 全量测试 + 实机 resize/全屏/关闭持久化冒烟

## v2 Requirements

已识别、验证后追加（触发条件见 `.planning/research/FEATURES.md` v1.x 节）。

### 差异化增强

- **DIFF-01**: Snap Layouts flyout——WM_NCHITTEST 对最大化按钮 rect 返回 HTMAXBUTTON（Win11 snap 布局浮层；须在 HTCAPTION 之前判、保留 WS_MAXIMIZEBOX+WS_THICKFRAME）
- **DIFF-02**: 可配置 resize 边缘宽度——命中宽度常量参数化（bitsdojo 硬编码）
- **DIFF-03**: 零 runner 配置承诺——若 v1 spike 降级走了 runner 一行路线，此为移除该行的后续目标
- **DIFF-04**: 任务栏闪烁 flash()——FlashWindowEx（Win）/ NSApp requestUserAttention（mac）/ X atoms（Linux）

### 探索目标（永不承诺）

- **EXPL-01**: 全屏零闪屏——绕开"样式重设"范式（WS_OVERLAPPEDWINDOW 切换即闪；宿主方案 A/B 实机撤回）；前置：采集撤回时具体症状
- **EXPL-02**: 虚拟边框 widget——非 Windows frameless 视觉兜底（吸收自 window_manager_plus）
- **EXPL-03**: 与其他窗口类插件共存的官方回归测试矩阵——多 GETMINMAXINFO 主张协商

## Out of Scope

| Feature | Reason |
|---------|--------|
| 多开窗口创建/管理 | 官方 windowing API 赛道（已实测可用但 AXTree/exit127 风险）；单窗口 chrome 定位，README 明示分工 |
| 窗口按钮/标题栏 UI 组件包 | 与设计系统耦合（fluent_ui/macos_ui 各有官方控件）；UI 进包 = 视觉演进变兼容性负担 |
| 窗口特效（acrylic/mica/blur） | flutter_acrylic 已成熟且天然搭配本包；维护三平台 DWM/GTK/NSVisualEffectView 矩阵违背"只做好一件事" |
| Web/mobile 支持 | 移动端无窗口概念，web 无原生消息钩子；伪支持 = no-op 污染 API 语义；检测到非桌面平台 fail-fast |
| bitsdojo/window_manager API 兼容层 | 固化旧 API 坏设计（appWindow 全局单例）；README 迁移映射表更诚实且零维护 |
| 显示器枚举自建 API | screen_retriever 是生态事实标准且为底座既有依赖；重复造 = 依赖树打架 |
| 托盘/菜单/偏好设置捆绑 | nativeapi 后继的路线；tray_manager 等独立包已成熟 |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PUB-01 | Phase 1 | Complete |
| CAPB-01 | Phase 2 | Pending |
| CAPB-02 | Phase 2 | Pending |
| CAPB-03 | Phase 2 | Pending |
| CAPB-04 | Phase 2 | Pending |
| CAPB-05 | Phase 2 | Pending |
| CAPB-06 | Phase 2 | Pending |
| CAPB-07 | Phase 2 | Pending |
| CAPB-08 | Phase 2 | Pending |
| CAPB-09 | Phase 2 | Pending |
| CAPB-10 | Phase 2 | Pending |
| CAPB-11 | Phase 2 | Pending |
| CAPB-12 | Phase 2 | Pending |
| FRAME-01 | Phase 3 | Pending |
| FRAME-02 | Phase 3 | Pending |
| FRAME-03 | Phase 3 | Pending |
| FRAME-04 | Phase 3 | Pending |
| XPLAT-02 | Phase 3 | Pending |
| XPLAT-03 | Phase 3 | Pending |
| XPLAT-01 | Phase 4 | Pending |
| XPLAT-04 | Phase 4 | Pending |
| XPLAT-05 | Phase 4 | Pending |
| PUB-02 | Phase 5 | Pending |
| PUB-03 | Phase 5 | Pending |
| PUB-04 | Phase 5 | Pending |
| HOST-01 | Phase 6 | Pending |

**Coverage:**

- v1 requirements: 26 total（2026-09-04 修正：原"22 total"为计数笔误）
- Mapped to phases: 26
- Unmapped: 0 ✓

映射说明：

- PUB-01 → Phase 1：Pitfall 2 要求 LICENSE stack 在首个含嫁接代码的 commit 之前落盘；Phase 5 发布门会再次复核归属
- XPLAT-02 → Phase 3：pubspec 依赖约束从 Phase 1 骨架期生效，完成验证随 Frame 嫁接单元落地
- XPLAT-01 → Phase 4：三平台分支齐备后"适配最新 Flutter/Dart"才可完整验证

---
*Requirements defined: 2026-09-04*
*Last updated: 2026-09-04 roadmap 创建时填充 Traceability*
