// 新码区（D-02 叠加层）—— SC5 的 channel 契约半边 + SC3 的 Dart 分发半边。
// 双向机制（probe 4/4 实证）：
//   Dart→native：setMockMethodCallHandler 捕获 facade 出站调用（方法名+参数形状）；
//   native→Dart：handlePlatformMessage 注入 onEvent 驱动 WindowListener 分发。
// 注意：上游壳 test/window_manager_test.dart 零 test() 块（Pitfall 2），
// 本文件是 `flutter test` 全绿的必要条件，必须与模板测试删除同 commit。
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:window_frame_kit/window_frame_kit.dart';

/// Records every WindowListener callback as `hook:<event>` / `raw:<event>`
/// so dispatch order and completeness can be asserted in one list compare.
class _Recorder with WindowListener {
  final List<String> hooks = [];

  @override
  void onWindowClose() => hooks.add('hook:close');

  @override
  void onWindowFocus() => hooks.add('hook:focus');

  @override
  void onWindowBlur() => hooks.add('hook:blur');

  @override
  void onWindowMaximize() => hooks.add('hook:maximize');

  @override
  void onWindowUnmaximize() => hooks.add('hook:unmaximize');

  @override
  void onWindowMinimize() => hooks.add('hook:minimize');

  @override
  void onWindowRestore() => hooks.add('hook:restore');

  @override
  void onWindowResize() => hooks.add('hook:resize');

  @override
  void onWindowResized() => hooks.add('hook:resized');

  @override
  void onWindowMove() => hooks.add('hook:move');

  @override
  void onWindowMoved() => hooks.add('hook:moved');

  @override
  void onWindowEnterFullScreen() => hooks.add('hook:enter-full-screen');

  @override
  void onWindowLeaveFullScreen() => hooks.add('hook:leave-full-screen');

  @override
  void onWindowDocked() => hooks.add('hook:docked');

  @override
  void onWindowUndocked() => hooks.add('hook:undocked');

  @override
  void onWindowEvent(String eventName) => hooks.add('raw:$eventName');
}

/// The 15 upstream event-name constants (window_manager 0.5.2 data strings).
const List<String> _kAllEventNames = [
  'close',
  'focus',
  'blur',
  'maximize',
  'unmaximize',
  'minimize',
  'restore',
  'resize',
  'resized',
  'move',
  'moved',
  'enter-full-screen',
  'leave-full-screen',
  'docked',
  'undocked',
];

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  // SC5 channel 名锚点断言的载体：facade 若改用其他 channel 名，
  // mock 不会命中 → MissingPluginException → 相关用例全红。
  const channel = MethodChannel('window_frame_kit');
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  test('channel name anchor: facade invokes on window_frame_kit', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return true;
    });
    await windowManager.ensureInitialized();
    await windowManager.setPreventClose(true);
    expect(calls.map((c) => c.method), [
      'ensureInitialized',
      'setPreventClose',
    ]);
    expect((calls[1].arguments as Map)['isPreventClose'], true);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('15 event-name constants verbatim (upstream 0.5.2)', () {
    expect(kWindowEventClose, 'close');
    expect(kWindowEventFocus, 'focus');
    expect(kWindowEventBlur, 'blur');
    expect(kWindowEventMaximize, 'maximize');
    expect(kWindowEventUnmaximize, 'unmaximize');
    expect(kWindowEventMinimize, 'minimize');
    expect(kWindowEventRestore, 'restore');
    expect(kWindowEventResize, 'resize');
    expect(kWindowEventResized, 'resized');
    expect(kWindowEventMove, 'move');
    expect(kWindowEventMoved, 'moved');
    expect(kWindowEventEnterFullScreen, 'enter-full-screen');
    expect(kWindowEventLeaveFullScreen, 'leave-full-screen');
    expect(kWindowEventDocked, 'docked');
    expect(kWindowEventUndocked, 'undocked');
  });

  test(
    'native onEvent drives all 15 named hooks + raw onWindowEvent',
    () async {
      final recorder = _Recorder();
      windowManager.addListener(recorder);
      for (final name in _kAllEventNames) {
        await messenger.handlePlatformMessage(
          channel.name,
          const StandardMethodCodec().encodeMethodCall(
            MethodCall('onEvent', {'eventName': name}),
          ),
          (data) {},
        );
      }
      // 上游分发序：先 onWindowEvent(raw) 后 funcMap 具名 hook。
      final expected = <String>[];
      for (final name in _kAllEventNames) {
        expected
          ..add('raw:$name')
          ..add('hook:$name');
      }
      expect(recorder.hooks, expected);
      windowManager.removeListener(recorder);
    },
  );

  test(
    'getBounds passes devicePixelRatio and parses x/y/width/height',
    () async {
      messenger.setMockMethodCallHandler(channel, (call) async {
        expect((call.arguments as Map).containsKey('devicePixelRatio'), true);
        return {'x': 1.0, 'y': 2.0, 'width': 800.0, 'height': 600.0};
      });
      expect(
        await windowManager.getBounds(),
        const Rect.fromLTWH(1, 2, 800, 600),
      );
      messenger.setMockMethodCallHandler(channel, null);
    },
  );

  test('setMinimumSize sends devicePixelRatio/width/height', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      captured = call;
      return null;
    });
    await windowManager.setMinimumSize(const Size(400, 300));
    expect(captured!.method, 'setMinimumSize');
    final args = captured!.arguments as Map;
    expect(args.keys.toSet(), {'devicePixelRatio', 'width', 'height'});
    expect(args['width'], 400.0);
    expect(args['height'], 300.0);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('maximize sends vertically flag (aero-snap variant)', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return null;
    });
    await windowManager.maximize();
    await windowManager.maximize(vertically: true);
    expect(calls.map((c) => c.method), ['maximize', 'maximize']);
    expect((calls[0].arguments as Map)['vertically'], false);
    expect((calls[1].arguments as Map)['vertically'], true);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('startResizing sends resizeEdge name + 4 edge bools', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      // Windows 上 startResizing 先查 isFullScreen（Dart 守卫，CAPB-09）。
      if (call.method == 'isFullScreen') return false;
      captured = call;
      return true;
    });
    await windowManager.startResizing(ResizeEdge.topLeft);
    expect(captured!.method, 'startResizing');
    final args = captured!.arguments as Map;
    expect(args['resizeEdge'], 'topLeft');
    expect(args['top'], true);
    expect(args['left'], true);
    expect(args['bottom'], false);
    expect(args['right'], false);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('startDragging fullscreen guard: no channel call on Windows', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return call.method == 'isFullScreen' ? true : null;
    });
    await windowManager.startDragging();
    if (Platform.isWindows) {
      // 守卫短路：fullscreen=true 时只查状态、绝不发 startDragging。
      expect(calls.map((c) => c.method), ['isFullScreen']);
    } else {
      // 非 Windows 平台守卫不生效（上游行为 verbatim）。
      expect(calls.map((c) => c.method), ['startDragging']);
    }
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('setBackgroundColor sends ARGB as 4 int keys', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      captured = call;
      return null;
    });
    await windowManager.setBackgroundColor(const Color(0x8000FF00));
    final args = captured!.arguments as Map;
    expect(args.keys.toSet(), {
      'backgroundColorA',
      'backgroundColorR',
      'backgroundColorG',
      'backgroundColorB',
    });
    expect(args['backgroundColorA'], 0x80);
    expect(args['backgroundColorR'], 0x00);
    expect(args['backgroundColorG'], 0xFF);
    expect(args['backgroundColorB'], 0x00);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('setTitleBarStyle sends style name + windowButtonVisibility', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      captured = call;
      return null;
    });
    await windowManager.setTitleBarStyle(
      TitleBarStyle.hidden,
      windowButtonVisibility: false,
    );
    final args = captured!.arguments as Map;
    expect(args['titleBarStyle'], 'hidden');
    expect(args['windowButtonVisibility'], false);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test('setIgnoreMouseEvents sends ignore + forward', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      captured = call;
      return null;
    });
    await windowManager.setIgnoreMouseEvents(true, forward: true);
    final args = captured!.arguments as Map;
    expect(args['ignore'], true);
    // forward 在 Windows native 被忽略（Pitfall 9），但 Dart 侧必须原样下发。
    expect(args['forward'], true);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test(
    'prevent-close pair: setPreventClose shape + isPreventClose bool',
    () async {
      final calls = <MethodCall>[];
      messenger.setMockMethodCallHandler(channel, (call) async {
        calls.add(call);
        return call.method == 'isPreventClose' ? true : null;
      });
      await windowManager.setPreventClose(true);
      expect(await windowManager.isPreventClose(), true);
      expect(calls.map((c) => c.method), ['setPreventClose', 'isPreventClose']);
      expect((calls[0].arguments as Map)['isPreventClose'], true);
      messenger.setMockMethodCallHandler(channel, null);
    },
  );

  test('popUpWindowMenu captured on the same channel', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return null;
    });
    await windowManager.popUpWindowMenu();
    expect(calls.map((c) => c.method), ['popUpWindowMenu']);
    messenger.setMockMethodCallHandler(channel, null);
  });

  // --- Phase 3: customFrame 嫁接通道(// FRAME: dart) ---

  test('setCustomFrame sends isCustomFrame bool flag', () async {
    MethodCall? captured;
    messenger.setMockMethodCallHandler(channel, (call) async {
      captured = call;
      return null;
    });
    await windowManager.setCustomFrame(true);
    expect(captured!.method, 'setCustomFrame');
    expect((captured!.arguments as Map)['isCustomFrame'], true);

    await windowManager.setCustomFrame(false);
    expect((captured!.arguments as Map)['isCustomFrame'], false);
    messenger.setMockMethodCallHandler(channel, null);
  });

  test(
    'WindowOptions without customFrame does not send setCustomFrame',
    () async {
      final calls = <MethodCall>[];
      messenger.setMockMethodCallHandler(channel, (call) async {
        calls.add(call);
        // 上游 waitUntilReadyToShow 会查询三项状态。
        return call.method == 'isFullScreen' ||
                call.method == 'isMaximized' ||
                call.method == 'isMinimized'
            ? false
            : null;
      });
      const options = WindowOptions(size: Size(800, 600));
      await windowManager.waitUntilReadyToShow(options);
      // 默认关闭保持上游兼容:无 setCustomFrame 出站调用。
      expect(calls.map((c) => c.method), isNot(contains('setCustomFrame')));
      messenger.setMockMethodCallHandler(channel, null);
    },
  );

  test('WindowOptions.customFrame=true applies after titleBarStyle, before state restore', () async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return call.method == 'isFullScreen' ||
              call.method == 'isMaximized' ||
              call.method == 'isMinimized'
          ? false
          : null;
    });
    const options = WindowOptions(
      titleBarStyle: TitleBarStyle.hidden,
      customFrame: true,
    );
    await windowManager.waitUntilReadyToShow(options);
    final methods = calls.map((c) => c.method).toList();
    final titleBarIdx = methods.indexOf('setTitleBarStyle');
    final customFrameIdx = methods.indexOf('setCustomFrame');
    final fullscreenIdx = methods.indexOf('isFullScreen');
    expect(titleBarIdx, isNot(-1), reason: 'titleBarStyle 应已下发');
    expect(customFrameIdx, isNot(-1), reason: 'customFrame 应已下发');
    // 规范初始化顺序:frame 接管先于全屏/最大化状态恢复。
    expect(customFrameIdx, greaterThan(titleBarIdx));
    expect(customFrameIdx, lessThan(fullscreenIdx));
    expect((calls[customFrameIdx].arguments as Map)['isCustomFrame'], true);
    messenger.setMockMethodCallHandler(channel, null);
  });
}
