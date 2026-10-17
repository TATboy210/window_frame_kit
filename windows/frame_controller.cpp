// // FRAME: custom-frame state machine implementation (0.3.0). Only this TU
// touches real HWNDs / system state; the geometry it applies comes from the
// pure functions in frame_geometry.h (unit-tested in
// windows/test/frame_controller_test.cpp).
#include "frame_controller.h"

#include <commctrl.h>

#include <utility>

namespace window_frame_kit {

namespace {

// Arbitrary but unique subclass id ("WFKF"), scoped to this plugin.
constexpr UINT_PTR kChildSubclassId = 0x57464B46;

using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

// The engine blocks a view resize for at most 100ms waiting for a frame of
// the new size (flutter_windows_view.cc kWindowResizeTimeout). A wait at or
// above this means it gave up and the stale frame stayed on screen.
constexpr double kEngineResizeTimeoutMs = 100.0;
constexpr double kTimeoutSlackMs = 5.0;

LONGLONG QpcNow() {
  LARGE_INTEGER now = {};
  ::QueryPerformanceCounter(&now);
  return now.QuadPart;
}

double QpcToMs(LONGLONG ticks) {
  static const LONGLONG frequency = []() {
    LARGE_INTEGER f = {};
    ::QueryPerformanceFrequency(&f);
    return f.QuadPart > 0 ? f.QuadPart : 1;
  }();
  return static_cast<double>(ticks) * 1000.0 /
         static_cast<double>(frequency);
}

constexpr UINT kFrameChangedFlags = SWP_FRAMECHANGED | SWP_NOMOVE |
                                    SWP_NOSIZE | SWP_NOZORDER |
                                    SWP_NOACTIVATE | SWP_NOOWNERZORDER;

}  // namespace

DWORD FrameOsBuildNumber() {
  static const DWORD build = []() -> DWORD {
    const HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
      return 0;
    }
    const auto fn = reinterpret_cast<RtlGetVersionFn>(
        ::GetProcAddress(ntdll, "RtlGetVersion"));
    if (fn == nullptr) {
      return 0;
    }
    RTL_OSVERSIONINFOW info = {};
    info.dwOSVersionInfoSize = sizeof(info);
    // STATUS_SUCCESS == 0.
    if (fn(&info) != 0) {
      return 0;
    }
    return info.dwBuildNumber;
  }();
  return build;
}

int FrameResizeBandForWindow(HWND hwnd) {
  // Per-monitor DPI of the window right now, so the band follows
  // WM_DPICHANGED / monitor moves without any cached state.
  const UINT dpi = ::GetDpiForWindow(hwnd);
  return FrameResizeBand(::GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi),
                         ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi));
}

UINT FrameAutoHideEdgesForMonitor(const RECT& monitorRect) {
  APPBARDATA state = {};
  state.cbSize = sizeof(state);
  const UINT_PTR taskbarState = ::SHAppBarMessage(ABM_GETSTATE, &state);
  if ((taskbarState & ABS_AUTOHIDE) == 0) {
    return 0;
  }
  UINT edges = 0;
  const UINT kEdges[] = {ABE_LEFT, ABE_TOP, ABE_RIGHT, ABE_BOTTOM};
  for (const UINT edge : kEdges) {
    APPBARDATA query = {};
    query.cbSize = sizeof(query);
    query.uEdge = edge;
    query.rc = monitorRect;
    // Multi-monitor aware: returns the auto-hide bar on this monitor edge.
    if (::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &query) != 0) {
      edges |= 1u << edge;
    }
  }
  return edges;
}

FrameController::~FrameController() {
  Disable();
}

bool FrameController::Enable(HWND root, HWND child,
                             std::function<bool()> isResizable,
                             std::function<bool()> isPluginFullscreen) {
  if (root == nullptr || !::IsWindow(root)) {
    return false;
  }
  if (enabled_ && root != root_) {
    Disable();  // Re-target: never leave a hook on a stale window.
  }
  if (verify_message_ == 0) {
    verify_message_ = ::RegisterWindowMessageW(L"WindowFrameKitVerifyViewSize");
  }
  is_resizable_ = std::move(isResizable);
  is_plugin_fullscreen_ = std::move(isPluginFullscreen);
  root_ = root;
  if (child_ == nullptr && child != nullptr && ::IsWindow(child)) {
    if (::SetWindowSubclass(child, &FrameController::ChildSubclassProc,
                            kChildSubclassId,
                            reinterpret_cast<DWORD_PTR>(this))) {
      child_ = child;
      RECT client = {};
      ::GetClientRect(child_, &client);
      engine_size_ = {client.right, client.bottom};
    }
  }
  if (!enabled_) {
    style_before_enable_ = ::GetWindowLongPtr(root_, GWL_STYLE);
    ::SetWindowLongPtr(root_, GWL_STYLE,
                       FrameEnabledStyle(style_before_enable_));
  }
  enabled_ = true;
  ApplyDwmPolicy(true);
  // Frame-only recalc so the client mapping applies on the next paint.
  ::SetWindowPos(root_, nullptr, 0, 0, 0, 0, kFrameChangedFlags);
  return true;
}

void FrameController::Disable() {
  if (!enabled_) {
    return;
  }
  enabled_ = false;  // Flip first so re-entrant messages see "off".
  interactive_sizing_ = false;
  RemoveChildHook();
  if (root_ != nullptr && ::IsWindow(root_)) {
    ApplyDwmPolicy(false);
    ::SetWindowLongPtr(
        root_, GWL_STYLE,
        FrameRestoredStyle(::GetWindowLongPtr(root_, GWL_STYLE),
                           style_before_enable_));
    ::SetWindowPos(root_, nullptr, 0, 0, 0, 0, kFrameChangedFlags);
  }
  root_ = nullptr;
}

void FrameController::RemoveChildHook() {
  if (child_ != nullptr) {
    ::RemoveWindowSubclass(child_, &FrameController::ChildSubclassProc,
                           kChildSubclassId);
    child_ = nullptr;
  }
}

void FrameController::ApplyDwmPolicy(bool enable) {
  const FrameDwmPolicy policy = FrameDwmPolicyForBuild(FrameOsBuildNumber());
  const MARGINS none = {0, 0, 0, 0};
  // Failures are non-fatal (e.g. DWM composition reset): the frame still
  // works, only the shadow/corner hint is lost.
  ::DwmExtendFrameIntoClientArea(root_, enable ? &policy.margins : &none);
  if (policy.requestRoundCorners) {
    const DWM_WINDOW_CORNER_PREFERENCE corner =
        enable ? DWMWCP_ROUND : DWMWCP_DEFAULT;
    ::DwmSetWindowAttribute(root_, DWMWA_WINDOW_CORNER_PREFERENCE, &corner,
                            sizeof(corner));
  }
}

FrameWindowState FrameController::CurrentState() const {
  const bool fullscreen =
      (is_plugin_fullscreen_ && is_plugin_fullscreen_()) ||
      IsExternalFullscreenStyle(::GetWindowLongPtr(root_, GWL_STYLE));
  return FrameStateFrom(::IsZoomed(root_) != FALSE, fullscreen);
}

LRESULT FrameController::HitTestScreenPoint(LONG x, LONG y) const {
  if (!enabled_ || root_ == nullptr) {
    return HTCLIENT;
  }
  RECT window = {};
  if (!::GetWindowRect(root_, &window)) {
    return HTCLIENT;
  }
  const bool resizable = is_resizable_ ? is_resizable_() : true;
  return FrameHitTest(x, y, window, FrameResizeBandForWindow(root_),
                      CurrentState(), resizable);
}

std::optional<LRESULT> FrameController::HandleTopLevelMessage(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!enabled_ || hwnd != root_) {
    return std::nullopt;
  }
  switch (message) {
    case WM_NCCALCSIZE: {
      if (!wParam) {
        return std::nullopt;  // Plain RECT form: default handling.
      }
      auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
      const FrameWindowState state = CurrentState();
      RECT work = params->rgrc[0];
      UINT autoHide = 0;
      if (state == FrameWindowState::kMaximized) {
        // Nearest monitor of the proposed rect, not of the HWND: after a
        // restore-from-minimized the HWND monitor can be stale
        // (window_manager issue #489).
        const HMONITOR monitor =
            ::MonitorFromRect(&params->rgrc[0], MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = {};
        info.cbSize = sizeof(info);
        if (monitor != nullptr && ::GetMonitorInfo(monitor, &info)) {
          work = info.rcWork;
          autoHide = FrameAutoHideEdgesForMonitor(info.rcMonitor);
        }
      }
      FrameClientRect(&params->rgrc[0], state, work, autoHide);
      return FrameNcCalcSizeResult(state, interactive_sizing_);
    }
    case WM_NCHITTEST:
      // No non-client area exists, so every point is client: classify the
      // bands ourselves (the child forwards band hits here via
      // HTTRANSPARENT).
      return HitTestScreenPoint(FrameXFromLParam(lParam),
                                FrameYFromLParam(lParam));
    case WM_WINDOWPOSCHANGING:
      if (lParam != 0) {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lParam);
        // Pure moves must pass through untouched (see FrameIsSizeChange).
        RECT current = {};
        const bool sizeChange =
            ::GetWindowRect(root_, &current) &&
            FrameIsSizeChange(pos->flags, current.right - current.left,
                              current.bottom - current.top, pos->cx, pos->cy);
        if (sizeChange) {
          PreSyncViewSize(*pos);
        }
        if (interactive_sizing_ && pre_sync_enabled_ && sizeChange) {
          // Never let the system BitBlt the old client pixels to the new
          // origin (left/top edge drags): the view repaints anyway. Never
          // on a pure move (see FrameIsSizeChange).
          pos->flags |= SWP_NOCOPYBITS;
        }
      }
      return std::nullopt;
    case WM_ERASEBKGND:
      // The Flutter view covers the whole client; erasing would only flash
      // the class brush during resize (bitsdojo_window.cpp:425).
      return 1;
    case WM_ENTERSIZEMOVE:
      interactive_sizing_ = true;
      return std::nullopt;  // Upstream still tracks move/resize events.
    case WM_EXITSIZEMOVE:
      interactive_sizing_ = false;
      return std::nullopt;
    default:
      break;
  }
  if (verify_message_ != 0 && message == verify_message_) {
    // A pre-sync told the engine a size the view never actually got (the
    // window change was vetoed or adjusted): tell it the real size.
    if (child_ != nullptr) {
      RECT client = {};
      ::GetClientRect(child_, &client);
      RECT host = {};
      ::GetClientRect(root_, &host);
      if (client.right != host.right || client.bottom != host.bottom) {
        // Re-fit the view to the host client like the runner's WM_SIZE
        // handler (which does not run when the change was vetoed).
        ::SetWindowPos(child_, nullptr, 0, 0, host.right, host.bottom,
                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE |
                           SWP_NOOWNERZORDER);
      } else if (client.right != engine_size_.cx ||
                 client.bottom != engine_size_.cy) {
        ::SendMessageW(child_, WM_SIZE, SIZE_RESTORED,
                       MAKELPARAM(client.right, client.bottom));
      }
    }
    return 0;
  }
  switch (message) {
    case WM_SYSCOMMAND:
      // Fullscreen must not be resizable via the system menu (Alt+Space).
      if ((wParam & 0xFFF0) == SC_SIZE &&
          CurrentState() == FrameWindowState::kFullscreen) {
        return 0;
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

// --- view resize pre-sync -----------------------------------------------
//
// Root cause of stale/stretched frames during interactive resizing (engine
// shell/platform/windows, Flutter 3.47): the engine learns a new view size
// only from the FLUTTERVIEW child's WM_SIZE (flutter_window.cc WM_SIZE ->
// FlutterWindowsView::OnWindowSizeChanged), which arrives AFTER the runner
// already moved the child (win32_window.cpp WM_SIZE -> MoveWindow). The
// engine then blocks the platform thread until a frame of the new size is
// presented (resize_status_ kResizeStarted -> kFrameGenerated -> kDone, max
// kWindowResizeTimeout = 100ms). For that whole wait the child HWND already
// has the new size while its swap chain (EGL_FIXED_SIZE_ANGLE, egl/manager.cc)
// still holds the old-size buffer, and DWM composes it stretched.
//
// Pre-sync: on the top-level WM_WINDOWPOSCHANGING of an interactive resize
// (before any geometry changes) the child receives WM_SIZE with the size it
// is about to get. The engine renders and presents the new-size frame and
// DwmFlush()es, and only then does the window geometry change - the
// remaining mismatch is the few microseconds between the present and the
// MoveWindow, which normally falls inside one DWM composition. The later
// real WM_SIZE carries the same size, so the engine does not wait again
// (SurfaceWillUpdate() == false).

void FrameController::PreSyncViewSize(const WINDOWPOS& pos) {
  if (!pre_sync_enabled_ || !interactive_sizing_ || sending_pre_sync_ ||
      child_ == nullptr || pos.hwnd != root_ || (pos.flags & SWP_NOSIZE) ||
      pos.cx <= 0 || pos.cy <= 0) {
    return;
  }
  if (CurrentState() != FrameWindowState::kNormal || ::IsIconic(root_)) {
    return;
  }
  // Pure moves never get here (the caller checks FrameIsSizeChange).
  const SIZE client = FrameNormalClientSize(pos.cx, pos.cy);
  if (client.cx == engine_size_.cx && client.cy == engine_size_.cy) {
    return;
  }
  sending_pre_sync_ = true;
  ::SendMessageW(child_, WM_SIZE, SIZE_RESTORED,
                 MAKELPARAM(client.cx, client.cy));
  sending_pre_sync_ = false;
  // The engine has presented a frame of the new size into the still-old
  // view HWND. Give the view its new size right now instead of waiting for
  // the host's WM_SIZE -> MoveWindow, which only runs after the system has
  // committed the top-level geometry (measured ~3ms later): while growing
  // the larger view is clipped by the old top-level rect (top-left
  // anchored, nothing stretched); while shrinking the top-level rect
  // shrinks in the same SetWindowPos. The resulting WM_SIZE carries the
  // size the engine already has, so it does not block again.
  ::SetWindowPos(child_, nullptr, 0, 0, client.cx, client.cy,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOCOPYBITS);
  // If this window change is vetoed or adjusted later, correct the view
  // and the engine once the message queue drains (HandleTopLevelMessage).
  if (verify_message_ != 0) {
    ::PostMessageW(root_, verify_message_, 0, 0);
  }
}

void FrameController::OnChildSize(HWND hwnd, WPARAM wParam, LPARAM lParam,
                                  LRESULT* result) {
  const SIZE size = {static_cast<LONG>(LOWORD(lParam)),
                     static_cast<LONG>(HIWORD(lParam))};
  const bool preSync = sending_pre_sync_;
  const LONGLONG entry = QpcNow();
  if (!preSync && pending_mismatch_since_ != 0) {
    // The HWND now gets the size the pre-synced frame was rendered for.
    RecordMismatch(pending_mismatch_since_, entry);
    pending_mismatch_since_ = 0;
  }
  const bool engineChange = size.cx > 0 && size.cy > 0 &&
                            (size.cx != engine_size_.cx ||
                             size.cy != engine_size_.cy);
  // The engine's FlutterWindow handles WM_SIZE and blocks here until a
  // frame of `size` was presented (or its 100ms timeout elapsed).
  *result = ::DefSubclassProc(hwnd, WM_SIZE, wParam, lParam);
  const LONGLONG exit = QpcNow();
  if (!engineChange) {
    return;
  }
  engine_size_ = size;
  ++stats_.steps;
  const double waitMs = QpcToMs(exit - entry);
  if (waitMs > stats_.maxSyncWaitMs) {
    stats_.maxSyncWaitMs = waitMs;
  }
  if (waitMs >= kEngineResizeTimeoutMs - kTimeoutSlackMs) {
    ++stats_.syncTimeouts;
  }
  if (preSync) {
    // New-size frame presented into the still-old HWND until it is moved.
    pending_mismatch_since_ = exit;
  } else {
    // HWND already resized; the old frame was shown for the whole wait.
    RecordMismatch(entry, exit);
  }
}

void FrameController::RecordMismatch(LONGLONG from, LONGLONG to) {
  if (to <= from) {
    return;
  }
  stats_.mismatchMs += QpcToMs(to - from);
  DWM_TIMING_INFO timing = {};
  timing.cbSize = sizeof(timing);
  // hwnd must be null on Windows 8.1+ (global composition timing).
  if (SUCCEEDED(::DwmGetCompositionTimingInfo(nullptr, &timing))) {
    stats_.staleComposedFrames += FrameVblanksBetween(
        from, to, static_cast<LONGLONG>(timing.qpcVBlank),
        static_cast<LONGLONG>(timing.qpcRefreshPeriod));
  }
}

ResizeSyncStats FrameController::TakeResizeSyncStats() {
  const ResizeSyncStats taken = stats_;
  stats_ = ResizeSyncStats();
  return taken;
}

ResizeSyncStats FrameController::RunResizeSweep(int steps, bool preSync) {
  if (!enabled_ || root_ == nullptr || steps <= 0) {
    return ResizeSyncStats();
  }
  RECT origin = {};
  if (!::GetWindowRect(root_, &origin)) {
    return ResizeSyncStats();
  }
  const bool savedPreSync = pre_sync_enabled_;
  const bool savedInteractive = interactive_sizing_;
  pre_sync_enabled_ = preSync;
  interactive_sizing_ = true;  // Same code path as an edge drag.
  stats_ = ResizeSyncStats();
  const LONG w0 = origin.right - origin.left;
  const LONG h0 = origin.bottom - origin.top;
  for (int i = 0; i < steps; ++i) {
    // Deltas 40..119px, alternating grow/shrink on each of the four edges;
    // left/top steps move the origin like a real left/top edge drag.
    const LONG d = 40 + (i * 37) % 80;
    LONG x = origin.left;
    LONG y = origin.top;
    LONG w = w0;
    LONG h = h0;
    switch (i % 8) {
      case 0: w += d; break;               // right edge out
      case 2: h += d; break;               // bottom edge out
      case 4: x -= d; w += d; break;       // left edge out
      case 6: y -= d; h += d; break;       // top edge out
      default: break;                      // odd steps: back to origin
    }
    ::SetWindowPos(root_, nullptr, x, y, w, h,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  }
  ::SetWindowPos(root_, nullptr, origin.left, origin.top, w0, h0,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  pre_sync_enabled_ = savedPreSync;
  interactive_sizing_ = savedInteractive;
  return TakeResizeSyncStats();
}

// static
LRESULT CALLBACK FrameController::ChildSubclassProc(HWND hwnd, UINT message,
                                                    WPARAM wParam,
                                                    LPARAM lParam,
                                                    UINT_PTR subclassId,
                                                    DWORD_PTR refData) {
  auto* self = reinterpret_cast<FrameController*>(refData);
  switch (message) {
    case WM_SIZE:
      if (self != nullptr && self->enabled_) {
        LRESULT result = 0;
        self->OnChildSize(hwnd, wParam, lParam, &result);
        return result;
      }
      break;
    case WM_NCHITTEST: {
      if (self != nullptr && self->enabled_) {
        const LRESULT hit = self->HitTestScreenPoint(
            FrameXFromLParam(lParam), FrameYFromLParam(lParam));
        if (FrameIsResizeHit(hit)) {
          // Same-thread parent receives the hit test and answers with the
          // resize code (bitsdojo_window.cpp:334-344 child recipe).
          return HTTRANSPARENT;
        }
      }
      break;
    }
    case WM_NCDESTROY:
      ::RemoveWindowSubclass(hwnd, &FrameController::ChildSubclassProc,
                             subclassId);
      if (self != nullptr && self->child_ == hwnd) {
        self->child_ = nullptr;
      }
      break;
    default:
      break;
  }
  return ::DefSubclassProc(hwnd, message, wParam, lParam);
}

}  // namespace window_frame_kit
