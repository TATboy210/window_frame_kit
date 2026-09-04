import 'package:flutter/foundation.dart';
import 'package:window_frame_kit/window_frame_kit.dart';

/// 控制台事件日志 — D-04 实机验证载体（SC3 证据源）。
///
/// Prints one line per [WindowListener] hook:
/// `[window_frame_kit] {ISO timestamp} {hook/event name}`, so the UAT
/// checklist can be ticked directly from console output. Lives on the
/// example side on purpose — `lib/` stays byte-verbatim with upstream
/// (SC5), and Phase 4 reuses this same file on Linux/macOS.
class WindowEventLogger with WindowListener {
  /// Single sink for every hook: timestamp + name, one line each.
  void _log(String name) {
    debugPrint('[window_frame_kit] ${DateTime.now().toIso8601String()} $name');
  }

  @override
  void onWindowClose() => _log('onWindowClose');

  @override
  void onWindowFocus() => _log('onWindowFocus');

  @override
  void onWindowBlur() => _log('onWindowBlur');

  @override
  void onWindowMaximize() => _log('onWindowMaximize');

  @override
  void onWindowUnmaximize() => _log('onWindowUnmaximize');

  @override
  void onWindowMinimize() => _log('onWindowMinimize');

  @override
  void onWindowRestore() => _log('onWindowRestore');

  @override
  void onWindowResize() => _log('onWindowResize');

  @override
  void onWindowResized() => _log('onWindowResized');

  @override
  void onWindowMove() => _log('onWindowMove');

  @override
  void onWindowMoved() => _log('onWindowMoved');

  @override
  void onWindowEnterFullScreen() => _log('onWindowEnterFullScreen');

  @override
  void onWindowLeaveFullScreen() => _log('onWindowLeaveFullScreen');

  @override
  void onWindowDocked() => _log('onWindowDocked');

  @override
  void onWindowUndocked() => _log('onWindowUndocked');

  @override
  void onWindowEvent(String eventName) => _log('onWindowEvent:$eventName');
}
