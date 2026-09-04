# Phase 2: Windows 基座移植 - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-09-04
**Phase:** 2-Windows 基座移植
**Areas discussed:** Dart API 面保真度, 实机对等验证载体形态

---

## Dart API 面保真度

### Q1: Dart 公开 API 标识符与上游 window_manager 0.5.2 的保真度？

| Option | Description | Selected |
|--------|-------------|----------|
| 全保真：标识符逐字同上游 | windowManager 单例 + 全部类/枚举名不变；宿主迁移≈只换 import；SC5 diff 噪音最小 | ✓ |
| 折中：facade 改名，底层保留 | 顶层改名（WindowFrameKit/wfk），枚举/Options/Listener 保留；diff 噪音集中在 763 行核心文件 | |
| 全改名：新包新 API | 全部重命名；逐字证明不可机器化、cherry-pick 路径废弃、与 SC5 直接冲突 | |

**User's choice:** 全保真：标识符逐字同上游 (Recommended)
**Notes:** 无附加条件。

### Q2: 上游 test/ 如何处置？

| Option | Description | Selected |
|--------|-------------|----------|
| 上游测试逐字移植 + 叠加锚点断言 | 只改 import/channel，另加 channel 名与 15 事件名常量一致性断言（SC5 机器门） | ✓ |
| 全自写契约测试 | 按 CAPB-01~12 自写；与上游断链，无法 cherry-pick 测试修复 | |

**User's choice:** 上游测试逐字移植 + 叠加锚点断言 (Recommended)

### Q3: 宿主迁移期同名类共存的处置纪律？

| Option | Description | Selected |
|--------|-------------|----------|
| 互斥换 import，同文件禁双导入 | 迁移以文件为单位整换 import；同名冲突结构性不存在；写进 README 迁移说明 | ✓ |
| 提供前缀别名兼容层 | export 前缀别名 wfk_*；与全保真相悖，为不存在的场景付成本（YAGNI） | |

**User's choice:** 互斥换 import，同文件禁双导入 (Recommended)

**事实核定（免讨论）：** WM 0.5.2 零 @deprecated 成员——"裁剪弃用面"灰区不存在。

---

## 实机对等验证载体形态

### Q4: 实机对等验证的载体形态？（两轮：用户质疑复杂度后重选简化版）

第一轮选项（被用户质疑）：

| Option | Description | Selected |
|--------|-------------|----------|
| 上游 example 移植 + 补事件日志面板 | 上游 demo 为底 + 新写屏上日志面板 UI | |
| 最小手动清单 | example 不动，UAT 文档手工驱动 | |
| 全自建仪表盘 | 按 CAPB 分组全新建 | |

**User's challenge:** "只是做一个 flutter 依赖而已有必要这么复杂吗" → 要求重选。

第二轮（简化版）：

| Option | Description | Selected |
|--------|-------------|----------|
| 上游 demo 白拿 + 控制台事件日志 | example = 上游三件逐字移植；11 hook 用 debugPrint 打控制台；零新增 UI | ✓ |
| 上游 demo + 屏上日志面板 | 补屏上 UI 面板——即用户质疑的复杂度 | |
| example 不动，纯手动 UAT | 验证时无现成按钮面，每验一个能力要临时改代码 | |

**User's choice:** 上游 demo 白拿 + 控制台事件日志 (Recommended)
**Notes:** 用户明确反对为验证载体新写 UI；"白拿上游 + 控制台日志"是复杂度最低且覆盖 SC1-4 的方案。屏上日志面板记入 Deferred（Phase 5 可重提）。

### Q5: 实机验证的节奏（用户是验证执行人）？

| Option | Description | Selected |
|--------|-------------|----------|
| 分批实机 checkpoint（3 批左右） | ①初始化+几何 ②状态+查询+事件流 ③拦截/标题栏/拖拽/菜单；每批小规模实机确认；退出门=末批全量 UAT | ✓ |
| 一次性全量 UAT | 全部完成后一次大验证；打断少但早期系统性问题会污染全部后续批次 | |

**User's choice:** 分批实机 checkpoint（3 批左右） (Recommended)

---

## Claude's Discretion

- 逐字 diff 证明协议（diff 脚本对 pub cache + 重命名白名单 + DEVIATIONS.md 记账）——用户未选该区讨论，按推荐默认
- native C++ 命名（默认保留骨架 WindowFrameKitPlugin 结构，WM 实现移植进内层）
- Dart lib/src 内部文件名（默认保留上游名）
- doc comment 语言、UAT 条目分组细节、批内任务切分、控制台日志具体格式

## Deferred Ideas

- 屏上事件日志面板（Phase 5 example 强化时可重提）
