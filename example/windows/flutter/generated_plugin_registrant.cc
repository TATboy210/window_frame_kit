//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <screen_retriever_windows/screen_retriever_windows_plugin_c_api.h>
#include <tray_manager/tray_manager_plugin.h>
#include <window_frame_kit/window_frame_kit_plugin_c_api.h>

void RegisterPlugins(flutter::PluginRegistry* registry) {
  ScreenRetrieverWindowsPluginCApiRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("ScreenRetrieverWindowsPluginCApi"));
  TrayManagerPluginRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("TrayManagerPlugin"));
  WindowFrameKitPluginCApiRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("WindowFrameKitPluginCApi"));
}
