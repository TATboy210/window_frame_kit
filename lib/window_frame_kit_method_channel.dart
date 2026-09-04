import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'window_frame_kit_platform_interface.dart';

/// An implementation of [WindowFrameKitPlatform] that uses method channels.
class MethodChannelWindowFrameKit extends WindowFrameKitPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('window_frame_kit');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>(
      'getPlatformVersion',
    );
    return version;
  }
}
