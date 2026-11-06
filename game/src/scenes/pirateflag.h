// Pirate Flag Tag, Cubby's game in the Lost Boys' hideout, reconstructed from "Tinker Bell.exe": four Lost Boys
// dressed as ghosts wander about the hideout; click each one to tag it with the pirate flag before the cannon's
// fuse burns down.
//   GhostPath (0x40 bytes, no vtable): pickTarget 0x447f10
//   Ghost     (0x88 bytes, no vtable): ctor 0x446810, dtor 0x446b20, walk 0x446ba0, newPath 0x446d00, idle 0x446e80,
//             face 0x446f50 (the per-frame step is inlined at each use)
//   PirateFlagGame (0x4c bytes, no vtable): ctor 0x447050, dtor 0x447090, start 0x4470c0, stop 0x447240,
//             load 0x447270, intro 0x4473a0, outro 0x4475d0, cleanup 0x4477b0, play 0x447860, update 0x4479e0,
//             tag 0x447b90, tagged 0x447c00, cursor 0x447c70, nearest 0x447cd0
//   FlagCursor (vtable 0x5142f4, 0x2d0 bytes, ctor 0x442e70, dtor 0x442f40, load v15 0x442fc0, unload v16
//             0x443000, draw v14 0x443030): the flag, waving over a ghost
// The globals 0x5b6d94 (the level: the wander radius is 2000 / level * 6 at most) and 0x5b6d98 (the chance of
// a high spot, 350 up) are set by start.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../raytrace/raytrace.h"
#include "voacts.h"
#include <string>

namespace Game {

struct GhostPath {
    Raytrace::RayTrace *rt = nullptr;                // +0x00
    float from[3] = {}, to[3] = {}, delta[3] = {}, cur[3] = {};   // +0x04 +0x10 +0x1c +0x28
    int duration = 0;                                // +0x34 (ms: 4 per unit)
    int elapsed = 0;                                 // +0x38
    uint32_t last = 0;                               // +0x3c
    // 0x447f10: a spot within 2000 of (3350, 3350), not in a box, not within 100 of the camera; the path's end
    // (toEnd) or its start and current place
    void pickTarget(bool toEnd);
};

struct Ghost {
    Ghost(const char *name, Raytrace::RayTrace *rt); // 0x446810
    ~Ghost();                                        // 0x446b20
    void tick();                                     // (inlined in the game's phases) every 30 ms by state
    void walk();                                     // 0x446ba0 (state 1)
    void newPath();                                  // 0x446d00: up to five tries at a free way
    void idle();                                     // 0x446e80 (state 2)
    void face();                                     // 0x446f50: the walk towards the heading seen from the camera
    char name[0x28] = {};                            // +0x00
    Raytrace::Sprite3D *sprite = nullptr;            // +0x28
    Raytrace::RayTrace *rt;                          // +0x2c
    GhostPath *path = nullptr;                       // +0x30
    uint32_t last = 0;                               // +0x34
    uint32_t wait = 0;                               // +0x38: an idle ends after this (0 none)
    float yoff = 0;                                  // +0x3c: the sprite's height above its path
    int state = 0;                                   // +0x40: 0 placed, 1 walking, 2 idle, 3 tagged
    uint32_t dt = 0;                                 // +0x44
    std::string awww;                                // +0x48: said when tagged
    Vision::VoList taunts;                                 // +0x54
};

class FlagCursor : public Vision::Graphic {
public:
    FlagCursor();                                    // 0x442e70
    void load() override;                            // v15 0x442fc0
    void unload() override;                          // v16 0x443000
    void draw() override;                            // v14 0x443030
    bool waving = false;                             // +0x98
    Vision::Animation wave, flag;                    // +0xa0 "PIRT_CU_flagwave_", +0x1b8 "PIRT_CU_flag_"
};

class PirateFlagGame {
public:
    explicit PirateFlagGame(Raytrace::RayTrace *rt); // 0x447050
    ~PirateFlagGame();                               // 0x447090
    void start();                                    // 0x4470c0: the skill's level and time
    void stop();                                     // 0x447240 (escape)
    void load();                                     // 0x447270 (phase 2)
    void intro();                                    // 0x4473a0 (phase 1): to the ghosts' spot through the dark
    void play();                                     // 0x447860 (phase 3)
    void outro();                                    // 0x4475d0 (phase 5)
    void cleanup();                                  // 0x4477b0
    void update();                                   // 0x4479e0: every 30 ms from the scene's run
    bool tag(Raytrace::Sprite3D *s);                 // 0x447b90: a free ghost clicked
    int tagged();                                    // 0x447c00: how many (the halfway and one-more lines)
    FlagCursor *cursor();                            // 0x447c70: waving over a free ghost
    bool nearest(Raytrace::Sprite3D **out);          // 0x447cd0: the nearest free ghost (made to wait 9 s)
    void tickGhosts();                               // (the loop inlined in phases 1, 3, 4 and 5)
    Raytrace::RayTrace *rt;                          // +0x00
    Ghost *ghosts[4] = {};                           // +0x04..+0x10
    int phase = 0;                                   // +0x14: 0 off, 2 load, 1 intro, 3 play, 4 end, 5 outro
    uint32_t last = 0;                               // +0x18
    uint32_t ended = 0;                              // +0x1c
    uint32_t t0 = 0;                                 // +0x20: the intro's / outro's start
    bool loaded = false;                             // +0x24
    int secs = 0;                                    // +0x28: the cannon's time
    FlagCursor *flag = nullptr;                      // +0x2c
    Vision::Ras *pack = nullptr;                     // +0x40 "pirate_flag"
    uint32_t taunted = 0;                            // +0x44
    bool prompted = false;                           // +0x48
};

}  // namespace Game
