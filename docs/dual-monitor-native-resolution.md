# Native resolutions for dual-monitor sessions

Status: Windows x64 build and automated tests passed for `3dcc8a3a` on
2026-11-10. Physical monitor and disconnect/recovery testing remain pending.

Build: https://github.com/Donnowhoo/Sunshine/actions/runs/42759779381

The suite passed 691 tests (including all 13 new native-resolution tests) and
skipped 6 environment-dependent tests. The initial build of `c08dd6ca` failed
because an exception variable shadowed the error logger; `3dcc8a3a` fixes it.
Only Windows AMD64 was built. The release targets `3dcc8a3a`.

Installer SHA256:
`e2351112f92b4d40ebd274847a9b0786c31726a853c996d608feb4f143d33593`.
This does not resolve the separately reported Chrome Google logout issue.

A paired dual-monitor client already sends individual monitor rectangles. Before
capture starts, Sunshine matches these to active Windows device IDs and requests
the corresponding native resolutions. For QHD+QHD hosts and QHD+FHD clients, the
right desktop is rendered at 1920x1080 instead of scaling a 2560x1440 capture.

The transaction saves all original active display modes using the existing
libdisplaydevice recovery state, reads the saved data back, rechecks the topology,
and only then applies the modes. HDR, primary display and refresh rate changes are
not requested. Ambiguous layouts, cloned displays and outstanding recovery state
prevent application. A failure uses the existing capture fallback and is logged;
it must not be reported as successful native-resolution operation.

The last disconnect, failed preparation and abandoned handshake request recovery.
After a process crash, Sunshine's existing startup recovery reads the same journal.
Disconnected hardware can prevent immediate recovery. Application window positions,
DPI changes and sudden-power-loss durability are not guaranteed. No driver is
installed and physical monitor disappearance remains unsupported.

Non-span requests keep the existing configuration path. Existing sessions prevent
new native mode changes. Moonlight needs no protocol change for this feature.

Tests in `tests/unit/test_native_resolution.cpp` use fake devices and in-memory
storage. Run these and the full Windows x64 suite before distributing a build.
