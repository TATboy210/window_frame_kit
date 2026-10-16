// // FRAME: 0.3.0 opt-in resize diagnostics. Measures, instead of eyeballing,
// whether interactive resizing keeps up: per rendered frame it records the
// frame time (FrameTiming.totalSpan) and whether the rendered surface size
// matched the native window size at that moment. A mismatch means the
// compositor showed a surface of the previous size (stretched or clipped
// content), which is what "tearing / stale frame" looks like on screen.
import 'dart:async';
import 'dart:math' as math;
import 'dart:ui' as ui;

import 'package:flutter/foundation.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';
import 'package:window_frame_kit/src/diagnostics/resize_sync_stats.dart';
import 'package:window_frame_kit/src/window_listener.dart';
import 'package:window_frame_kit/src/window_manager.dart';

/// One rendered frame observed while a resize was in progress.
@immutable
class ResizeFrameSample {
  const ResizeFrameSample({
    required this.frameTime,
    required this.surfaceSize,
    required this.windowSize,
    required this.tolerance,
  });

  /// Build + raster span of the frame (FrameTiming.totalSpan).
  final Duration frameTime;

  /// Logical size the frame was rendered at.
  final ui.Size surfaceSize;

  /// Logical native window size reported closest to the frame, or null when
  /// no size had been reported yet.
  final ui.Size? windowSize;

  /// Allowed difference in logical pixels per axis.
  final double tolerance;

  /// True when the surface matched the window within [tolerance]. Unknown
  /// window sizes count as matched (nothing to compare against yet).
  bool get matched {
    final window = windowSize;
    if (window == null) {
      return true;
    }
    return (surfaceSize.width - window.width).abs() <= tolerance &&
        (surfaceSize.height - window.height).abs() <= tolerance;
  }
}

/// Aggregate of one resize gesture (or of a recorder window).
@immutable
class ResizeDiagnosticsReport {
  const ResizeDiagnosticsReport(this.samples, {this.native});

  final List<ResizeFrameSample> samples;

  /// Native resize-sync telemetry for the same gesture (Windows with
  /// `customFrame` only). This is the authoritative stale-frame measurement;
  /// the Dart-side [mismatchedFrames] compares asynchronously sampled sizes
  /// and is only a coarse indicator.
  final ResizeSyncStats? native;

  int get frameCount => samples.length;

  int get mismatchedFrames => samples.where((s) => !s.matched).length;

  /// Share of frames whose surface did not match the window, 0..1.
  double get mismatchRatio =>
      frameCount == 0 ? 0 : mismatchedFrames / frameCount;

  Duration get maxFrameTime => samples.isEmpty
      ? Duration.zero
      : samples.map((s) => s.frameTime).reduce((a, b) => a > b ? a : b);

  /// Nearest-rank percentile of the frame time (p in 0..100).
  Duration percentileFrameTime(double p) {
    if (samples.isEmpty) {
      return Duration.zero;
    }
    final sorted = samples.map((s) => s.frameTime).toList()..sort();
    final rank = (p / 100 * sorted.length).ceil().clamp(1, sorted.length);
    return sorted[rank - 1];
  }

  @override
  String toString() {
    String ms(Duration d) => (d.inMicroseconds / 1000).toStringAsFixed(1);
    final dart =
        'ResizeDiagnostics: $frameCount frames, '
        '$mismatchedFrames size-sample mismatches '
        '(${(mismatchRatio * 100).toStringAsFixed(1)}%, approximate), '
        'frame time p50 ${ms(percentileFrameTime(50))} ms / '
        'p95 ${ms(percentileFrameTime(95))} ms / '
        'max ${ms(maxFrameTime)} ms';
    final nativeStats = native;
    return nativeStats == null ? dart : '$dart | $nativeStats';
  }
}

/// Pure sample collector behind [ResizeDiagnostics]; usable directly in
/// tests or with custom frame sources.
class ResizeDiagnosticsRecorder {
  ResizeDiagnosticsRecorder({this.tolerance = 2.0, this.maxSamples = 10000});

  final double tolerance;

  /// Oldest samples are dropped beyond this bound (memory stays flat during
  /// long sessions).
  final int maxSamples;

  final List<ResizeFrameSample> _samples = [];
  ui.Size? _windowSize;
  bool _resizing = false;

  bool get isResizing => _resizing;

  /// Starts a gesture. A begin while already resizing continues the same
  /// gesture; a begin after [endResize] starts a fresh one, so every
  /// gesture is reported on its own (not cumulatively).
  void beginResize() {
    if (_resizing) {
      return;
    }
    _samples.clear();
    _resizing = true;
  }

  void endResize() => _resizing = false;

  /// Latest native window size (logical pixels).
  void updateWindowSize(ui.Size size) => _windowSize = size;

  /// Records a frame; ignored unless a resize is in progress.
  void recordFrame(Duration frameTime, ui.Size surfaceSize) {
    if (!_resizing) {
      return;
    }
    _samples.add(
      ResizeFrameSample(
        frameTime: frameTime,
        surfaceSize: surfaceSize,
        windowSize: _windowSize,
        tolerance: tolerance,
      ),
    );
    final overflow = _samples.length - math.max<int>(1, maxSamples);
    if (overflow > 0) {
      _samples.removeRange(0, overflow);
    }
  }

  /// Report of the current (or last finished) gesture.
  ResizeDiagnosticsReport report({ResizeSyncStats? native}) =>
      ResizeDiagnosticsReport(List.unmodifiable(_samples), native: native);

  void clear() => _samples.clear();
}

/// Debug/profile-only resize diagnostic. Opt in explicitly:
///
/// ```dart
/// final diagnostics = ResizeDiagnostics()..start();
/// // ... drag a window edge for a while ...
/// debugPrint(diagnostics.stop().toString());
/// ```
///
/// While started it listens to window resize events and to
/// [SchedulerBinding.addTimingsCallback] and reports each resize gesture
/// separately through [onReport]. On Windows with `customFrame` the report
/// carries [ResizeDiagnosticsReport.native]: exact engine resize-wait and
/// stale-composition counts measured in the native view hook - use those
/// to judge tearing. The Dart-side size comparison only relates
/// asynchronously sampled sizes (frame timings arrive in batches of up to
/// ~100 ms), so it over-reports during fast drags. Does nothing in release
/// builds.
///
/// The comparison is meaningful when the Flutter surface covers the whole
/// window - i.e. with `customFrame` or a hidden title bar. With a native
/// title bar the surface is smaller than the window by the caption height
/// and every frame reports a mismatch.
class ResizeDiagnostics with WindowListener {
  ResizeDiagnostics({
    WindowManager? manager,
    double tolerance = 2.0,
    this.onReport,
  }) : _manager = manager ?? WindowManager.instance,
       _recorder = ResizeDiagnosticsRecorder(tolerance: tolerance);

  final WindowManager _manager;
  final ResizeDiagnosticsRecorder _recorder;

  /// Called with that gesture's report each time a resize gesture ends.
  final void Function(ResizeDiagnosticsReport report)? onReport;

  bool _running = false;
  bool _sizeRequestInFlight = false;

  bool get isRunning => _running;

  /// Starts collecting. No-op in release builds or when already running.
  void start() {
    if (kReleaseMode || _running) {
      return;
    }
    _running = true;
    _manager.addListener(this);
    SchedulerBinding.instance.addTimingsCallback(_onTimings);
  }

  /// Stops collecting and returns the current or last gesture's report.
  ResizeDiagnosticsReport stop() {
    if (_running) {
      _running = false;
      _manager.removeListener(this);
      SchedulerBinding.instance.removeTimingsCallback(_onTimings);
    }
    return _recorder.report();
  }

  void _onTimings(List<ui.FrameTiming> timings) {
    final views = SchedulerBinding.instance.platformDispatcher.views;
    if (views.isEmpty) {
      return;
    }
    final view = views.first;
    final logical = view.physicalSize / view.devicePixelRatio;
    for (final timing in timings) {
      _recorder.recordFrame(timing.totalSpan, logical);
    }
  }

  Future<void> _refreshWindowSize() async {
    if (_sizeRequestInFlight) {
      return; // One query at a time; resize events arrive per mouse move.
    }
    _sizeRequestInFlight = true;
    try {
      _recorder.updateWindowSize(await _manager.getSize());
    } finally {
      _sizeRequestInFlight = false;
    }
  }

  @override
  void onWindowResize() {
    if (!_recorder.isResizing) {
      // New gesture: discard native telemetry gathered before it.
      unawaited(_takeNative());
    }
    _recorder.beginResize();
    unawaited(_refreshWindowSize());
  }

  @override
  void onWindowResized() {
    _recorder.endResize();
    unawaited(_finishGesture());
  }

  Future<ResizeSyncStats?> _takeNative() async {
    try {
      return await _manager.takeResizeSyncStats();
    } on PlatformException catch (error) {
      debugPrint('ResizeDiagnostics: native stats unavailable: $error');
      return null;
    } on MissingPluginException {
      return null;
    }
  }

  Future<void> _finishGesture() async {
    final native = await _takeNative();
    onReport?.call(_recorder.report(native: native));
  }
}
