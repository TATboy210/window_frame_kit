import 'package:flutter_test/flutter_test.dart';
import 'package:window_frame_kit/window_frame_kit.dart';
import 'package:window_frame_kit/window_frame_kit_platform_interface.dart';
import 'package:window_frame_kit/window_frame_kit_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockWindowFrameKitPlatform
    with MockPlatformInterfaceMixin
    implements WindowFrameKitPlatform {
  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final WindowFrameKitPlatform initialPlatform =
      WindowFrameKitPlatform.instance;

  test('$MethodChannelWindowFrameKit is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelWindowFrameKit>());
  });

  test('getPlatformVersion', () async {
    WindowFrameKit windowFrameKitPlugin = WindowFrameKit();
    MockWindowFrameKitPlatform fakePlatform = MockWindowFrameKitPlatform();
    WindowFrameKitPlatform.instance = fakePlatform;

    expect(await windowFrameKitPlugin.getPlatformVersion(), '42');
  });
}
