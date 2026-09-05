// // FRAME: pure-function tests for the Phase 3 frame graft (03-02).
// No live HWND involved: geometry math, work-area clamping and the
// cooperative GETMINMAXINFO merge are exercised as injectable math so they
// stay deterministic under gtest. State-machine bits (subclass install /
// teardown) need a real window tree and are covered by the UAT batch.
#include <gtest/gtest.h>

#include "frame_controller.h"

namespace window_frame_kit {
namespace test {

// 800x600 window at (100, 200) on screen, 8px edge band.
static constexpr RECT kWin = {100, 200, 900, 800};
static constexpr int kEdge = 8;

// --- FrameEdgeHitTest: pure geometry, 4 edges + 4 corners + interior ---

TEST(FrameEdgeHitTest, TopEdgeCenterHitsHttop) {
  const auto hit = FrameEdgeHitTest(500, 204, kWin, kEdge);  // 4px inside top
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(*hit, HTTOP);
}

TEST(FrameEdgeHitTest, BottomEdgeCenterHitsHtbottom) {
  const auto hit = FrameEdgeHitTest(500, 796, kWin, kEdge);
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(*hit, HTBOTTOM);
}

TEST(FrameEdgeHitTest, LeftEdgeCenterHitsHtleft) {
  const auto hit = FrameEdgeHitTest(104, 500, kWin, kEdge);
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(*hit, HTLEFT);
}

TEST(FrameEdgeHitTest, RightEdgeCenterHitsHtright) {
  const auto hit = FrameEdgeHitTest(896, 500, kWin, kEdge);
  ASSERT_TRUE(hit.has_value());
  EXPECT_EQ(*hit, HTRIGHT);
}

TEST(FrameEdgeHitTest, FourCornersHitCornerCodes) {
  EXPECT_EQ(*FrameEdgeHitTest(100, 200, kWin, kEdge), HTTOPLEFT);
  EXPECT_EQ(*FrameEdgeHitTest(900, 200, kWin, kEdge), HTTOPRIGHT);
  EXPECT_EQ(*FrameEdgeHitTest(100, 800, kWin, kEdge), HTBOTTOMLEFT);
  EXPECT_EQ(*FrameEdgeHitTest(900, 800, kWin, kEdge), HTBOTTOMRIGHT);
}

TEST(FrameEdgeHitTest, InteriorHasNoHit) {
  EXPECT_FALSE(FrameEdgeHitTest(500, 500, kWin, kEdge).has_value());
}

TEST(FrameEdgeHitTest, BandBoundaryInclusiveOutsideExclusive) {
  // Exactly edgeWidth px from the left border -> hit; one pixel further -> no.
  EXPECT_TRUE(FrameEdgeHitTest(100 + kEdge, 500, kWin, kEdge).has_value());
  EXPECT_FALSE(FrameEdgeHitTest(100 + kEdge + 1, 500, kWin, kEdge).has_value());
}

TEST(FrameEdgeHitTest, CornerWinsOverEdgeWhenInsideBothBands) {
  // 3px from top AND 3px from left: corner code, not HTTOP/HTLEFT.
  EXPECT_EQ(*FrameEdgeHitTest(103, 203, kWin, kEdge), HTTOPLEFT);
}

// --- Fullscreen/resizable gating is applied by callers via HitAllowed;
//     its truth table is pinned here so both call sites agree. ---

TEST(FrameHitAllowed, DisabledWhenFullscreenOrNotResizable) {
  EXPECT_TRUE(FrameHitAllowed(/*resizable=*/true, /*fullscreen=*/false));
  EXPECT_FALSE(FrameHitAllowed(true, true));
  EXPECT_FALSE(FrameHitAllowed(false, false));
  EXPECT_FALSE(FrameHitAllowed(false, true));
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

// --- FrameAdjustNccalcSizeToWork: maximized borderless content clamp ---

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
