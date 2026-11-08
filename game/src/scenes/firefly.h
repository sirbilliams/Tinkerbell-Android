// The Firefly game (scene "firefly", the static at 0x5d91b0, vtable 0x5152bc, ctor 0x47db70, dtor 0x47dda0),
// reconstructed from "Tinker Bell.exe" (namespace Firefly in its RTTI; its files are "DCBL_"). Seen from above at
// dusk: swarms of fireflies drift over the ground; the player catches them in a jar (six fill it), then lights a
// lamp or a turtle's lantern with them. Peter does the same. Eight lights win; the cannon's fuse is the clock.
//   notify2 v9 0x47c880 (mouse 0x47c350, keys 0x47c6c0), run v10 0x47c930, load2 v18 0x47d430, enter v19 0x47d6b0,
//   leave v20 0x47d9f0, setup 0x47d8f0, setupPeter 0x47d520, cursorUpdate 0x47c5a0
// Its pieces:
//   FireflyCursor       vtable 0x515268 (ctor 0x47bda0, dtor 0x47c140, load 0x47c200, unload 0x47c290, draw 0x47c2d0)
//   PeterControlScript  vtable 0x5152b4 (0x30 bytes; animate v0 0x47e030, setState 0x47e000, decide 0x47e230,
//                       move 0x47e460, home 0x47e610, dtor 0x47dff0)
//   Turtle              vtable 0x515320 (ctor 0x47e7a0, load 0x47e9c0, unload 0x47ea40, draw 0x47ea80)
//   Swarm               vtable 0x51536c (ctor 0x47eb90, dtor 0x47ed00, load 0x47edb0, unload 0x47ee00, near 0x47ee30,
//                       draw 0x47ee60, start 0x47ef10)
//   BackgroundScroller  vtable 0x5153b8 (ctor 0x47f040, load 0x47f300, unload 0x47f5d0, update v18 0x47f680,
//                       draw 0x47f7f0, canLight 0x47f890, light 0x47f9b0, catchAt 0x47fb20, nearestSwarm 0x47fbc0,
//                       nearestLight 0x47fc70)
//   TurtleGroup         (dtor 0x47fdd0, nearest 0x47ff20, spawn 0x480100)
//   FFInterface         vtable 0x515408 (ctor 0x480430, dtor 0x480820, draw 0x480a50)
#pragma once
#include "game2d.h"
#include "peterguide.h"
#include "acts.h"
#include "voacts.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include <vector>

namespace Game {

class Cannon;
class ContinueDialog;

class FireflyCursor : public Vision::Graphic {       // 0x1f8 bytes
public:
    FireflyCursor();                                 // 0x47bda0
    ~FireflyCursor() override;                       // 0x47c140
    void draw() override;                            // v14 0x47c2d0
    void load() override;                            // v15 0x47c200
    void unload() override;                          // v16 0x47c290
    Vision::Animation *idle[7] = {}, *ready[7] = {};   // +0x98 +0xb4: the jar with 0..6 fireflies
    Vision::Animation hand_anim;                     // +0xd0 "glob_cu_active_"
    uint32_t t = 0;                                  // +0x1e8
    bool hand = false, near = false;                 // +0x1ec +0x1ed
    int count = 0;                                   // +0x1f0
    uint32_t t2 = 0;                                 // +0x1f4
};

class Turtle : public Vision::Graphic {              // 0x3f8 bytes
public:
    Turtle();                                        // 0x47e7a0
    ~Turtle() override;
    void draw() override;                            // v14 0x47ea80
    void load() override;                            // v15 0x47e9c0
    void unload() override;                          // v16 0x47ea40
    Vision::Animation *off = nullptr, *on = nullptr; // +0x9c +0xa0
    uint32_t t = 0;                                  // +0xa4
    bool active = false, lit = false;                // +0xa8 +0xa9
    int vx = 0;                                      // +0xac
    Vision::Sound sound;                             // +0xb0 "dcbl_fx_turtle"
};

class Swarm : public Vision::Graphic {               // 0x400 bytes
public:
    explicit Swarm(int n);                           // 0x47eb90
    ~Swarm() override;                               // 0x47ed00
    void draw() override;                            // v14 0x47ee60
    void load() override;                            // v15 0x47edb0
    void unload() override;                          // v16 0x47ee00
    bool near(int x, int y);                         // 0x47ee30
    void start();                                    // 0x47ef10
    Vision::Animation *anim = nullptr;               // +0x9c
    uint32_t t = 0;                                  // +0xa0
    bool active = false;                             // +0xa4
    int n;                                           // +0xa8: its fireflies (1..3)
    int vx = 0, vy = 0;                              // +0xac +0xb0
    Vision::Sound sound;                             // +0xb4 "dcbl_fx_swarm"
};

struct TurtleGroup {                                 // 0x10 bytes
    ~TurtleGroup();                                  // 0x47fdd0
    bool nearest(int x, int y, int *ox, int *oy);    // 0x47ff20
    void spawn();                                    // 0x480100
    std::vector<Turtle *> turtles;                   // +4
};

class BackgroundScroller : public ScrollGround {     // 0x418 bytes
public:
    BackgroundScroller(const char *a, const char *b);   // 0x47f040
    ~BackgroundScroller() override;
    void draw() override;                            // v14 0x47f7f0
    void load() override;                            // v15 0x47f300
    void unload() override;                          // v16 0x47f5d0
    void update() override;                          // v18 0x47f680
    bool canLight(int x, int y);                     // 0x47f890
    bool light(int x, int y);                        // 0x47f9b0
    int catchAt(int x, int y);                       // 0x47fb20
    bool nearestSwarm(int x, int y, int *ox, int *oy);   // 0x47fbc0
    bool nearestLight(int x, int y, int *ox, int *oy);   // 0x47fc70
    Vision::Image target1, target2;                  // +0x2a0 +0x348 (where turtles cannot walk)
    Vision::Image *lampA[6] = {}, *lampB[6] = {};    // +0x26c +0x284
    bool litA[6] = {}, litB[6] = {};                 // +0x3f0 +0x3f6
    Swarm *swarms[5] = {};                           // +0x3fc
    TurtleGroup *group = nullptr;                    // +0x410
};

class FFInterface : public Vision::Graphic {         // 0x2b8 bytes
public:
    FFInterface();                                   // 0x480430
    ~FFInterface() override;                         // 0x480820
    void draw() override;                            // v14 0x480a50
    Vision::Image bar, dark, lit;                    // +0x98 +0x140 +0x1e8
    std::vector<Vision::Image *> mine, his;          // +0x290 +0x2a0: the light places "DCBL_IF_target_01..16"
    int p = 0, s = 0;                                // +0x2b0 +0x2b4
};

struct PeterControlScript {                          // 0x30 bytes
    virtual ~PeterControlScript() {}                 // 0x47dff0
    virtual void animate();                          // v0 0x47e030
    void setState(int st);                           // 0x47e000
    void decide();                                   // 0x47e230
    void move();                                     // 0x47e460
    void home();                                     // 0x47e610
    int tx = 0, ty = 0;                              // +8 +0xc
    int count = 0;                                   // +0x10: fireflies carried
    TopDownCharacterRenderer *r = nullptr;           // +0x14
    int dir = 0, lastdir = 0;                        // +0x18 +0x1c
    Vision::Animation *cur = nullptr;                // +0x20
    int state = 0;                                   // +0x24: 0 start, 1 deciding, 2 flying, 3 to a light
    int counter = 0;                                 // +0x28
    bool stop = false;                               // +0x2c
};

class Firefly_Scene : public Game2D_Scene {
public:
    Firefly_Scene();                                 // 0x47db70
    ~Firefly_Scene() override;                       // 0x47dda0
    void notify2(Vision::Service *s) override;       // v9 0x47c880
    void run() override;                             // v10 0x47c930
    void load2() override;                           // v18 0x47d430
    void enter() override;                           // v19 0x47d6b0
    void leave() override;                           // v20 0x47d9f0
    void onMouse();                                  // 0x47c350
    void onKey();                                    // 0x47c6c0
    void setup();                                    // 0x47d8f0
    void setupPeter();                               // 0x47d520
    void cursorUpdate();                             // 0x47c5a0

    Vision::Mouse mouse;                             // +0x15c
    Vision::Keyboard keyboard;                       // +0x190
    Vision::Ras *pack = nullptr;                     // +0x1c8
    struct SoundSlot : Vision::Sound { SoundSlot() : Sound(nullptr) {} };
    SoundSlot sounds[3];                             // +0x1cc: lantlit, gdrop, cuclicky
    PeterGuide guide;                                // +0xba8 (its task at +0xc40)
    bool escaped = false;                            // +0x1228
    Inactivity inactivity;                           // +0x122c
    Scoreboard *board = nullptr;                     // +0x15cc
    std::vector<const char *> nice;                  // +0x15d0
    BackgroundScroller *scroller = nullptr;          // +0x15e0
    FFInterface *iface = nullptr;                    // +0x15e4
    FireflyCursor *cursor = nullptr;                 // +0x15e8
    TopDownCharacterRenderer *peter_r = nullptr;     // +0x15ec
    PeterControlScript *ai = nullptr;                // +0x15f0
    bool up = false, down = false, left = false, right = false;   // +0x15f4..+0x15f7
    bool over = false;                               // +0x15f8
    int secs = 0;                                    // +0x15fc
    ContinueDialog *dialog = nullptr;                // +0x1600
    Vision::Sound music;                             // +0x1604
    GameEndVo endvo;                                 // +0x194c
    bool playing = false;                            // +0x1abc
};

Firefly_Scene &firefly_scene();                      // the static at 0x5d91b0

}  // namespace Game
