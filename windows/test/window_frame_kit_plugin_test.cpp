#include <gtest/gtest.h>

// DEV: placeholder native test. The ported window_manager 0.5.2 classes
// (WindowManagerPlugin / WindowManager) require a live
// flutter::PluginRegistrarWindows to construct -- which is why upstream ships
// no native tests -- and the anonymous-namespace types are intentionally not
// reachable from a test TU. This placeholder keeps the gtest target
// (window_frame_kit_test.vcxproj) and the CMAKE_POLICY_VERSION_MINIMUM 3.5
// compatibility patch alive so Phase 3 can add pure-function tests here.
// See DEVIATIONS.md.
namespace window_frame_kit {
namespace test {

TEST(WindowFrameKitPluginPlaceholder, GtestHarnessIsAlive) {
  EXPECT_TRUE(true);
}

}  // namespace test
}  // namespace window_frame_kit
