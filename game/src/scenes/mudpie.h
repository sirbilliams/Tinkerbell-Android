// The Mud Pie game (scene "mudpie", the static at 0x5a96a0, vtable 0x513f2c, ctor 0x436fb0, dtor 0x437390),
// reconstructed from "Tinker Bell.exe". Seen from above: the Lost Boys (rabbit, raccoon, cub, fox) scroll down the
// ground and throw mud pies and gems up or down the screen; the player clicks them out of the air, the skunk
// (Flier "skn") flies after them too; ten each fill the score bars. The cannon's fuse is the clock.
//   notify2 v9 0x436240 (mouse 0x435e90, keys 0x436110), run v10 0x4366c0, load2 v18 0x437680, enter v19 0x437a90,
//   leave v20 0x4381e0, setup 0x437cb0, start 0x4362f0, result 0x436610, spawn 0x438490, nearest 0x4387b0,
//   nextSpot 0x4388d0, overlap 0x4389d0, targets 0x437770
// Its pieces:
//   ScrollGround  vtable 0x516450 (ctor 0x4ac0e0, load 0x4ac2c0, unload 0x4ac350, update v18 0x4ac3e0, draw
//                 0x4ac550): two ground images scrolling down (the up and down keys set the speed), left and right
//                 slide it; MudGround (vtable 0x513ecc, update 0x435c70) moves the throwers and pies with it
//   MudCursor     vtable 0x513f88 (ctor 0x4396d0): the cursor graphic showing one of the cursors
//   MudScore      vtable 0x514010 (ctor 0x438a80, 0x1f8 bytes): the bars and the caught pies' icons
//   Thrower       vtable 0x51405c (ctor 0x439930, 0x310 bytes): a Lost Boy standing, throwing at a release frame
//   Flyer         vtable 0x5140a8 (ctor 0x439ca0, 0x2e8 bytes): a pie or gem in the air
//   Flier         vtable 0x5140f4 (ctor 0x43a2d0): a character flying around the screen (the skunk)
//   SkunkAI       (scene +0x1e40, modes 0x43a050 / 0x43a160): where the skunk flies next
//   Scoreboard    vtable 0x516678 (ctor 0x4ada80, 0x168 bytes): the two scores at the end
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
class MudPie_Scene;

class ScrollGround : public Vision::Graphic {
public:
    ScrollGround(const char *first, const char *second);   // 0x4ac0e0
    ~ScrollGround() override;                        // 0x4ac230
    void draw() override;                            // v14 0x4ac550
    void load() override;                            // v15 0x4ac2c0
    void unload() override;                          // v16 0x4ac350
    virtual void update();                           // v18 0x4ac3e0
    void setDir(int d);                              // 0x4ac380: 0 none, 1 left (+0xe8), 2 right (+0xe9)
    void setMult(int m) { mult = m; }                // 0x4ac3c0
    void setAuto(bool a) { autoupdate = a; }         // 0x4ac3d0
    char name1[0x28], name2[0x28];                   // +0x98 +0xc0
    bool left_ = false, right_ = false;              // +0xe8 +0xe9
    int speed = 30;                                  // +0xec (ms per unit)
    uint32_t last = 0;                               // +0xf0
    bool autoupdate = false;                         // +0xf4
    float accx = 0, accy = 0;                        // +0xf8 +0xfc
    int row1 = 0, row2 = 0;                          // +0x100 +0x104
    int dx = 0, dy = 0;                              // +0x108 +0x10c: this update's moves
    int mult = 2;                                    // +0x110
    Vision::Image img1, img2;                        // +0x118 +0x1c0
};

class MudGround : public ScrollGround {              // vtable 0x513ecc
public:
    MudGround();
    void update() override;                          // v18 0x435c70
};

class MudCursor : public Vision::Graphic {           // 0x4396d0
public:
    MudCursor();
    ~MudCursor() override;                           // 0x4397b0
    void draw() override;                            // v14 0x439850
    void load() override;                            // v15 0x439880
    void unload() override;                          // v16 0x4398f0
    Vision::Graphic *cur = nullptr;                  // +0x98
    Vision::Animation notactive, active;             // +0xa0 +0x1b8
    Vision::Graphic *normal = nullptr, *hand = nullptr;   // +0x2d0 +0x2d4: the game's two cursors
};

class MudScore : public Vision::Graphic {            // 0x438a80
public:
    MudScore();
    ~MudScore() override;                            // 0x438e30
    void draw() override;                            // v14 0x4392d0
    void reset();                                    // 0x439170
    void addPlayer(int type);                        // 0x439370
    void addSkunk(int type);                         // 0x439520
    Vision::Animation bars;                          // +0x98 "MUDP_IF_interface_"
    std::vector<Vision::Animation *> player, skunk;  // +0x1b0 +0x1c0
    std::vector<Vision::Image *> pslots, sslots;     // +0x1d0 +0x1e0
    int pscore = 0, sscore = 0;                      // +0x1f0 +0x1f4
};

class Flyer;

class Thrower : public Vision::Graphic {             // 0x439930
public:
    Thrower(const char *stand, const char *thrw, int dx, int dy, int release);
    ~Thrower() override;                             // 0x439a70
    void draw() override;                            // v14 0x439b90
    void throwAt(Flyer *f);                          // 0x439b30
    VoList taunts;                                   // +0x98
    Vision::Animation stand, thrw;                   // +0xd0 +0x1e8
    int dx, dy;                                      // +0x300 +0x304: where the pie leaves the hand
    Flyer *pie = nullptr;                            // +0x308
    int release;                                     // +0x30c: the throw's frame that lets go
};

class Flyer : public Vision::Graphic {               // 0x439ca0
public:
    explicit Flyer(int type);
    ~Flyer() override;                               // 0x439dc0
    void draw() override;                            // v14 0x439e80
    bool hitAt(int x, int y);                        // 0x43a000
    void launch(float x, float y);                   // 0x439ec0
    Vision::Animation dissolve, thrown;              // +0xa0 +0x1b8
    uint32_t t0 = 0;                                 // +0x2d0
    float vx = 0, vy = 0, x0 = 0, y0 = 0;            // +0x2d4 +0x2d8 +0x2dc +0x2e0
    int type;                                        // +0x2e4: 0 mud, 1 diamond, 2 emerald, 3 ruby
};

class Flier : public Vision::Graphic {               // 0x43a2d0
public:
    explicit Flier(const char *who);
    ~Flier() override;                               // 0x43a5e0
    void draw() override;                            // v14 0x43adf0
    void load() override;                            // v15 0x43a6e0
    void unload() override;                          // v16 0x43a850
    void moveTo(int x, int y);                       // 0x43a8c0
    bool step();                                     // 0x43a9d0: true once landed
    void animate();                                  // 0x43ac10
    Vision::Animation *strAnim();                    // 0x43ab70
    Vision::Animation *flyAnim();                    // 0x43abc0
    Vision::Animation *current();                    // (inlined: by the phase)
    void reset();                                    // (inlined: target far away, landed, the clocks now)
    Vision::Animation *fly_no, *fly_so, *fly_ea, *fly_we;   // +0x98..+0xa4
    Vision::Animation *str_no, *str_so, *str_ea, *str_we;   // +0xa8..+0xb4: taking off / landing
    Vision::Animation *idle;                         // +0xb8
    int tx = -999999, ty = -999999;                  // +0xbc +0xc0
    int phase = 0;                                   // +0xc4: 0 idle, 1 taking off, 2 flying, 3 landing
    int dir = 0;                                     // +0xc8: 1 north, 2 south, 3 east, 4 west
    uint32_t anim_t = 0, step_t = 0;                 // +0xcc +0xd0
};

class Scoreboard : public Vision::Graphic {          // 0x4ada80
public:
    explicit Scoreboard(const char *bg);
    ~Scoreboard() override;                          // 0x4add80
    void draw() override;                            // v14 0x4adf70
    Vision::Image bg;                                // +0x98
    std::vector<Vision::Image *> digits, slots;      // +0x140 +0x150
    int s1 = 0, s2 = 0;                              // +0x160 +0x164
};

class MudPie_Scene : public Game2D_Scene {
public:
    MudPie_Scene();                                  // 0x436fb0
    ~MudPie_Scene() override;                        // 0x437390
    void notify2(Vision::Service *s) override;       // v9 0x436240
    void run() override;                             // v10 0x4366c0
    void load2() override;                           // v18 0x437680
    void enter() override;                           // v19 0x437a90
    void leave() override;                           // v20 0x4381e0
    void onMouse();                                  // 0x435e90
    void onKey();                                    // 0x436110
    void setup();                                    // 0x437cb0
    void start();                                    // 0x4362f0
    void result();                                   // 0x436610
    void spawn();                                    // 0x438490
    Flyer *nearest(int x, int y);                    // 0x4387b0
    void nextSpot(int *x, int *y);                   // 0x4388d0
    bool overlap(int x, int y);                      // 0x4389d0
    void targets();                                  // 0x437770
    void aiCatch();                                  // 0x43a050 (on the AI)
    void aiChoose();                                 // 0x43a160 (on the AI)

    bool escaped = false;                            // +0x15c: the skip key
    struct SoundSlot : Vision::Sound { SoundSlot() : Sound(nullptr) {} };   // 0x437290: Sound(0)
    SoundSlot sounds[7];                             // +0x160: throw, squish, gemcatch, success, dud, click y/n
    bool input_on = false;                           // +0x1858
    Vision::Mouse mouse;                             // +0x185c
    Vision::Keyboard keyboard;                       // +0x1890
    Vision::Ras *pack = nullptr;                     // +0x18c4 "mud_pie"
    MudCursor cursor;                                // +0x18c8
    MudScore *score = nullptr;                       // +0x1ba0
    MudGround ground;                                // +0x1ba8
    ContinueDialog *dialog = nullptr;                // +0x1e10
    Scoreboard *board = nullptr;                     // +0x1e14
    std::vector<Flyer *> flyers;                     // +0x1e18
    std::vector<Thrower *> throwers;                 // +0x1e28
    struct AI {                                      // +0x1e40 (the pointer at +0x1e38)
        float chance = 30.0f;                        // +0
        Flier skunk{"skn"};                          // +8
        int mode = 1;                                // +0xe0: 0 flying to a spot, 1 choosing
    } ai_;
    AI *ai = nullptr;                                // +0x1e38
    Cannon *cannon = nullptr;                        // +0x1f28
    uint32_t next_throw = 0;                         // +0x1f2c
    uint32_t last_click;                             // +0x1f30
    int secs = 0;                                    // +0x1f38
    float speed = 0;                                 // +0x1f3c
    int gems = 0;                                    // +0x1f40: of the ten pies, how many are gems
    std::vector<int *> spots;                        // +0x1f44: (x, y) pairs
    int spot = 0;                                    // +0x1f54
    Vision::Sound music;                             // +0x1f58 "MUDP_MU_music"
    PeterGuide guide;                                // +0x22a0
    Inactivity inactivity;                           // +0x2920
    GameEndVo endvo;                                 // +0x2cc0
    bool playing = false;                            // +0x2e30
};

MudPie_Scene &mudpie_scene();                        // the static at 0x5a96a0

}  // namespace Game
