// Backwards Go, Cubby's game in Starry Skies, reconstructed from "Tinker Bell.exe" (played in the Starry Skies scene
// itself; its object is the scene's +0x3fe0). The camera stands with Tink and the Lost Boys on a little grid; Cubby
// calls a move ("go left!") or a backwards one ("backwards go left!": the other way), the player makes it with the
// arrows (Shift up, Ctrl down) and everyone copies the player; a boy who gets it wrong flies off. The player wins
// when every boy has been caught out, and loses on a wrong move or by waiting too long.
//   BackwardsGoCharacter     vtable 0x5150e8, 0x728 bytes: ctor 0x46fa00, dtor 0x46fc60, init v1 0x46fce0,
//                            release v2 0x46fd70, step v3 0x470120, anim 0x46fdc0, flyAway 0x46ff20, begin 0x470000
//   BackwardsGoTinkerbell    vtable 0x515100, 0x730 bytes (made by 0x470920 / 0x470aa0): step v3 0x4703e0
//   BackwardsGoDoppleganger  vtable 0x515114, 0x54 bytes: ctor 0x470480, sync 0x470510 (a copy of the raccoon,
//                            120 nearer: the raccoon twins)
//   the game (no RTTI, 0x298 bytes): ctor 0x4705c0, dtor 0x470640, clear 0x470730, init 0x4707b0, setup 0x470860,
//   addTink 0x470920, addChar 0x4709e0, setup5 0x470aa0, begin 0x470dd0, hint 0x470f00, command 0x470f30,
//   keys 0x4711b0, play 0x471390, update 0x471650, start 0x471980, skip 0x4719a0
#pragma once
#include "../raytrace/character.h"
#include "../vision/sound.h"
#include "voacts.h"

namespace Game {

class BackwardsGoGame;

class BackwardsGoCharacter : public Raytrace::RayTraceCharacter {
public:
    BackwardsGoCharacter(Raytrace::RayTrace *rt, const char *name);   // 0x46fa00
    ~BackwardsGoCharacter() override;                // 0x46fc60
    void init() override;                            // v1 0x46fce0
    void release() override;                         // v2 0x46fd70
    // v3 0x470120: one step toward the player's move (dx, dy, dz); true once the move is made as told
    virtual bool step(int dx, int dy, int dz);
    void anim(const char *move, bool loop);          // 0x46fdc0: "back_ca_<name><move>_"
    bool flyAway();                                  // 0x46ff20: up and off (true once gone)
    void begin();                                    // 0x470000: the move's speeds and animation

    Vision::Ras *pack = nullptr;                     // +0x50 "<name>_backwardsgo"
    // +0x54: a random number object (0x425d10); its draws are the game's rand() (0x425d40)
    int gx = 0, gy = 0, gz = 0;                      // +0x58 +0x5c +0x60: its square and height (0 / 1)
    int dx = 0, dy = 0, dz = 0;                      // +0x64 +0x68 +0x6c: the move
    float vx = 0, vy = 0, vz = 0;                    // +0x70 +0x74 +0x78: a step
    float dist = 0, target = 0;                      // +0x7c +0x80
    Vision::Sound aww, rats;                         // +0x84 +0x3cc: lost, caught out
    bool flying = false;                             // +0x714: the animation is a flying one
    int st = 0;                                      // +0x718: 0 choosing, 1 moving, 2 done
    bool caught = false;                             // +0x71c: got it wrong
    int chance = 0x50;                               // +0x720: % of copying the player (else the backwards move)
    float fly_y = 100.0f;                            // +0x724: its height when up
};

class BackwardsGoTinkerbell : public BackwardsGoCharacter {
public:
    BackwardsGoTinkerbell(Raytrace::RayTrace *rt, BackwardsGoGame *g);   // (0x470920)
    bool step(int dx, int dy, int dz) override;      // v3 0x4703e0: copies the player (or shows the right move)
    BackwardsGoGame *game;                           // +0x728
    bool show = false;                               // +0x72c: the hint: she makes the move asked
};

class BackwardsGoDoppleganger : public Raytrace::RayTraceCharacter {
public:
    BackwardsGoDoppleganger(Raytrace::RayTraceCharacter *src, const char *name);   // 0x470480
    ~BackwardsGoDoppleganger() override {}           // 0x4704d0
    void sync();                                     // 0x470510: the source's animation, frame, place and scale
    Raytrace::RayTraceCharacter *src;                // +0x50
};

class BackwardsGoGame {
public:
    BackwardsGoGame();                               // 0x4705c0
    ~BackwardsGoGame();                              // 0x470640
    void clear();                                    // 0x470730: the characters gone, state 0
    void init();                                     // 0x4707b0 (state 0)
    void setup();                                    // 0x470860 (state 2): the camera on the grid, the scene's Peter,
                                                     // Foxy and Tink away
    void addTink();                                  // 0x470920
    void addChar(int slot, const char *name, float dx, float dz);   // 0x4709e0
    void setupAll();                                 // 0x470aa0 (state 4): the five boys and the doppleganger
    void begin();                                    // 0x470dd0 (state 5)
    bool hint();                                     // 0x470f00: the help bell during a move: Tink shows it
    void command();                                  // 0x470f30 (state 6): Cubby's next call
    void keys();                                     // 0x4711b0: the player's move
    void play();                                     // 0x471390 (state 8)
    void update();                                   // 0x471650: every 30 ms while it is on
    void start();                                    // 0x471980
    void skip();                                     // 0x4719a0: Escape

    float cub_x = 0, cub_z = 0;                      // +0x04 +0x08: Cubby's place before
    int count = 0;                                   // +0x0c: the boys playing (2 the first time, then 5)
    BackwardsGoCharacter *chars[5] = {};             // +0x10
    BackwardsGoDoppleganger *dopple = nullptr;       // +0x24
    BackwardsGoTinkerbell *tink = nullptr;           // +0x28
    int px = 0, py = 0, pz = 0;                      // +0x2c +0x30 +0x34: the player's square and height
    std::string line;                                // +0x38: Cubby's call
    int cx = 0, cy = 0, cz = 0;                      // +0x60 +0x64 +0x68: the move it asks for
    uint32_t timeout = 0;                            // +0x6c
    uint32_t t0 = 0;                                 // +0x70
    int move = 0;                                    // +0x74: the player's: 0 none yet, 1 moving, 2 made
    int mz = 0, my = 0, mx = 0;                      // +0x78 +0x7c +0x80: the player's move
    bool f84 = false;                                // +0x84
    float vx = 0, vy = 0, vz = 0;                    // +0x88 +0x8c +0x90: the camera's step
    float dist = 0, target = 0;                      // +0x94 +0x98
    int bits = 0;                                    // +0x9c
    int state = 0;                                   // +0xa0
    bool first = false;                              // +0xa4: the first call is a plain one
    Vision::Reminder reminder;                       // +0xa8 "FLYH_LY_shiftcnt_01", "BACK_VO_PETupdown"
};

}  // namespace Game
