// // FRAME: Phase 3 custom-frame graft (03-02, redesigned 2026-09-05 to the
// equal-width NC-band route). Self-drawn-titlebar WM_NCCALCSIZE with a real
// non-client resize band on all four sides + cooperative WM_GETMINMAXINFO +
// dual-path fullscreen guard, grafted as one self-contained unit next to the
// verbatim window_manager 0.5.2 port. Zero runner wiring, zero Dart win32
// dependencies. Pure geometry/math helpers are inline here so the gtest TU
// can pin their behavior; the HWND-touching state machine lives in the .cpp.
// NOTE: windowsx.h is deliberately NOT included - it defines function-like
// macros such as `IsMaximized(hwnd) -> IsZoomed(hwnd)` which would hijack
// the verbatim upstream `window_manager->IsMaximized()` method calls in the
// same translation unit and break the build.
#pragma once

#include <windows.h>

#include <functional>
#include <optional>
// commctrl.h (SetWindowSubclass) no longer needed: the band design uses only
// the top-level WM_NCCALCSIZE path, no child-window subclass.

namespace window_frame_kit {

// // FRAME: lParam cursor-X/Y extraction (windowsx.h GET_X/Y_LPARAM
// equivalents). windowsx.h itself is deliberately NOT included: it defines
// function-like macros such as `IsMaximized(hwnd) -> IsZoomed(hwnd)` which
// would hijack the verbatim upstream `window_manager->IsMaximized()` method
// calls in the same translation unit and break the build. Signed short keeps
// multi-monitor negative coordinates intact.
inline LONG FrameXFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(LOWORD(lParam));
}
inline LONG FrameYFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(HIWORD(lParam));
}

/// // FRAME: NC-band inset (2026-09-05 redesign per user target: self-drawn
/// titlebar PLUS the system border - Win11 1px border + rounded corners -
/// and uniform 4-edge resize). Left/right/bottom shrink by `band` px - REAL
/// non-client area served natively by DefWindowProc (no themed-border flash:
/// the resize loop repaints the band itself). The TOP shrinks by exactly 1px
/// (bitsdojo recipe): a full top band makes the system paint a titlebar-black
/// strip above the self-drawn titlebar, while 1px keeps the DWM border line.
/// Top-edge resize is recovered by FrameTopBandHitTest. WS_CAPTION is never
/// touched so DWM keeps drawing the border/corners.
inline void FrameApplyEdgeBands(RECT* rect, int band) {
  rect->left += band;
  rect->top += 1;  // 1px top NC = the DWM border line only (bitsdojo recipe:
                   // a full top band makes the system paint a titlebar-black
                   // strip above the self-drawn titlebar).
  rect->right -= band;
  rect->bottom -= band;
}

/// // FRAME: Top-edge hit test over the CLIENT area. The left/right/bottom
/// bands are real NC area (served by DefWindowProc), but the top band is
/// only 1px - too thin to grab - so the delegate maps cursor positions in
/// the top `band` px of the client to HTTOP (bitsdojo's self-computed
/// NCHITTEST approach, minus its missing fullscreen guard). Corner codes
/// win when the cursor is also inside the side bands, so the top corners
/// behave exactly like the native side-band corners.
inline std::optional<LONG> FrameTopBandHitTest(LONG x, LONG y,
                                               const RECT& rect, int band) {
  if (y >= rect.top && y <= rect.top + band) {
    if (x <= rect.left + band) {
      return HTTOPLEFT;
    }
    if (x >= rect.right - band) {
      return HTTOPRIGHT;
    }
    return HTTOP;
  }
  return std::nullopt;
}

/// // FRAME: External-fullscreen predicate - media_kit's fullscreen path
/// strips WS_OVERLAPPEDWINDOW from the top-level style (host-project verified
/// behavior). window_manager's own fullscreen only strips
/// WS_THICKFRAME|WS_MAXIMIZEBOX, so this style check and the plugin's
/// IsFullScreen() are complementary, not redundant.
inline bool IsExternalFullscreenStyle(LONG_PTR style) {
  return (style & WS_OVERLAPPEDWINDOW) == 0;
}

/// // FRAME: Cooperative GETMINMAXINFO merge. Applies the logical min/max
/// sizes scaled by pixelRatio (0 min / -1 max mean unconstrained, matching
/// upstream storage semantics), and when the window is maximized clamps the
/// maximized position/size to the monitor work area - without this, Windows
/// maximizes a borderless window past the taskbar and content gets cropped.
/// Deliberately NOT bitsdojo's unconditional `return 0` (DEVIATIONS #1).
inline void FrameAdjustMinMaxInfo(MINMAXINFO* info, int minX, int minY,
                                  int maxX, int maxY, double pixelRatio,
                                  const RECT& workRect, bool maximized) {
  if (minX > 0) {
    info->ptMinTrackSize.x = static_cast<LONG>(minX * pixelRatio);
  }
  if (minY > 0) {
    info->ptMinTrackSize.y = static_cast<LONG>(minY * pixelRatio);
  }
  if (maxX > 0) {
    info->ptMaxTrackSize.x = static_cast<LONG>(maxX * pixelRatio);
  }
  if (maxY > 0) {
    info->ptMaxTrackSize.y = static_cast<LONG>(maxY * pixelRatio);
  }
  if (maximized) {
    info->ptMaxPosition.x = workRect.left;
    info->ptMaxPosition.y = workRect.top;
    info->ptMaxSize.x = workRect.right - workRect.left;
    info->ptMaxSize.y = workRect.bottom - workRect.top;
  }
}

/// // FRAME: NCCALCSIZE clamp for a maximized borderless window - map the
/// system-inflated window rect onto the nearest monitor's work area so the
/// visible client area neither crops nor covers the taskbar (host issue #489
/// shape: caller resolves the work area via MonitorFromRect, this function
/// only performs the rect mapping).
inline void FrameAdjustNccalcSizeToWork(RECT* windowRect,
                                        const RECT& workRect) {
  windowRect->left = workRect.left;
  windowRect->top = workRect.top;
  windowRect->right = workRect.right;
  windowRect->bottom = workRect.bottom;
}

/// // FRAME: Per-monitor-DPI resize band: standard sizing frame plus padded
/// border for this window's DPI (Win10 1607+ APIs; the package floor is
/// Windows 10). Non-pure (queries the system) so tests inject band widths
/// into FrameApplyEdgeBands directly.
int FrameEdgeWidthForWindow(HWND hwnd);

/// // FRAME: Runtime state machine for the graft. The top-level message path
/// lives in the plugin's existing HandleWindowProc delegate, which consults
/// IsEnabled() first and falls through to the upstream branches otherwise.
/// No child-window subclass is needed in the band design: resize bands are
/// real non-client area, so edge hit-testing is served by DefWindowProc and
/// the Flutter view (client area only) never sees edge messages.
class FrameController {
 public:
  FrameController() = default;

  FrameController(const FrameController&) = delete;
  FrameController& operator=(const FrameController&) = delete;

  ~FrameController();  // Disables (restores upstream frame) if still enabled.

  bool IsEnabled() const { return enabled_; }

  /// Arms the graft and forces a frame recalc. Both state probes are
  /// injected callbacks (the plugin wires them to
  /// WindowManager::is_resizable_ / IsFullScreen) so values are read live at
  /// message time. `root` is the top-level HWND the bands apply to.
  bool Enable(HWND root, std::function<bool()> isResizable,
              std::function<bool()> isPluginFullscreen);

  /// Disarms the graft, restoring the upstream frame mapping.
  /// Idempotent: disabling twice is a no-op.
  void Disable();

 private:
  bool enabled_ = false;
  HWND root_ = nullptr;
  // Live probes, not snapshots: setResizable/setFullScreen must reflect in
  // the graft immediately without re-arming it.
  std::function<bool()> is_resizable_;
  std::function<bool()> is_plugin_fullscreen_;
};

}  // namespace window_frame_kit
