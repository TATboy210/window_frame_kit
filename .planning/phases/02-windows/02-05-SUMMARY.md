---
phase: 02-windows
plan: 05
status: partial
requirements-completed: []
---

# 02-05：无托盘 example 已实现，phase 退出门仍未通过

## 已完成

- 按 WINDOWS #7 用户指令移除 tray_manager、菜单和 listener，重新生成三平台 registrant/lock。
- example 的鼠标穿透改为五秒自动恢复，串行平台写入防止迟到开启覆盖关闭；dispose 尽力恢复，异常上报。插件 API 与原生代码零修改。
- 新增6个测试：先因缺模块 RED，实现后 GREEN；覆盖率 27/29 行（93.1%）。
- CI 加入新增恢复测试及格式检查，移除 Linux appindicator 依赖。
- DEVIATIONS #7 登记例外。逐字门现在为16 Dart文件零差 + 1个固定归一化摘要的home.dart例外，C++仍22+2；不是关闭检查。
- 批③人工验收表已经落盘，CAPB-11改验自动恢复，不再要求不存在的托盘菜单。

## 本轮验证（2026-09-05）

- flutter analyze：No issues found。
- root flutter test：13/13通过。
- example 新增恢复测试：6/6通过；coverage 93.1%。
- 格式正名单5文件：零变化。
- verbatim_diff：16零差+1固定example偏差，C++22+2通过。
- flutter build windows --release：成功（googletest旧CMake兼容性警告不影响构建）。
- 新Release启动5秒存活，随后仅结束自己启动的PID；此项不等于UI实测。
- git diff cc55c7f -- lib windows linux macos：为空。

## 尚未完成 / 不声称通过

- 所有合批人工UAT及最终phase gate签核。
- 新提交的远程三平台CI；旧HEAD的CI绿不证明本次修改。
- 本轮 publish dry-run 尚未执行。
- 旧 example/test/widget_test.dart 仍为 WINDOWS #6 挂账模板测试，本次未运行也未改断言；CI仅新增恢复测试，不假称example全套通过。
- 既有integration_test的isFocused环境失败不重跑、不篡改。
- Phase 3不可开始，REQUIREMENTS不勾选。

## 聚焦审查

flutter-code-reviewer 已核验串行调用、定时器、销毁恢复、home.dart 固定摘要与 CI 范围，未发现本轮可复现的阻塞问题。该结论只属于代码审查，不是人工 UAT 签核。
