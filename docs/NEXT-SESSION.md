# Next session

## State at the end of session 1 (Oct 24, 2026, ~20% to goal)
- `main` = `integ`: boots on its own Disney bumper -> London intro (title) -> sign-in room on the desktop,
  ASan/UBSan clean over 3,500 frames. The sign-in room is drawn but not interactive yet.
- First APK (`latest` release) failed on the Thor at start: `dlopen failed: library "libavformat.so" not
  found` (the FFmpeg libs were packaged as lib<name>-<major>.so but their sonames are lib<name>.so). Fixed in
  ecaae33 (packaged under their sonames; CI now checks every NEEDED entry of the APK's native libs).
  **First thing: check that CI run is green, then ask Bill to retest the new `latest` APK on the Thor.**
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
