// // FRAME: custom-frame state machine (0.3.0 rewrite). Owns the HWND-touching
// half of the frame takeover: the top-level message handling (consulted
// first by the plugin's existing top-level WindowProc delegate, so Flutter's
// own delegate order is never changed), a SetWindowSubclass hook on the
// FLUTTERVIEW child that forwards resize-band hits to the parent via
// HTTRANSPARENT, and the per-OS DWM attributes. Every geometric decision is
// delegated to the pure functions in frame_geometry.h.
// NOTE: windowsx.h is deliberately NOT included (see frame_geometry.h).
#pragma once

#include <windows.h>

#include <functional>
#include <optional>

#include "frame_geometry.h"

namespace window_frame_kit {

/// Windows build number from RtlGetVersion (not GetVersionEx, which lies
/// without a compatibility manifest). Cached after the first call; 0 if
/// ntdll does not export it (never on supported systems).
DWORD FrameOsBuildNumber();

/// Resize band for the window's current DPI:
/// GetSystemMetricsForDpi(SM_CXSIZEFRAME) + (SM_CXPADDEDBORDER).
int FrameResizeBandForWindow(HWND hwnd);

/// Bitmask (kFrameEdge*) of the edges of `monitorRect` that host an
/// auto-hide appbar (taskbar), via SHAppBarMessage.
UINT FrameAutoHideEdgesForMonitor(const RECT& monitorRect);

/// Resize synchronization telemetry since the last TakeResizeSyncStats().
/// "Mismatch" = time during which the Flutter view HWND size differed from
/// the size of the last frame the engine presented into it; DWM composes
/// the view stretched during such time. staleComposedFrames estimates how
/// many DWM compositions fell inside mismatch intervals
/// (FrameVblanksBetween with DwmGetCompositionTimingInfo).
struct ResizeSyncStats {
  int steps = 0;                // view size changes seen by the engine
  int syncTimeouts = 0;         // engine resize waits that hit its timeout
  LONGLONG staleComposedFrames = 0;
  double mismatchMs = 0;        // total HWND/surface mismatch time
  double maxSyncWaitMs = 0;     // longest single engine resize wait
};

class FrameController {
 public:
  FrameController() = default;
  FrameController(const FrameController&) = delete;
  FrameController& operator=(const FrameController&) = delete;

  /// Disables (restoring the upstream frame) if still enabled.
  ~FrameController();

  bool IsEnabled() const { return enabled_; }

  /// Arms the frame on `root` and (when non-null) hooks the Flutter view
  /// `child`. Probes are read live at message time so setResizable /
  /// setFullScreen take effect without re-arming. Idempotent.
  bool Enable(HWND root, HWND child, std::function<bool()> isResizable,
              std::function<bool()> isPluginFullscreen);

  /// Restores the upstream frame, DWM attributes and removes the child hook.
  /// Idempotent.
  void Disable();

  /// Top-level messages owned by the frame while enabled. Returns nullopt
  /// to let the plugin's upstream branches (and DefWindowProc) run.
  std::optional<LRESULT> HandleTopLevelMessage(HWND hwnd, UINT message,
                                               WPARAM wParam, LPARAM lParam);

  /// Resize hit for a screen point against the root window.
  LRESULT HitTestScreenPoint(LONG x, LONG y) const;

  /// Returns and resets the resize telemetry.
  ResizeSyncStats TakeResizeSyncStats();

  /// Debug harness: `steps` back-to-back SetWindowPos calls alternating
  /// right/bottom/left/top edge growth and shrink (no input simulation),
  /// with view pre-sync on or off, then restores the original rect.
  /// Returns the telemetry of the sweep. Requires the frame to be enabled.
  ResizeSyncStats RunResizeSweep(int steps, bool preSync);

 private:
  static LRESULT CALLBACK ChildSubclassProc(HWND hwnd, UINT message,
                                            WPARAM wParam, LPARAM lParam,
                                            UINT_PTR subclassId,
                                            DWORD_PTR refData);

  FrameWindowState CurrentState() const;
  void PreSyncViewSize(const WINDOWPOS& pos);
  void OnChildSize(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT* result);
  void RecordMismatch(LONGLONG from, LONGLONG to);
  void ApplyDwmPolicy(bool enable);
  void RemoveChildHook();

  bool enabled_ = false;
  bool interactive_sizing_ = false;
  LONG_PTR style_before_enable_ = 0;
  HWND root_ = nullptr;
  HWND child_ = nullptr;
  std::function<bool()> is_resizable_;
  std::function<bool()> is_plugin_fullscreen_;

  // --- view resize pre-sync (see PreSyncViewSize) ---
  bool pre_sync_enabled_ = true;
  bool sending_pre_sync_ = false;
  // Size the engine was last told via WM_SIZE (real or pre-sync).
  SIZE engine_size_ = {0, 0};
  // QPC time a pre-synced frame was presented into the still-old HWND.
  LONGLONG pending_mismatch_since_ = 0;
  UINT verify_message_ = 0;
  ResizeSyncStats stats_;
};

}  // namespace window_frame_kit
