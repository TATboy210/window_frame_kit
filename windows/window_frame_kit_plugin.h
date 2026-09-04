#ifndef FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
#define FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace window_frame_kit {

class WindowFrameKitPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  WindowFrameKitPlugin();

  virtual ~WindowFrameKitPlugin();

  // Disallow copy and assign.
  WindowFrameKitPlugin(const WindowFrameKitPlugin&) = delete;
  WindowFrameKitPlugin& operator=(const WindowFrameKitPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace window_frame_kit

#endif  // FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
