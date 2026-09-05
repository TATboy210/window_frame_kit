import 'dart:ui';

import 'package:window_frame_kit/src/title_bar_style.dart';

/// WindowOptions
class WindowOptions {
  const WindowOptions({
    this.size,
    this.center,
    this.minimumSize,
    this.maximumSize,
    this.alwaysOnTop,
    this.fullScreen,
    this.backgroundColor,
    this.skipTaskbar,
    this.title,
    this.titleBarStyle,
    this.windowButtonVisibility,
    this.customFrame, // // FRAME: graft customFrame option (deviation from upstream 0.5.2)
  });

  final Size? size;
  final bool? center;
  final Size? minimumSize;
  final Size? maximumSize;
  final bool? alwaysOnTop;
  final bool? fullScreen;
  final Color? backgroundColor;
  final bool? skipTaskbar;
  final String? title;
  final TitleBarStyle? titleBarStyle;
  final bool? windowButtonVisibility;

  /// // FRAME: When true, the native FrameController takes over the window
  /// frame (borderless NCCALCSIZE + 4-edge/4-corner hit-test + cooperative
  /// GETMINMAXINFO) right after [titleBarStyle] is applied. Defaults to null
  /// (no channel call) to preserve upstream window_manager 0.5.2 behavior.
  final bool? customFrame;
}
