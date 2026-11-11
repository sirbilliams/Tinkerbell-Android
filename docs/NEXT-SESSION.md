# Next session

## Latest (Nov 11, Starry Skies, ~73%)
- game/src/scenes/starry.*: StarrySkies_Scene (0x477b30, "starry", static 0x5d2c60). Cubby stands at the
  Backwards Go sign, Peter at Color the Clouds' (that game is its own scene), Foxy at Cloud Burst's. The three
  hidden animal pieces (State::starry, "Coken Tokens": the exe's spelling) are found with dust; the 2D bar
  (StarryInterface2D) shows the ones won. Leaving by Peter's sign flies up to "colorthecloud".
  - State: TokenInfo starry[5] (+0x1628) with loadStarryTokens 0x4ed9c0 (called from the sign-in save, before
    the Lost Boys' as in the exe) and saveStarryTokens 0x4edd90; +0x7a8 back_ended; +0x64c (never read).
  - The map piece: StarryMapPiece (0x4e8860) in mappiece.*. Tink on/off: TinkerbellRaytraceScene::tinkOn 0x4ab5c0 /
    tinkOff 0x4ab660. Tink's "point down": Tinkerbell3D::pointDown 0x4dba20.
- game/src/scenes/backwardsgo.*: Backwards Go (0x46fa00..0x4719a0), played in the scene. The camera stands on a
  5 x 5 grid with Tink and the boys (2 the first time, then 5 with the raccoon twin); Cubby calls a move or a
  backwards one; arrows, Shift up, Ctrl down. The boys copy (80..95 % by skill), a wrong one flies off; all
  caught out wins.
- game/src/scenes/cloudburst.*: Cloud Burst (0x478b10..0x47bc10 and the riddle pool 0x471a20..0x472120). 42
  riddles in three groups; how many of each by skill; the animal asked about walks in, the answer is clicked on
  the shelf (cat dog elephant mouse tiger); the fuse from the second game on.
- Exe quirks kept: the win list has Peter's first win line three times; Cubby registered for the rabbit's calls,
  Peter for "MCOV_VO_RABcall3", the inactivity prompts at Peter's spot are the rabbit's; "star_ax_birdwing"
  doesn't exist; the 2D bar never unloads the elephant; the riddle pools are filled again on every game (on top
  of what is there); MSVC 6's random_shuffle (never leaves an element in place).
- Test aids: TB_DEBUG_STARRY=1 (signs, clicks, both games' states), TB_STARRY_TOKENS=44444 (the pieces' states),
  TB_BACK_AUTO=1 (the right move), TB_CLOUD_AUTO=1 (the first answer). Backwards Go:
  `TB_RT_CAM="3881,2700,90,0.1" ... TB_FAKE_INPUT="200:key 32;260:key 32;300:move 318,300;310:click 318,300"`.
  Cloud Burst: `TB_STARRY_TOKENS=44444 TB_RT_CAM="6000,2700,0,0.1" ... "...;300:move 60,330;310:click 60,330"`.
- Next: Indian Village ("indian_village" 0x467fc0), Mermaid Cove ("mermaid" 0x44d9c0), Skull Rock ("skull"
  0x488260), "treasure" (0x48f660), and their games. The other areas' token loads at sign-in (0x4e6390, 0x4ed270,
  0x4ee110, 0x4eefc0) come with them.

## Earlier (Nov 8 night, Firefly, ~69%)
- game/src/scenes/firefly.*: Firefly_Scene (0x47db70, "firefly", static 0x5d91b0; files "DCBL_").
  - Swarms drift over the ground (Swarm 0x47eb90). The jar cursor catches up to six fireflies (FireflyCursor
    0x47bda0), then a click lights a lamp or a walking turtle's lantern (BackgroundScroller 0x47f040, on
    ScrollGround; Turtle 0x47e7a0, TurtleGroup spawn 0x480100).
  - Peter catches and lights too (PeterControlScript 0x47e000..0x47e610). Eight lights win (FFInterface 0x480430).
- Exe quirks kept:
  - "dcbl_fx_gdrop" doesn't exist (the file is ..._Gdop).
  - The swarm's timer never restarts.
  - nearestLight returns only the turtles' answer.
  - The nearest searches move "best" before testing the screen bounds.
- Both endings go to "Skull" (not ported yet).
- Test aid: TB_DEBUG_FIREFLY=1; `tinkerbell firefly` + TB_FAKE_INPUT="60:key 81".
- The map: game/src/scenes/map.* Map_Scene (0x4a52a0, "map", vtable 0x515d60). The map icon in every scene now
  works. Area buttons play Peter's "trans" line, then go to the area (PreSceneVo::start 0x4c5d60). The other
  buttons go to the credits, "pickagame" (only with the treasure), quit (asked twice), signin (asked twice), the
  treasure map, "treasure" and back (to State::last_scene). F1..F3 set the skill (the exe's developer keys).
  Test aid: TB_DEBUG_MAP=1 logs the buttons.
- Pick-a-game room: game/src/scenes/pickagame.* PickAGame_Scene (0x4ae6a0, "pickagame"). It has fifteen
  GameButtons (descriptors from 0x4ae270..0x4ae5f0) that set the area's spot (setAreaSpot) and go there, plus the
  return button to the map. "GLOB_FX_butclick" isn't in the data, so it is never heard. TB_DEBUG_PICK=1.

## Latest (Nov 8 night, Color the Clouds, ~67%)
- game/src/scenes/colorthecloud.*: ColorTheCloud_Scene (0x46ce80, "colorthecloud", static 0x5d0058).
  - Clouds drift down (ColorableCloud 0x46d790). Paint balloons burst on them (CTCPaintBalloon 0x46e560/0x46e6a0).
  - A splash grows rings of colour in the cloud's own pixels (paint 0x46dae0, all three pixel layouts). The
    port then drops the blitter's decoded copy (blit_forget).
  - A cloud leaving the bottom scores its size for its last colour (CloudSpawner 0x46e150/0x46e1f0).
  - Peter decides, flies, and throws (PeterAI 0x46e880..0x46eeb0, on the new shared TopDownCharacterRenderer and
    AnimationOrganizer in game2d.*, 0x4ac5f0..0x4acc70).
  - The score bar is CTCInterface 0x46ef70; the cursor is CTCCursor 0x46d3c0.
- Exe quirks kept:
  - The cursor's chime "COLR_FX_CUactivate01" is never found (the file is ..._01), so it is retried every frame.
  - A balloon's timer is never restarted, so after 50 ms it steps every draw.
  - Peter's "throw" test names an animation he doesn't have.
  - The run never finishes after the end (it keeps the cursor updated).
- The colours come from State::frame_color (the sign-in frame). Both endings go to "Starry", which isn't ported yet.
- Test aid: TB_DEBUG_CLOUD=1; `tinkerbell colorthecloud` + TB_FAKE_INPUT="60:key 32" (space skips Peter's welcome).

## Latest (Nov 8, Bird Spy, ~65%)
- game/src/scenes/birdspy.*: BirdSpy_Scene (0x459f50, "birdspy", static 0x5c3d08). Big birds (GeneralBirdTask
  0x45d890 with BirdSpyGeneral / BirdSpySingleAnim) and small ones (Bird8Task 0x45e3f0 with BirdSpy8 /
  BirdSpy8Anim) fly over the ground, clouds over them (CloudTask 0x45f490). Peter flies about and spots birds
  (BirdPeterControlTask 0x45d680, BirdPeterRenderer 0x45c950). The rings fill with icons (BirdScore 0x45f880).
  The glass cursor is BirdCursor 0x45f100, which uses the new Vision::Lens (vision/lens.*, 0x414dd0: a 2x
  magnifier on the 16-bit canvas). The in-place fast path 0x415350 is drawn with the buffered one (marked port:).
- Exe quirks kept:
  - pick() marks every bird hit on a click as caught, even ones behind the winner.
  - nearest() returns the root of the last bird's distance, not the nearest one's.
  - The birds' 10 s timer test only holds on the first step.
  - The medium cloud is never chosen.
- Both endings go to change_scene("Indian_Village"), which isn't ported yet (as Mermaid for coconut).
- Test aid: TB_DEBUG_BIRDSPY=1; `tinkerbell birdspy` + TB_FAKE_INPUT="60:key 81".

## Latest (Nov 6 evening, Coconuts Away, ~63%)
- game/src/scenes/coconut.*: Coconut_Scene (0x449410, "coconut", static 0x5b70f0): pirate rowboats (Boat 0x44b7a0,
  BoatGroup 0x44c370, the yells SoundCycle 0x4cc220..), coconuts and splashes (CocoItem 0x44c700, CocoGroup move
  0x44cb70), the rabbit (Rabbit 0x44a710: aim at a boat 0x44adb0 / random spot 0x44af00, land 0x44b090, throw
  0x44b240), the count bar (CocoScore 0x44a1d0, NumberShow 0x44cdd0), the cursor, keys as mud pie's, the
  play-again sign (again = Task::reset of the run in place). Ends in change_scene("Mermaid"), which is not ported
  yet (the lagoon): a win aborts there for now. Exe quirk kept: the rabbit's throw looks its coconut up by the
  last throw's time, so only its first throw of a game flies.
- ScrollGround and Scoreboard moved to game2d.* (shared); GRAPHIC_ARGS and ftol there too.
- Test aid: TB_DEBUG_COCONUT=1; `tinkerbell coconut` + TB_FAKE_INPUT="300:key 81".
- Next: "birdspy" (0x5c3d08), "colorthecloud" (0x5d0058), "firefly" (0x5d91b0), then the areas.

## Latest (Nov 6, Mud Pie, ~62%)
- game/src/scenes/mudpie.*: MudPie_Scene (0x436fb0, "mudpie", the static 0x5a96a0) on Game2D_Scene (game2d.*):
  Peter's top-down greeting (PeterGuide), the scrolling ground (ScrollGround 0x4ac0e0 / MudGround 0x435c70), the
  four Lost Boys throwing (Thrower 0x439930), the pies and gems (Flyer 0x439ca0), the skunk (Flier 0x43a2d0 + AI
  0x43a050/0x43a160), the score bars (MudScore 0x438a80), the end scoreboard (0x4ada80), the play-again sign
  (continue_dialog.*, 0x4e5330, shared by the mini games), the cursor (MudCursor), keys (up/down speed,
  left/right slide), the end lines (GameEndVo). Tested headless: a full round, the scoreboard, both answers of the
  dialog (again: the scene re-enters and replays; no: back to "lost"); ASan clean.
- Engine fixes found on the way (all scenes):
  - Node::add links in BEFORE the given node (0x424d10); Layer::add / Screen::activate walk from the end (0x41e930,
    0x41edb0) and Layer::sort is by descending z (0x41e9c0): the highest z draws first, -1000 on top. The port had
    the order reversed (it only showed in 2D scenes; the overlay screen (-10000) is now on top too).
  - The kind-3 (escape-byte RLE) decoder dropped every literal pixel (bounds check against a local): sprites in
    that format came out in pieces (the Lost Boys from above, Peter's top-down art).
  - Task::reset / finish restart the task's run (the exe clears +0x3c so run() starts over): a scene re-entered
    while its run was suspended now starts again.
  - Scene::again (0x41dde0), tasks_finish_all (0x4210d0), CloudBank::cover (0x4acf20).
- Test aid: TB_DEBUG_MUDPIE=1 logs the run's stages. `tinkerbell mudpie` + TB_FAKE_INPUT="300:key 81" (Q skips
  Peter); the dialog's sign (no) is at 235,240, the X (again) at 400,240.
- Not checked: entering from the hideout's mud pie sign (the click spot), catching pies by mouse (works in
  code; no test clicks on moving pies yet).
- Next: the other four 2D games, same base (Game2D_Scene, PeterGuide, ScrollGround, ContinueDialog, GameEndVo,
  Scoreboard): "coconut" (static 0x5b70f0, ctor 0x449410, vtable 0x514554: notify2 0x4487a0, run 0x448a20, load2
  0x449a80, enter 0x449b70, leave 0x44a060, setup 0x448850, mouse 0x448340, keys 0x448670 (as mud pie's), ground
  0x44a4f0, score 0x44a1d0, boats 0x44c370/0x44b7a0, coconut and splash groups 0x44c700/0x44cb70; pack
  "coconuts_away"; the player throws coconuts at pirate rowboats, the rabbit flies), then "birdspy" (0x5c3d08),
  "colorthecloud" (0x5d0058), "firefly" (0x5d91b0); then the other areas.

## Latest (Nov 6, the Lost Boys' hideout, ~60%)
- game/src/scenes/lost_scene.cpp: Lost_Scene (0x43e5b0, "lost"): load2/enter/leave, run (arrival, welcome, the
  mud pie end, sign games, bucket end with Peter's map prompt or cap), mouse (bell help, map, signs, dust on hiding
  places, pieces on the ground, flag tags, bucket throws, character clicks), keys (Escape ends a game), cursor.
  Tink's "lost" help (0x4de970 branch) and the hidden-piece help (0x4de8d0) live there.
- bucket.* (Bucket Toss), pirateflag.* (Pirate Flag Tag), peter_reward.* (PeterCapReward 0x4bf280, PeterPrompt
  0x4bfea0), ambient.*, tokens.*, mappiece.*, area_events.* (shared by the five areas).
- RayTrace: pickAttached 0x42c950, hitAttached 0x42ca00, findAttached 0x42dd00, stickerPos 0x42dd90, hide/show
  stickers, Sprite3D::unload 0x42f2b0. Saves::hasCap 0x4e9af0 (all 15 games' Info 2). TRS: chars (+0x2820) and
  the hand cursor 0x4abda0, leaving (+0x2831).
- Tested: `tinkerbell lost` starts, arrives, Cubby/skunk at their signs, walking; ASan clean. Not yet played
  through: the bucket and flag games, the hidden pieces (need a signed-in save; Escape in London quits the
  headless run, so `tinkerbell lost` without a sign-in has no hidden pieces).
- Next: play-test the two games and the pieces; then "mudpie" (the third Lost Boys game), then the other areas.

## Latest (Oct 28, 23:30, the map tutorial, ~57%)
- game/src/scenes/map.*: Map_Scene_Base (0x4a5b90: the map screen, 12 buttons (Button@MapScene), the five
  areas' token displays, the treasure map mode, the credits roll (Credits), the music) and MapTutorial_Scene
  (0x4a9340, "map_tutorial"): the cursor tours the areas and the buttons while Peter explains, then
  change_scene("lost") (not ported: the game stops there). State: the areas' game records (zero; what fills
  them comes with the areas) and the save's treasure map / X spot queries.
- Next: "lost" (Lost_Scene 0x43e5b0, the hideout proper: the three mini games' doors), then its mini games.

## Earlier (Oct 28 night, the Lost Boys' tutorial, ~55%)
- game/src/scenes/lost.*: Lost_Scene_Base (0x443b40: area 3, max eye 291, the three game signs with the
  caps/stars from the save, the three tunes) and LostTutorial_Scene (0x444440, "lost_tutorial"): the six Lost
  Boys and Peter (TutorialCharacter, tutorial_char.*: idles, talking with mouth attachment and the raccoon
  twins' shared lines, waves, the flying idle with dust), the descent through the clouds (0x4ab760), the bag,
  the dust on a Lost Boy, the butterfly (fill the bag), the bell (Tink's help, she points), the map, then
  change_scene("map_tutorial") (not ported: the game stops there). Q (developer) / Escape skips to "lost".
- TinkerbellRaytraceScene now has its corner buttons (toolbar.*: HelpBellIcon, NeverlandMapIcon, PixieDustBag
  with the dust amount in the save, regen every 15-19 s), the dust cursor (AnimationPlayer "GLOB_CU_pixiedust_"),
  the dust lift (dustlift.*, 0x4befe0: up 3 s / hold 5 s / down 3 s, stepped by the 200 ms cap every frame since
  the exe never sets its clock: a quick hop), the base cursor choice (v30 0x4ab6b0), gravity 0 after open (the
  exe's +0x16b4 = 0; London had kept 100), Global3DTask waits 30 ms like the exe.
- RayTrace: refreshBox 0x4294b0 (and the per-frame refresh of flagged sprites' boxes), cameraInBox 0x429710,
  the max eye as a settable global (0x42b110 / 0x42b120). Tink: TinkCmdPoint (0x4db7b0 / 0x4dcef0).
- GemSign moved to gemsign.*; state: dust (+0x3c8), dusting (+0x3c4), throwdust sound, area spots, last scene.
- Test aids: TB_DEBUG_LOST=1 logs the stages with the input frame; TB_LOST_AUTO=1 catches the butterfly after
  3 s. A full run: `tinkerbell lost_tutorial` with TB_LOST_AUTO=1 TB_FAKE_INPUT="3300:click 100,449;4200:click
  280,330;7000:click 45,440;8800:click 590,450" (bag, Cubby, bell, map).
- Not ported yet: 0x42c950 (the attachment under the dust; nothing uses it so far).
- Next: "map_tutorial" (MapTutorial_Scene@MapScene) and the map, then "lost" (Lost_Scene, 0x43e5b0).

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
