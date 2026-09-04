import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'window_frame_kit_method_channel.dart';

abstract class WindowFrameKitPlatform extends PlatformInterface {
  /// Constructs a WindowFrameKitPlatform.
  WindowFrameKitPlatform() : super(token: _token);

  static final Object _token = Object();

  static WindowFrameKitPlatform _instance = MethodChannelWindowFrameKit();

  /// The default instance of [WindowFrameKitPlatform] to use.
  ///
  /// Defaults to [MethodChannelWindowFrameKit].
  static WindowFrameKitPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [WindowFrameKitPlatform] when
  /// they register themselves.
  static set instance(WindowFrameKitPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
