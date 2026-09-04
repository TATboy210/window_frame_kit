# Phase 1: 包骨架与许可基座 - Research

**Researched:** 2026-09-04
**Domain:** Flutter plugin scaffolding (`flutter create --template=plugin`), MIT attribution stacking, pub.dev publish pipeline (pana / publish --dry-run), GitHub Actions 三平台 CI
**Confidence:** HIGH — 本文件所有关键论断都在本会话用真实工件验证过：实际跑了 `flutter create`（Flutter 3.47.2，91 文件逐一检视）、实际跑了 `dart pub publish --dry-run`（修复前后各一次）、实际装了 pana 0.23.19 并发现其 Windows 不可用、实际完成了 example 的 `flutter build windows` 全流程（含两个真实坑的修复）、逐字读取了 pub cache 中两个上游 LICENSE。标注 `[ASSUMED]` 的仅剩 GitHub 云端 runner 行为等本机无法验证项。

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**包身份与仓库**
- **D-01:** GitHub 仓库放个人账号 `github.com/TATboy210/window_frame_kit`——pubspec 的 repository/homepage/issues 字段全部指向它；个人号直接发 pub.dev 完全成立 — **Reversibility:** one-way — pub.dev 包页与已发布版本会永久链接首次上传的 repository URL，改仓库名=断链历史
- **D-02:** LICENSE 三段式叠加：window_manager 版权行（逐字）+ bitsdojo_window 版权行（逐字）+ 本包版权行 `Copyright (c) 2026 TATboy210`；MIT 正文一份共享，三行版权声明并排
- **D-03:** pubspec description 用英文（pub.dev 搜索主语种），覆盖关键词：frameless / frame takeover / window management / events / Windows/Linux/macOS——具体措辞规划期定稿
- **D-04:** pubspec topics 全量 5 个：`desktop, window, window-manager, frameless, custom-titlebar`（pub.dev 上限 5 个，从中选组合）

**CI 矩阵**
- **D-05:** GitHub Actions 从骨架期即上**三平台全量矩阵**（windows / ubuntu / macos runner 并行）——三平台同步 v1 是硬需求，平台问题越早暴露越好；公共仓库三平台 runner 全免费，无额度顾虑
- **D-06:** 触发时机 = push 到主分支 + 所有 PR（反馈最及时）
- **D-07:** 每平台跑 4 项检查：① `flutter analyze` + `dart format --set-exit-if-changed` ② `flutter test`（Dart 单测；channel/API 契约测试可无头跑，视觉窗口行为留给实机清单）③ pana 静态评分（发布分数从第一天守护）④ `flutter build windows/linux/macos` native 编译验证（抓模板/CMake 问题）
- **D-08:** 注意：骨架期 Linux/macOS 的 build 在纯 GitHub runner 上可行（模板自足），但 pana 分数里 example/README 完整度分项在 Phase 5 才满分——CI 里 pana 设下限（通过线）而非满分级

**上游同步策略（讨论中被跳过，按推荐默认执行）**
- **D-09:** upstream remote 直接指 `https://github.com/leanflutter/window_manager`（用户 GitHub fork 不是必须——remote 指上游即可 cherry-pick/对 diff）；DEVIATIONS.md 条目格式 = 上游 file:line 锚点 + 偏离理由，按功能块记录（每处 `// FRAME:` 标记对应一条）— **Reversibility:** costly — remote/账本结构后期改动要重写全部偏差记录

### Claude's Discretion
- README Derivation 节的具体排版
- DEVIATIONS.md 的 frontmatter/表格样式（只要含 file:line 锚点 + 理由即可）
- CI workflow 文件拆分（单文件 matrix vs 多文件）
- example app 的最小占位内容（Phase 5 才强化）
- pubspec description 的最终英文措辞（含 D-03 关键词即可）

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope

**CONTEXT.md Specifics（生效条款）：**
- LICENSE 必须在**首个含嫁接代码的 commit 之前**落盘
- channel 名 `window_frame_kit`——绝不复用 `window_manager`
- pubspec 依赖白名单：`path` + `screen_retriever`，任何 win32 出现都视为违规（C++-only 架构的骨架期生效条款）

**注意一处文档冲突（已由 STATE.md 2026-09-04 裁决解决）：** `.claude/CLAUDE.md` 的 Technology Stack 节仍写着 "Dependencies: win32 ^6.x"（生成自旧版 STACK.md），但 REQUIREMENTS.md XPLAT-02 + CONTEXT.md specifics + STATE.md Decisions 已裁决为 **C++-only 零 win32 Dart 依赖**。规划以 CONTEXT/REQUIREMENTS 为准，不按 CLAUDE.md 的 stack 节执行。
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PUB-01 | LICENSE 叠加双上游 MIT 归属——window_manager 与 bitsdojo 版权声明逐字保留（pub.dev 不验证，须自查） | 两条上游版权行已从 pub cache LICENSE 逐字读出（见 §Code Examples Example 1）；MIT 多版权行叠加惯例已核实（每持有人一行、保留全部、新增自己的行）；模板自带的 `LICENSE`（BSD-3）需整体替换 |
</phase_requirements>

## Summary

本 phase 的全部内容是"把坑先踩掉"：本次研究**在本机实际执行了** `flutter create --template=plugin --platforms=windows,linux,macos`、`dart pub publish --dry-run`（修复前后）、pana 安装与运行、以及 example 的完整 `flutter build windows`——发现并验证了两个**模板自带的构建阻断**（CMake 4.x 撞 gtest 1.11、native_assets 目录缺失导致 INSTALL 失败）和一个 CI 级别的事实（pana 在 Windows 本机完全跑不起来）。这些都是执行者裸跑模板时必然撞上的问题，现已全部拿到已验证的解法。

模板结构方面有一个对 Phase 2 至关重要的发现：Flutter 3.47 的 Windows 插件模板用 **C-API 包装层模式**——pubspec 的 `pluginClass` 是 `WindowFrameKitPluginCApi`（薄壳），真正的插件类是内层 `WindowFrameKitPlugin`；而上游 window_manager 0.5.2 三个平台都是单一 `WindowManagerPlugin`、没有 C-API 壳。Phase 2 移植时必须保留模板壳、把 WM 逻辑装进内层类，而不是照抄上游的注册方式。另外好消息：**channel 名由包名自动派生**，`flutter create` 生成的 Dart/C++/GTK/Swift 四处默认就是 `window_frame_kit`，D 的红线要求自动满足（前提是 scaffold 用这个包名）。

发布管线从骨架期即可全绿已验证：`flutter pub publish --dry-run` 在补齐 4 个 pubspec 字段（description/homepage/repository/issue_tracker + topics）后从 1 warning 变 **0 warnings**；pana 走 CI（ubuntu runner），通过线用 `--exit-code-threshold` 设定。LICENSE 三行叠加的两条上游版权行已逐字核实，MIT 叠加惯例（每持有人一行、保留上游全部、追加自己的行）有多个来源印证。

**Primary recommendation:** 按"先 LICENSE → 再 scaffold → 立即修 CMake 坑 → 补 pubspec 四字段 → 建 CI（pana 仅 ubuntu、threshold 记录基线后钉死）→ 实机启动 example"的顺序执行；不要在本机 Windows 跑 pana（已验证不可行）；把 `plugin_platform_interface` 明确写进依赖说明避免被误判为白名单违规。

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| 包骨架与目录结构 | scaffold（`flutter create`） | 手工模板清理 | 模板自足生成三平台；清理仅限占位字段 |
| channel 命名 `window_frame_kit` | scaffold（包名派生） | — | 四处（Dart/Win/Linux/macOS）自动一致，无需手改 [VERIFIED: 本会话 scaffold 检视] |
| LICENSE 归属 | 仓库根 LICENSE 文件 | README Derivation 节 | MIT 义务 = 保留版权与许可声明；pub.dev 不校验 |
| 上游合并路径 | git remote 配置 | DEVIATIONS.md | D-09；remote + 账本是嫁接纪律的载体 |
| 发布管线（dry-run/pana） | CI（ubuntu runner） | 本机（仅 dry-run 可用） | pana 0.23.19 在 Windows 本机必然崩溃 [VERIFIED: 本会话两次复现] |
| 三平台 native 编译验证 | CI 三 runner | 本机 Windows | Linux/macOS 编译只能靠 CI（本机无 GTK/Xcode） |
| 实机启动验证 | 人（实机清单） | — | SC1 是实机项；CI 无头不可判（宿主 UAT 证据标准适用） |

## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Flutter SDK | 3.47.2 stable（D:/flutter，比项目文档写的 3.47.0 新一个 patch）[VERIFIED: `flutter --version` 本会话] | scaffold 与开发 | 项目钦定环境；模板输出以它为准 |
| Dart SDK | 3.13.2 | 语言 | 随 Flutter；模板 pubspec 生成 `sdk: ^3.13.2` [VERIFIED: 模板输出 pubspec.yaml:7] |
| `flutter create --template=plugin --platforms=windows,linux,macos` | 同 SDK | 骨架生成 | 91 文件全量生成，结构见 §Architecture Patterns [VERIFIED: 本会话实跑] |
| screen_retriever | ^0.2.2 | 显示器信息（唯一功能性依赖） | 上游 WM 0.5.2 同款约束；其 pubspec 声明 `sdk: ">=3.0.0 <4.0.0"`、`flutter: ">=3.3.0"`，不会抬高本包 floor [VERIFIED: pub cache screen_retriever-0.2.2/pubspec.yaml:11-13] |
| path | ^1.9.1 | 白名单依赖（WM 同款） | cache 中最新 1.9.1 [VERIFIED: pub cache 目录] |
| plugin_platform_interface | ^2.0.2（模板自带） | 模板 Dart 平台接口层 | 模板 `WindowFrameKitPlatform` 模式所需；**非 win32**，需在计划中注明以免被误判违规（见 §Open Questions 3）[VERIFIED: 模板 pubspec.yaml:13] |
| pana | 0.23.19（`dart pub global activate pana`） | 发布分数检查 | pub.dev 官方评分工具；仅 CI Linux 可跑（见 Pitfall 4）[VERIFIED: 本会话安装+运行] |
| GitHub Actions | actions/checkout@v5 + subosito/flutter-action@v2 | CI | v5 为当前大版本（Node 24，2025-08）[CITED: github.com/actions/checkout/releases/tag/v5.0.0]；flutter-action v2 当前（v2.22.2，2025-12）[CITED: github.com/subosito/flutter-action/releases/tag/v2.22.2] |

### Supporting
| Tool | Purpose | When to Use |
|---------|---------|-------------|
| flutter_lints ^6.0.0 | 模板自带 dev_dep，满足 pana analysis 分项 | 保持模板值即可 [VERIFIED: 模板 pubspec.yaml:18] |
| gh CLI 2.94.0 | 创建 D-01 仓库 + `git remote add` | 建仓库任务 [VERIFIED: `gh --version`] |
| git 2.54.0 | upstream remote（D-09） | `git remote add upstream https://github.com/leanflutter/window_manager` [VERIFIED: 上游 raw 文件可访问，本会话 200 OK] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| 模板 Dart 三件套（platform_interface + method_channel） | 直接上 WM 的单文件 facade 结构（Phase 2 反正要换） | 骨架期保持模板件可让 `flutter create` 后续 add-platform 等工具链一致；Phase 2 移植时再替换 |
| CMake 修复用 `CMAKE_POLICY_VERSION_MINIMUM 3.5` 一行 | 把 googletest URL 升到 release-1.14+（其 min 3.13 兼容 CMake 4） | 一行方案 diff 最小且已验证；升级 gtest 可顺带消掉 deprecation warning，但引入新下载与新变量。骨架期推荐一行，升级 gtest 留作后续可选 |
| DEVIATIONS.md | THIRD-PARTY-NOTICES.md / NOTICE | D-09 已锁 DEVIATIONS.md；且 NOTICE 族文件语义是"第三方声明"而非"上游偏离账本" |

**Installation:**
```bash
# 骨架（在 D:\window_frame_kit 已有 git 仓库根执行；注意 --org 只影响 example 的 bundle id）
flutter create --template=plugin --platforms=windows,linux,macos --org com.example .
# 依赖（pubspec 内声明）：path ^1.9.1 / screen_retriever ^0.2.2 / plugin_platform_interface ^2.0.2
flutter pub get
# 本机可跑的两道发布检查
dart pub publish --dry-run     # 修复 4 字段后 0 warnings [VERIFIED]
dart format . && flutter analyze && flutter test
# pana 只能跑在 CI ubuntu runner（见 Pitfall 4）
```

**Version verification:** 本会话已对全部推荐包用 pub cache 实源 + registry 核实（screen_retriever 0.2.2 / path 1.9.1 / pana 0.23.19 / flutter_lints ^6.0.0 模板生成即当前主版本）。

## Package Legitimacy Audit

> 本 phase 引入的外部包全部来自 `flutter create` 模板或上游 WM 0.5.2 的既有依赖树，无 WebSearch/训练记忆新发现的包名。

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| screen_retriever | pub.dev | WM 同款 0.2.2（2026-07 活跃） | ~982k 总量 | github.com/leanflutter/screen_retriever | OK | Approved [VERIFIED: pub cache pubspec + STACK.md 2026-09-04 实源核实] |
| path | pub.dev | 1.9.1（dart-lang 官方） | 极高 | github.com/dart-lang/core | OK | Approved |
| plugin_platform_interface | pub.dev | ^2.0.2（flutter.dev 官方） | 极高 | github.com/flutter/packages | OK | Approved（模板自带） |
| pana | pub.dev | 0.23.19（dart.dev 官方工具） | 高 | github.com/dart-lang/pana | OK | Approved（仅 dev 工具，不进依赖） |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
**win32 零出现声明：** 上述依赖树无一携带 win32；scaffold 后应以 `grep -r win32 pubspec.lock` 复核一次并留证据（XPLAT-02 生效条款）。

## Architecture Patterns

### System Architecture Diagram

```text
[flutter create --template=plugin]           [上游 pub cache]
  91 files 落盘                                window_manager-0.5.2
       │                                       bitsdojo_window_windows-0.1.6
       ▼                                            │
┌─────────────────────── 仓库 D:\window_frame_kit ───────────────────────┐
│  LICENSE（三行叠加 MIT）◄────────── 逐字版权行 ──────────┘              │
│  pubspec.yaml（channel=包名派生；deps: path+screen_retriever+ppi）      │
│  DEVIATIONS.md（D-09 账本，空表头）   .git/remote: upstream→leanflutter │
│  lib/ …模板三件套（Phase 2 被 WM facade 替换）                          │
│  windows/ linux/ macos/ …模板原生壳（Phase 2/3/4 被移植码填充）         │
│  example/ …可运行三平台宿主                                             │
│  .github/workflows/ci.yml（三平台 4 项检查）                            │
└────────────────────────────────────────────────────────────────────────┘
       │ push / PR（D-06）
       ▼
[GitHub Actions windows-latest | ubuntu-latest | macos-latest]
  ① analyze+format  ② flutter test  ③ pana(仅 ubuntu, --exit-code-threshold)
  ④ flutter build {windows|linux|macos} --release
       │
       ▼
[实机清单] Windows 启动 example 显示窗口（SC1，人工判定）
```

### Recommended Project Structure（模板实际输出，[VERIFIED: 本会话 91 文件清单]）
```text
window_frame_kit/
├── pubspec.yaml                    # 需补：description/homepage/repository/issue_tracker/topics
├── LICENSE                         # 整体替换为三行叠加 MIT（模板默认是 BSD-3 风格占位）
├── README.md / CHANGELOG.md        # README 需重写（模板有 Android/iOS 错误措辞）；CHANGELOG 头格式已合法
├── analysis_options.yaml           # include flutter_lints；analyzer exclude 掉三平台目录（保留）
├── .gitignore                      # 已忽略 /pubspec.lock、*.iml、.idea/、.dart_tool/（模板生成正确）
├── lib/
│   ├── window_frame_kit.dart                    # Dart facade（Channel 名见 method_channel）
│   ├── window_frame_kit_platform_interface.dart # 平台接口层
│   └── window_frame_kit_method_channel.dart     # MethodChannel('window_frame_kit') ← 第10行
├── windows/
│   ├── CMakeLists.txt              # PLUGIN_SOURCES 清单 + gtest 1.11 FetchContent（需 CMake4 补丁）
│   ├── include/window_frame_kit/window_frame_kit_plugin_c_api.h
│   ├── window_frame_kit_plugin_c_api.cpp        # C-API 薄壳 → 注册内层类
│   ├── window_frame_kit_plugin.cpp|.h           # namespace window_frame_kit; Channel 在 :23
│   └── test/window_frame_kit_plugin_test.cpp    # 原生 gtest 骨架
├── linux/
│   ├── CMakeLists.txt              # cmake_minimum_required 3.10；PLUGIN_SOURCES 清单
│   ├── include/window_frame_kit/window_frame_kit_plugin.h
│   ├── window_frame_kit_plugin.cc               # Channel 在 :69
│   ├── window_frame_kit_plugin_private.h
│   └── test/window_frame_kit_plugin_test.cc
├── macos/
│   ├── window_frame_kit.podspec    # 占位 summary/homepage/author 需清理
│   └── window_frame_kit/
│       ├── Package.swift           # swift-tools 5.9 / macOS 12.0（与 WM 0.5.2 同布局，pana 加分项保留）
│       └── Sources/window_frame_kit/
│           ├── WindowFrameKitPlugin.swift       # Channel 在 :6
│           └── PrivacyInfo.xcprivacy
├── example/                        # publish_to:'none'；path 依赖 ../；三平台可运行
│   └── integration_test/plugin_integration_test.dart
└── test/                           # 2 个 Dart 测试（模板自带，骨架期全绿）
```

### Pattern 1: Channel 名自动派生（D 红线的免费满足）
**What:** `flutter create` 把包名直接用作 MethodChannel 名，四处一致。
**When to use:** 只要 scaffold 时包名是 `window_frame_kit`，无需任何手改。
**Example:**
```dart
// Source: 本会话 scaffold 产物 lib/window_frame_kit_method_channel.dart:10
@visibleForTesting
final methodChannel = const MethodChannel('window_frame_kit');
```
对应原生三处：`windows/window_frame_kit_plugin.cpp:23`（`"window_frame_kit"`）、`linux/window_frame_kit_plugin.cc:69`、`macos/.../WindowFrameKitPlugin.swift:6`。

### Pattern 2: Windows C-API 壳模式（Phase 2 移植的结构约束）
**What:** 3.47 模板的 Windows 插件入口是 `WindowFrameKitPluginCApi`（pubspec pluginClass），`window_frame_kit_plugin_c_api.cpp` 仅 12 行，负责把 registrar 转交给内层 `window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar`。上游 WM 0.5.2 则是单一 `WindowManagerPlugin` 无壳。
**When to use:** Phase 2 移植 WM 的 plugin.cpp 时，保留壳文件与 pubspec pluginClass，把 WM 的注册/消息逻辑装进内层类。
**Example:**
```cpp
// Source: 本会话 scaffold 产物 windows/window_frame_kit_plugin_c_api.cpp（全文 12 行）
void WindowFrameKitPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
```

### Pattern 3: 新增原生源文件要登记进 CMake（Phase 2/3 的预埋）
**What:** 两个 CMakeLists 都有 `list(APPEND PLUGIN_SOURCES ...)` 清单，注释明言 "Any new source files that you add to the plugin should be added here"。
**When to use:** Phase 2/3 每落一个 .cpp/.h（如 frame_custom.cpp）都要同步登记；`PLUGIN_NAME`/目标名不可改（模板注释明言 tooling 依赖）。
**Example:**
```cmake
# Source: 本会话 scaffold 产物 windows/CMakeLists.txt:19-23
# Any new source files that you add to the plugin should be added here.
list(APPEND PLUGIN_SOURCES
  "window_frame_kit_plugin.cpp"
  "window_frame_kit_plugin.h"
)
```
另有硬约束：`apply_standard_settings` 给所有目标上 `/W4 /WX`（警告即错误）——Phase 2/3 移植的上游 C++ 必须告警级干净编译 [VERIFIED: example/windows/CMakeLists.txt:42]。

### Pattern 4: 原生测试块由 example 侧变量门控
**What:** `example/windows/CMakeLists.txt:56` `set(include_window_frame_kit_tests TRUE)` 打开后，插件的 gtest 块才会生效并 FetchContent 下载 googletest release-1.11.0。
**When to use:** 骨架期保留（CI 编译 example 会连测件一起编）；若 CI 命中 CMake/gtest 问题而时间紧，可临时置 FALSE 让 native build 先行。

### Anti-Patterns to Avoid
- **失败后不删 build 缓存直接重试：** 第一次 configure 失败会把 CMake cache 写坏（详见 Pitfall 2 的 C:/Program Files 变体），必须 `rm -rf example/build` 再重试
- **在本机 Windows 调试 pana：** 已验证必然崩溃（Pitfall 4），纯属浪费时间；本机只跑 dry-run
- **改模板文件时不留痕：** 对模板 CMake/pubspec 的修改虽不属上游偏离，但应在 PR/commit message 说明；对上游移植文件的偏离才进 DEVIATIONS.md
- **LICENSE 拖到 Phase 2/3 再落：** Pitfall 2 红线——嫁接 commit 之前必须就位

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 发布检查 | 自己写脚本检查 pubspec 字段/文件清单 | `dart pub publish --dry-run` | 工具校验与 pub.dev 服务端一致；0 warnings 已验证可达 |
| 评分守护 | 自算分数规则 | pana `--exit-code-threshold` | 官方语义：`(max - granted) > threshold` 时非零退出 [VERIFIED: pana --help 原文] |
| Flutter 环境安装 | 手写脚本装 SDK | subosito/flutter-action@v2 | 生态标准，支持版本钉死与缓存 |
| MIT 归属格式 | 自创多许可混排 | 三行同格式版权行叠加 | MIT 正文完全一致（两个上游 LICENSE 正文逐字相同），只需并排版权行 |
| 上游对 diff | 手工下载源码比对 | `git remote add upstream` + merge/diff | D-09；remote 成本一条命令（PITFALLS 技术债表原话） |

**Key insight:** 本 phase 的每件事都有官方工具直接支持；唯一的"手工活"是 LICENSE 三行叠加和 DEVIATIONS.md 账本格式——而这两件恰恰是 Pitfall 1/2 定义的纪律本体，不能外包给工具。

## Common Pitfalls

### Pitfall 1: CMake 4.x 撞模板 gtest 1.11 —— example 首次 Windows 编译必失败
**What goes wrong:** VS 2026（本机 generator "Visual Studio 18 2026"）捆绑 **CMake 4.3.1-msvc1**，而插件模板 `windows/CMakeLists.txt` 的测试块 FetchContent googletest release-1.11.0，其内部 `cmake_minimum_required(VERSION 3.1)` 触发 CMake 4 的硬错误 `Compatibility with CMake < 3.5 has been removed`，example 的 `flutter build windows` 直接无法 configure。
**Why it happens:** 模板 gtest 固定 1.11.0；新 VS 的 CMake 4 移除了对 <3.5 的兼容。裸跑模板必然命中（本机已实跑复现）。
**How to avoid:** 在 `windows/CMakeLists.txt` 的 `FetchContent_MakeAvailable(googletest)` 之前加一行（已验证 configure 通过）：
```cmake
# googletest 1.11.0 declares cmake_minimum_required < 3.5, which CMake 4.x
# (bundled with VS 2026) rejects outright; this opt-in restores compatibility.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
```
Linux 模板同款 gtest 块在 CI ubuntu（CMake 3.x）不受影响。备选：把 gtest URL 升到 release-1.14+。
**Warning signs:** configure 阶段报 `Compatibility with CMake < 3.5 has been removed`，指向 `build/.../_deps/googletest-src/CMakeLists.txt:4`。

### Pitfall 2: 失败的 configure 会毒化构建缓存 —— 修完 CMake 还会冒出第二个错
**What goes wrong:** Pitfall 1 的失败 configure 留下半套 cache；修复后重试，INSTALL 步骤报 `file cannot create directory: C:/Program Files/window_frame_kit_example`（install prefix 卡回默认值）。
**Why it happens:** `CMAKE_INSTALL_PREFIX` 的 FORCE 赋值被 `CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT` 门控，失败/恢复的 configure 序列让该条件不再成立。
**How to avoid:** 修完 CMakeLists 后**必须** `rm -rf example/build` 全清再编（本会话实测：不清理→Program Files 错；清理→进入 Pitfall 3 的错误；处理完→构建成功）。
**Warning signs:** 修了 gtest 错误后反而出现 MSB3073 + `cmake_install.cmake` / Program Files 字样。

### Pitfall 3: native_assets 目录缺失 → INSTALL 失败（flutter 已知问题族）
**What goes wrong:** 全新 configure 后 INSTALL 报 `file INSTALL cannot find ".../example/build/native_assets/windows"`。该目录由工具链的 native assets 步骤创建，骨架包无任何 native-asset 包时可能不创建（本机：宿主 simple_player_flutter 有此空目录、全新插件 example 没有）。
**Why it happens:** `install(DIRECTORY ...)` 源目录不存在时报错；社区同族问题 `file INSTALL cannot find ... native_assets/...` 跨版本反复出现 [CITED: github.com/flutter/flutter/issues/135671、/161225、dart-lang/sdk/issues/328282]。
**How to avoid:** 构建前确保目录存在：`mkdir -p example/build/native_assets/windows` 再 `flutter build`（本会话实测该两步组合后 `✓ Built ...window_frame_kit_example.exe`）。CI 上若命中，把 mkdir 加进 workflow 的 Windows job；也可能因 runner 环境不同而不复现（GitHub runner 的 VS/CMake 组合不同）——首跑 CI 时观察并记录。
**Warning signs:** INSTALL 步骤失败且错误指向 `native_assets` 路径。

### Pitfall 4: pana 0.23.19 在 Windows 本机完全不可用
**What goes wrong:** `pana .` 崩溃：`Invalid argument (outputFolder): Sandbox output folder must not contain ":"`——sandbox 临时目录含盘符冒号。`--no-dartdoc` 同样崩（pub outdated 也走 sandbox），两次复现 [VERIFIED: 本会话]。
**Why it happens:** pana 的 sandbox_runner 在 Windows 盘符路径上先天不兼容；WSL 本机未安装（已检查）。
**How to avoid:** pana 只进 CI（ubuntu-latest）：`dart pub global activate pana && dart pub global run pana --exit-code-threshold N`。本机用 `dart pub publish --dry-run`（已验证可跑）做本地反馈。从 Git Bash 调用 pana 用 `dart pub global run pana:pana`（PATH 上的 `pana.bat` 直接调用不可靠）。
**Warning signs:** 任何人在 Windows 本机跑 pana——立即制止，改 CI。

### Pitfall 5: 新生成的骨架 `dart format --set-exit-if-changed` 会失败
**What goes wrong:** 模板 Dart 文件与新 formatter 有漂移：本会话实测 8 文件中 3 个会被重排（如嵌套闭包缩进），D-07 的 format 检查裸跑即红。
**Why it happens:** 模板产出落后于 formatter 当前规则。
**How to avoid:** scaffold 后立即 `dart format .` 一次（已验证：之后 format 检查 exit 0，analyze 仍 0 issue，test 仍 3 过）。
**Warning signs:** CI format job 在第一天就红。

### Pitfall 6: LICENSE 归属自查（pub.dev 不校验）
**What goes wrong:** pana 只检查"存在一个 LICENSE"；换成自己的 MIT 丢了上游 © 行不会被发现，但 MIT 条款要求 "the above copyright notice and this permission notice shall be included in all copies or substantial portions"。
**Why it happens:** 义务只有一句话且通常被假设为"别人的 fork 的事"。
**How to avoid:** 三行叠加（见 §Code Examples Example 1，两行上游 © 已逐字核实）；README 加 Derivation 节；首个嫁接 commit 前落盘。
**Warning signs:** `pana` license 项满分但 grep 不到 `LiJianying`/`Hobeanu`。

### Pitfall 7: 模板占位符残留 → dry-run warning / 包页难看
**What goes wrong:** 裸模板 dry-run 有 1 warning（缺 homepage/repository）[VERIFIED]；podspec 里 `s.summary = 'A new Flutter plugin project.'`、`s.homepage='http://example.com'`、`s.author='Your Company'`；README 提"Android and/or iOS"（对纯桌面插件是错误描述）。
**How to avoid:** pubspec 四字段 + topics 一次补齐（补后 0 warnings 已验证）；podspec/README 占位符清单化替换。
**Warning signs:** dry-run 输出 "It's strongly recommended to include a 'homepage' or 'repository' field"。

### Pitfall 8: 模板 SDK 约束过高（无意识采纳）
**What goes wrong:** 模板生成 `sdk: ^3.13.2`——这是**生成工具当时的 Dart 版本**，等于要求宿主 Flutter ≥3.47；而上游 WM 0.5.2 自己只要 `flutter >=3.3.0`。
**Why it happens:** `flutter create` 现在把 sdk 下限钉在工具自带 Dart 上，规划者容易照单全收。
**How to avoid:** 骨架期做出**有意识的**选择并记录：保守=保留 `^3.13.2`（与开发/验证环境一致，不虚报兼容）；宽=降至依赖链真实下限（path/screen_retriever 均 `>=3.0.0`）。XPLAT-01 规定 floor 在 Phase 4 规划期按依赖链锁定——骨架期先用保守值，Phase 4 复核放宽，两头都有据。
**Warning signs:** pubspec floor 与"我们只在 3.47 验证过"的事实不符。

## Code Examples

### Example 1: LICENSE 三行叠加（版权行逐字已核实）
```text
MIT License

Copyright (c) 2022-present LiJianying <lijy91@foxmail.com>
Copyright (c) 2020-2021 Bogdan Hobeanu
Copyright (c) 2026 TATboy210

Permission is hereby granted, free of charge, to any person obtaining a copy
...（MIT 正文一份共享；两个上游 LICENSE 的正文逐字相同，
    源：pub cache window_manager-0.5.2/LICENSE 与 bitsdojo_window_windows-0.1.6/LICENSE，
    上游行分别取自各自文件第 3 行）
```
来源与惯例：MIT 叠加惯例 = 每持有人一行、保留上游全部、追加自己的行；行序（新在上/旧在上）皆为通行做法 [CITED: choosealicense.com/licenses/mit、opensource.stackexchange.com 惯例讨论]。D-02 指定本包行为第三行。

### Example 2: pubspec 修复集（dry-run 0 warnings 的最小完备形态）
```yaml
# Source: 本会话实测（wfk-fixed 探针：补齐后 "Package has 0 warnings."）
name: window_frame_kit
description: "Frame takeover and complete window management events for Flutter
  desktop apps (Windows, Linux, macOS): frameless windows, four-edge native
  resizing, and full window-manager parity APIs."
version: 0.0.1
homepage: https://github.com/TATboy210/window_frame_kit
repository: https://github.com/TATboy210/window_frame_kit
issue_tracker: https://github.com/TATboy210/window_frame_kit/issues

topics:
  - desktop
  - window
  - window-manager
  - frameless
  - custom-titlebar
```
（description 为 D-03 关键词齐备的可用措辞示例，最终措辞属规划期裁量。topics 为 D-04 原文。）

### Example 3: CI workflow 骨架（单文件三平台 matrix，D-05/06/07/08 落地形）
```yaml
# Source: 结构参考上游 window_manager 实际 workflow（raw 读取 2026-09-04）+ pana --help
name: ci
on:
  push:
    branches: [master]
  pull_request:

jobs:
  checks:
    strategy:
      fail-fast: false
      matrix:
        os: [windows-latest, ubuntu-latest, macos-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v5
      - uses: subosito/flutter-action@v2
        with:
          channel: stable
          flutter-version: "3.47.2"   # 钉版本保证可复现（上游 WM 同样钉死）
          cache: true
      - run: flutter pub get
      - run: flutter analyze
      - run: dart format --set-exit-if-changed --output=none .
      - run: flutter test
      # ④ native build：按平台分派
      - run: flutter build windows --release
        if: matrix.os == 'windows-latest'
      - run: |
          sudo apt-get update
          sudo apt-get install -y clang cmake ninja-build pkg-config libgtk-3-dev
          flutter build linux --release
        if: matrix.os == 'ubuntu-latest'
      - run: flutter build macos --release
        if: matrix.os == 'macos-latest'

  pana:                                 # 仅 Linux（Pitfall 4）
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v5
      - uses: subosito/flutter-action@v2
        with: { channel: stable, flutter-version: "3.47.2", cache: true }
      - run: dart pub global activate pana
      # 首跑先记录基线分数；随后把 threshold 钉在"基线-0"（D-08：通过线而非满分级）
      - run: dart pub global run pana:pana --exit-code-threshold <基线定值后回填> .
```
Linux 依赖清单对齐上游 WM 的 build.yml（clang cmake ninja-build pkg-config libgtk-3-dev；WM 另装 liblzma-dev/libayatana-appindicator3-dev，本包骨架不需要，Phase 4 按需加）。macOS 的 `integration_test` 在上游被 flutter#118469 阻断——本 phase 不跑 integration_test，不受影响。

### Example 4: DEVIATIONS.md 条目格式（D-09：file:line 锚点 + 理由，按功能块）
```markdown
# DEVIATIONS

本包相对上游的**有意偏离**账本。每条对应代码中的 `// FRAME:` 或 `// DEV:` 标记。
上游合并（`git merge upstream/main`）时逐条核对，防止偏离被上游改动无声覆盖。

| # | 上游锚点 (file:line) | 本包位置 | 偏离内容 | 理由 | 对应标记 |
|---|----------------------|----------|----------|------|----------|
| D-1 | bitsdojo_window_windows-0.1.6/windows/bitsdojo_window.cpp:550-568 | （Phase 3 回填） | GETMINMAXINFO 无条件 return 0 → 协作式合并 | 上游缺陷架空 setMinimumSize（宿主实证） | // FRAME: |
```
（Phase 1 只建文件与表头、写好"如何记账"的说明；条目随 Phase 2/3 嫁接回填。生态现状：235 个 pub cache 包无一携带 DEVIATIONS.md [VERIFIED: 本会话枚举]；该文件名作为 fork 偏离账本的真实先例存在于 OpenSAFELY job-runner、MailPoet 等仓库 [CITED: github.com/opensafely-core/job-runner、github.com/mailpoet/mailpoet]。格式细节属 D-09 的裁量区。）

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| actions/checkout@v3（上游 WM 在用） | @v5（Node 24；v6 已预告） | 2025-08 | 新仓库直接 v5；勿抄上游的 v2/v3 |
| 模板 Windows 插件单类注册 | C-API 壳（`*PluginCApi` + include/ 头） | 3.24+ 模板 | Phase 2 结构约束（Pattern 2） |
| pana 本地跑 + 手工看分 | CI `--exit-code-threshold` 门禁 | pana 支持 flag 已久，但 Windows 本机不可用使其成为唯一路径 | D-08 直接落地为 CI threshold |
| gtest 1.11 随模板 | CMake 4.x（VS 2026）拒绝 min<3.5 | CMake 4（2024+）| 模板必补丁（Pitfall 1） |

**Deprecated/outdated:**
- 上游 WM workflow 里的 `bluefireteam/melos-action`：单包无需 melos，跳过
- `axel-op/dart-package-analyzer` 等第三方 pana 包装 action：维护停滞，直接用 `dart pub global activate pana`

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | GitHub 公共仓库三平台标准 runner 免费（含 windows/macos） | Standard Stack / D-05 支撑 | 若政策变化影响 CI 成本预期——但 D-05 决策已含"公共仓库全免费"前提，与 docs.github.com 现行表述一致 [CITED: docs.github.com/actions/reference/usage-limits-billing-and-administration："GitHub Actions usage is free for standard GitHub-hosted runners in public repositories"] |
| A2 | windows-latest runner 的 VS 捆绑 CMake 为 3.x（gtest 坑不复现）、ubuntu-latest/macos-latest 自足可编模板 | Pitfalls 1/3、CI 骨架 | 若 runner 已升 CMake 4，CI Windows build 也会红——首跑 CI 即可验证；workaround（policy 行）本就要进仓库，风险低 |
| A3 | `flutter build linux/macos --release` 在对应 runner 上一次通过（模板自足；Linux 需 apt GTK 依赖） | CI 骨架 | 若命中首跑问题按输出修——D-07 设 native build 检查的本意正是让这类问题暴露在 CI 而非发布期 |
| A4 | 三行叠加 LICENSE 的行序（上游两行在前、本包行在后）符合惯例 | Code Examples 1 | 行序本身两种通行方向皆有 [CITED: stackexchange 惯例讨论]；法律效果不受影响 |
| A5 | 骨架期 pana 基线分数将落在"接近满分、扣 example/README 完整度分项"的区间（基于已验证的评分类目推断，本会话未能实跑 pana——Windows 不可用） | D-08 / Validation | 实际基线以 CI 首跑为准并回填 threshold；不影响任何结构决策 |

## Open Questions

1. **SDK floor 的骨架期取值（`^3.13.2` 保守 vs 放宽）**
   - What we know: 模板生成 `^3.13.2`；依赖链真实下限为 `>=3.0.0`（path/screen_retriever）；XPLAT-01 把最终 floor 的锁定放在 Phase 4
   - What's unclear: 骨架期是否就用保守值（风险=零虚报）还是先放宽（风险=未验证的兼容声明）
   - Recommendation: 骨架期**保留模板 `^3.13.2`** 并在 pubspec 注释标注"Phase 4 复核"；与开发/验证环境一致，诚实且零成本
2. **`plugin_platform_interface` 是否算白名单违规**
   - What we know: CONTEXT 白名单写"path + screen_retriever"；模板 Dart 平台接口层编译需要 plugin_platform_interface ^2.0.2；其意图（XPLAT-02）是"零 win32"
   - What's unclear: 白名单的严格字面 vs 意图解释
   - Recommendation: 按意图执行——骨架期保留该模板依赖并在计划里显式声明"白名单=path+screen_retriever+plugin_platform_interface（模板平台接口层，非 win32）；win32 出现即违规"；Phase 2 移植 WM 单文件 facade 时自然消解（WM 不依赖它做直接依赖，届时从 pubspec 删除并复核）
3. **CMake 修复风格（一行 policy vs 升级 gtest）**
   - What we know: 一行 `CMAKE_POLICY_VERSION_MINIMUM 3.5` 已验证；gtest 1.14+ 兼容 CMake 4 且可消 deprecation warning
   - Recommendation: 骨架期一行方案（最小 diff）；升级 gtest 若干年后自然发生
4. **CI 首跑的 runner 环境事实核对**（A2/A3）
   - Recommendation: 计划把"首跑 CI 全绿"作为一个显式任务节点；任何红项按输出修复并回写本研究的假设表

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Flutter SDK | scaffold/build/test | ✓ | 3.47.2 stable @ D:/flutter | — |
| Dart SDK | format/analyze/pub | ✓ | 3.13.2 | — |
| Visual Studio C++ 工具链 | windows native build | ✓ | VS 2026 ("Visual Studio 18 2026", v180) @ D:\vs community | — |
| CMake | windows native build | ✓（VS 捆绑） | 4.3.1-msvc1 | 触发 Pitfall 1，一行补丁已验证 |
| git | remote/commit | ✓ | 2.54.0 | — |
| gh CLI | 创建 D-01 仓库 | ✓ | 2.94.0 | 网页创建亦可 |
| pana（本机） | 评分检查 | ✗ | 0.23.19 已装但 Windows 必崩 | CI ubuntu runner（唯一正道） |
| WSL | 本机 pana 备选 | ✗ | 未安装 | 无需——CI 已覆盖 |
| GTK 开发头文件 | linux native build | ✗（本机） | — | CI ubuntu apt 安装（D-05 矩阵覆盖） |
| Xcode | macos native build | ✗（本机） | — | CI macos runner |

**Missing dependencies with no fallback:** 无阻断项——Linux/macOS 编译本就设计为 CI 完成。
**Missing dependencies with fallback:** 本机 pana（→CI）、GTK/Xcode（→CI runner 自带）。

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | flutter_test（SDK 内置）+ 原生 gtest 1.11（模板自带，经 Pitfall 1 补丁后可用） |
| Config file | 无独立配置（模板 analysis_options.yaml + CMake 测试块） |
| Quick run command | `flutter analyze && dart format --set-exit-if-changed --output=none . && flutter test`（约 20s） |
| Full suite command | `flutter test` + `dart pub publish --dry-run` + CI 三平台 workflow |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PUB-01 | LICENSE 含两条上游 © 行 + 本包行 | 单元（脚本断言） | `grep -c "LiJianying\|Hobeanu\|TATboy210" LICENSE` = 3 | ❌ Wave 0（一行 grep 即可，无需新框架） |
| SC-1 | example Windows 实机启动显示窗口 | **manual-only**（宿主 UAT 证据标准：控制台无崩溃 ≠ UI 正确呈现） | 人工启动 + 截图/确认 | —（实机清单项） |
| SC-4 | pubspec 依赖仅白名单、channel 名正确 | 单元 | `grep -E "win32" pubspec.lock`（须无输出）+ `grep -c "window_frame_kit" lib/*_method_channel.dart` | ✅ 模板自带可查 |
| SC-5 | dry-run 0 warnings | 集成 | `flutter pub publish --dry-run 2>&1 \| grep "0 warnings"` | ❌ Wave 0（CI 或任务内执行） |
| SC-5 | pana ≥ 通过线 | 集成（仅 CI） | `dart pub global run pana:pana --exit-code-threshold N .` | ❌ Wave 0（workflow 文件本身） |
| D-09 | upstream remote 存在 | 冒烟 | `git remote -v \| grep upstream` | ❌ 任务执行 |

### Sampling Rate
- **Per task commit:** quick run command（analyze+format+test）
- **Per wave merge:** `flutter test` 全量 + `dart pub publish --dry-run`
- **Phase gate:** CI 三平台 workflow 全绿 + pana threshold 生效 + 实机启动确认（dated sign-off）

### Wave 0 Gaps
- [ ] `.github/workflows/ci.yml` — D-05~08 的载体（pana 仅 ubuntu job；threshold 先占位、首跑记录基线后钉死）
- [ ] `DEVIATIONS.md` — D-09 表头 + 记账说明（条目 Phase 2/3 回填）
- [ ] LICENSE 三行叠加替换（首嫁接 commit 之前必须完成——时间上即本 phase）
- [ ] windows/CMakeLists.txt 的 `CMAKE_POLICY_VERSION_MINIMUM 3.5` 补丁（Pitfall 1）

## Security Domain

> security_enforcement 未在 config.json 显式关闭，按启用处理。本 phase 为包骨架（无运行时攻击面），以下为适用项的务实裁剪。

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | 部分（骨架期） | PathValidator 模式 Phase 2 引入；本 phase 无输入面 |
| V6 Cryptography | no | 无加密需求 |
| V14 Config/Supply Chain | **yes（主战场）** | 依赖白名单（XPLAT-02）+ Package Legitimacy Audit + CI 全绿后才可发布；publish 凭据永不入库（用 `flutter pub publish` 的本地凭据/token 流程） |
| V1-Secure Design | yes | channel 命名隔离（不与 window_manager 冲突 → 防宿主双包期消息串台） |

### Known Threat Patterns for {pub.dev package supply chain}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| 依赖树投毒（新增非必要依赖） | Tampering | 白名单生效条款：任何 win32/新增依赖都需在计划中显式出现并过 legitimacy audit |
| pub.dev 包页误导（占位 repository） | Spoofing/Repudiation | D-01 三字段指向真实仓库；dry-run 0 warnings 门禁 |
| 凭据泄漏（publisher token） | Information Disclosure | token 只在发布者本机/keyring；CI 不保存 publish 凭据（本 phase 无 publish job，Phase 5 再评估） |

## Sources

### Primary (HIGH confidence — 本会话实际执行/读取)
- 本机实跑：`flutter create --template=plugin --platforms=windows,linux,macos`（Flutter 3.47.2，91 文件清单 + 关键文件逐行阅读）、`flutter pub publish --dry-run`（修复前 1 warning / 修复后 0 warnings）、`dart pub global activate pana`（0.23.19）、`pana` Windows 崩溃复现 ×2、`flutter analyze`（0 issue）、`flutter test`（3 pass）、`dart format`（3 文件漂移→格式化后 exit 0）、`flutter build windows --release`（两坑修复后 `✓ Built ...window_frame_kit_example.exe`）
- Pub cache 逐字读取：`window_manager-0.5.2/LICENSE`（© 行=第 3 行）、`bitsdojo_window_windows-0.1.6/LICENSE`（© 行=第 3 行）、`window_manager-0.5.2/pubspec.yaml`（topics/repository/依赖树=白名单原型）、`screen_retriever-0.2.2/pubspec.yaml`（SDK 约束）
- 环境实测：`flutter --version`（3.47.2/3.13.2）、`gh --version`（2.94.0）、VS 2026 + CMake 4.3.1-msvc1（cmake.exe @ `D:\vs community\...`）、WSL 未安装、pana.bat 路径

### Secondary (MEDIUM — 官方来源引用)
- 上游 window_manager 实际 CI workflow（raw 读取 2026-09-04）：`build.yml`/`lint.yml`/`test.yml`（runner 集合、apt 依赖清单、版本钉死习惯、macOS integration 被 flutter#118469 阻断）
- pub.dev/help/scoring（六类评分；≥20% dartdoc；illustrative example；SwiftPM 加分）
- dart.dev/tools/pub/publishing（dry-run 语义边界：校验 pubspec/layout、列文件；git 状态检查属真实发布）
- actions/checkout v5（Node 24, 2025-08）、subosito/flutter-action v2.22.2（2025-12）release notes
- MIT 叠加惯例：choosealicense.com、opensource.stackexchange.com 惯例讨论
- DEVIATIONS.md 真实先例：github.com/opensafely-core/job-runner（"documentation for all local changes"）、github.com/mailpoet/mailpoet
- docs.github.com Actions billing（公共仓库标准 runner 免费）

### Tertiary (LOW — 社区问题定位，已被本机复现部分抵消)
- flutter/flutter#135671、flutter/flutter#161225、dart-lang/sdk#328282（native_assets INSTALL 失败问题族——本会话实测确认其骨架期变体并验证 workaround）

## Metadata

**Confidence breakdown:**
- Standard stack / 模板结构：HIGH — 全部来自本会话实跑产物与 pub cache 逐字读取
- Pitfalls：HIGH — Pitfall 1/2/3/4/5 全部本机复现并拿到已验证解法；Pitfall 6/7/8 有官方文档/惯例支撑
- CI 形态：MEDIUM-HIGH — 上游实际 workflow 为底、action 版本经核实；runner 端行为（A2/A3）待首跑确认

**Research date:** 2026-09-04
**Valid until:** 2026-10-04（模板/action 版本稳定域；Flutter stable 一个月内若无大版本变更不需重研）
