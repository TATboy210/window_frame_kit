#!/usr/bin/env bash
# SC5 逐字证明机器门 — A 段（Dart 面，02-01 落定）。
#
# 对照源 $1 = 上游 window_manager-0.5.2 目录（本地 pub cache 或 CI curl 解包）。
# 协议（02-RESEARCH Pattern 3）：单条归一化 sed 's/window_frame_kit/window_manager/g'
# 同时消掉 import 路径、channel 常量、example 包名/title 串——它们全部由包名派生。
# 归一化后 17 个 Dart 逐字区文件必须与上游字节级零差。
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

# 16 对普通逐字区文件（main.dart 走下方特例）
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
  [example/lib/pages/home.dart]=example/lib/pages/home.dart
  [example/lib/utils/config.dart]=example/lib/utils/config.dart
  [example/integration_test/window_manager_test.dart]=example/integration_test/window_manager_test.dart
)

for ours in "${!DART_PAIRS[@]}"; do
  if ! diff <(norm "$ours") "$WM/${DART_PAIRS[$ours]}" >/dev/null; then
    echo "VERBATIM FAIL: $ours"
    fail=1
  fi
done

# main.dart 特例：logger 注册恰 2 行（import + addListener），滤除后归一化零差
LOGGER_LINES=$(grep -ciE 'window_event_logger|windoweventlogger' example/lib/main.dart)
if [ "$LOGGER_LINES" -ne 2 ]; then
  echo "VERBATIM FAIL: example/lib/main.dart logger-line count $LOGGER_LINES != 2"
  fail=1
fi
if ! diff <(grep -viE 'window_event_logger|windoweventlogger' example/lib/main.dart \
    | sed 's/window_frame_kit/window_manager/g') \
    "$WM/example/lib/main.dart" >/dev/null; then
  echo "VERBATIM FAIL: example/lib/main.dart"
  fail=1
fi

# B. C++ 面——精确残留行数（结构偏离已记 DEVIATIONS.md ②③④，改动即重审）
n1=$(diff windows/window_frame_kit_plugin.cpp "$WM/windows/window_manager_plugin.cpp" | grep -c '^[<>]')
n2=$(diff windows/window_manager.cpp "$WM/windows/window_manager.cpp" | grep -c '^[<>]')
if [ "$n1" -ne 22 ]; then
  echo "plugin.cpp residue $n1 != 22"
  fail=1
fi
if [ "$n2" -ne 2 ]; then
  echo "window_manager.cpp residue $n2 != 2"
  fail=1
fi

if [ "$fail" -eq 0 ]; then
  echo "VERBATIM PROOF OK (17 dart files zero-diff, cpp residue 22+2)"
fi
exit "$fail"
