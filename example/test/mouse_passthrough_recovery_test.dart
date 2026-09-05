import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:window_frame_kit_example/utils/mouse_passthrough_recovery.dart';

void main() {
  testWidgets('开启成功五秒后恢复鼠标交互', (tester) async {
    final calls = <bool>[];
    final recovery = MousePassthroughRecovery(
      apply: (value) async => calls.add(value),
      onError: (error, stack) => fail('$error'),
    );
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    await tester.pump(const Duration(seconds: 4));
    expect(calls, [true]);
    await tester.pump(const Duration(seconds: 1));
    expect(calls, [true, false]);
    await recovery.dispose();
  });

  testWidgets('重复开启重置计时且显式关闭取消恢复任务', (tester) async {
    final calls = <bool>[];
    final recovery = MousePassthroughRecovery(
      apply: (value) async => calls.add(value),
      onError: (error, stack) => fail('$error'),
    );
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    await tester.pump(const Duration(seconds: 3));
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    await tester.pump(const Duration(seconds: 3));
    expect(calls, [true, true]);
    unawaited(recovery.setEnabled(false));
    await tester.pump();
    await tester.pump(const Duration(seconds: 6));
    expect(calls, [true, true, false]);
    await recovery.dispose();
  });

  testWidgets('未完成的开启必须先于关闭落到平台，防迟到开启覆盖恢复', (tester) async {
    final calls = <bool>[];
    final gate = Completer<void>();
    final recovery = MousePassthroughRecovery(
      apply: (value) async {
        calls.add(value);
        if (value) await gate.future;
      },
      onError: (error, stack) => fail('$error'),
    );
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    unawaited(recovery.setEnabled(false));
    await tester.pump();
    expect(calls, [true]);
    gate.complete();
    await tester.pump();
    await tester.pump(const Duration(seconds: 6));
    expect(calls, [true, false]);
    await recovery.dispose();
  });

  testWidgets('销毁时恢复且后续操作无效', (tester) async {
    final calls = <bool>[];
    final recovery = MousePassthroughRecovery(
      apply: (value) async => calls.add(value),
      onError: (error, stack) => fail('$error'),
    );
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    unawaited(recovery.dispose());
    await tester.pump();
    await recovery.dispose();
    await recovery.setEnabled(true);
    await tester.pump(const Duration(seconds: 6));
    expect(calls, [true, false]);
  });

  testWidgets('闲置销毁不调用平台', (tester) async {
    final calls = <bool>[];
    final recovery = MousePassthroughRecovery(
      apply: (value) async => calls.add(value),
      onError: (error, stack) => fail('$error'),
    );
    await recovery.dispose();
    expect(calls, isEmpty);
  });

  testWidgets('平台异常可观察且不阻断后续恢复', (tester) async {
    final errors = <Exception>[];
    final calls = <bool>[];
    final recovery = MousePassthroughRecovery(
      apply: (value) async {
        calls.add(value);
        if (value) throw PlatformException(code: 'test-failure');
      },
      onError: (error, stack) => errors.add(error),
    );
    unawaited(recovery.setEnabled(true));
    await tester.pump();
    expect(errors, hasLength(1));
    // 即便开启返回错误，也可能已改变原生状态，销毁仍需补发恢复。
    unawaited(recovery.dispose());
    await tester.pump();
    expect(calls, [true, false]);
  });
}
