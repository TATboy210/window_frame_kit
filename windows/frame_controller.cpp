// // FRAME: Phase 3 custom-frame graft - state machine implementation (03-02,
// redesigned 2026-09-05 to the equal-width NC-band route per user target:
// self-drawn titlebar PLUS system border/corners and uniform 4-edge resize).
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
  // If the plugin dies before the window tree, restore the upstream frame
  // mapping; Disable() is idempotent so double-teardown is safe.
  Disable();
}

bool FrameController::Enable(HWND root, std::function<bool()> isResizable,
                             std::function<bool()> isPluginFullscreen) {
  if (root == nullptr) {
    return false;
  }
  is_resizable_ = std::move(isResizable);
  is_plugin_fullscreen_ = std::move(isPluginFullscreen);
  root_ = root;
  enabled_ = true;
  // Force the top-level WM_NCCALCSIZE through the plugin delegate so the
  // band mapping applies on the very next paint. No geometry change, frame
  // change only.
  SetWindowPos(root_, nullptr, 0, 0, 0, 0,
               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE);
  return true;
}

void FrameController::Disable() {
  if (!enabled_) {
    return;  // Idempotent: repeated disable / disable-before-enable is safe.
  }
  enabled_ = false;  // Flip first so a re-entrant message sees "off".
  if (root_ != nullptr) {
    // Restore the upstream frame mapping with the same frame-only recalc.
    SetWindowPos(root_, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE);
  }
  root_ = nullptr;
}

}  // namespace window_frame_kit
