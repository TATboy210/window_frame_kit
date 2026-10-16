// // FRAME: pure-function tests for the 0.3.0 frame geometry (no HWND). Covers
// Win10 vs Win11 policy, normal / maximized / fullscreen client mapping,
// DPI 96/120/144/192 band thickness, and edge-vs-corner hit-testing.
#include <gtest/gtest.h>

#include "frame_geometry.h"

namespace window_frame_kit {
namespace test {

// 800x600 window at (100, 200) on screen.
static constexpr RECT kWin = {100, 200, 900, 800};
// 1920x1080 monitor with a 48px bottom taskbar.
static constexpr RECT kWork = {0, 0, 1920, 1032};

// --- OS build policy -------------------------------------------------------

TEST(FrameOsPolicy, Windows11StartsAtBuild22000) {
  EXPECT_FALSE(FrameIsWindows11Build(19045));  // Win10 22H2
  EXPECT_FALSE(FrameIsWindows11Build(21999));
  EXPECT_TRUE(FrameIsWindows11Build(22000));   // Win11 21H2
  EXPECT_TRUE(FrameIsWindows11Build(26100));   // Win11 24H2
  EXPECT_FALSE(FrameIsWindows11Build(0));      // unknown -> Win10 path
}

TEST(FrameOsPolicy, Win10NeverGetsWin11OnlyAttributes) {
  const FrameDwmPolicy win10 = FrameDwmPolicyForBuild(19045);
  EXPECT_FALSE(win10.requestRoundCorners);
  const FrameDwmPolicy win11 = FrameDwmPolicyForBuild(22631);
  EXPECT_TRUE(win11.requestRoundCorners);
}

TEST(FrameOsPolicy, ShadowMarginIsOnePixelTopOnBothOses) {
  for (const DWORD build : {19045u, 22631u}) {
    const MARGINS m = FrameDwmPolicyForBuild(build).margins;
    EXPECT_EQ(m.cxLeftWidth, 0);
    EXPECT_EQ(m.cxRightWidth, 0);
    EXPECT_EQ(m.cyTopHeight, 1);
    EXPECT_EQ(m.cyBottomHeight, 0);
  }
}

// --- State resolution ------------------------------------------------------

TEST(FrameState, FullscreenWinsOverMaximized) {
  EXPECT_EQ(FrameStateFrom(false, false), FrameWindowState::kNormal);
  EXPECT_EQ(FrameStateFrom(true, false), FrameWindowState::kMaximized);
  EXPECT_EQ(FrameStateFrom(true, true), FrameWindowState::kFullscreen);
  EXPECT_EQ(FrameStateFrom(false, true), FrameWindowState::kFullscreen);
}

TEST(FrameState, ExternalFullscreenStyleDetection) {
  EXPECT_FALSE(IsExternalFullscreenStyle(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
  EXPECT_TRUE(IsExternalFullscreenStyle(WS_POPUP | WS_VISIBLE));
  // window_manager's own fullscreen strips only THICKFRAME|MAXIMIZEBOX.
  EXPECT_FALSE(IsExternalFullscreenStyle(WS_OVERLAPPEDWINDOW &
                                         ~(WS_THICKFRAME | WS_MAXIMIZEBOX)));
}

// --- NCCALCSIZE client mapping --------------------------------------------

TEST(FrameClientRect, NormalHasNoFrameOnAnyEdgeAndTopOverhang) {
  RECT r = kWin;
  FrameClientRect(&r, FrameWindowState::kNormal, kWork, 0);
  // No non-client band anywhere -> nothing for DWM to paint in the accent
  // color on Win10; top pushed 1px above the window hides the shadow margin.
  EXPECT_EQ(r.left, kWin.left);
  EXPECT_EQ(r.right, kWin.right);
  EXPECT_EQ(r.bottom, kWin.bottom);
  EXPECT_EQ(r.top, kWin.top - 1);
}

TEST(FrameClientRect, MaximizedMapsToWorkAreaNotOverhangingWindow) {
  // Maximized window overhangs the monitor by the 8px frame.
  RECT r = {-8, -8, 1928, 1040};
  FrameClientRect(&r, FrameWindowState::kMaximized, kWork, 0);
  EXPECT_EQ(r.left, 0);
  EXPECT_EQ(r.top, 0);
  EXPECT_EQ(r.right, 1920);
  EXPECT_EQ(r.bottom, 1032);
}

TEST(FrameClientRect, MaximizedLeavesOnePixelPerAutoHideEdge) {
  const RECT monitor = {0, 0, 1920, 1080};  // auto-hide: work == monitor
  RECT r = {-8, -8, 1928, 1088};
  FrameClientRect(&r, FrameWindowState::kMaximized, monitor,
                  kFrameEdgeBottom);
  EXPECT_EQ(r.bottom, 1079);
  EXPECT_EQ(r.top, 0);
  RECT all = {-8, -8, 1928, 1088};
  FrameClientRect(&all, FrameWindowState::kMaximized, monitor,
                  kFrameEdgeLeft | kFrameEdgeTop | kFrameEdgeRight |
                      kFrameEdgeBottom);
  EXPECT_EQ(all.left, 1);
  EXPECT_EQ(all.top, 1);
  EXPECT_EQ(all.right, 1919);
  EXPECT_EQ(all.bottom, 1079);
}

TEST(FrameClientRect, MaximizedOnSecondMonitorUsesThatWorkArea) {
  const RECT work2 = {1920, 0, 4480, 1400};
  RECT r = {1910, -10, 4490, 1410};
  FrameClientRect(&r, FrameWindowState::kMaximized, work2, 0);
  EXPECT_EQ(r.left, 1920);
  EXPECT_EQ(r.right, 4480);
}

TEST(FrameClientRect, FullscreenIsUntouched) {
  RECT r = {0, 0, 1920, 1080};
  FrameClientRect(&r, FrameWindowState::kFullscreen, kWork, kFrameEdgeBottom);
  EXPECT_EQ(r.left, 0);
  EXPECT_EQ(r.top, 0);
  EXPECT_EQ(r.right, 1920);
  EXPECT_EQ(r.bottom, 1080);
}

TEST(FrameNcCalcSizeResult, RedrawOnlyWhileInteractivelySizingNormal) {
  EXPECT_EQ(FrameNcCalcSizeResult(FrameWindowState::kNormal, true),
            static_cast<LRESULT>(WVR_REDRAW));
  EXPECT_EQ(FrameNcCalcSizeResult(FrameWindowState::kNormal, false), 0);
  EXPECT_EQ(FrameNcCalcSizeResult(FrameWindowState::kMaximized, true), 0);
  EXPECT_EQ(FrameNcCalcSizeResult(FrameWindowState::kFullscreen, true), 0);
}

// --- Band thickness per DPI -------------------------------------------------

TEST(FrameResizeBand, ScalesWithDpi) {
  // SM_CXSIZEFRAME = 4 and SM_CXPADDEDBORDER = 4 at 96 DPI (default
  // metrics); GetSystemMetricsForDpi scales both linearly.
  struct Case {
    UINT dpi;
    int band;
  };
  for (const Case c : {Case{96, 8}, Case{120, 10}, Case{144, 12},
                       Case{192, 16}}) {
    const int band = FrameResizeBand(FrameScaleForDpi(4, c.dpi),
                                     FrameScaleForDpi(4, c.dpi));
    EXPECT_EQ(band, c.band) << "dpi " << c.dpi;
  }
}

TEST(FrameResizeBand, NeverBelowOnePixel) {
  EXPECT_EQ(FrameResizeBand(0, 0), 1);
  EXPECT_EQ(FrameResizeBand(-3, 0), 1);
}

// --- Hit-testing -------------------------------------------------------------

TEST(FrameHitTest, AllFourEdgesShareTheSameThickness) {
  for (const int band : {8, 10, 12, 16}) {
    const auto hit = [&](LONG x, LONG y) {
      return FrameHitTest(x, y, kWin, band, FrameWindowState::kNormal, true);
    };
    const LONG midX = 500;
    const LONG midY = 500;
    // Inner-most band pixel on each edge resizes ...
    EXPECT_EQ(hit(midX, kWin.top + band - 1), HTTOP) << band;
    EXPECT_EQ(hit(midX, kWin.bottom - band), HTBOTTOM) << band;
    EXPECT_EQ(hit(kWin.left + band - 1, midY), HTLEFT) << band;
    EXPECT_EQ(hit(kWin.right - band, midY), HTRIGHT) << band;
    // ... and one pixel further in is client, symmetric on all edges.
    EXPECT_EQ(hit(midX, kWin.top + band), HTCLIENT) << band;
    EXPECT_EQ(hit(midX, kWin.bottom - band - 1), HTCLIENT) << band;
    EXPECT_EQ(hit(kWin.left + band, midY), HTCLIENT) << band;
    EXPECT_EQ(hit(kWin.right - band - 1, midY), HTCLIENT) << band;
  }
}

TEST(FrameHitTest, CornersWinOverEdges) {
  const int band = 8;
  const auto hit = [&](LONG x, LONG y) {
    return FrameHitTest(x, y, kWin, band, FrameWindowState::kNormal, true);
  };
  EXPECT_EQ(hit(kWin.left, kWin.top), HTTOPLEFT);
  EXPECT_EQ(hit(kWin.right - 1, kWin.top), HTTOPRIGHT);
  EXPECT_EQ(hit(kWin.left, kWin.bottom - 1), HTBOTTOMLEFT);
  EXPECT_EQ(hit(kWin.right - 1, kWin.bottom - 1), HTBOTTOMRIGHT);
  EXPECT_EQ(hit(kWin.left + band - 1, kWin.top + band - 1), HTTOPLEFT);
  EXPECT_EQ(hit(kWin.left + band, kWin.top), HTTOP);
}

TEST(FrameHitTest, MaximizedFullscreenAndFixedSizeHaveNoBands) {
  for (const FrameWindowState s :
       {FrameWindowState::kMaximized, FrameWindowState::kFullscreen}) {
    EXPECT_EQ(FrameHitTest(kWin.left, kWin.top, kWin, 8, s, true), HTCLIENT);
    EXPECT_EQ(FrameHitTest(500, kWin.top, kWin, 8, s, true), HTCLIENT);
  }
  EXPECT_EQ(FrameHitTest(500, kWin.top, kWin, 8, FrameWindowState::kNormal,
                         false),
            HTCLIENT);
}

TEST(FrameHitTest, NegativeMultiMonitorCoordinates) {
  const RECT left = {-1700, -300, -100, 600};
  EXPECT_EQ(FrameHitTest(-1700, 100, left, 8, FrameWindowState::kNormal, true),
            HTLEFT);
  EXPECT_EQ(FrameHitTest(-101, 599, left, 8, FrameWindowState::kNormal, true),
            HTBOTTOMRIGHT);
  // lParam round trip keeps the sign.
  const LPARAM lp = MAKELPARAM(static_cast<WORD>(-1700),
                               static_cast<WORD>(-300));
  EXPECT_EQ(FrameXFromLParam(lp), -1700);
  EXPECT_EQ(FrameYFromLParam(lp), -300);
}

TEST(FrameHitTest, OutsideWindowIsClientAndResizeHitClassifier) {
  EXPECT_EQ(FrameHitTest(50, 50, kWin, 8, FrameWindowState::kNormal, true),
            HTCLIENT);
  EXPECT_TRUE(FrameIsResizeHit(HTTOP));
  EXPECT_TRUE(FrameIsResizeHit(HTBOTTOMLEFT));
  EXPECT_FALSE(FrameIsResizeHit(HTCLIENT));
  EXPECT_FALSE(FrameIsResizeHit(HTCAPTION));
}

}  // namespace test
}  // namespace window_frame_kit
