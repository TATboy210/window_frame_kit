//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <window_frame_kit/window_frame_kit_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) window_frame_kit_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "WindowFrameKitPlugin");
  window_frame_kit_plugin_register_with_registrar(window_frame_kit_registrar);
}
