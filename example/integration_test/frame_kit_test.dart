// window_frame_kit 0.3.0 end-to-end checks on a real native window (no input
// simulation): frameless state, geometry, maximize/restore and fullscreen.
//
// Run: `flutter test integration_test/frame_kit_test.dart -d <platform>`
// (Linux CI: under `xvfb-run` with a window manager; without a WM the
// maximize/fullscreen requests are not honoured by X11, see README).
import 'dart:io';
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:window_frame_kit/window_frame_kit.dart';

/// Polls [probe] until it returns [expected] or the timeout elapses; window
/// state changes are asynchronous on every platform.
Future<bool> _eventually(
  Future<bool> Function() probe, {
  bool expected = true,
  Duration timeout = const Duration(seconds: 5),
}) async {
  final deadline = DateTime.now().add(timeout);
  while (DateTime.now().isBefore(deadline)) {
    if (await probe() == expected) {
      return true;
    }
    await Future<void>.delayed(const Duration(milliseconds: 100));
  }
  return await probe() == expected;
}

Future<void> main() async {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();
  await windowManager.waitUntilReadyToShow(
    const WindowOptions(
      size: Size(800, 600),
      center: true,
      title: 'window_frame_kit_frame_test',
      titleBarStyle: TitleBarStyle.hidden,
      customFrame: true,
    ),
    () async {
      await windowManager.show();
      await windowManager.focus();
    },
  );

  testWidgets('custom frame is active', (tester) async {
    expect(await windowManager.isCustomFrame(), isTrue);
    expect(await windowManager.detectWindowManagerConflict(), isFalse);
  });

  testWidgets('Flutter surface covers the whole window (frameless)', (
    tester,
  ) async {
    await tester.pumpWidget(const SizedBox.expand());
    final windowSize = await windowManager.getSize();
    final view = tester.view;
    final surface = view.physicalSize / view.devicePixelRatio;
    // Frameless: no system caption/border inset. 2 logical px of slack
    // covers the 1px Windows shadow overhang and DPI rounding.
    expect((surface.width - windowSize.width).abs(), lessThanOrEqualTo(2));
    expect((surface.height - windowSize.height).abs(), lessThanOrEqualTo(2));
  });

  testWidgets('setSize / getSize round trip', (tester) async {
    await windowManager.setSize(const Size(720, 540));
    expect(
      await _eventually(() async {
        final size = await windowManager.getSize();
        return (size.width - 720).abs() <= 1 && (size.height - 540).abs() <= 1;
      }),
      isTrue,
    );
  });

  testWidgets(
    'maximize / unmaximize',
    (tester) async {
      await windowManager.maximize();
      expect(await _eventually(windowManager.isMaximized), isTrue);
      await windowManager.unmaximize();
      expect(
        await _eventually(windowManager.isMaximized, expected: false),
        isTrue,
      );
    },
    // Under xvfb without a window manager X11 ignores maximize requests.
    skip: Platform.isLinux && Platform.environment['WFK_HAS_WM'] != '1',
  );

  testWidgets('fullscreen enter / leave', (tester) async {
    await windowManager.setFullScreen(true);
    expect(
      await _eventually(
        windowManager.isFullScreen,
        timeout: const Duration(seconds: 10),
      ),
      isTrue,
    );
    await windowManager.setFullScreen(false);
    expect(
      await _eventually(windowManager.isFullScreen, expected: false),
      isTrue,
    );
    // The frame survives the round trip.
    expect(await windowManager.isCustomFrame(), isTrue);
  }, skip: Platform.isLinux && Platform.environment['WFK_HAS_WM'] != '1');

  testWidgets('programmatic resize sweep: surface follows the window', (
    tester,
  ) async {
    // No mouse input: setSize drives the same native WM_NCCALCSIZE/WM_SIZE
    // path as an edge drag. After each step one frame is pumped and the
    // rendered surface is compared with the native window size; the
    // ResizeDiagnosticsRecorder report is printed as measurable evidence.
    await tester.pumpWidget(const SizedBox.expand());
    final recorder = ResizeDiagnosticsRecorder()..beginResize();
    for (var i = 0; i < 24; i++) {
      final size = Size(640.0 + i * 20, 480.0 + (i % 6) * 30);
      final started = DateTime.now();
      await windowManager.setSize(size);
      await tester.pump();
      final surface = tester.view.physicalSize / tester.view.devicePixelRatio;
      recorder
        ..updateWindowSize(await windowManager.getSize())
        ..recordFrame(DateTime.now().difference(started), surface);
    }
    recorder.endResize();
    final report = recorder.report();
    // ignore: avoid_print
    print('frame_kit resize sweep: $report');
    expect(report.frameCount, 24);
    // The final state must be in sync once the resize has settled.
    await tester.pump(const Duration(milliseconds: 200));
    final finalSize = await windowManager.getSize();
    final surface = tester.view.physicalSize / tester.view.devicePixelRatio;
    expect((surface.width - finalSize.width).abs(), lessThanOrEqualTo(2));
    expect((surface.height - finalSize.height).abs(), lessThanOrEqualTo(2));
  });

  testWidgets('custom frame toggles off and back on', (tester) async {
    await windowManager.setCustomFrame(false);
    expect(await windowManager.isCustomFrame(), isFalse);
    await windowManager.setCustomFrame(true);
    expect(await windowManager.isCustomFrame(), isTrue);
  });
}
