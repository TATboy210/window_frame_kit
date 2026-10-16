// DEV: screen_retriever 0.3.0 adaptation (DEVIATIONS #14). 0.3.0 rebuilt the
// package on nativeapi's DisplayManager and moved the 0.2.x
// `screenRetriever` facade to a deprecated `legacy.dart` bridge. This file
// reads the native API directly and keeps the upstream alignment math in a
// pure function so it can be unit-tested without a native display backend.
import 'package:flutter/material.dart';
import 'package:screen_retriever/screen_retriever.dart';

/// A display snapshot reduced to the two rects the placement math needs:
/// the full monitor [bounds] (used to find the display under the cursor) and
/// the [workArea] (the monitor minus taskbars/docks, where the window lands).
@immutable
class DisplayArea {
  const DisplayArea({
    required this.bounds,
    required this.workArea,
    this.isPrimary = false,
  });

  final Rect bounds;
  final Rect workArea;
  final bool isPrimary;
}

/// Picks the display whose full bounds contain [cursor], falling back to the
/// primary display, then to the first one. Returns null for an empty list.
DisplayArea? selectDisplayArea(List<DisplayArea> displays, Offset cursor) {
  if (displays.isEmpty) {
    return null;
  }
  for (final display in displays) {
    if (display.bounds.contains(cursor)) {
      return display;
    }
  }
  for (final display in displays) {
    if (display.isPrimary) {
      return display;
    }
  }
  return displays.first;
}

/// Computes the window origin for [alignment] inside [workArea]. Named
/// alignments land exactly on edges/centres; any other [Alignment] is
/// interpolated linearly (upstream window_manager 0.5.2 semantics).
Offset alignWindowInWorkArea(
  Size windowSize,
  Alignment alignment,
  Rect workArea,
) {
  final freeWidth = workArea.width - windowSize.width;
  final freeHeight = workArea.height - windowSize.height;
  // Alignment(-1,-1) is topLeft and Alignment(1,1) is bottomRight, so the
  // free space splits as (1 + a) / 2 on each axis - identical to the
  // upstream per-constant branches for the nine named alignments.
  final left = freeWidth / 2 + alignment.x * (freeWidth / 2);
  final top = freeHeight / 2 + alignment.y * (freeHeight / 2);
  return Offset(workArea.left + left, workArea.top + top);
}

DisplayArea _toArea(Display display) {
  final position = display.position.toOffset();
  final size = display.size.toSize();
  return DisplayArea(
    bounds: position & size,
    workArea: display.workArea.toRect(),
    isPrimary: display.isPrimary,
  );
}

/// Window origin for [alignment] on the display under the cursor.
Future<Offset> calcWindowPosition(Size windowSize, Alignment alignment) async {
  const manager = DisplayManager.instance;
  final displays = manager.getAll().map(_toArea).toList(growable: false);
  final cursor = manager.getCursorPosition().toOffset();
  final current = selectDisplayArea(displays, cursor);
  if (current == null) {
    return Offset.zero;
  }
  return alignWindowInWorkArea(windowSize, alignment, current.workArea);
}
