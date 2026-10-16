// // FRAME: 0.3.0 pure frame geometry. Every decision the custom frame makes
// (client inset per window state, resize hit-testing, band thickness per
// DPI, per-OS-build DWM policy, NCCALCSIZE return value) lives here as a
// side-effect-free inline function over plain values - no HWND, no system
// calls - so windows/test/frame_controller_test.cpp can pin the behavior
// for Win10 vs Win11, normal/maximized/fullscreen and DPI 96..192 without a
// live window tree. The HWND-touching glue lives in frame_controller.cpp.
//
// Techniques and their upstream sources:
// - Full-client frameless rect with the client top pushed 1px ABOVE the
//   window (`top -= 1`) plus a 1px top DwmExtendFrameIntoClientArea margin:
//   bitsdojo_window_windows-0.1.6 bitsdojo_window.cpp:166 and :312. The 1px
//   extended-frame strip sits in the client row that lies outside the window
//   rect, so DWM keeps the drop shadow while nothing frame-colored (no Win10
//   accent line) is ever visible.
// - Resize bands computed in WM_NCHITTEST over the client, thickness =
//   SM_CXSIZEFRAME + SM_CXPADDEDBORDER for the window DPI:
//   bitsdojo_window.cpp:152-162 (getResizeMargin).
// - Maximized client = monitor work area, minus 1px on every edge that hosts
//   an auto-hide appbar so the taskbar can still be revealed: the Windows
//   Terminal (NonClientIslandWindow::_OnNcCalcSize) / Chromium
//   (HWNDMessageHandler::OnNCCalcSize) recipe.
// - WVR_REDRAW only during an interactive size-move: Chromium
//   HWNDMessageHandler::OnNCCalcSize ("Returning WVR_REDRAW avoids an extra
//   paint ... of the old client pixels in the (now wrong) location").
#pragma once

#include <windows.h>

#include <dwmapi.h>
#include <shellapi.h>

namespace window_frame_kit {

/// First Windows 11 build number (21H2 GA). Decided at runtime from
/// RtlGetVersion, never at compile time.
constexpr DWORD kFrameFirstWindows11Build = 22000;

inline bool FrameIsWindows11Build(DWORD build) {
  return build >= kFrameFirstWindows11Build;
}

/// Window state as seen by the frame. Fullscreen wins over maximized.
enum class FrameWindowState { kNormal, kMaximized, kFullscreen };

inline FrameWindowState FrameStateFrom(bool maximized, bool fullscreen) {
  if (fullscreen) {
    return FrameWindowState::kFullscreen;
  }
  return maximized ? FrameWindowState::kMaximized : FrameWindowState::kNormal;
}

/// Auto-hide appbar edge bitmask (one bit per ABE_* edge).
constexpr UINT kFrameEdgeLeft = 1u << ABE_LEFT;
constexpr UINT kFrameEdgeTop = 1u << ABE_TOP;
constexpr UINT kFrameEdgeRight = 1u << ABE_RIGHT;
constexpr UINT kFrameEdgeBottom = 1u << ABE_BOTTOM;

/// Scales a 96-DPI metric to `dpi`, rounding like MulDiv (the same rounding
/// GetSystemMetricsForDpi applies to frame metrics).
inline int FrameScaleForDpi(int value96, UINT dpi) {
  return MulDiv(value96, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
}

/// Resize band thickness from the system metrics already scaled for the
/// window DPI. Never below 1px so a band always exists on a resizable
/// window, even under exotic metric settings.
inline int FrameResizeBand(int sizeFrame, int paddedBorder) {
  const int band = sizeFrame + paddedBorder;
  return band < 1 ? 1 : band;
}

/// Client rect for WM_NCCALCSIZE (`rect` = proposed window rect in, client
/// rect out).
/// - kNormal: the whole window is client (no non-client frame on any edge,
///   hence no Win10 accent border), top pushed 1px above the window so the
///   1px DWM shadow margin is never visible.
/// - kMaximized: the monitor work area (the window itself overhangs the
///   monitor by the frame thickness), minus 1px per auto-hide appbar edge.
/// - kFullscreen: the whole window rect, untouched.
inline void FrameClientRect(RECT* rect, FrameWindowState state,
                            const RECT& workArea, UINT autoHideEdges) {
  switch (state) {
    case FrameWindowState::kNormal:
      rect->top -= 1;
      return;
    case FrameWindowState::kMaximized: {
      // Work area clipped to the proposed window rect: a maximumSize
      // constraint can make the maximized window smaller than the work
      // area, and the client must never extend past the window.
      const RECT window = *rect;
      *rect = workArea;
      rect->left = rect->left > window.left ? rect->left : window.left;
      rect->top = rect->top > window.top ? rect->top : window.top;
      rect->right = rect->right < window.right ? rect->right : window.right;
      rect->bottom =
          rect->bottom < window.bottom ? rect->bottom : window.bottom;
      if (autoHideEdges & kFrameEdgeLeft) {
        rect->left += 1;
      }
      if (autoHideEdges & kFrameEdgeTop) {
        rect->top += 1;
      }
      if (autoHideEdges & kFrameEdgeRight) {
        rect->right -= 1;
      }
      if (autoHideEdges & kFrameEdgeBottom) {
        rect->bottom -= 1;
      }
      return;
    }
    case FrameWindowState::kFullscreen:
      return;
  }
}

/// WM_NCCALCSIZE return value for wParam == TRUE. WVR_REDRAW during an
/// interactive edge drag stops the system from BitBlt-ing stale client
/// pixels to the new origin (visible as content "swimming" when dragging
/// the left/top edge); 0 otherwise, so programmatic frame changes keep the
/// default (cheap) behavior.
inline LRESULT FrameNcCalcSizeResult(FrameWindowState state,
                                     bool interactiveSizing) {
  if (state == FrameWindowState::kNormal && interactiveSizing) {
    return WVR_REDRAW;
  }
  return 0;
}

/// Resize hit-test over the window rect (screen coordinates). Every edge and
/// corner uses the same `band`, so all four edges are equally thick.
/// Maximized / fullscreen / non-resizable windows never report a resize
/// code. Returns HTCLIENT when the point is not on a band.
inline LRESULT FrameHitTest(LONG x, LONG y, const RECT& window, int band,
                            FrameWindowState state, bool resizable) {
  if (!resizable || state != FrameWindowState::kNormal) {
    return HTCLIENT;
  }
  if (x < window.left || x >= window.right || y < window.top ||
      y >= window.bottom) {
    return HTCLIENT;  // Outside the window: not ours to classify.
  }
  const bool left = x < window.left + band;
  const bool right = x >= window.right - band;
  const bool top = y < window.top + band;
  const bool bottom = y >= window.bottom - band;
  if (top && left) {
    return HTTOPLEFT;
  }
  if (top && right) {
    return HTTOPRIGHT;
  }
  if (bottom && left) {
    return HTBOTTOMLEFT;
  }
  if (bottom && right) {
    return HTBOTTOMRIGHT;
  }
  if (top) {
    return HTTOP;
  }
  if (bottom) {
    return HTBOTTOM;
  }
  if (left) {
    return HTLEFT;
  }
  if (right) {
    return HTRIGHT;
  }
  return HTCLIENT;
}

/// True for the HT codes FrameHitTest can produce on a band.
inline bool FrameIsResizeHit(LRESULT hit) {
  switch (hit) {
    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTBOTTOM:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
      return true;
    default:
      return false;
  }
}

/// External-fullscreen predicate: some fullscreen paths (e.g. media_kit's)
/// strip WS_OVERLAPPEDWINDOW from the top-level style, complementing the
/// plugin's own IsFullScreen() state (which only strips WS_THICKFRAME).
inline bool IsExternalFullscreenStyle(LONG_PTR style) {
  return (style & WS_OVERLAPPEDWINDOW) == 0;
}

/// Window style while the custom frame is enabled: WS_CAPTION removed,
/// everything else (WS_THICKFRAME for native resize/Aero Snap, the
/// min/max boxes for taskbar + Win+Arrow behavior) kept. With WS_CAPTION
/// present, DefWindowProc repaints a themed caption/border inside the
/// interactive resize loop even though the client covers the whole window
/// (observed on this package's 0.1.0, fixed in 0.1.1 - the
/// Chromium/Electron frameless recipe).
inline LONG_PTR FrameEnabledStyle(LONG_PTR style) {
  return style & ~static_cast<LONG_PTR>(WS_CAPTION);
}

/// Style to restore on disable: the current style (which may have been
/// changed meanwhile by setResizable/fullscreen) plus whatever WS_CAPTION
/// bits the window had before the frame was enabled.
inline LONG_PTR FrameRestoredStyle(LONG_PTR current, LONG_PTR beforeEnable) {
  return current | (beforeEnable & static_cast<LONG_PTR>(WS_CAPTION));
}

/// Client size the frame will assign to a normal-state window of
/// `windowWidth` x `windowHeight` (see FrameClientRect: the client is the
/// whole window plus the 1px row above it). Used to tell the Flutter view
/// its next size before the window geometry changes.
inline SIZE FrameNormalClientSize(LONG windowWidth, LONG windowHeight) {
  RECT rect = {0, 0, windowWidth, windowHeight};
  FrameClientRect(&rect, FrameWindowState::kNormal, rect, 0);
  return {rect.right - rect.left, rect.bottom - rect.top};
}

/// Number of DWM composition instants (vblank + k * period) inside the
/// half-open interval (from, to], all in QPC ticks. A view whose surface
/// size differs from its HWND size during such an instant is composed
/// stretched - this counts those frames. Returns 0 for empty/invalid input.
inline LONGLONG FrameVblanksBetween(LONGLONG from, LONGLONG to,
                                     LONGLONG vblank, LONGLONG period) {
  if (period <= 0 || to <= from) {
    return 0;
  }
  // floor((t - vblank) / period) with correct rounding for negatives.
  const auto index = [vblank, period](LONGLONG t) {
    const LONGLONG d = t - vblank;
    return d >= 0 ? d / period : -((-d + period - 1) / period);
  };
  return index(to) - index(from);
}

/// Per-OS DWM policy applied while the custom frame is enabled.
struct FrameDwmPolicy {
  /// DwmExtendFrameIntoClientArea margins: 1px top keeps the drop shadow
  /// (paired with the `top -= 1` client overhang, see FrameClientRect).
  MARGINS margins;
  /// Win11 only: request DWMWCP_ROUND corners explicitly - a window without
  /// WS_CAPTION is not rounded by default. The attribute does not exist
  /// before build 22000, so it is never sent to Win10.
  bool requestRoundCorners;
};

inline FrameDwmPolicy FrameDwmPolicyForBuild(DWORD build) {
  FrameDwmPolicy policy = {};
  policy.margins = {0, 0, 1, 0};
  policy.requestRoundCorners = FrameIsWindows11Build(build);
  return policy;
}

// lParam cursor extraction (windowsx.h GET_X/Y_LPARAM equivalents).
// windowsx.h itself is deliberately NOT included: its function-like
// `IsMaximized(hwnd)` macro would hijack the upstream
// `window_manager->IsMaximized()` method calls in the plugin TU. Signed
// short keeps multi-monitor negative coordinates intact.
inline LONG FrameXFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(LOWORD(lParam));
}
inline LONG FrameYFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(HIWORD(lParam));
}

}  // namespace window_frame_kit
