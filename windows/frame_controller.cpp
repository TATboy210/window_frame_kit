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
  is_resizable_ = std::move(isResizable);
  is_plugin_fullscreen_ = std::move(isPluginFullscreen);
  root_ = root;
  if (child_ == nullptr && child != nullptr && ::IsWindow(child)) {
    if (::SetWindowSubclass(child, &FrameController::ChildSubclassProc,
                            kChildSubclassId,
                            reinterpret_cast<DWORD_PTR>(this))) {
      child_ = child;
    }
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

// static
LRESULT CALLBACK FrameController::ChildSubclassProc(HWND hwnd, UINT message,
                                                    WPARAM wParam,
                                                    LPARAM lParam,
                                                    UINT_PTR subclassId,
                                                    DWORD_PTR refData) {
  auto* self = reinterpret_cast<FrameController*>(refData);
  switch (message) {
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
