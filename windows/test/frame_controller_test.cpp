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

TEST(FrameApplyEdgeBands, InsetsAllFourSidesEqually) {
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, /*band=*/8);
  EXPECT_EQ(rect.left, 108);
  EXPECT_EQ(rect.top, 208);
  EXPECT_EQ(rect.right, 892);
  EXPECT_EQ(rect.bottom, 792);
  // Uniform band on every side: 8px each.
  EXPECT_EQ(rect.left - kWin.left, 8);
  EXPECT_EQ(kWin.right - rect.right, 8);
  EXPECT_EQ(rect.top - kWin.top, 8);
  EXPECT_EQ(kWin.bottom - rect.bottom, 8);
}

TEST(FrameApplyEdgeBands, ZeroBandIsIdentity) {
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, 0);
  EXPECT_EQ(rect.left, kWin.left);
  EXPECT_EQ(rect.top, kWin.top);
  EXPECT_EQ(rect.right, kWin.right);
  EXPECT_EQ(rect.bottom, kWin.bottom);
}

TEST(FrameApplyEdgeBands, BandScalesWithDpiInjectedValue) {
  // 150% DPI: the caller computes a ~6px band and applies it; the math just
  // honors whatever band it is given.
  RECT rect = kWin;
  FrameApplyEdgeBands(&rect, /*band=*/6);
  EXPECT_EQ(rect.left, 106);
  EXPECT_EQ(rect.top, 206);
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
