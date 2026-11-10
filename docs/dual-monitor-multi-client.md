# Independent Windows captures for concurrent clients

## Build status

Implementation commits: `8c604d2a`, `70c57838`, `6889c1fe`.
Windows x64 build/release passed:
https://github.com/Donnowhoo/Sunshine/actions/runs/42765978233

697 tests passed; 6 environment-dependent tests were skipped. All six new tests
passed: reservation ordering (dual-first and single-first), abandoned/pending
handshakes, stack capture-owner lifetime, and display-shortcut isolation.
Hardware capture/encoder concurrency still requires testing on the host GPU.

## Behavior

Each Windows connection owns its asynchronous or synchronous capture worker,
display selection event, reinitialization signal, and viewport mapping. A single
client no longer subscribes to another client's spanned image. The global encoder
choice and physical Windows desktop remain shared.

Display leases span launch/resume preparation and stream ownership. Initial
preparation is serialized so another request cannot race encoder probing. Only
the first reservation prepares native physical modes. Background setting retries
are stopped before capture; subsequent clients fit video to their own requested
layout. An outstanding recovery request survives reconnects. Recovery is deferred
until all reservations, including pending handshakes, end.

This policy intentionally favors stable existing connections over native FHD for
a later client. A tablet-first session may leave the host at QHD, so a later FHD
client receives scaled video. This was explicitly selected by the user.

The desktop cursor, keyboard and applications are shared. This is not a separate
Windows user session per client. Supported concurrent client count depends on GPU
capture/encoding resources and network capacity; it is not unlimited.

## Clients

Windows client `e60627ed` was rebuilt successfully for x64:
https://github.com/Donnowhoo/moonlight-qt/actions/runs/42765489900

Galaxy Book5 Pro 360 + Galaxy Tab S9 was clarified as Windows Second Screen in
Extend mode. Run Windows Moonlight on the Galaxy Book only for that two-screen
configuration. Android Moonlight is for standalone tablet connections; the
official v12.2 APK was downloaded rather than building an unnecessary Android
external-display fork. No Android dual-display implementation is claimed.

## Artifacts verified against release SHA256

- Sunshine x64 MSI (`6889c1fe`): `2d20dfe0c7fcaf175d310b9eb28648fa249dd74c395881ba6dfd4ebcc1e91d26`
- Moonlight Windows x64 ZIP (`e60627ed`): `aaee1508e8b8a80302eb160ba3204a7886fd9aa7c5af9affc63c718d51088246`
- Official Android v12.2 non-root APK: `a871f6190365386ef3ac5722a016d4da015ee1555631ec257f6e5dc3f4a45fe0`

## Manual verification remaining

Test dual then single, single then dual, each client's independent F1/F2 selection,
one client leaving while another remains, final disconnect restoration, and
repeated reconnects. Test tablet touch and Windows Second Screen as well.
Do not restart/install Sunshine remotely as part of automated verification.
The separately reported Chrome Google logout issue remains unresolved.
