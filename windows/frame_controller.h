// // FRAME: Phase 3 custom-frame graft (03-02).
// Borderless WM_NCCALCSIZE + 4-edge/4-corner DPI-aware WM_NCHITTEST +
// cooperative WM_GETMINMAXINFO, grafted as one self-contained unit next to
// the verbatim window_manager 0.5.2 port. Zero runner wiring, zero Dart win32
// dependencies. Pure geometry/math helpers are inline here so the gtest TU
// can pin their behavior; the HWND-touching state machine lives in the .cpp.
#pragma once

#include <windows.h>
#include <commctrl.h>

#include <functional>
#include <optional>

namespace window_frame_kit {

// // FRAME: lParam cursor extraction (windowsx.h GET_X_LPARAM equivalents).
// windowsx.h itself is deliberately NOT included: it defines function-like
// macros such as `IsMaximized(hwnd) -> IsZoomed(hwnd)` which would hijack the
// verbatim upstream `window_manager->IsMaximized()` method calls in the same
// translation unit and break the build. Signed short keeps multi-monitor
// negative coordinates intact.
inline LONG FrameXFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(LOWORD(lParam));
}
inline LONG FrameYFromLParam(LPARAM lParam) {
  return static_cast<SHORT>(HIWORD(lParam));
}

// Subclass id used for the Flutter-view child window hook (single hook per
// plugin instance; the id is namespace-local to our SetWindowSubclass calls).
constexpr UINT_PTR kFrameChildSubclassId = 0x57464B31;  // 'WFK1'

/// // FRAME: Pure geometry - map a screen point to an HT* resize code for the
/// window rect, using an inclusive edge band of `edgeWidth` px. Corners win
/// over edges when inside both bands. Returns nullopt for the interior; the
/// caller decides the interior result (child window -> HTCLIENT, top-level
/// delegate -> nullopt so the system default applies).
inline std::optional<LONG> FrameEdgeHitTest(LONG x, LONG y,
                                            const RECT& rect, int edgeWidth) {
  const LONG dx = x - rect.left;
  const LONG dy = y - rect.top;
  const LONG dr = rect.right - x;
  const LONG db = rect.bottom - y;
  const bool nearLeft = dx >= 0 && dx <= edgeWidth;
  const bool nearRight = dr >= 0 && dr <= edgeWidth;
  const bool nearTop = dy >= 0 && dy <= edgeWidth;
  const bool nearBottom = db >= 0 && db <= edgeWidth;
  if (nearLeft && nearTop) return HTTOPLEFT;
  if (nearRight && nearTop) return HTTOPRIGHT;
  if (nearLeft && nearBottom) return HTBOTTOMLEFT;
  if (nearRight && nearBottom) return HTBOTTOMRIGHT;
  if (nearLeft) return HTLEFT;
  if (nearRight) return HTRIGHT;
  if (nearTop) return HTTOP;
  if (nearBottom) return HTBOTTOM;
  return std::nullopt;
}

/// // FRAME: Edge resizing is allowed only when the window is resizable and
/// not fullscreen. Fullscreen covers BOTH paths (see IsExternalFullscreenStyle
/// and WindowManager::IsFullScreen) - bitsdojo's missing guard is the bug this
/// graft must not reproduce.
inline constexpr bool FrameHitAllowed(bool resizable, bool fullscreen) {
  return resizable && !fullscreen;
}

/// // FRAME: Style mask for the graft - the Chromium/Electron frameless
/// recipe. WM_NCCALCSIZE returning 0 only remaps the client rect; during a
/// drag-resize modal loop DefWindowProc still repainted the non-client area
/// computed from WS_CAPTION, so the themed border flashed back mid-drag
/// (2026-09-05 real-machine finding). Dropping WS_CAPTION while keeping
/// WS_THICKFRAME removes what the loop would repaint while preserving
/// native edge resizing. Applied on Enable and restored on Disable.
inline LONG_PTR FrameCalcGraftedStyle(LONG_PTR style) {
  const LONG_PTR withoutCaption = style & ~static_cast<LONG_PTR>(WS_CAPTION);
  return withoutCaption | WS_THICKFRAME;
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
/// into FrameEdgeHitTest directly.
int FrameEdgeWidthForWindow(HWND hwnd);

/// // FRAME: Runtime state machine for the graft. Owns the child-window
/// subclass; the top-level message path stays in the plugin's existing
/// HandleWindowProc delegate, which consults IsEnabled() first and falls
/// through to the upstream branches otherwise.
class FrameController {
 public:
  FrameController() = default;

  FrameController(const FrameController&) = delete;
  FrameController& operator=(const FrameController&) = delete;

  ~FrameController();  // Disables (removes the subclass) if still enabled.

  bool IsEnabled() const { return enabled_; }

  /// Installs the child subclass and forces a frame recalc. Both state
  /// probes are injected callbacks (the plugin wires them to
  /// WindowManager::is_resizable_ / IsFullScreen) so values are read live at
  /// hit-test time and this class needs no dependency on the
  /// anonymous-namespace port types. Returns false (no partial state) when
  /// the subclass cannot be installed.
  bool Enable(HWND root, HWND child,
              std::function<bool()> isResizable,
              std::function<bool()> isPluginFullscreen);

  /// Removes the subclass, restores the system frame, forces a recalc.
  /// Idempotent: disabling twice is a no-op.
  void Disable();

  /// Subclass proc for the Flutter-view child window. Handles WM_NCHITTEST
  /// edge/corner dispatch (fullscreen or non-resizable -> HTCLIENT) and
  /// self-unloads on WM_NCDESTROY.
  static LRESULT CALLBACK ChildProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam, UINT_PTR uIdSubclass,
                                    DWORD_PTR dwRefData);

 private:
  bool enabled_ = false;
  HWND root_ = nullptr;
  HWND child_ = nullptr;
  // Pre-graft GWL_STYLE, restored verbatim on Disable.
  LONG_PTR style_before_graft_ = 0;
  // Live probes, not snapshots: setResizable/setFullScreen must reflect in
  // the graft immediately without re-arming it.
  std::function<bool()> is_resizable_;
  std::function<bool()> is_plugin_fullscreen_;
};

}  // namespace window_frame_kit
