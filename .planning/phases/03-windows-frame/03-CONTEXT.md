# Phase 3 执行约束（2026-09-05）

- 用户希望加速连续完成剩余四阶段；明确允许Phase2剩余人工验收合并到最终，基座固定f192dd5。只能改变顺序，不能伪造验证结果。
- 已实测通过：无托盘、鼠标穿透、五秒自动恢复三项。其他人工项仍在02-UAT-EVIDENCE挂账。
- 最小架构：window_manager0.5.2底座保留，独立C++ FrameController，零Dart win32依赖、零宿主runner接线。
- customFrame默认关闭以保留上游行为，明确开关后开启父子窗口原生命中/无边框；确保与min/max、最大化、全屏协作。
- 不照搬bitsdojo忽视全屏的边缘命中bug：window_manager全屏状态及media_kit摘除WS_OVERLAPPEDWINDOW路径都不可缩放。
- 保留MIT版权与DEVIATIONS；不改media_kit、SDK或pub cache。
- 复用已有研究，精简规划；原生/通道测试先行，构建后一次人工验收。
- Phase4可独立推进；无mac/Linux实机不冒充已测。pub.dev发布及宿主替换仍需会话内明确确认。
