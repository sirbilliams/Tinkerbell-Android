// The Bird Spy game (scene "birdspy", the static at 0x5c3d08, vtable 0x514a30, ctor 0x459f50, dtor 0x45b070),
// reconstructed from "Tinker Bell.exe". Seen from above, birds fly over the ground and under the clouds; the player
// looks at them through a magnifying glass and clicks to spot them, Peter flies about spotting them too. Each spot
// puts the bird's icon on the player's side of the rings (a dark one for a bird already spotted); the one with more
// different birds when the fuse burns out (or either reaches ten icons) wins.
//   notify2 v9 0x458060 (mouse 0x457dc0, keys 0x457eb0), run v10 0x458120, load2 v18 0x459680, enter v19
//   0x459840, leave v20 0x459a40, setup 0x45b8b0, pick 0x459410, hitNear 0x459140, nearest 0x4592a0,
//   birdIndex 0x459730, soundIndex 0x457cb0
// Its pieces (class names from the exe's RTTI):
//   BirdSpySingleAnim   vtable 0x514a8c (ctor 0x45bef0, load 0x45c070, unload 0x45c200, draw 0x45c240): one
//                       direction's flight of a big bird
//   BirdSpy8Anim        vtable 0x514ad8 (load 0x45c630, unload 0x45c7e0, draw 0x45c820): a small bird's eight
//   BirdSpyGeneral      vtable 0x514ba0 (ctor 0x45c280, draw 0x45c3d0, hit 0x45c440): a big bird in the air
//   BirdSpy8            vtable 0x514bec (draw 0x45c900): a small bird in the air
//   BirdSpyCloud        vtable 0x514cec: a cloud over the birds
//   BirdCursor          vtable 0x514b24 (ctor 0x45f100, load 0x45f2e0, unload 0x45f360, draw 0x45f3b0)
//   BirdPeterRenderer   vtable 0x514c38 (ctor 0x45c950, load 0x45cbb0, unload 0x45cd00, animate 0x45cda0,
//                       draw 0x45cf60)
//   BirdPeterControlTask vtable 0x514b70 (run 0x45d680, dtor 0x45cfd0; move 0x45d050, bounds 0x45d120,
//                       wander 0x45d1d0, chase 0x45d290, spot 0x45d3e0)
//   GeneralBirdTask     vtable 0x514c8c (ctor 0x45d890, dtor 0x45e030, run 0x45e120)
//   Bird8Task           vtable 0x514cbc (ctor 0x45e3f0, dtor 0x45ec00, run 0x45ecf0)
//   CloudTask           vtable 0x514d38 (ctor 0x45f490, dtor 0x45f6b0, run 0x45f780)
//   BirdScore           vtable 0x514d68 (ctor 0x45f880, dtor 0x45f970, draw 0x45fb80, add 0x45fc20,
//                       addPeter 0x45ff50, peterHas 0x460280)
#pragma once
#include "game2d.h"
#include "peterguide.h"
#include "acts.h"
#include "voacts.h"
#include "../vision/animation.h"
#include "../vision/lens.h"
#include "../vision/sound.h"
#include <vector>

namespace Game {

class Cannon;
class ContinueDialog;

class BirdSpySingleAnim : public Vision::Graphic {   // 0xd0 bytes
public:
    BirdSpySingleAnim(const char *name, int dir);    // 0x45bef0 (and inline in the scene's ctor)
    ~BirdSpySingleAnim() override;                   // 0x45bfc0
    void draw() override;                            // v14 0x45c240
    void load() override;                            // v15 0x45c070
    void unload() override;                          // v16 0x45c200
    Vision::Animation *anim = nullptr;               // +0x98
    char name[0x28];                                 // +0x9c
    int dir;                                         // +0xc4: 0 no, 1 ne, 2 ea, 3 se, 4 so, 5 sw, 6 we, 7 nw
    int frame = 0;                                   // +0xc8
};

class BirdSpy8Anim : public Vision::Graphic {        // 0xe8 bytes
public:
    explicit BirdSpy8Anim(const char *name);
    ~BirdSpy8Anim() override;
    void draw() override;                            // v14 0x45c820
    void load() override;                            // v15 0x45c630
    void unload() override;                          // v16 0x45c7e0
    Vision::Animation *anims[8] = {};                // +0x98..+0xb4
    char name[0x20];                                 // +0xb8
    int dir = 0;                                     // +0xe0
    int frame = 0;                                   // +0xe4
};

class BirdSpyGeneral : public Vision::Graphic {      // 0xe8 bytes
public:
    BirdSpyGeneral();                                // 0x45c280
    void draw() override;                            // v14 0x45c3d0
    bool hitAt(int x, int y);                        // 0x45c440
    int frame = 0, step = 1;                         // +0x98 +0x9c
    char name[0x28] = {};                            // +0xa0
    int count = 0;                                   // +0xc8
    BirdSpySingleAnim *list[4] = {};                 // +0xcc..+0xd8
    int dir = -1;                                    // +0xdc
    bool caught = false;                             // +0xe0
};

class BirdSpy8 : public Vision::Graphic {            // 0xd8 bytes
public:
    explicit BirdSpy8(BirdSpy8Anim *a);
    void draw() override;                            // v14 0x45c900
    int frame = 0, step = 1;                         // +0x98 +0x9c
    char name[0x28] = {};                            // +0xa0
    BirdSpy8Anim *anim8;                             // +0xc8
    int dir = 0;                                     // +0xcc
    bool caught = false;                             // +0xd0
};

class BirdSpyCloud : public Vision::Graphic {        // 0xa0 bytes
public:
    explicit BirdSpyCloud(Vision::Image *i);
    void draw() override;                            // v14 0x439850 (shared): the image at x, y
    Vision::Image *img;                              // +0x98
};

class BirdCursor : public Vision::Graphic {          // 0x45f100
public:
    BirdCursor();
    void draw() override;                            // v14 0x45f3b0
    void load() override;                            // v15 0x45f2e0
    void unload() override;                          // v16 0x45f360
    Vision::Image active, lensImg, notactive;        // +0x98 +0x140 +0x1e8
    Vision::Animation hand_anim;                     // +0x290 "glob_cu_active_"
    uint32_t hand_t = 0;                             // +0x3a8
    Vision::Lens lens;                               // +0x3b0
    bool on = false;                                 // +0x460: over a bird (scene +0x3f60)
    bool hand = false;                               // +0x461: over the map icon (scene +0x3f61)
    uint32_t t = 0;                                  // +0x464
};

class BirdPeterRenderer : public Vision::Graphic {   // 0x45c950 (0xa88 bytes)
public:
    BirdPeterRenderer();
    void draw() override;                            // v14 0x45cf60
    void load() override;                            // v15 0x45cbb0
    void unload() override;                          // v16 0x45cd00
    void animate();                                  // 0x45cda0
    Vision::Animation fly_no, fly_so, fly_ea, fly_we, idle;   // +0xa0 +0x1b8 +0x2d0 +0x3e8 +0x500
    Vision::Animation str_no, str_so, str_ea, str_we;         // +0x618 +0x730 +0x848 +0x960
    Vision::Animation *cur = nullptr;                // +0xa78
    int lastdir = 666;                               // +0xa7c
    uint32_t timer = 0;                              // +0xa80
    int dir = 666;                                   // +0xa84: 0 north, 2 east, 4 south, 6 west, 666 hovering
};

class BirdPeterControlTask : public Vision::Task {   // 0xf0 bytes
public:
    BirdPeterControlTask();
    ~BirdPeterControlTask() override;                // 0x45cfd0
    void run() override;                             // 0x45d680
    void move();                                     // 0x45d050
    void bounds();                                   // 0x45d120
    void wander();                                   // 0x45d1d0
    void chase();                                    // 0x45d290
    void spot();                                     // 0x45d3e0
    BirdPeterRenderer *renderer = nullptr;           // +0xc0
    int x = 0, y = 0;                                // +0xc4 +0xc8
    bool moving = false;                             // +0xcc
    int counter = 0, duration = 0;                   // +0xd8 +0xdc
    uint32_t lastspot = 0;                           // +0xe0
    bool stop = false;                               // +0xe4
    int dir = 666;                                   // +0xe8
    int speed = 7;                                   // +0xec
};

class GeneralBirdTask : public Vision::Task {        // 0x41c bytes
public:
    GeneralBirdTask(Vision::Screen *s, const char *name);   // 0x45d890
    ~GeneralBirdTask() override;                     // 0x45e030
    void run() override;                             // 0x45e120
    uint32_t t0 = 0;                                 // +0xc0
    BirdSpyGeneral *g = nullptr;                     // +0xc4
    int speed = 0;                                   // +0xcc
    Vision::Sound sound;                             // +0xd0
    int soundIdx = 1;                                // +0x418
};

class Bird8Task : public Vision::Task {              // 0x424 bytes
public:
    Bird8Task(Vision::Screen *s, const char *name);  // 0x45e3f0
    ~Bird8Task() override;                           // 0x45ec00
    void run() override;                             // 0x45ecf0
    uint32_t t0 = 0;                                 // +0xc0
    bool entered = false;                            // +0xc4: has been on the screen
    int turn = 0;                                    // +0xc8: one in this many 30 ms steps it turns
    BirdSpy8 *g = nullptr;                           // +0xcc
    int speed = 0;                                   // +0xd4
    int soundIdx = 1;                                // +0xd8
    Vision::Sound sound;                             // +0xdc
};

class CloudTask : public Vision::Task {              // 0xc8 bytes
public:
    explicit CloudTask(Vision::Screen *s);           // 0x45f490
    ~CloudTask() override;                           // 0x45f6b0
    void run() override;                             // 0x45f780
    BirdSpyCloud *g = nullptr;                       // +0xc0
};

class BirdScore : public Vision::Graphic {           // 0x45f880 (0x168 bytes)
public:
    BirdScore();
    ~BirdScore() override;                           // 0x45f970
    void draw() override;                            // v14 0x45fb80
    void add(int idx);                               // 0x45fc20
    void addPeter(int idx);                          // 0x45ff50
    bool peterHas(int idx);                          // 0x460280
    Vision::Image rings;                             // +0x98 "BIRD_IF_rings_01"
    std::vector<Vision::Image *> player, peter;      // +0x140 +0x150
    int p = 0, s = 0;                                // +0x160 +0x164
};

class BirdSpy_Scene : public Game2D_Scene {
public:
    BirdSpy_Scene();                                 // 0x459f50
    ~BirdSpy_Scene() override;                       // 0x45b070
    void notify2(Vision::Service *s) override;       // v9 0x458060
    void run() override;                             // v10 0x458120
    void load2() override;                           // v18 0x459680
    void enter() override;                           // v19 0x459840
    void leave() override;                           // v20 0x459a40
    void onMouse();                                  // 0x457dc0
    void onKey();                                    // 0x457eb0
    void setup();                                    // 0x45b8b0
    int pick(int x, int y, char *name);              // 0x459410
    bool hitNear(Vision::Graphic *g, int x, int y);  // 0x459140
    int nearest(int x, int y, int *bx, int *by);     // 0x4592a0
    int birdIndex(const char *name);                 // 0x459730
    void zAdd(Vision::Graphic *g);                   // (inlined: the depth made unique among the birds', kept)
    void zRemove(Vision::Graphic *g);                // (inlined in the tasks' dtors)

    Vision::Mouse mouse;                             // +0x15c
    Vision::Keyboard keyboard;                       // +0x190
    Vision::Ras *pack = nullptr;                     // +0x1c4 "bird_spy"
    int zcount = 0;                                  // +0x1cc
    Vision::Graphic *zslots[0x1f4] = {};             // +0x1d0 (to +0x9a0)
    int cloudKind = 0, cloudSkill = 0;               // +0x9a0 +0x9a4: by the skill
    PeterGuide guide;                                // +0x9a8 (its task at +0xa40)
    Inactivity inactivity;                           // +0x1028
    bool clicking = false;                           // +0x13c8
    ScrollGround *ground = nullptr;                  // +0x13cc
    BirdSpy8Anim hum, cro, par, spa, tou;            // +0x13d0 +0x14b8 +0x15a0 +0x1688 +0x1770
    BirdSpySingleAnim pel[4], owl[4], haw[4], fla[2], cra[2];   // +0x1858.. (0xd0 apart)
    Vision::Image cloudlg, cloudmd, cloudsm;         // +0x2558 +0x2600 +0x26a8
    Vision::Image *small[10] = {}, *bigdk[10] = {}, *biglt[10] = {};   // +0x2750 +0x2de0 +0x3470 (loaded, not drawn)
    BirdCursor cursor;                               // +0x3b00
    uint32_t last_spot = 0;                          // +0x3f64
    bool up = false, down = false, left = false, right = false;   // +0x3f68..+0x3f6b (0x5c7c70..73)
    BirdScore *score = nullptr;                      // +0x3f6c (0x5c7c74)
    Scoreboard *board = nullptr;                     // +0x3f70
    BirdPeterControlTask *peter = nullptr;           // +0x3f74
    bool over = false;                               // +0x3f78 (0x5c7c80)
    int secs = 0;                                    // +0x3f7c
    ContinueDialog *dialog = nullptr;                // +0x3f80
    int leaves = 0;                                  // +0x3f84
    struct SoundSlot : Vision::Sound { SoundSlot() : Sound(nullptr) {} };
    SoundSlot calls[3];                              // +0x3f88: caw, chirp, tweet
    Vision::Sound music;                             // +0x4960
    struct Line { float a = 0, b = 0; } when[3];     // +0x4ca8: each call's next time, a * now + b
    float mult = 1.0f;                               // +0x4cc0 (0x5c89c8): the birds' speed by the skill
    GameEndVo endvo;                                 // +0x4cc4
    bool playing = false;                            // +0x4e34
};

BirdSpy_Scene &birdspy_scene();                      // the static at 0x5c3d08

}  // namespace Game
