// Bucket Toss, the raccoon twins' game in the Lost Boys' hideout, reconstructed from "Tinker Bell.exe":
//   BucketGame   (0x1108 bytes, no vtable): ctor 0x43b740, dtor 0x43b810, load 0x43b9c0, unload 0x43bc80,
//                end 0x43bcb0 (0x43c770 jumps there), update 0x43bd70, win 0x43be90, table 0x43bf80, sign 0x43c120,
//                hint 0x43c340, setupTable 0x43c420, start 0x43c5b0, hover 0x43c780, pick 0x43c890, toss 0x43c960
//   BucketCursor (vtable 0x514180, 0xb8 bytes, ctor 0x43c9a0, dtor 0x43cb50, load v15 0x43cc10, unload v16
//                0x43cc70, draw v14 0x43ccd0): the piece carried, one animation per colour
//   BucketBall   (vtable 0x5141cc, 0xae8 bytes, ctor 0x43cd40, dtor 0x43cf80, load v15 0x43d070, unload v16
//                0x43d130, throw 0x43d1b0, draw v14 0x43d2c0): the piece flying up into the tree
// A sign on the tree shows a colour now and then; the piece of that colour thrown into the bucket counts.
// Five hits, and the branch drops the map segment. Colours: 1 blue, 2 green, 3 purple, 4 red, 5 yellow, 6 none.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"
#include "acts.h"
#include "voacts.h"

namespace Game {

class BucketCursor : public Vision::Graphic {
public:
    BucketCursor();                                  // 0x43c9a0
    ~BucketCursor() override;                        // 0x43cb50
    void load() override;                            // v15 0x43cc10
    void unload() override;                          // v16 0x43cc70
    void draw() override;                            // v14 0x43ccd0
    Vision::Animation *anims[5] = {};                // +0x98..+0xa8 "BUCK_CU_BAL<col>_"
    bool loaded = false;                             // +0xac
    int colour = 6;                                  // +0xb4
};

class BucketBall : public Vision::Graphic {
public:
    explicit BucketBall(Raytrace::RayTrace *rt);     // 0x43cd40
    ~BucketBall() override;                          // 0x43cf80
    void load() override;                            // v15 0x43d070
    void unload() override;                          // v16 0x43d130
    // 0x43d1b0: from the screen spot, as far up the tree as the camera's z allows
    void toss(float x, float y, float camz, int colour);
    void draw() override;                            // v14 0x43d2c0
    Vision::Animation *anims[5] = {};                // +0x98..+0xa8 "BUCK_GA_thr<col>_"
    Raytrace::RayTrace *rt;                          // +0xac
    bool loaded = false;                             // +0xb0
    bool flying = false;                             // +0xb1
    uint32_t last = 0;                               // +0xb4
    int colour = 6;                                  // +0xb8
    int frame = 0;                                   // +0xbc
    int target = 0;                                  // +0xc0: the frame where it reaches the tree
    bool left = false, right = false, high = false;  // +0xc4 +0xc5 +0xc6: the miss drifts
    Vision::Sound *fx[3] = {};                       // +0xc8 (embedded, 0x348 each): puzgoin, BUCK_FX_woodknock,
                                                     // puzthud
    int wrong = 0, low = 0, highs = 0, misses = 0;   // +0xaa0 +0xaa4 +0xaa8 +0xaac
    bool first = true;                               // +0xab0
    VoList nice;                                     // +0xab4 "BUCK_VO_RACnice1".."6"
};

class BucketGame {
public:
    BucketGame(Raytrace::RayTrace *rt, Vision::Screen *screen);   // 0x43b740
    ~BucketGame();                                   // 0x43b810
    void load();                                     // 0x43b9c0: the tree and its stickers
    void unload();                                   // 0x43bc80
    void end();                                      // 0x43bcb0 (stop 0x43c770)
    void update();                                   // 0x43bd70: every frame from the scene's run
    void win();                                      // 0x43be90: the branch drops
    void table();                                    // 0x43bf80: the pieces on the table
    void sign();                                     // 0x43c120: the colour the sign shows
    void hint();                                     // 0x43c340: Tink points at the sign
    void setupTable();                               // 0x43c420
    void start();                                    // 0x43c5b0
    bool hover(Raytrace::Attachment *a);             // 0x43c780: the piece under the mouse lifts
    bool pick(Raytrace::Attachment *a);              // 0x43c890: a piece taken from the table
    void toss(float x, float y, int colour);         // 0x43c960
    Raytrace::RayTrace *rt;                          // +0x00
    Vision::Screen *screen;                          // +0x04
    BucketBall *ball = nullptr;                      // +0x08
    bool done[5] = {};                               // +0x0c..+0x10: the colours hit
    int sign_colour = 0;                             // +0x14: 0 spinning
    uint32_t spin_until = 0, show_until = 0;         // +0x18 +0x1c
    int delay = 0;                                   // +0x20: by skill
    uint32_t last = 0;                               // +0x24
    int phase = 0;                                   // +0x28: 0 off, 1 move the camera, 2 moving, 3 playing, 4 won
    Vision::Sound *fx[4] = {};                       // +0x2c (embedded): signspin, woodknock, puztoss, branchopn
    Inactivity prompt;                               // +0xd4c "BUCK_VO_RACprompt"
    uint32_t hint_until = 0;                         // +0x10ec
    Raytrace::CameraMover mover;                     // +0x10f0
};

}  // namespace Game
