# Next session

## Latest (Oct 28, London playable, ~50%)
- game/src/scenes/london.*: London_Scene enter/run/leave, mouse (v25) / keys (v26) / cursor (v30), the per-frame
  arrow picture, GemSign (the sign board), WallAnimation (John/Michael waving), RibbonInterface, LondonCursor,
  the ribbon (0x4b6bf0), and the four scripts: Keyboard (arrows), Ribbon (mouse + ribbon to Wendy), TickTock
  (sign, cannon timer, the "stop when she turns" clock game), Hunt (Peter, Tink, the flying lesson with
  Shift/Ctrl/Enter, Peter's hand, up through the clouds), then change_scene("lost_tutorial").
- Engine: RayTrace pick (0x42c710), wall links (0x42e760..), CameraMover (0x431600), CameraTween (0x4df3d0),
  Curve3 in raytrace/mover.*, Cannon (cannon.*), state camera spots (0x4ebad0), the state's hover keys
  (0x4ec5f0), Raytrace_Scene overlay screens + mouse/keyboard services, cloud banks + flyThroughClouds (0x4ab920).
- Fixes found on the way: VoiceOver was never loaded (no voice lines played anywhere); the state's +0x560 is the
  voice-over's "playing" flag (State::vo_busy); the cursor was drawn at the wrong place (mouse coordinates are
  centred); Animation clocks use frame_time (0x4207b0) like the exe.
- Test aids: TB_LONDON_SKIP=<n> skips the first n scripts, TB_DEBUG_LONDON=1 logs the lines, TB_RT_CAM.
- Pad: Y = Enter (hover), shoulders also send Shift/Ctrl key events in 3D scenes. Start = Escape = the exe's
  skip key in London (goes to "lost").
- Next: "lost_tutorial" (LostTutorial_Scene) and the pixie dust bag / map icon / help bell of
  TinkerbellRaytraceScene.

## Latest (Oct 25, Peter and Wendy, ~43%)
- game/src/scenes/london_chars.*: PeterInLondon (0x4bbf90) and WendyInLondon (0x4bdaf0) with all their state
  classes (v0 enter, v1 leave, v2 tick, v3 ready, v4 mouth, v5 rest), say() (0x4bc8b0 / 0x4be880: talk state
  per line + VoiceOver), update (0x4bc880 / 0x4be780), the request idiom, Land/Fly curves (0x401000..).
  Sprite3D attachments (0x42f350 / 0x42f390). London enter creates them, Tink hidden; run updates Wendy each
  frame (Peter only in the last script loop in the exe). Test aids: TB_RT_CAM now works in any 3D scene
  (e.g. "3000.1,2700.1,176,0.1" shows both), TB_LONDON_PETER=<ms> (update + land Peter), TB_LONDON_SAY=<line>.
- Next: London enter remainder (scripts 0x4b9b30, ribbon 0x4b68f0, waterfall, corner_block, waves, arrows,
  cursors, music), run 0x4b5300 with the four LS scripts, then the toolbar.

## Latest (Oct 25, 00:00, ~40%)
- London is enterable from the sign-in room (Big Ben): game/src/scenes/raytrace_scene.* (Raytrace_Scene open /
  physics / sprite frames, MovementInputTask, London_Scene = plain Raytrace_Scene for now). RayTrace::draw
  (0x426710), load (0x4281a0), release (0x429980). Gamepad: d-pad / left stick = arrows, R1 up, L1 down,
  right stick = cursor (tb_input_set_arrows). Start camera = Raytrace_Scene defaults (3000.1, 0.1, 3000.1,
  90.1: on the ground facing the hedge). `tinkerbell london` starts there; TB_FAKE_INPUT "down vk,0"/"up vk,0".
- Next: TinkerbellRaytraceScene 0x4ab0f0 (hover v32 0x443820, Tinkerbell3D, toolbar, global shell), London
  enter 0x4b5800 / run 0x4b5300 and its scripts.

## Earlier (Oct 24, 21:05, paused by Bill, ~37%)
- 3D engine (game/src/raytrace/): walls (walls.cpp), sprites (sprites.cpp), movement + collision
  (collision.cpp), camera setter, all pushed to integ (68c1ea4). Test: `TB_TEST_RT=london tinkerbell rttest`,
  TB_RT_CAM="x,z,deg,eye", TB_RT_WALK="step,turn" per frame. ASan/UBSan clean. Screenshots match the art.
- **Next, in order:** (1) RayTrace::draw override = 0x426710 (frame/dt, moved -> castColumns with maxdist=1.0,
  g_sky_top = 240 - eye/8, sky, backdrop +0x116c, ground 0x42c460 cache, drawWalls; sprite box refresh
  0x4294b0). (2) Raytrace_Scene (vtable 0x5162f4, ctor ~_free.c:92180: camera default 3000.1/0.1/3000.1/90.1,
  physics consts in v28 0x4a96d0), open v27 0x4a9ca0, physics v29 0x4a9760 (asm decoded in notes), isFlying
  v31 (eye > 5), sprite animation v22 0x4a9a70, MovementInputTask 0x4aaa30 (arrows/shift/ctrl into
  +0x1684..9, overrides +0xe0..+0xf4). (3) A first walkable "london" + APK. (4) TinkerbellRaytraceScene
  0x4ab0f0 (Tinkerbell3D 0x4da4a0, 0x4befe0, toolbar 0x4efd50, global shell 0x63171c), London enter
  0x4b5800 (inside r2's 0x4b5300 range), London run 0x4b5300 (scripts +0x28e8/+0x29c8/+0x2a94/+0x2efc in
  turn, random VO timers, then change_scene("lost_tutorial")).

## Latest (Oct 24, evening, ~28%)
- Sign-in room reconstructed by the lead (game/src/scenes/signin.*, acts.*, vision/faderect.*): frames as save
  slots, place/character/skill choice, Wendy with lip sync, idle prompts and blinking hints, basket delete,
  Big Ben -> "london", map -> "map" (map only after the tutorials). ASan/UBSan clean from boot through a scripted
  sign-in; Save\save5.txt written correctly. Leaving goes to "london", which isn't ported: the game stops with
  "Scene london not available". **Next: the London scene (London_Scene@LondonScene and its Peter/Wendy states),
  then the map.**
- Test aid: TB_FAKE_INPUT="<frame>:click x,y;<frame>:key vk" (platform/dinput.cpp). `tinkerbell signin` starts in
  the room. Clicks used: green frame 158,56; London 514,69; Hook 161,287; beginner 186,395; Big Ben 509,337.
- Agent A stalled for 90 min on this scene (no code); keep agent tasks small with a commit every ~30 min.

## State at the end of session 1 (Oct 24, 2026, ~20% to goal)
- `main` = `integ`: boots on its own Disney bumper -> London intro (title) -> sign-in room on the desktop,
  ASan/UBSan clean over 3,500 frames. The sign-in room is drawn but not interactive yet.
- First APK (`latest` release) failed on the Thor at start: `dlopen failed: library "libavformat.so" not
  found` (the FFmpeg libs were packaged as lib<name>-<major>.so but their sonames are lib<name>.so). Fixed in
  ecaae33 (packaged under their sonames; CI now checks every NEEDED entry of the APK's native libs).
  **Confirmed by Bill on the Thor (Oct 24, 13:08): the fixed APK imports the ISO and plays bumper -> intro -> sign-in room (25%).**
  Nothing else has been run on a device yet (importer, touch, audio, movies).
- Original exe under Wine: refuses with "CS:No disc inserted." (the copy protection's disc check) even with
  the ISO as a cdrom drive; loop mounting isn't allowed in this sandbox. Don't work around it. No reference
  runs; compare against the decoded art and the decompile.

## Unmerged work on backup branches (merge into integ, build, smoke, then continue)
- `backup/agent-b` f30e326: Sound and QTMovie derive from Service; Act base. Committed by lane B before the
  usage limit; builds. Lane B's task: voice-over classes (VoiceOver, PlayVos, DelayedVO, PreSceneVo,
  VoOneAfterAnother, QuestionRepeater, Reminder, MagpieCharacter lip sync from MagPies.dat) - not started.
- `backup/agent-c` dfb5f86: WIP Text/font classes (text.cpp/h, gdi/graphics edits, Liberation Mono fonts);
  builds + smoke test, unfinished. Lane C's task: Text classes (Text::v14, font class, TextField),
  Mouse/Keyboard/Joystick shells on Service, rotated sprites (0x40c8f0).
- `backup/agent-a`: nothing new since the merge. Lane A's task (it had just started reading): make the
  sign-in room interactive: buttons (0x4b0360, 0x4afe70), Wendy (0x4b4310), cursor (0x4afc70), game state
  object (0x4e72d0 -> 0x4ec6e0), saves (0x4ea4f0, under tb_save_dir()), pixel-exact hit tests (0x418910),
  a scripted-input env var for tests (TB_FAKE_INPUT), then on to the map.

## Notes
- Run the boot test: `cd <game copy> && DISPLAY=:91 SDL_AUDIODRIVER=dummy TB_FRAMES=4000 TB_SHOTS=<dir>
  TB_SHOT_EVERY=250 timeout 300 build/tinkerbell` (~30 s; the sign-in room appears around frame 3000).
  `tinkerbell signin` starts straight in the sign-in room.
- Task switching: the game's tasks resume into saved stack frames (x86-only); the port runs each task on
  its own thread, one at a time (`// port:` in game/src/vision/engine.cpp).
- Usage limits hit twice with 3 agents; agents must commit + push at every working step.
- The repo was renamed to sirbilliams/Tinkerbell-Android (remote updated).
