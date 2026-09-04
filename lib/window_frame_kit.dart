import 'window_frame_kit_platform_interface.dart';

class WindowFrameKit {
  Future<String?> getPlatformVersion() {
    return WindowFrameKitPlatform.instance.getPlatformVersion();
  }
}
