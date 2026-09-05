#include <flutter_linux/flutter_linux.h>

#include "include/window_frame_kit/window_frame_kit_plugin.h"

// This file exposes some plugin internals for unit testing. See
// https://github.com/flutter/flutter/issues/88724 for current limitations
// in the unit-testable API.

// The ported window_manager 0.5.2 implementation exposes no free functions
// (the template's get_platform_version helper was removed with the port);
// unit tests exercise the GObject type registration via the public header.
