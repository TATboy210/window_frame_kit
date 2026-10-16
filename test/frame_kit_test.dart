// 0.3.0 new-code tests: isCustomFrame channel, window_manager coexistence
// detection, display placement math, and the resize diagnostics recorder.
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:window_frame_kit/window_frame_kit.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;
  const channel = MethodChannel('window_frame_kit');
  const foreign = MethodChannel('window_manager');

  tearDown(() {
    messenger.setMockMethodCallHandler(channel, null);
    messenger.setMockMethodCallHandler(foreign, null);
  });

  group('isCustomFrame', () {
    test('returns the native flag', () async {
      messenger.setMockMethodCallHandler(channel, (call) async {
        return call.method == 'isCustomFrame' ? true : null;
      });
      expect(await windowManager.isCustomFrame(), isTrue);
    });

    test('null reply maps to false', () async {
      messenger.setMockMethodCallHandler(channel, (call) async => null);
      expect(await windowManager.isCustomFrame(), isFalse);
    });
  });

  group('window_manager coexistence', () {
    test('no foreign plugin -> no conflict', () async {
      expect(await windowManager.detectWindowManagerConflict(), isFalse);
    });

    test('foreign plugin answering -> conflict', () async {
      final probes = <String>[];
      messenger.setMockMethodCallHandler(foreign, (call) async {
        probes.add(call.method);
        return true;
      });
      expect(await windowManager.detectWindowManagerConflict(), isTrue);
      expect(probes, ['isVisible']);
    });

    test('foreign plugin rejecting the probe is still a conflict', () async {
      messenger.setMockMethodCallHandler(foreign, (call) async {
        throw PlatformException(code: 'denied');
      });
      expect(await windowManager.detectWindowManagerConflict(), isTrue);
    });

    test('warning names the conflicting package', () {
      expect(kWindowManagerConflictWarning, contains('window_manager'));
      expect(kWindowManagerConflictWarning, contains('pubspec.yaml'));
    });
  });

  group('display placement', () {
    const primary = DisplayArea(
      bounds: Rect.fromLTWH(0, 0, 1920, 1080),
      workArea: Rect.fromLTWH(0, 0, 1920, 1040),
      isPrimary: true,
    );
    const secondary = DisplayArea(
      bounds: Rect.fromLTWH(1920, -200, 2560, 1440),
      workArea: Rect.fromLTWH(1920, -200, 2560, 1400),
    );

    test('selects the display under the cursor', () {
      final picked = selectDisplayArea(const [
        primary,
        secondary,
      ], const Offset(2500, 100));
      expect(picked, same(secondary));
    });

    test('falls back to primary, then first, then null', () {
      expect(
        selectDisplayArea(const [secondary, primary], const Offset(-5, -5)),
        same(primary),
      );
      expect(
        selectDisplayArea(const [secondary], const Offset(-5, -5)),
        same(secondary),
      );
      expect(selectDisplayArea(const [], Offset.zero), isNull);
    });

    test('named alignments match upstream window_manager math', () {
      const size = Size(800, 600);
      const work = Rect.fromLTWH(100, 50, 1920, 1040);
      expect(
        alignWindowInWorkArea(size, Alignment.topLeft, work),
        const Offset(100, 50),
      );
      expect(
        alignWindowInWorkArea(size, Alignment.center, work),
        const Offset(100 + 560, 50 + 220),
      );
      expect(
        alignWindowInWorkArea(size, Alignment.bottomRight, work),
        const Offset(100 + 1120, 50 + 440),
      );
      expect(
        alignWindowInWorkArea(size, Alignment.topCenter, work),
        const Offset(100 + 560, 50),
      );
      expect(
        alignWindowInWorkArea(size, Alignment.centerLeft, work),
        const Offset(100, 50 + 220),
      );
    });
  });

  group('per-gesture reporting', () {
    test('each gesture starts a fresh window (not cumulative)', () {
      final recorder = ResizeDiagnosticsRecorder()
        ..beginResize()
        ..recordFrame(Duration.zero, const Size(1, 1))
        ..recordFrame(Duration.zero, const Size(1, 1))
        ..endResize();
      expect(recorder.report().frameCount, 2);
      recorder
        ..beginResize()
        ..recordFrame(Duration.zero, const Size(1, 1));
      expect(recorder.report().frameCount, 1);
      // begin while resizing continues the same gesture
      recorder
        ..beginResize()
        ..recordFrame(Duration.zero, const Size(1, 1))
        ..endResize();
      expect(recorder.report().frameCount, 2);
    });

    test('ResizeDiagnostics reports each gesture with native stats', () async {
      var takes = 0;
      messenger.setMockMethodCallHandler(channel, (call) async {
        if (call.method == 'takeResizeSyncStats') {
          takes++;
          return {
            'steps': 10 * takes,
            'syncTimeouts': 0,
            'staleComposedFrames': takes,
            'mismatchMs': 1.5,
            'maxSyncWaitMs': 7.0,
          };
        }
        if (call.method == 'getBounds') {
          return {'x': 0.0, 'y': 0.0, 'width': 800.0, 'height': 600.0};
        }
        return null;
      });
      final reports = <ResizeDiagnosticsReport>[];
      final diagnostics = ResizeDiagnostics(onReport: reports.add)..start();
      for (var gesture = 0; gesture < 2; gesture++) {
        diagnostics.onWindowResize();
        diagnostics.onWindowResize();
        diagnostics.onWindowResized();
        await pumpEventQueue();
      }
      diagnostics.stop();
      expect(reports, hasLength(2));
      if (Platform.isWindows) {
        // begin-of-gesture take resets, end-of-gesture take reports.
        expect(reports[0].native?.steps, 20);
        expect(reports[1].native?.steps, 40);
        expect(reports[1].toString(), contains('native:'));
      } else {
        expect(reports[0].native, isNull);
      }
    });
  });

  group('ResizeSyncStats', () {
    test('parses the channel map and rejects other values', () {
      final stats = ResizeSyncStats.fromMap({
        'steps': 200,
        'syncTimeouts': 1,
        'staleComposedFrames': 50,
        'mismatchMs': 812.5,
        'maxSyncWaitMs': 101.0,
      });
      expect(stats?.steps, 200);
      expect(stats?.staleFramesPerStep, 0.25);
      expect(stats.toString(), contains('1 engine timeouts'));
      expect(ResizeSyncStats.fromMap(null), isNull);
      expect(ResizeSyncStats.fromMap({})?.staleFramesPerStep, 0);
    });

    test('takeResizeSyncStats / debugResizeSweep use the channel', () async {
      final calls = <MethodCall>[];
      messenger.setMockMethodCallHandler(channel, (call) async {
        calls.add(call);
        return {'steps': 3};
      });
      final taken = await windowManager.takeResizeSyncStats();
      final swept = await windowManager.debugResizeSweep(
        steps: 3,
        preSync: false,
      );
      if (Platform.isWindows) {
        expect(taken?.steps, 3);
        expect(swept?.steps, 3);
        expect(calls.map((c) => c.method), [
          'takeResizeSyncStats',
          'debugResizeSweep',
        ]);
        expect(calls[1].arguments, {'steps': 3, 'preSync': false});
      } else {
        expect(taken, isNull);
        expect(calls, isEmpty);
      }
    });
  });

  group('ResizeDiagnosticsRecorder', () {
    test('records only while a resize is in progress', () {
      final recorder = ResizeDiagnosticsRecorder();
      recorder.recordFrame(const Duration(milliseconds: 5), const Size(1, 1));
      expect(recorder.report().frameCount, 0);
      recorder
        ..beginResize()
        ..updateWindowSize(const Size(800, 600))
        ..recordFrame(const Duration(milliseconds: 4), const Size(800, 600))
        ..recordFrame(const Duration(milliseconds: 9), const Size(780, 600))
        ..endResize()
        ..recordFrame(const Duration(milliseconds: 1), const Size(1, 1));
      final report = recorder.report();
      expect(report.frameCount, 2);
      expect(report.mismatchedFrames, 1);
      expect(report.mismatchRatio, 0.5);
      expect(report.maxFrameTime, const Duration(milliseconds: 9));
    });

    test('tolerance absorbs sub-pixel frame overhang', () {
      final recorder = ResizeDiagnosticsRecorder(tolerance: 2)
        ..beginResize()
        ..updateWindowSize(const Size(800, 600))
        ..recordFrame(Duration.zero, const Size(800, 601));
      expect(recorder.report().mismatchedFrames, 0);
    });

    test('unknown window size counts as matched', () {
      final recorder = ResizeDiagnosticsRecorder()
        ..beginResize()
        ..recordFrame(Duration.zero, const Size(10, 10));
      expect(recorder.report().mismatchedFrames, 0);
    });

    test('percentiles use nearest rank and bound memory', () {
      final recorder = ResizeDiagnosticsRecorder(maxSamples: 10)..beginResize();
      for (var i = 1; i <= 20; i++) {
        recorder.recordFrame(Duration(milliseconds: i), const Size(1, 1));
      }
      final report = recorder.report();
      expect(report.frameCount, 10); // oldest dropped
      expect(report.percentileFrameTime(50), const Duration(milliseconds: 15));
      expect(report.percentileFrameTime(100), const Duration(milliseconds: 20));
      expect(report.toString(), contains('10 frames'));
      expect(
        const ResizeDiagnosticsReport([]).percentileFrameTime(95),
        Duration.zero,
      );
    });

    test('ResizeDiagnostics start/stop is idempotent', () {
      final diagnostics = ResizeDiagnostics();
      diagnostics.start();
      diagnostics.start();
      expect(diagnostics.isRunning, isTrue);
      expect(diagnostics.stop().frameCount, 0);
      expect(diagnostics.isRunning, isFalse);
      diagnostics.stop();
    });
  });
}
