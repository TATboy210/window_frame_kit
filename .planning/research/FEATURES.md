# Feature Research

**Domain:** Flutter desktop window management plugin (frame takeover + window capability API)
**Researched:** 2026-09-04
**Confidence:** MEDIUM — API surfaces verified against official pub.dev docs / package autodocs via context7 (MEDIUM) and pub.dev page fetches (LOW per seam tier); ecosystem-status claims (which packages exist/maintained) came via web search (LOW) and are explicitly flagged where unverifiable.

> 面向 downstream 的裁剪原则：本包 = window_manager 0.5.2 的 API 面（作为"门槛线"）+ bitsdojo_window 的 frame 能力（作为"入场线"）+ 若干 2026 生态没有人做对的差异化。表 stakes 判定基准 = 赛道第一 window_manager（652k 下载）——比它少就是缺陷，不是特色。

## Feature Landscape

### Table Stakes (Users Expect These)

用户从 window_manager/bitsdojo 迁移过来时默认存在的能力。缺任何一项 = 包感觉不完整，pub.dev 评论第一条就是它。

#### A. 能力 API（window_manager 基线，全部必须）

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| `ensureInitialized()` + `waitUntilReadyToShow(WindowOptions)` | 所有竞品的统一入口；WindowOptions（size/center/minimumSize/titleBarStyle/backgroundColor/title/skipTaskbar）是事实标准初始化协议 | LOW | 直接以 window_manager MIT 源码为底座 |
| 几何读写：`getSize/setSize/getPosition/setPosition/getBounds/setBounds` | window_manager 全套；几何持久化（宿主项目 window_persistence_coordinator 场景）的最底层依赖 | LOW | 底座已有；注意逻辑像素 vs 物理像素换算 |
| `setMinimumSize/setMaximumSize/setAspectRatio` | 用户期望 minSize 真正生效（bitsdojo 的 GETMINMAXINFO 无条件 return 0 恰恰架空了它——本包立项动机之一） | LOW | **协作式 GETMINMAXINFO 是硬要求**，见 Differentiator #6 |
| `center()/setAlignment(Alignment)` | 对话框式窗口、工具窗口默认体验 | LOW | 依赖 screen_retriever 的工作区信息（window_manager 同款依赖，非自建显示枚举） |
| 状态查询：`isFullScreen/isMaximized/isMinimized/isFocused/isVisible/isAlwaysOnTop/isResizable/isPreventClose/...` | UI 状态同步（ValueNotifier 驱动的宿主 UI）需要可查询镜像 | LOW | 与事件流成对出现，缺一不可 |
| 状态操作：`maximize/unmaximize/minimize/restore/setFullScreen/setResizable/setMovable/setMaximizable/setMinimizable/setClosable/setSkipTaskbar` | window_manager 基线；播放器/工具类 app 全都要 | LOW-MEDIUM | `setFullScreen` 在 frameless 窗口有已知缺陷（PRIOR-CONTEXT §六：wm.setFullScreen frameless 缺陷），v1 至少做到"正确"，零闪屏另列探索 |
| `setAlwaysOnTop(bool)` / `setAlwaysOnBottom` | 置顶小窗/画中画场景，window_manager 有完整状态位 | LOW | 底座已有 |
| **事件流 WindowListener**：`onWindowClose/Focus/Blur/Maximize/Unmaximize/Minimize/Restore/Resize/Move/EnterFullScreen/LeaveFullScreen` | 立项核心半边："可监听窗口"。几何持久化、关闭确认、状态机都靠它 | LOW-MEDIUM | 底座（window_manager）事件流完备，保留 listener 模式；宿主项目已用 onWindowClose 实现持久化-后销毁 |
| `setPreventClose(bool)` + `isPreventClose()` | 关闭拦截 → 确认对话框/后台常驻，window_manager 文档第一大 example；宿主项目 setPreventClose 钩子是关键路径 | LOW | 与 onWindowClose 成对 |
| `setTitleBarStyle(TitleBarStyle.normal/hidden, windowButtonVisibility)` + `getTitleBarHeight()` | 自定义标题栏的开关钥匙；window_manager 生态事实标准 | MEDIUM | hidden 与 frame 接管天然耦合（见依赖图）；macOS 端 windowButtonVisibility 映射 NSWindow 按钮显隐 |
| `startDragging()` | 自定义标题栏拖动窗口的唯一官方途径；MoveWindow widget 的底层 | LOW | Windows 全屏下禁用是底座现状，文档化即可 |
| `startResizing(ResizeEdge)` | 边缘拖拽从 widget 侧发起（软边缘），frame 接管后硬边缘由原生命中补 | LOW-MEDIUM | window_manager 已有，保留枚举签名 |
| `popUpWindowMenu()` | Alt+Space 系统菜单等价物；frameless 后用户唯一找回"移动/大小/最小化"的路径 | MEDIUM | window_manager 有 API；window_plus 把 frameless 下的原生系统菜单做对了，值得对齐验收标准 |
| `setTitle()/getTitle()`, `setIcon()`, `setHasShadow()`, `setOpacity/setBackgroundColor` | 常规外观面 | LOW | 底座已有 |
| `setBrightness(Brightness)` | 深色标题栏（DWMWA_USE_IMMERSIVE_DARK_MODE）；2026 年 Win11 深色 app 不做深色标题栏会被当成 bug | LOW | 底座已有，v1 必须三平台语义一致 |
| `setIgnoreMouseEvents(bool, {forward})` | click-through 悬浮窗场景（OSD、桌面小组件） | LOW | forward 参数（鼠标穿透转发）是 window_manager 细节，保留 |

#### B. Frame 接管（bitsdojo 基线，全部必须）

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| 无边框窗口：`WM_NCCALCSIZE return 0`（免系统主题色边框、免 Win11 顶部 inset） | 立项核心半边；bitsdojo 的招牌 | MEDIUM | 已验证路线（宿主双包实战 + 8px 内缩缺口根因已知，见 PRIOR-CONTEXT：插件 NCCALCSIZE 8px 内缩 / runner 仅在 WS_OVERLAPPEDWINDOW 摘除时抢先 return 0） |
| 四边等宽拖拽缩放：`WM_NCHITTEST` 命中分发（HTLEFT/HTRIGHT/HTTOP/... + 八角） | frameless 后原生缩放边缘消失，必须原生层重建 | MEDIUM | **必须由插件层接管命中**（Flutter 子窗口盖住顶边导致 runner 收不到 WM_NCHITTEST——宿主实战硬事实） |
| `minSize/maxSize` 真正生效 | 双包时代靠双通道同值绕过 bitsdojo 的 GETMINMAXINFO 缺陷；单包必须根治 | LOW | 协作式 GETMINMAXINFO：hook 内先让 DefWindowProc/其他插件处理，无主张才 return 0 |
| 拖拽区域 widget（MoveWindow 等价物） | 用户不写 platform channel 就能把标题栏区域变成拖动区 | LOW | 一个 GestureDetector→startDragging 即可；是否带 onDoubleTap→maximizeOrRestore 对齐 bitsdojo 行为 |
| 程序级 frame 开关（BDW_CUSTOM_FRAME 等价物，但**纯 Dart/插件配置**） | bitsdojo 要求改 runner main.cpp；用户期望 `flutter pub add` + main() 一行即可 | MEDIUM | 见 Differentiator #1；最低要求：runner 改动 ≤ 1 行且有文档 |

#### C. 2026 时代门槛（不来自任何单一竞品，但 pub.dev 评审会看）

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| win32 ^6.x 依赖 | 本包立项动机：解锁 file_picker 12 / flutter_secure_storage 11 等下游；锁 5.x = 复刻 bitsdojo 死因 | MEDIUM | win32 6.4.0 类型迁移清单已在宿主项目实测可用（HWND extension type 构造、GetSystemMetricsForDpi→Win32Result.value，见 PRIOR-CONTEXT §三/⑤） |
| Per-monitor DPI 正确 | Flutter 引擎 manifest 默认 PMv2；边缘宽度/minSize/几何若不按 DPI 缩放，跨显示器即错位 | MEDIUM | GetSystemMetricsForDpi 已验证可用；所有像素常量走 DPI 缩放 |
| 三平台 v1（Windows/Linux/macOS） | PROJECT.md Key Decision：避免 Windows-only 包的生态印象 | HIGH | Windows 参考实现；macOS = NSWindow styleMask.fullSizeContentView + titlebarAppearsTransparent；Linux = GTK gtk_window_set_decorated（CSD/SSD 分裂是主要工作量） |
| 最新 Flutter 3.47 / Dart 3.13 适配 | "紧跟最新"是立项卖点 | LOW | 开发环境即 3.47 |
| pub.dev 发布就绪（LICENSE 含上游 MIT 归属、README、example、CHANGELOG） | pub points/信任分 | LOW | 必须保留 window_manager 与 bitsdojo 版权声明 |

### Differentiators (Competitive Advantage)

不是"有 vs 没有"，是"只有本包有"。按影响力排序。

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **#1 单包 = frame 接管 + 事件流**（立项核心） | 截至 2026-09 无任何单包提供该组合（生态扫描 + 本次复核确认）；用户不再双包并用、不再背两套 API 与冲突风险 | MEDIUM | 组合本身工作量 = 底座 + 嫁接，真正的工程风险在**共存正确性**（GETMINMAXINFO/消息钩子不互相架空） |
| **#2 解锁 win32 6.x 依赖树** | bitsdojo 锁 ^5.1.1 拖死下游（file_picker 退 11、secure_storage 退 9.2.4）；本包 ^6.x 直接解锁 | MEDIUM | 已实测的类型迁移清单在手；这是对双包用户最直接的迁移理由 |
| **#3 Snap Layouts flyout（Win11）** | 悬停自定义最大化按钮弹出 snap 布局浮层 = `WM_NCHITTEST` 对按钮 rect 返回 `HTMAXBUTTON`（须在 HTCAPTION 之前判、窗口须保留 WS_MAXIMIZEBOX+WS_THICKFRAME、不得按 !IsZoomed 门控）。**bitsdojo/window_manager 都没做**；原生 app 用户视之为 Win11 基本礼仪 | MEDIUM | 机制文档明确（Raymond Chen / Microsoft Learn）；真正的招牌级小功能——工作量小、辨识度高 |
| **#4 可配置 resize 边缘宽度** | bitsdojo 硬编码边缘宽度（宿主魔改挂账清单第一条）；触屏/高 DPI/大边缘设计都需要 | LOW | 原生命中宽度常量 → 参数化，顺手的事 |
| **#5 协作式 GETMINMAXINFO（根治 bitsdojo 缺陷）** | bitsdojo hook 无条件 return 0 架空一切其他插件的 setMinimumSize；本包改为"先协商后兜底"，成为**可与其他窗口类插件共存**的包 | LOW | 单包化让冲突面消失大半，此项保证未来共存 |
| **#6 零 runner 配置 frameless**（v1 尽力，可降级） | bitsdojo 要改 runner main.cpp；若插件层子类化窗口即可完成 NCCALCSIZE+HITTEST 接管，则 `pub add` 即用——对 pub.dev 转化率影响巨大 | MEDIUM | 前置硬事实支持：frame 命中本来就必须由插件层接管（runner 收不到顶边命中）；风险=窗口子类化时序与 window_manager 现有消息处理的耦合，spike 验证后再承诺 |
| **#7 任务栏闪烁 `flash()`** | 吸引用户注意（FlashWindowEx）；window_manager 没有、window_plus（已消失）有——老用户会找 | LOW | 单个 Win32 调用；macOS = NSApp requestUserAttention，Linux = W atoms |
| **#8 全屏零闪屏**（探索目标，非承诺） | WS_OVERLAPPEDWINDOW 切换导致 DWM 闪老样式是 frameless 全屏的著名难题；PROJECT.md 裁决为探索目标——**成了是招牌，失败不阻塞发布** | HIGH | 上游档案：方案 A（FFI 桥）/B（DWM 禁用）实机不理想已撤回；重启前必须先采集撤回时具体症状（PRIOR-CONTEXT §六 前置条件） |
| **#9 与官方 windowing API 时代共存叙事** | 官方多窗口 API 走"多开管理"赛道，本包占"单窗口 chrome/能力"——README 明说分工，避免被误当竞品 | LOW | 纯文档工作，但决定生态定位 |

**值得吸收但降级处理的生态能力**（来自已消失的 window_plus 与 fork window_manager_plus）：
- 虚拟边框 widget（window_manager_plus `VirtualWindowFrameInit`）：非 Windows 平台 frameless 时画一圈假边框，视觉兜底 → v1.x 可选 widget
- 顶角/屏幕对齐（window_plus screen-corner alignment）→ 已被 setAlignment 覆盖，不必单独做
- click-through 区域 widget 化 → setIgnoreMouseEvents 已覆盖，widget 化 defer

### Anti-Features (Commonly Requested, Often Problematic)

明确不建。每条都是 scope creep 的真实诱因，写下来防止未来 milestone 被拉走。

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **多开窗口创建/管理**（desktop_multi_window、window_manager_plus、官方 windowing API 赛道） | "顺手支持多窗口呗"——需求真实存在 | 与本包单窗口定位完全不同的消息/生命周期模型；官方 windowing API（已实测：showDialog→独立原生窗口可用，但 AXTree 变体切换+exit127）正在收编该赛道；自建 = 与 Flutter 官方对撞 | 单窗口 chrome 做到极致；文档明示与官方 windowing 的分工（Differentiator #9） |
| **窗口按钮/标题栏 UI 组件包**（bitsdojo 的 WindowButton/WindowCaption 全家） | bitsdojo 用户习惯拿现成按钮 | 与具体设计系统耦合（fluent_ui/macos_ui 各有官方控件）；UI 进包 = 视觉演进变成包的兼容性负担，违背"只做好一件事" | 提供 MoveWindow/拖拽区等**行为** widget（非视觉）；按钮让位给宿主的设计系统。example 里给参考实现即可 |
| **窗口特效**（acrylic/mica/aero glass——flutter_acrylic 的地盘） | Win11 质感诉求强烈，fluent_ui 用户常问 | flutter_acrylic 已成熟且与本包天然搭配（其 README 官方示例就是 windowManager.ensureInitialized + Window.setEffect）；重复造 = 维护三平台 DWM/GTK/NSVisualEffectView 矩阵 | v1 预留钩子（setBackgroundColor 通路 + 文档指引 flutter_acrylic 搭配）；v1.x 若社区呼声高再评估 Windows-only mica 快捷路径（DwmSetWindowAttribute 单调用，成本其实不高） |
| **Web/mobile 支持** | "pub.dev 加个平台徽章好看" | 移动端无窗口概念，web 无原生消息钩子；伪支持 = 三平台接口里塞 no-op 污染 API 语义（flutter_windowmanager_plus 等即此类） | 桌面三平台专属，pubspec 平台声明明示；检测到其他平台 fail-fast 报错 |
| **bitsdojo/window_manager API 兼容层** | 平滑迁移的表面吸引力 | PROJECT.md 已裁决不背历史包袱；兼容层固化旧 API 的坏设计（appWindow 全局单例等） | README 给逐行迁移映射表（appWindow.minSize → controller.setMinimumSize 等），比兼容层诚实且零维护 |
| **显示/显示器枚举自成一套 API** | 多显示器需求真实 | screen_retriever（leanflutter，window_manager 同款依赖）已做且是生态事实标准；重复造 = 与底座依赖树打架 | 直接依赖 screen_retriever；工作区/多显示器正确性是本包义务（Table Stakes C），枚举 API 不是 |
| **托盘/菜单/对话框捆绑** | nativeapi（window_manager 官方后继）把 tray/menu/preferences 全捆一起 | 违背"只做好一件事"；tray_manager 等独立包已成熟 | 只做窗口；README 推荐搭配组合 |

## Feature Dependencies

```
[NCCALCSIZE frame 接管]
    └──requires──> [插件层 WM_NCHITTEST 命中接管]
                       └──requires──> [DPI 缩放的边缘几何]
                       └──enhances──> [#3 HTMAXBUTTON Snap Layouts]
                       └──enhances──> [#4 可配置边缘宽度]

[setTitleBarStyle(hidden)] ──requires──> [NCCALCSIZE frame 接管 + startDragging]
                                            （hidden 后无原生标题栏，拖动/缩放全靠本包）

[setMinimumSize/maxSize 生效] ──requires──> [#5 协作式 GETMINMAXINFO]
                                                └──conflicts──> [bitsdojo 式无条件 return 0 hook]
                                                     （同窗口只能有一个 GETMINMAXINFO 主张——单包化的根源理由）

[#2 win32 ^6.x] ──blocks──> [一切 Windows 原生工作]
                                （先迁移类型再写新代码，否则返工）

[setPreventClose] ──requires──> [onWindowClose 事件流]

[setFullScreen 正确性] ──requires──> [frame 接管下的 WS_OVERLAPPEDWINDOW 处理]
[#8 全屏零闪屏] ──conflicts──> [setAsFrameless 式样式重设路径]
                                （样式切换即闪；零闪屏探索须绕开"重设样式"范式——宿主方案 A/B 教训）

[三平台 v1] ──requires──> [平台分支接口抽象（WindowBridge 式 interface）]
                              （Windows 参考实现先行，macOS/Linux 平行填充）
```

### Dependency Notes

- **frame 接管 requires 命中接管（不可拆卖）**：只做 NCCALCSIZE 不做 HITTEST = 无边框但无法缩放（bitsdojo 早期 version 的著名残废态）。两者必须同一 phase 落地。
- **#3 Snap Layouts enhances 命中接管**：HTMAXBUTTON 是 WM_NCHITTEST 分发里的一个 case，搭车实现，不必独立 phase；但它是**独立可验收的特性**（对比竞品的演示点）。
- **#5 GETMINMAXINFO conflicts 无条件 return 0**：这是"单包 vs 双包"的技术根源论证——双包必然抢这个消息，只有单包能根治。写进 README 的"Why"。
- **#6 零 runner 配置 conflicts 现有 window_manager waitUntilReadyToShow 时序**：插件子类化窗口与底座的 hidden/ready 时序有纠缠（宿主 8-03 白边修复记忆：hidden 会重置 is_frameless_ 须在其后调）——spike 后才能承诺，承诺前 #6 降级为"runner 一行配置"。
- **#8 零闪屏 conflicts setFullScreen 现有实现范式**：探索目标独立排期，绝不混入 v1 全屏正确性工作（PROJECT.md 裁决：非硬指标，不阻塞发布）。

## MVP Definition

### Launch With (v1)

v1 = "能替换宿主项目双包架构" + "pub.dev 上站得住"。

- [ ] **Windows 参考实现全量**：NCCALCSIZE frame + WM_NCHITTEST 边缘 + 协作式 GETMINMAXINFO + 可配置边缘宽度 + win32 ^6.x（Table Stakes A+B+C 的 Windows 侧）
- [ ] **window_manager-parity 能力 API + 事件流**（上表 A 区全部）——以底座源码保留为起点，不重写
- [ ] **startDragging + 拖拽区行为 widget**（B 区）
- [ ] **三平台 v1 分支**：macOS（NSWindow fullSizeContentView 路线）/ Linux（GTK 路线）各自最小可用 frame + 同名 API，实机冒烟过
- [ ] **#3 HTMAXBUTTON Snap Layouts**（Windows）——小成本招牌
- [ ] **pub.dev 发布包**：MIT + 双上游归属、README（含双包迁移映射表）、example、CHANGELOG
- [ ] **宿主验收**：simple_player_flutter 双包 → 本包替换，`flutter analyze 0 error` + 全量 test + 实机 resize/全屏冒烟（PROJECT.md 质量红线）

### Add After Validation (v1.x)

- [ ] **#6 零 runner 配置**（插件层完全接管，runner 零改动）——触发条件：spike 验证子类化时序与 window_manager 初始化共存
- [ ] **虚拟边框 widget**（frameless 非 Windows 平台视觉兜底，吸收自 window_manager_plus）——触发条件：Linux/macOS 用户反馈视觉突兀
- [ ] **#7 任务栏闪烁**（本在 v1 候选，若三平台工作量挤压则顺延）——触发条件：单 Windows call 的窗口期
- [ ] **Windows-only mica/acrylic 快捷路径评估**——触发条件：issue 区呼声 + flutter_acrylic 生态状态复查
- [ ] **popUpWindowMenu 在 frameless 下的原生菜单打磨**——触发条件：宿主实机验收时发现的差距

### Future Consideration (v2+)

- [ ] **#8 全屏零闪屏**——前置：采集宿主方案 A/B 撤回时具体症状（PRIOR-CONTEXT §六硬前置）；成了是招牌，永远不承诺
- [ ] **窗口特效全平台矩阵**——前置：Windows-only 路径验证有需求；否则让 flutter_acrylic 继续拥有这块
- [ ] **与其他窗口类插件共存保证的官方测试矩阵**（多个 GETMINMAXINFO 主张协商的回归测试集）——前置：真实共存 issue 出现

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| NCCALCSIZE frame + WM_NCHITTEST 边缘（插件层） | HIGH | MEDIUM | P1 |
| 能力 API parity + 事件流（A 区） | HIGH | LOW-MEDIUM（底座在手） | P1 |
| 协作式 GETMINMAXINFO | HIGH | LOW | P1 |
| win32 ^6.x 迁移 | HIGH | MEDIUM（清单已验证） | P1 |
| setPreventClose + onWindowClose | HIGH | LOW | P1 |
| setFullScreen 正确（frameless 缺陷修掉） | HIGH | MEDIUM | P1 |
| 三平台 v1 分支 | HIGH（用户裁决） | HIGH | P1（最小可用） |
| HTMAXBUTTON Snap Layouts | MEDIUM-HIGH | MEDIUM | P1.5 |
| 可配置边缘宽度 | MEDIUM | LOW | P1.5 |
| startDragging + 拖拽区 widget | HIGH | LOW | P1 |
| pub 发布包 + 迁移文档 | HIGH | LOW | P1 |
| 零 runner 配置 | HIGH（转化率） | MEDIUM | P2 |
| 任务栏闪烁 | MEDIUM | LOW | P2 |
| 虚拟边框 widget | LOW-MEDIUM | LOW | P2 |
| mica/acrylic 快捷路径 | MEDIUM | LOW-MEDIUM | P3 |
| 全屏零闪屏 | HIGH（成了的话） | HIGH | P3（探索，永不承诺） |

**Priority key:** P1 = Must have for launch; P2 = Should have, add when possible; P3 = Nice to have / future consideration

## Competitor Feature Analysis

| Feature | window_manager 0.5.2 | bitsdojo_window 0.1.6 | window_manager_plus 1.0.5 | window_plus（已消失） | **window_frame_kit** |
|---------|----------------------|------------------------|----------------------------|----------------------|----------------------|
| Frame 接管（NCCALCSIZE） | setAsFrameless（样式重设路线，有闪现/缺陷面） | **有**（招牌） | 部分（TitleBarStyle.hidden + DragToResizeArea widget 软边缘） | 有（原生 HITTEST） | **有**（原生，插件层） |
| 原生边缘缩放 | 无（靠系统或 widget 软边缘） | **有**（硬编码宽度） | widget 软边缘（DragToResizeArea） | **有** | **有 + 宽度可配** |
| 能力 API 完整度 | **最高**（见 Table Stakes A） | 低（appWindow 小面） | 高（fork window_manager + 多窗口） | 中 | **window_manager parity** |
| 事件流 | **完备**（WindowListener） | 无 | 完备 + 跨窗口事件 | 有 | 完备（继承底座） |
| 维护状态 | 活跃（2 个月前发布）但**已挂迁移公告 → nativeapi** | 死（2023-12 最后一发，锁 win32 5.x） | 低频（23 个月前） | **404（pub+GitHub 2026-09-04 双确认）** | 本包立项即解 |
| win32 版本 | 6.x 兼容 | 锁 ^5.1.1 | — | — | **^6.x** |
| 多窗口 | 无 | 无 | **有**（createWindow/invokeMethodToWindow） | 无 | **明确不做**（anti-feature） |
| Snap Layouts (HTMAXBUTTON) | 无 | 无 | 未证实 | 宣称支持 | **P1.5 承诺** |
| minSize 生效 | 有（但会被 bitsdojo 架空） | **被自家 hook 架空** | 有 | 有 | **协作式根治** |
| 官方后继风险 | 迁往 nativeapi 0.2.0（FFI 路线，11 天前首版，无 frameless/fullscreen 确认） | 无 | 无 | 已死 | 本包即"window_manager 路线的 frame 增补延续" |

**生态结论（2026-09-04 复核）**：赛道第一 window_manager 活跃但方向转向 nativeapi；bitsdojo 死锁旧依赖；plus 系要么消失要么转向多窗口。**"frame 接管 + 事件流 + 新依赖链"的组合空位比立项扫描时更空**——此前记录的"唯一备胎" window_plus 本次双 404，无法确认仍可用（LOW confidence，但两路独立核查一致）。

## Sources

- [window_manager 0.5.2 — pub.dev 包页](https://pub.dev/packages/window_manager)（0.5.2、2 个月前发布、652k 下载、迁移公告）— webfetch, LOW
- [window_manager — WindowManager 类 API 参考](https://pub.dev/documentation/window_manager/latest/window_manager/WindowManager-class.html)（全方法面）— webfetch, LOW；与 context7 /leanflutter/window_manager（quick-start/docs, MEDIUM）交叉一致
- [bitsdojo_window — GitHub autodocs（context7 /bitsdojo/bitsdojo_window, MEDIUM）](https://github.com/bitsdojo/bitsdojo_window)（WindowBorder/MoveWindow/WindowButton/appWindow API）
- [desktop_multi_window 0.2.1 — pub.dev](https://pub.dev/packages/desktop_multi_window)（rustdesk-org，多窗口）— websearch, LOW
- [window_manager_plus — pub.dev 搜索页 + context7 /pichillilorenzo/window_manager_plus](https://pub.dev/packages?q=window_plus)（多窗口 fork、DragToResizeArea/VirtualWindowFrameInit）— context7 MEDIUM / websearch LOW
- window_plus / flutter_native_window：pub.dev 直链与搜索均 404/无匹配（2026-09-04 实测）— webfetch+websearch, LOW（存在性结论：无法确认）
- [flutter_acrylic — pub.dev](https://pub.dev/packages/flutter_acrylic)（WindowEffect 矩阵：transparent/aeroGlass/acrylic/mica/tabbed/disabled/solid；macOS NSVisualEffectView 材质）— websearch, LOW
- Snap Layouts / HTMAXBUTTON 机制：Microsoft Learn WM_NCHITTEST 文档、Raymond Chen (The Old New Thing) snap layouts 系列 — websearch, LOW
- Flutter Windows resize 闪烁：flutter/flutter#35902 系（DirectComposition 迁移后大幅缓解未根绝）；PMv2 默认启用 — websearch, LOW
- PRIOR-CONTEXT-window-layer.md（宿主实战硬事实：8px 内缩根因、GETMINMAXINFO 缺陷、插件层命中接管必要性、win32 6.4 迁移清单、全屏方案 A/B 撤回档案）— 项目内部档案, HIGH
- 上游维护信号：nativeapi 0.2.0（leanflutter.dev，2026-08 发布，FFI 统一核心）— webfetch, LOW

---
*Feature research for: window_frame_kit — Flutter desktop frame takeover + window capability plugin*
*Researched: 2026-09-04*
