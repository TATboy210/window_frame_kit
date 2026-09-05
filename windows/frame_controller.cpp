// // FRAME: Phase 3 custom-frame graft - state machine implementation (03-02).
// The pure geometry/math lives inline in frame_controller.h so the gtest TU
// pins it without a live window tree; this TU only touches real HWNDs.
#include "frame_controller.h"

namespace window_frame_kit {

int FrameEdgeWidthForWindow(HWND hwnd) {
  const UINT dpi = GetDpiForWindow(hwnd);
  // Standard resize frame + padded border, both DPI-scaled: at 150% this is
  // ~6px instead of the 4px logical value, keeping hit bands physically
  // consistent across monitors (XPLAT-03).
  const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
  const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
  return frame + padded;
}

FrameController::~FrameController() {
  // If the plugin dies before the window tree, the subclass must not outlive
  // its ref-data owner; Disable() also skips the SetWindowPos because
  // enabled_ flips first.
  Disable();
}

bool FrameController::Enable(HWND root, HWND child,
                             std::function<bool()> isResizable,
                             std::function<bool()> isPluginFullscreen) {
  if (root == nullptr || child == nullptr) {
    return false;
  }
  is_resizable_ = std::move(isResizable);
  is_plugin_fullscreen_ = std::move(isPluginFullscreen);
  // Install first, flip state second: if the subclass call fails we leave no
  // half-enabled state behind (caller surfaces the failure to Dart).
  if (!SetWindowSubclass(child, ChildProc, kFrameChildSubclassId,
                         reinterpret_cast<DWORD_PTR>(this))) {
    return false;
  }
  child_ = child;
  root_ = root;
  enabled_ = true;
  // Force the top-level WM_NCCALCSIZE through the plugin delegate so the
  // borderless mapping applies on the very next paint, not after the first
  // resize. No geometry change, frame change only.
  SetWindowPos(root, nullptr, 0, 0, 0, 0,
               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE);
  return true;
}

void FrameController::Disable() {
  if (!enabled_) {
    return;  // Idempotent: repeated disable / disable-before-enable is safe.
  }
  enabled_ = false;  // Flip first so a re-entrant hit-test sees "off".
  if (child_ != nullptr) {
    RemoveWindowSubclass(child_, ChildProc, kFrameChildSubclassId);
  }
  child_ = nullptr;
  if (root_ != nullptr) {
    // Restore the upstream frame mapping with the same frame-only recalc.
    SetWindowPos(root_, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE);
  }
}

LRESULT CALLBACK FrameController::ChildProc(HWND hWnd, UINT message,
                                            WPARAM wParam, LPARAM lParam,
                                            UINT_PTR uIdSubclass,
                                            DWORD_PTR dwRefData) {
  auto* self = reinterpret_cast<FrameController*>(dwRefData);
  if (message == WM_NCDESTROY) {
    // The Flutter view can be torn down independently (hot restart); unload
    // the hook so a stale ref-data pointer is never dereferenced afterwards.
    RemoveWindowSubclass(hWnd, ChildProc, uIdSubclass);
  } else if (self != nullptr && self->enabled_ &&
             message == WM_NCHITTEST) {
    // Dual-path fullscreen guard: the plugin's own fullscreen state plus the
    // external style-strip path (media_kit). Either one disables resizing.
    // Both probes are live callbacks - a setResizable/setFullScreen call
    // mid-session is honored on the next hit-test without re-arming.
    const HWND root = GetAncestor(hWnd, GA_ROOT);
    const bool externalFullscreen =
        IsExternalFullscreenStyle(GetWindowLongPtr(root, GWL_STYLE));
    const bool pluginFullscreen =
        self->is_plugin_fullscreen_ && self->is_plugin_fullscreen_();
    const bool resizable = self->is_resizable_ && self->is_resizable_();
    if (FrameHitAllowed(resizable,
                        pluginFullscreen || externalFullscreen)) {
      const LONG x = FrameXFromLParam(lParam);
      const LONG y = FrameYFromLParam(lParam);
      RECT rect;
      GetWindowRect(hWnd, &rect);
      // The child fills the top-level client area; with WM_NCCALCSIZE
      // returning 0 that equals the full window rect, so hit bands computed
      // here cover all four edges including the top one.
      const auto hit = FrameEdgeHitTest(
          x, y, rect, FrameEdgeWidthForWindow(hWnd));
      if (hit.has_value()) {
        return *hit;
      }
    }
    // Interior (or resizing disallowed): default client hit so Flutter keeps
    // normal pointer handling.
    return HTCLIENT;
  }
  return DefSubclassProc(hWnd, message, wParam, lParam);
}

}  // namespace window_frame_kit
