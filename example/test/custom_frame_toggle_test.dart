// // FRAME: example widget test (05-01) — the custom-frame runtime toggle.
// Drives the real PreferenceList switch in home.dart against a mocked
// window_frame_kit channel: flipping the switch must call setCustomFrame
// with the flipped value, and the row state must follow.
//
// Replaces the upstream-template widget_test.dart (WINDOWS #6): that file
// asserted MyApp rendered a 'Running on:' caption the real app never had,
// so it was fail-if-run and CI skipped it entirely.
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:preference_list/preference_list.dart';
import 'package:window_frame_kit_example/pages/home.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const channel = MethodChannel('window_frame_kit');
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  setUp(() {
    messenger.setMockMethodCallHandler(channel, (call) async {
      // HomePage 的查询类条目只在点击时触发;pump 阶段提供保守兜底。
      return call.method.startsWith('is') ? false : null;
    });
  });

  tearDown(() {
    messenger.setMockMethodCallHandler(channel, null);
  });

  Future<void> pumpHome(WidgetTester tester) async {
    await tester.pumpWidget(const MaterialApp(home: HomePage()));
    await tester.pumpAndSettle();
  }

  Finder customFrameRow() {
    final title = find.text('setCustomFrame');
    return find.ancestor(
      of: title,
      matching: find.byType(PreferenceListSwitchItem),
    );
  }

  testWidgets('cold-start default mirrors main.dart customFrame: true', (
    tester,
  ) async {
    await pumpHome(tester);
    final row = tester.widget<PreferenceListSwitchItem>(customFrameRow());
    expect(row.value, true);
  });

  testWidgets('flipping the switch calls setCustomFrame and mirrors state', (
    tester,
  ) async {
    final calls = <MethodCall>[];
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return call.method.startsWith('is') ? false : null;
    });

    await pumpHome(tester);
    final row = tester.widget<PreferenceListSwitchItem>(customFrameRow());
    expect(row.onChanged, isNotNull);

    // // FRAME: drive the same callback the tap gesture would invoke.
    // onChanged returns void (async callback), so fire-and-pump: the mock
    // channel completes inside a microtask that pumpAndSettle flushes.
    row.onChanged!(false);
    await tester.pump();
    await tester.pumpAndSettle();

    final frameCalls = calls
        .where((c) => c.method == 'setCustomFrame')
        .toList();
    expect(frameCalls, hasLength(1));
    expect((frameCalls.first.arguments as Map)['isCustomFrame'], false);

    final updated = tester.widget<PreferenceListSwitchItem>(customFrameRow());
    expect(updated.value, false);
  });
}
