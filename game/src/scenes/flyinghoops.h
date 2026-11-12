// Flying Hoops, the raccoons' game in the Indian Village (played in the scene itself; its object is the scene's
// +0x4658), reconstructed from "Tinker Bell.exe" (RTTI FlyingHoops::FlyingHoopsGame, vtable 0x514dcc, 0x690 bytes,
// ctor 0x4611e0, dtor 0x4614c0, run v10 0x4616c0). Ten white hoops hang on the poles and teepees; flying through one
// (moving forward) turns it red. The fuse is the clock. Smoke rises from some of the camp fires (more with skill):
// flying into it is not allowed.
//   smoke 0x4604c0, hoops 0x4608b0, update 0x460ac0, setup 0x460da0, start 0x460e70, results 0x460f50,
//   skip 0x461040, hover 0x461070, nextHoop 0x4619e0
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"
#include "voacts.h"
#include "acts.h"
#include <vector>

namespace Game {

class FlyingHoopsGame : public Vision::Task {
public:
    FlyingHoopsGame();                               // 0x4611e0
    void run() override;                             // v10 0x4616c0
    void smoke();                                    // 0x4604c0 (enter): smoke over `level` of the fires
    void hoops();                                    // 0x4608b0 (enter): a white and a red hoop at each pole
    void update();                                   // 0x460ac0: hoops passed, smoke flown into
    void setup();                                    // 0x460da0 (enter): the time and the smoke by skill
    void start();                                    // 0x460e70: after the fade
    void results();                                  // 0x460f50
    void skip();                                     // 0x461040 (the cheat key)
    void hover();                                    // 0x461070: the eye eases toward the nearest hoop's height
    bool nextHoop(Raytrace::Sprite3D **out);         // 0x4619e0: the nearest white hoop (Tink's hint)

    bool done[10] = {};                              // +0xc0: hoop passed
    int passed = 0;                                  // +0xcc
    int last = 0;                                    // +0xd0: passed at the last update
    bool on = false;                                 // +0xd4 (the scene's +0x472c)
    int bits = 0;                                    // +0xd8 (the scene's +0x4730)
    Vision::Reminder reminder;                       // +0xe0 "FLYH_LY_shiftcnt_01" / "FLYH_VO_PETupdown"
    uint32_t t0 = 0;                                 // +0x2d0: the end's fade
    int level = 0;                                   // +0x2d4: fires with smoke
    int secs = 0;                                    // +0x2d8
    Inactivity inactivity;                           // +0x2dc ("FLYH_VO_RACprompt")
    std::vector<const char *> nice;                  // +0x67c: FLYH_VO_RACnice1..4
};

}  // namespace Game
