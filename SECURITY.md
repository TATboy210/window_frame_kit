# Security Policy

## Supported versions

Only the latest published minor version of `window_frame_kit` receives
fixes. At the time of writing that is **0.3.x**.

## Reporting a vulnerability

Please do **not** open a public issue for security problems. Use GitHub's
private vulnerability reporting instead:
<https://github.com/TATboy210/window_frame_kit/security/advisories/new>

Include the platform (Windows build number, macOS version, Linux
distribution + X11/Wayland), the package version, and a minimal
reproduction. You should get an acknowledgement within 7 days.

## Scope

window_frame_kit runs native code inside your application process
(Win32 window procedures, GTK and AppKit calls). Issues such as memory
corruption in the native plugins, privilege boundaries crossed through the
method channel, or crashes triggerable by untrusted input are in scope.
The package performs no network access and stores no data.
