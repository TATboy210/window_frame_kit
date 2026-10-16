// // FRAME: 0.3.0 native resize synchronization telemetry (Windows).
import 'package:flutter/foundation.dart';

/// Native (Windows) measurement of how well the Flutter view kept up with
/// window size changes, recorded by the custom frame's view hook.
///
/// "Mismatch" is time during which the view window had a different size
/// than the last frame the engine presented into it - DWM composes such a
/// view stretched. [staleComposedFrames] estimates how many DWM
/// compositions happened inside those intervals (counted against
/// `DwmGetCompositionTimingInfo` vblank timing), i.e. how many stale or
/// stretched frames reached the screen.
@immutable
class ResizeSyncStats {
  const ResizeSyncStats({
    required this.steps,
    required this.syncTimeouts,
    required this.staleComposedFrames,
    required this.mismatchMs,
    required this.maxSyncWaitMs,
  });

  /// Parses the platform-channel map; returns null for anything else.
  static ResizeSyncStats? fromMap(Object? value) {
    if (value is! Map) {
      return null;
    }
    num read(String key) {
      final v = value[key];
      return v is num ? v : 0;
    }

    return ResizeSyncStats(
      steps: read('steps').toInt(),
      syncTimeouts: read('syncTimeouts').toInt(),
      staleComposedFrames: read('staleComposedFrames').toInt(),
      mismatchMs: read('mismatchMs').toDouble(),
      maxSyncWaitMs: read('maxSyncWaitMs').toDouble(),
    );
  }

  /// View size changes the engine was asked to render.
  final int steps;

  /// Engine resize waits that hit its 100 ms timeout (the stale frame then
  /// stayed on screen until a later frame).
  final int syncTimeouts;

  /// Estimated DWM compositions that showed a size-mismatched view.
  final int staleComposedFrames;

  /// Total view/surface size mismatch time in milliseconds.
  final double mismatchMs;

  /// Longest single engine resize wait in milliseconds.
  final double maxSyncWaitMs;

  /// Stale composed frames per resize step (0 when there were no steps).
  double get staleFramesPerStep => steps == 0 ? 0 : staleComposedFrames / steps;

  @override
  String toString() =>
      'native: $steps steps, $staleComposedFrames stale composed frames '
      '(${staleFramesPerStep.toStringAsFixed(2)}/step), '
      '$syncTimeouts engine timeouts, '
      'mismatch ${mismatchMs.toStringAsFixed(1)} ms, '
      'max wait ${maxSyncWaitMs.toStringAsFixed(1)} ms';
}
