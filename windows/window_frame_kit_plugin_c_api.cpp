#include "include/window_frame_kit/window_frame_kit_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "window_frame_kit_plugin.h"

void WindowFrameKitPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  window_frame_kit::WindowFrameKitPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
