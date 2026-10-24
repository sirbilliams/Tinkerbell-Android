# Handoff: where the project stands and how to continue

## The project
A faithful decompile-and-recompile of "Disney's You Can Fly! with Tinker Bell" (PC, 2002, "Tinker Bell.exe",
MSVC6 x86) into a native Android app for Bill's AYN Thor (arm64; 1920x1080 top screen, 1240x1080 bottom
touch screen, built-in gamepad). Desktop Linux build for testing. Method and app shell follow
sirbilliams/neutron2-android (finished) and neutron1-android; their OMT engine code is not used here.

### Bill's standing rules
- Never commit game files or bytes from the exe. Values come from the player's exe at run time
  (`platform/exe_data.cpp`); the app installs the game from the player's ISO. The repo is public.
- No leaked source code. Faithful: same logic, order, quirks; deviations marked `// port:`.
- Commit as `git -c user.name=sirbilliams -c user.email=bravehooves@gmail.com`, with the session's
  attribution lines. Push to main only after a build + smoke test; every push to main builds the
  "latest" APK. ASan/UBSan desktop build (`-DTB_SANITIZE=ON`) before sending an APK.
- Short status notes; APKs and screenshots when there's something new. "Check in" / "percentage to
  goal" = brief answer. On "stop": agents commit what builds, update this file and NEXT-SESSION.md, push.
- Agents on Sonnet for porting, Opus for hard core/debugging; at most 3 at once; brief: docs/agent-brief.md.
- Blocked here (ask Bill): pushing tags, deleting branches, Actions secrets.

## The copy protection
The retail exe has a copy-protection wrapper (.cms_t/.cms_d sections; import calls routed through a
dispatcher at 0x64b2f8; the CRT pages 0x4f3000-0x4f7fff encrypted; the CRT start moved into the wrapper).
We do not analyse, emulate, decrypt or remove it (Bill agreed, Oct 24). Everything is done from the plain
game code; see docs/agent-brief.md for how the dispatcher calls and CRT calls are named.

## Percentage scale (agreed Oct 24)
Phase 0 5% | desktop reaches the sign-in room 20% | first APK on the Thor 25% | scenes + mini-games
25->85% by share of game code ported and checked | full playthrough on the Thor 90% | port features 100%.

## Setting up a workspace (fresh cloud machine, Ubuntu 24.04)
- Network: apt works over HTTPS only (`sed -i 's#http://#https://#g' /etc/apt/sources.list.d/ubuntu.sources`;
  disable docker.list); PyPI only through the proxy (`NO_PROXY=localhost,127.0.0.1 pip install ...`).
- `apt-get install libsdl2-dev zlib1g-dev xvfb x11-utils meson ninja-build pkg-config wine wine32:i386`
  (after `dpkg --add-architecture i386`); `pip install --break-system-packages pycdlib pefile capstone unicorn r2pipe`.
- radare2 6.2.4 and r2ghidra c1287cc from GitHub, `--prefix=/opt/r2`; r2ghidra needs its subprojects
  first: `cd subprojects && make -f ghidra-native.mk ghidra-native && make -f zlib.mk zlib`, then configure
  with `PKG_CONFIG_PATH=/opt/r2/lib/pkgconfig PATH=/opt/r2/bin:$PATH LD_LIBRARY_PATH=/opt/r2/lib`.
  Wrappers `/usr/local/bin/r2` and `/usr/local/bin/radare2` (r2pipe needs the latter):
  `#!/bin/sh` / `LD_LIBRARY_PATH=/opt/r2/lib exec /opt/r2/bin/radare2 "$@"`.
- Game files: private repo sirbilliams/negatron-gamefiles, release `iso`: `Tinker.Bell.iso` (asset
  665286433, 495,497,216 bytes, sha256 c3e324e2...5fbc). Attach the repo (add_repo), then
  `gh api -H "Accept: application/octet-stream" repos/sirbilliams/negatron-gamefiles/releases/assets/665286433 > Tinker.Bell.iso`.
  Extract with pycdlib (Joliet); `setup/game/` is the install (master copy: /home/claude/gamefiles/tb/game).
  `tinker.rar` (440 KB) in the same release is not used.
- Decomp inputs: `decomp/tb-decomp-inputs.tar.gz` in negatron-gamefiles
  (`mkdir -p /home/claude/decomp && tar xzf ... -C /home/claude/decomp`, then copy the exe in as TinkerBell.exe),
  or regenerate: `tools/decomp/rtti.py`, `decompile_all.py` (~25 min on 2 CPUs), `abi.py`, `api_sites.py`,
  `globals_inventory.py`.
- Desktop build: `cmake -S . -B build && make -C build -j2`; run from the game folder (see agent-brief).

## Repo map
| Path | What |
|---|---|
| `platform/` | Win32/DirectX shim on SDL2: core (log, paths, headless), exe_data (values from the player's exe), ddraw (lane A), dsound + movie (lane B), dinput + gdi + crash_android (lane C). |
| `game/src/vision/` | The reconstructed Vision engine: winmain, engine (services, tasks, scenes), object, ras, graphics, sound, wv6, qtmovie. |
| `game/src/scenes/` | Game classes (phase 2). |
| `android/` | App: SetupActivity (ISO import, Play, Share log), GameActivity (SDL). |
| `tools/decomp`, `tools/formats`, `tools/android/build-ffmpeg.sh`, `tools/wine/run_original.sh` | Decomp inputs, format checkers, FFmpeg for Android, Wine runner (the original refuses to start under Wine). |
| `docs/` | agent-brief.md (lanes, rules), formats.md, api-sites.md, lane-c.md, this file, NEXT-SESSION.md. |

## Status
See docs/NEXT-SESSION.md.
