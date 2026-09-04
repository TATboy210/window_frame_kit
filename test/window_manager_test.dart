import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
// import 'package:window_frame_kit/window_frame_kit.dart';

void main() {
  const MethodChannel channel = MethodChannel('window_frame_kit');

  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(
      channel,
      (MethodCall methodCall) async {
        return '42';
      },
    );
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(
      channel,
      null,
    );
  });
}
