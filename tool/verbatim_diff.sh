#!/usr/bin/env bash
# SC5 逐字证明机器门 — A 段（Dart 面，02-01 落定）。
#
# 对照源 $1 = 上游 window_manager-0.5.2 目录（本地 pub cache 或 CI curl 解包）。
# 协议（02-RESEARCH Pattern 3）：单条归一化 sed 's/window_frame_kit/window_manager/g'
# 同时消掉 import 路径、channel 常量、example 包名/title 串——它们全部由包名派生。
# 归一化后 16 个 Dart 文件与上游字节级零差；home.dart 按授权偏差固定摘要。
#
# main.dart 特例（D-04/Open Q2 定案）：先滤除事件日志注册行
# （含 window_event_logger / WindowEventLogger，大小写不敏感），滤除行数必须恰为 2。
#
# B 段（C++ 精确残留行数断言 22+2，02-02 落定）：残留行 = DEVIATIONS.md 记账的
# 结构偏离（plugin.cpp 3 hunk：L1 include / L112 channel / 尾块桥接；
# window_manager.cpp 1 hunk：L1 include）——任何行数漂移即禁改区被触碰，红灯重审。
set -u
WM="${1:?usage: verbatim_diff.sh <window_manager-0.5.2 dir>}"
fail=0

# 包名派生串一次归一（逐字区唯一允许的机械偏离）
norm() { sed 's/window_frame_kit/window_manager/g' "$1"; }

# 13 对普通逐字区文件（main.dart/home.dart 走精确特例；window_options.dart
# 与 window_manager.dart 自 03-01 起为 customFrame 嫁接偏差区，走下方快照门）
declare -A DART_PAIRS=(
  [lib/window_frame_kit.dart]=lib/window_manager.dart
  [lib/src/window_listener.dart]=lib/src/window_listener.dart
  [lib/src/resize_edge.dart]=lib/src/resize_edge.dart
  [lib/src/title_bar_style.dart]=lib/src/title_bar_style.dart
  [lib/src/utils/calc_window_position.dart]=lib/src/utils/calc_window_position.dart
  [lib/src/widgets/drag_to_move_area.dart]=lib/src/widgets/drag_to_move_area.dart
  [lib/src/widgets/drag_to_resize_area.dart]=lib/src/widgets/drag_to_resize_area.dart
  [lib/src/widgets/virtual_window_frame.dart]=lib/src/widgets/virtual_window_frame.dart
  [lib/src/widgets/window_caption.dart]=lib/src/widgets/window_caption.dart
  [lib/src/widgets/window_caption_button.dart]=lib/src/widgets/window_caption_button.dart
  [test/window_manager_test.dart]=test/window_manager_test.dart
  [example/lib/utils/config.dart]=example/lib/utils/config.dart
  [example/integration_test/window_manager_test.dart]=example/integration_test/window_manager_test.dart
)

for ours in "${!DART_PAIRS[@]}"; do
  if ! diff <(norm "$ours") "$WM/${DART_PAIRS[$ours]}" >/dev/null; then
    echo "VERBATIM FAIL: $ours"
    fail=1
  fi
done

# 03-01 customFrame 嫁接偏差区（DEVIATIONS #8/#9）：归一化 diff 必须与
# tool/verbatim_expected/ 下钉住的快照逐字节一致——内容级校验而非行数，
# 改动嫁接行之外的任何内容（或删偏差）都会红灯，防止静默漂移。
pinned_diff() { # $1=ours $2=upstream-relative $3=snapshot name
  local actual expected
  actual=$(diff <(norm "$1" | tr -d '\r') "$WM/$2" | tr -d '\r')
  expected=$(tr -d '\r' < "tool/verbatim_expected/$3")
  if [ "$actual" != "$expected" ]; then
    echo "VERBATIM FAIL: $1 differs from pinned snapshot tool/verbatim_expected/$3"
    fail=1
  fi
}
pinned_diff lib/src/window_options.dart lib/src/window_options.dart window_options.diff
pinned_diff lib/src/window_manager.dart lib/src/window_manager.dart window_manager.diff

# home.dart 唯一额外例外：用户授权移除托盘（DEVIATIONS #7）。
# 固定已审查版本的归一化 blob，不能任意编辑 example 后仍蒙混过门。
# tr 去除 CR 使 Windows checkout 与 Linux CI 的行尾一致。
HOME_HASH=$(norm example/lib/pages/home.dart | tr -d '\r' | git hash-object --stdin)
if [ "$HOME_HASH" != "80e84d0eaa991dcf443d4142fec9dc3dfa8fa9ac" ]; then
  echo "VERBATIM FAIL: home.dart differs from reviewed no-tray deviation #7"
  fail=1
fi

# main.dart 特例：logger 注册恰 2 行（import + addListener），滤除后归一化
# diff 与快照逐字节一致（03-04 起快照含 customFrame 冷启动演示行）
LOGGER_LINES=$(grep -ciE 'window_event_logger|windoweventlogger' example/lib/main.dart)
if [ "$LOGGER_LINES" -ne 2 ]; then
  echo "VERBATIM FAIL: example/lib/main.dart logger-line count $LOGGER_LINES != 2"
  fail=1
fi
MAIN_ACTUAL=$(diff <(grep -viE 'window_event_logger|windoweventlogger' example/lib/main.dart \
    | sed 's/window_frame_kit/window_manager/g' | tr -d '\r') \
    "$WM/example/lib/main.dart" | tr -d '\r')
MAIN_EXPECTED=$(tr -d '\r' < tool/verbatim_expected/example_main.diff)
if [ "$MAIN_ACTUAL" != "$MAIN_EXPECTED" ]; then
  echo "VERBATIM FAIL: example/lib/main.dart differs from pinned snapshot"
  fail=1
fi

# B. C++ 面——精确残留行数（结构偏离已记 DEVIATIONS.md，改动即重审）
# plugin.cpp 146 = 原 22 结构偏离 + // FRAME: 嫁接块（03-03：include/成员/析构/
# HandleWindowProc frame 分支/setCustomFrame handler，DEVIATIONS #10/#11）
n1=$(diff windows/window_frame_kit_plugin.cpp "$WM/windows/window_manager_plugin.cpp" | grep -c '^[<>]')
n2=$(diff windows/window_manager.cpp "$WM/windows/window_manager.cpp" | grep -c '^[<>]')
if [ "$n1" -ne 146 ]; then
  echo "plugin.cpp residue $n1 != 146"
  fail=1
fi
if [ "$n2" -ne 2 ]; then
  echo "window_manager.cpp residue $n2 != 2"
  fail=1
fi

if [ "$fail" -eq 0 ]; then
  echo "VERBATIM PROOF OK (13 dart zero-diff + 2 customFrame pinned diffs + 1 pinned example deviation, cpp residue 146+2)"
fi
exit "$fail"
