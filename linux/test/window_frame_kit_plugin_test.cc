#include <flutter_linux/flutter_linux.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "include/window_frame_kit/window_frame_kit_plugin.h"

// Ported window_manager 0.5.2 implementation (04-01): the plugin has no
// free helper functions to unit test and the register entry point needs a
// live FlPluginRegistrar (upstream ships no native tests for the same
// reason). The GObject class registration, however, is context-free and
// pins both the exported symbol and the G_DEFINE_TYPE wiring.
//
// Once you have built the plugin's example app, you can run these tests
// from the command line. For instance, for a plugin called my_plugin
// built for x64 debug, run:
// $ build/linux/x64/debug/plugins/my_plugin/my_plugin_test
#include "window_frame_kit_plugin_private.h"

// This demonstrates a simple unit test of the C portion of this plugin's
// implementation.
//
// Once you have built the plugin's example app, you can run these tests
// from the command line. For instance, for a plugin called my_plugin
// built for x64 debug, run:
// $ build/linux/x64/debug/plugins/my_plugin/my_plugin_test

namespace window_frame_kit {
namespace test {

TEST(WindowFrameKitPlugin, GetTypeRegistersObjectClass) {
  const GType type = window_frame_kit_plugin_get_type();
  ASSERT_NE(type, 0);
  EXPECT_TRUE(g_type_is_a(type, G_TYPE_OBJECT));
}

}  // namespace test
}  // namespace window_frame_kit
