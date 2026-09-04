#ifndef FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
#define FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
#include <flutter/plugin_registrar_windows.h>
namespace window_frame_kit {
// DEV: registration bridge for the C-API shell
// (window_frame_kit_plugin_c_api.cpp). The ported window_manager 0.5.2
// implementation lives in window_frame_kit_plugin.cpp's anonymous namespace;
// this class only forwards RegisterWithRegistrar into it. See DEVIATIONS.md.
class WindowFrameKitPlugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);
};
}  // namespace window_frame_kit
#endif  // FLUTTER_PLUGIN_WINDOW_FRAME_KIT_PLUGIN_H_
