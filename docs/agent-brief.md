# Agent brief: Disney's You Can Fly! with Tinker Bell (2002) port. Usage is limited: be economical.

Faithful C++ reconstruction of "Tinker Bell.exe" (MSVC6, x86, Jan 2002) for Android (AYN Thor, arm64), with a
desktop Linux build used for testing. Repo: sirbilliams/tinkerbell-android. Read `docs/formats.md`; read the
parts of `docs/HANDOFF.md` you need.

## How the port is built
- `game/src/vision/`: the game's own engine (C++ namespace `Vision` in the RTTI: Stage, Scene, Task, Service,
  Screen, Ras/Ras::Device, Resource + Factories, Pixelmap, Image, Palette, Alpha, Graphic, Layer, Text,
  Sound, SoundBuffer, Mouse, Keyboard, Joystick, Thread, File, ...). `game/src/scenes/`: the game classes.
  Both are reconstructed from the decompile: same logic, same order, same quirks.
- `platform/`: a thin shim with the Win32/DirectX behaviour the game relies on, on SDL2. The reconstructed
  code calls it where the original called DirectDraw / DirectSound / DirectInput / user32 / gdi32 /
  kernel32, in the same order, so the game logic stays the original's. The shim imitates only what the
  game uses (find it in the decompile), not whole APIs.
- `platform/tb.h` (lead-owned): logging, game folder, headless settings, `tb_exe_*` (values from the
  player's exe), `tb_game_main` (the reconstructed WinMain).

## The exe and the copy protection (important)
The retail exe carries a copy-protection wrapper (sections .cms_t/.cms_d). **Do not analyse, emulate,
decrypt or bypass it, and don't read its sections.** We don't need it:
- Game code is plain `.text` (0x401000-0x513000) except 0x4f3000-0x4f7fff, encrypted MSVC6 runtime
  (operator new/delete, malloc, string/stdio, C++ EH). Calls into it (`fcn.004f3xxx`..`004f7xxx`, also
  shown as `int.004f...`) are C runtime: name them from how they're called and use our runtime.
  `/home/claude/decomp/crt_targets.txt` lists the 87 targets by call count. Add names you confirm to
  `docs/crt-names.md` (address, name, evidence), one line each.
- API calls through the wrapper's dispatcher appear as `(*redirected_api)(...)` / `call [0x64b2f8]`. Name
  each from context (arguments, use of the result, nearby strings) against the game's import list
  (`/home/claude/decomp/api_sites.json`: `not_direct` = the 127 APIs these sites can be). Add confirmed
  ones to `docs/api-sites.md` (site, API, evidence). Direct `imp.<Name>` calls are already named.

## Inputs (read-only, shared)
`/home/claude/decomp/`: `TinkerBell.exe`, `raw/<Class>.c` (virtual methods), `raw/_free.c` (everything else),
`raw/functions.json`, `classes.json` (RTTI: bases, vtables), `abi.json` (ret_pop = stack args popped,
uses_ecx = thiscall/fastcall, fret = float return), `globals.json`, `api_sites.json`, `crt_targets.txt`.
Disassembly: `r2 -2 -qc 'pd 40 @ 0x...' /home/claude/decomp/TinkerBell.exe` or
`python3 /home/claude/tools-local/dis.py /home/claude/decomp/TinkerBell.exe <hexaddr> <n>`; x87 float code
and stack arguments are often decompiled wrong (MSVC6): check the assembly when it matters.

## Hard rules
- Never commit game files or bytes from the exe (no tables, strings, images dumped in bulk). Values the
  code needs come from `tb_exe_*` at run time. Small literals the code itself uses are fine.
- No leaked source code of any kind. Work only from the decompile/disassembly.
- Faithful. Every deviation is marked `// port: <reason>`. Comment each reconstructed function with its
  address: `// 0x41c100 Ras::v1`.
- Portable C++11 for x86-64 Linux and arm64 Android: no x86 asm, no Windows headers, never store pointers
  in 32-bit ints (use real pointers/structs; the exe's byte offsets don't need to match), no
  `long` for 32-bit values (use int32_t/uint32_t). Endianness: little.
- Commit only what builds and passes the smoke test:
  `git -c user.name=sirbilliams -c user.email=bravehooves@gmail.com commit`, message ending with a blank
  line and:
  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
  Claude-Session: https://claude.ai/code/session_015ZWpKyqAwm71bD4ikeLLEB
  Then `git push -f origin HEAD:backup/agent-<x>`. Never push main or other branches; never tag.
- Don't touch other agents' worktrees, build dirs, game copies or scratch. Files outside your lane:
  minimal edits only, listed in your report.

## Your workspace (<x> = your letter, <d> = your display number)
- Worktree `/home/claude/wt/<x>` (branch `agent-<x>`). Build (2-CPU machine, never more than -j1 per agent):
  `cmake -S . -B build-<x> -DCMAKE_BUILD_TYPE=RelWithDebInfo && make -C build-<x> -j1`
  (sources are globbed; re-run cmake after adding files).
- Game copy `/home/claude/gamefiles/tb-<x>` (don't use the master `/home/claude/gamefiles/tb/game`).
  Scratch `/home/claude/scratch-<x>`.
- Smoke test (start your Xvfb once: `Xvfb :<d> -screen 0 1280x1024x24 &`):
  `cd /home/claude/gamefiles/tb-<x> && DISPLAY=:<d> SDL_AUDIODRIVER=dummy TB_FRAMES=120 TB_SCREENSHOT=/home/claude/scratch-<x>/shot.bmp timeout 120 /home/claude/wt/<x>/build-<x>/tinkerbell`
  Success = "CLEAN EXIT" and no crash. Look at the screenshot when your change is visual.
- Testing economy: build + smoke test before each commit, plus one quick check that your change works.

## Report (<= 12 lines)
Commits, what works (with a screenshot path if visual), what's left (with addresses), shared files touched.

## Lanes (phase 1: the platform layer, desktop reaching the sign-in room)
- **A, core/video (Opus).** The reconstructed WinMain (`tb_game_main`), window + message loop, Stage, Scene,
  Task, Service, Thread, Screen and Ras::Device on a DirectDraw-like shim (`platform/ddraw_*`): a
  640x480 RGB565 back buffer presented to an SDL texture, letterboxed. Ras/Resource/File loading (game
  folder paths, `\` to `/`, case-insensitive lookup), Pixelmap/Image/Palette/Alpha with the blitters,
  headless frames/screenshots (tb_headless()). Goal: boots and draws the first scenes.
- **B, sound + movies (Sonnet).** DirectSound-like shim on SDL audio (`platform/dsound_*`: buffers, lock,
  play/loop/stop, volume/pan, position), Sound/SoundBuffer (the `.wv6` decoder with the game's quirk),
  and QTMovie on FFmpeg (`platform/movie_*`; desktop: system libav*). Goal: sounds and both movies play.
- **C, input + text + Android (Sonnet).** DirectInput-like shim (`platform/dinput_*`), Mouse, Keyboard,
  Joystick: SDL mouse; on the Thor, touch on the top screen = mouse (tap = click, drag = held button), the
  gamepad moves the cursor and clicks. GDI text the game draws (TextOutA, fonts: a bundled free font,
  `// port:`). The Android app from sirbilliams/neutron2-android (`/home/claude/sirbilliams/neutron2-android`,
  read-only): launcher (SetupActivity), ISO importer for this disc (Joliet; copy `setup/game`), app id
  `com.sirbilliams.tinkerbell`, crash backtrace to the log + "Share log", CI (`.github/workflows`, Linux
  check + APK + "latest" release on main), signing with the NEG_KEYSTORE_B64 / NEG_KEYSTORE_PASSWORD
  secrets. No OMT/engine code from the Jimmy repos.
