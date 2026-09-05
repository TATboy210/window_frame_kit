// // FRAME: pure-function tests for the Phase 3 frame graft (03-02,
// 2026-09-05 equal-width NC-band redesign). No live HWND involved: band
// math, work-area clamping and the cooperative GETMINMAXINFO merge are
// exercised as injectable math so they stay deterministic under gtest.
// The state machine (Enable/Disable frame recalcs) needs a real window tree
// and is covered by the UAT batch.
#include <gtest/gtest.h>

#include "frame_controller.h"

namespace window_frame_kit {
namespace test {

// 800x600 window at (100, 200) on screen.
static constexpr RECT kWin = {100, 200, 900, 800};

// --- FrameApplyEdgeBands: equal-width NC-band inset (the redesign core) ---

TEST(FrameApplyEdgeBands, InsetsSidesEquallyTopOnePixel) {
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, /*band=*/8);
  EXPECT_EQ(rect.left, 108);
  EXPECT_EQ(rect.right, 892);
  EXPECT_EQ(rect.bottom, 792);
  // Top gets exactly 1px: the DWM border line only - a full top band makes
  // the system paint a titlebar-black strip above the self-drawn titlebar.
  EXPECT_EQ(rect.top - kWin.top, 1);
  // L/R/bottom uniform 8px bands.
  EXPECT_EQ(rect.left - kWin.left, kWin.right - rect.right);
  EXPECT_EQ(rect.left - kWin.left, kWin.bottom - rect.bottom);
}

TEST(FrameTopBandHitTest, ClientTopBandMapsToHttop) {
  // Window top at y=200, 8px band: y in [200, 208] hits HTTOP.
  EXPECT_EQ(*FrameTopBandHitTest(500, 204, kWin, 8), HTTOP);
  EXPECT_EQ(*FrameTopBandHitTest(500, 200, kWin, 8), HTTOP);
  EXPECT_EQ(*FrameTopBandHitTest(500, 208, kWin, 8), HTTOP);
  // One pixel below the band: interior.
  EXPECT_FALSE(FrameTopBandHitTest(500, 209, kWin, 8).has_value());
  EXPECT_FALSE(FrameTopBandHitTest(500, 500, kWin, 8).has_value());
}

TEST(FrameTopBandHitTest, CornersWinOverPlainTop) {
  // Cursor inside the top band AND the left/right side band -> corner codes,
  // matching the native side-band corner behavior.
  EXPECT_EQ(*FrameTopBandHitTest(104, 204, kWin, 8), HTTOPLEFT);
  EXPECT_EQ(*FrameTopBandHitTest(896, 204, kWin, 8), HTTOPRIGHT);
  // Just outside the side bands -> plain HTTOP.
  EXPECT_EQ(*FrameTopBandHitTest(109, 204, kWin, 8), HTTOP);
  EXPECT_EQ(*FrameTopBandHitTest(891, 204, kWin, 8), HTTOP);
}

TEST(FrameApplyEdgeBands, ZeroBandKeepsOnePixelTop) {
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, 0);
  EXPECT_EQ(rect.left, kWin.left);
  EXPECT_EQ(rect.right, kWin.right);
  EXPECT_EQ(rect.bottom, kWin.bottom);
  // Top is always +1 regardless of band: it is the DWM border line, not a
  // resize band.
  EXPECT_EQ(rect.top, kWin.top + 1);
}

TEST(FrameApplyEdgeBands, BandScalesWithDpiInjectedValue) {
  // 150% DPI: the caller computes a ~6px band and applies it to the sides;
  // the top stays at the fixed 1px border line.
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, /*band=*/6);
  EXPECT_EQ(rect.left, 106);
  EXPECT_EQ(rect.top, 201);
  EXPECT_EQ(rect.right, 894);
  EXPECT_EQ(rect.bottom, 794);
}

// --- FrameAdjustMinMaxInfo: cooperative merge (DEVIATIONS #1) ---

TEST(FrameAdjustMinMaxInfo, MaximizedClampsToWorkArea) {
  MINMAXINFO info{};
  info.ptMaxPosition = POINT{111, 222};  // sentinels: must be overwritten
  info.ptMaxSize = POINT{333, 444};
  const RECT work = {0, 0, 1920, 1040};  // taskbar eats 40px of 1080
  FrameAdjustMinMaxInfo(&info, /*minX=*/854, /*minY=*/480, /*maxX=*/-1,
                        /*maxY=*/-1, /*pixelRatio=*/1.0, work,
                        /*maximized=*/true);
  EXPECT_EQ(info.ptMaxPosition.x, 0);
  EXPECT_EQ(info.ptMaxPosition.y, 0);
  EXPECT_EQ(info.ptMaxSize.x, 1920);
  EXPECT_EQ(info.ptMaxSize.y, 1040);
}

TEST(FrameAdjustMinMaxInfo, NotMaximizedLeavesPtMaxAlone) {
  MINMAXINFO info{};
  info.ptMaxPosition = POINT{111, 222};
  info.ptMaxSize = POINT{333, 444};
  const RECT work = {0, 0, 1920, 1040};
  FrameAdjustMinMaxInfo(&info, 854, 480, -1, -1, 1.0, work, /*maximized=*/false);
  EXPECT_EQ(info.ptMaxPosition.x, 111);
  EXPECT_EQ(info.ptMaxPosition.y, 222);
  EXPECT_EQ(info.ptMaxSize.x, 333);
  EXPECT_EQ(info.ptMaxSize.y, 444);
}

TEST(FrameAdjustMinMaxInfo, MinTrackScalesByPixelRatio) {
  MINMAXINFO info{};
  // 150% DPI: logical 854x480 must become physical 1281x720.
  FrameAdjustMinMaxInfo(&info, 854, 480, -1, -1, 1.5, kWin,
                        /*maximized=*/false);
  EXPECT_EQ(info.ptMinTrackSize.x, 1281);
  EXPECT_EQ(info.ptMinTrackSize.y, 720);
}

TEST(FrameAdjustMinMaxInfo, MaxTrackScalesAndZeroMinIsNoop) {
  MINMAXINFO info{};
  info.ptMinTrackSize = POINT{7, 8};  // sentinel: zero min must not touch
  FrameAdjustMinMaxInfo(&info, /*minX=*/0, /*minY=*/0, /*maxX=*/1000,
                        /*maxY=*/800, /*pixelRatio=*/1.0, kWin, false);
  EXPECT_EQ(info.ptMinTrackSize.x, 7);
  EXPECT_EQ(info.ptMinTrackSize.y, 8);
  EXPECT_EQ(info.ptMaxTrackSize.x, 1000);
  EXPECT_EQ(info.ptMaxTrackSize.y, 800);
}

// --- FrameAdjustNccalcSizeToWork: maximized band clamp ---

TEST(FrameAdjustNccalcSizeToWork, ShrinksInflatedRectToWorkArea) {
  // System inflates a borderless maximized window 8px past the work area.
  RECT rgrc = {-8, -8, 1928, 1048};
  const RECT work = {0, 0, 1920, 1040};
  FrameAdjustNccalcSizeToWork(&rgrc, work);
  EXPECT_EQ(rgrc.left, 0);
  EXPECT_EQ(rgrc.top, 0);
  EXPECT_EQ(rgrc.right, 1920);
  EXPECT_EQ(rgrc.bottom, 1040);
}

TEST(FrameAdjustNccalcSizeToWork, RectAlreadyMatchingWorkIsUntouched) {
  RECT rgrc = {0, 0, 1920, 1040};
  const RECT work = {0, 0, 1920, 1040};
  FrameAdjustNccalcSizeToWork(&rgrc, work);
  EXPECT_EQ(rgrc.left, 0);
  EXPECT_EQ(rgrc.top, 0);
  EXPECT_EQ(rgrc.right, 1920);
  EXPECT_EQ(rgrc.bottom, 1040);
}

TEST(FrameAdjustNccalcSizeToWork, RestoredWindowOnSecondMonitorClamps) {
  // Host issue #489 shape: minimized-restore lands on the wrong monitor; the
  // caller resolves the nearest work area and this function only maps rects.
  RECT rgrc = {2000 - 8, 100 - 8, 2000 + 1280 + 8, 100 + 720 + 8};
  const RECT work = {2000, 100, 3280, 820};
  FrameAdjustNccalcSizeToWork(&rgrc, work);
  EXPECT_EQ(rgrc.left, 2000);
  EXPECT_EQ(rgrc.top, 100);
  EXPECT_EQ(rgrc.right, 3280);
  EXPECT_EQ(rgrc.bottom, 820);
}

// --- IsExternalFullscreen style predicate (media_kit strips the styles) ---

TEST(IsExternalFullscreenStyle, MatchesWindowManagerSetStyleMask) {
  // Fullscreen present: all OVERLAPPEDWINDOW bits cleared.
  EXPECT_TRUE(IsExternalFullscreenStyle(WS_POPUP | WS_VISIBLE));
  // Windowed: style keeps the full OVERLAPPEDWINDOW set.
  EXPECT_FALSE(IsExternalFullscreenStyle(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
}

}  // namespace test
}  // namespace window_frame_kit
