// Shadow Boxing, the skunk's game at Skull Rock (played in the scene itself; the game is the scene's +0x43c0),
// reconstructed from "Tinker Bell.exe". Five animals (a duck, a bunny, a turtle, a monkey, a squirrel) pop up from
// behind five rocks in the lagoon and duck again; a click with the net on one catches it (its icon on the bar).
// Eight catches before the fuse burns down win. Now and then the pirates row in; dust on the crocodile scares them
// off (the crocodile snaps), else they leave by themselves after 30 s. While they are there the animals stay down.
//   ShadowCursor   (vtable 0x515458, 0xb8 bytes; ctor 0x480f10, dtor 0x480fa0, load v15 0x481060, unload v16
//                   0x481150, draw v14 0x4811a0, miss 0x481230, setActive 0x481280): the net
//   ShadowAnimal   (0x58 bytes, no vtable; init 0x4812d0, detach 0x481310, makePrefix 0x481330, setup 0x481420,
//                   step 0x4816b0)
//   ShadowGroup    (0x2d4 bytes; ctor inlined in 0x482600, dtor 0x481810, add 0x481870, update 0x4818b0)
//   ShadowPirates  (0x13e4 bytes; teardown 0x481b30, start 0x481bd0, anim 0x481ca0, update 0x481d50)
//   ShadowGator    (0x9ec bytes; dtor 0x4821d0, make 0x4821f0, eyes 0x482300)
//   ShadowInterface (vtable 0x5154b0, 0xf8 bytes; ctor 0x483740, dtor 0x483810, load v15 0x4838a0, unload v16
//                   0x483b20, draw v14 0x483c50): the catches on a bar
//   ShadowBoxing   (0x378 bytes, plain new; ctor 0x482360, dtor 0x482590, setup 0x482600, teardown 0x482830,
//                   gatorDust 0x482910, count 0x482950, start 0x482960, update 0x482ab0, playing 0x482c20,
//                   click 0x482fa0, wantNet 0x4832b0, hint 0x4832f0, hover 0x483590)
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"
#include <vector>

namespace Game {

class Cannon;

// (vtable 0x515458) the net cursor: inactive, or active over an animal that is up.
class ShadowCursor : public Vision::Graphic {
public:
    ShadowCursor();                                  // 0x480f10
    ~ShadowCursor() override;                        // 0x480fa0
    void load() override;                            // v15 0x481060
    void unload() override;                          // v16 0x481150
    void draw() override;                            // v14 0x4811a0
    void miss();                                     // 0x481230: a click with the net (ten off an animal: help)
    void setActive(bool b);                          // 0x481280
    Vision::Animation *inactive = nullptr;           // +0x98 "SHBX_CU_netinactiv_"
    Vision::Animation *active_ = nullptr;            // +0x9c "SHBX_CU_netactiv_"
    bool loaded_ = false;                            // +0xa4
    uint32_t last = 0;                               // +0xa8
    int misses = 0;                                  // +0xac
    int state = 0;                                   // +0xb0: 0 inactive, 1 active (2: never set)
};

// An animal behind its rock (0x58 bytes).
struct ShadowAnimal {
    ShadowAnimal();                                  // 0x4812d0
    void detach();                                   // 0x481310: its attachment off the rock
    void makePrefix();                               // 0x481330: "shbx_ca_<ABB><phase>_" into buf
    void setup(int rock_no, int type, Raytrace::RayTrace *rt);   // 0x481420
    void step();                                     // 0x4816b0
    int type = 0;                                    // +0x00: 1 bunny, 2 monkey, 3 turtle, 4 squirrel, 5 duck
    Raytrace::Sprite3D *rock = nullptr;              // +0x04 "AA_shbx_am_rock<n>_01"
    Raytrace::Attachment *att = nullptr;             // +0x08 "<animal><nn>"
    char buf[0x28] = {};                             // +0x0c: the last prefix made
    bool up = false;                                 // +0x34
    uint32_t last = 0;                               // +0x38: the last frame step
    uint32_t period = 50;                            // +0x3c
    int phase = 0;                                   // +0x40: 1 emerging, 2 idle, 3 hiding
    int rock_no = 0;                                 // +0x44
    uint32_t t0 = 0;                                 // +0x48: when it last hid (or came up)
    Raytrace::RayTrace *rt = nullptr;                // +0x4c
    float dist = 0;                                  // +0x50: the rock's floor distance (every second; never read)
    uint32_t dist_t = 0;                             // +0x54
};

struct ShadowGroup {
    explicit ShadowGroup(Raytrace::RayTrace *r);     // (inlined in 0x482600)
    ~ShadowGroup();                                  // 0x481810
    void add(int rock_no, int type);                 // 0x481870
    void update();                                   // 0x4818b0
    ShadowAnimal animals[8];                         // +0x000
    Raytrace::RayTrace *rt;                          // +0x2c0
    uint32_t last;                                   // +0x2c4
    uint32_t interval = 50;                          // +0x2c8
    int count = 0;                                   // +0x2d0
};

struct ShadowPirates {
    explicit ShadowPirates(Raytrace::RayTrace *r);   // (inlined in 0x482600)
    ~ShadowPirates();                                // 0x481b30 (the teardown, then the sounds)
    void teardown();                                 // (0x481b30's head, inlined in 0x482830)
    void start();                                    // 0x481bd0: the boat made, hidden
    void anim();                                     // 0x481ca0: the state's animation
    void update();                                   // 0x481d50
    bool on = false;                                 // +0x00
    bool made = false;                               // +0x01
    int state = 0;                                   // +0x04: 1 rowing in, 2 idle, 3 scream, 4 rowing out
    uint32_t frame_t = 0;                            // +0x0c
    uint32_t last = 0;                               // +0x10
    uint32_t t_on = 0;                               // +0x14
    Raytrace::RayTrace *rt;                          // +0x1c
    Raytrace::Sprite3D *spr = nullptr;               // +0x20 "pirates"
    float tx = 0, tz = 0;                            // +0x24 +0x28
    Vision::Sound *snd[6] = {};                      // +0x2c (embedded, 0x348 each)
    int f6b4 = 0;                                    // +0x6b4 (inside snd[1]: the exe's write, 7)
    uint32_t next = 0;                               // +0x13dc: the next line
    int visits = 0;                                  // +0x13e0
};

struct ShadowGator {
    explicit ShadowGator(Raytrace::RayTrace *r);     // (inlined in 0x482600)
    ~ShadowGator();                                  // 0x4821d0
    void make();                                     // 0x4821f0
    void eyes();                                     // 0x482300: back to the eyes once an animation ends
    void step();                                     // (the jump tables 0x482c08 / 0x482f88)
    Raytrace::RayTrace *rt;                          // +0x00
    Raytrace::Sprite3D *base = nullptr;              // +0x04 "gator_base"
    Raytrace::Attachment *att = nullptr;             // +0x08 "gator"
    bool made = false;                               // +0x0c
    int state = 0;                                   // +0x10: 0 eyes, 1 fly up, 2 snap, 3 fly down
    Vision::Sound *snd[3] = {};                      // +0x14 (embedded, 0x348 each)
};

// (vtable 0x5154b0) the bar and each catch's icon at its box.
class ShadowInterface : public Vision::Graphic {
public:
    ShadowInterface();                               // 0x483740
    ~ShadowInterface() override;                     // 0x483810
    void load() override;                            // v15 0x4838a0
    void unload() override;                          // v16 0x483b20
    void draw() override;                            // v14 0x483c50
    Vision::Image *boxes[8] = {};                    // +0x98 "SHBX_IF_boxtar<n>_01" (not drawn: their spots)
    Vision::Image *duc = nullptr, *mon = nullptr, *rab = nullptr, *squ = nullptr, *tur = nullptr;   // +0xb8..+0xc8
    Vision::Image *bar = nullptr;                    // +0xcc "SHBX_IF_interfacebar_01"
    int count = 0;                                   // +0xd0: catches
    int types[8] = {};                               // +0xd4
};

class ShadowBoxing {
public:
    ShadowBoxing(Raytrace::RayTrace *rt, Vision::Screen *over);   // 0x482360
    ~ShadowBoxing();                                 // 0x482590
    void setup();                                    // 0x482600 (enter)
    void teardown();                                 // 0x482830 (leave)
    void gatorDust();                                // 0x482910: dust on the crocodile: it flies up
    int count();                                     // 0x482950: catches
    void start();                                    // 0x482960
    void update();                                   // 0x482ab0: every frame of the scene's run
    void playing();                                  // 0x482c20
    void click(Raytrace::Attachment *a);             // 0x482fa0
    bool wantNet(Raytrace::Attachment *a);           // 0x4832b0: an animal up there
    void hint();                                     // 0x4832f0
    void hover();                                    // 0x483590: the eye to the nearest rock's height
    int on = 0;                                      // +0x00
    Raytrace::RayTrace *rt;                          // +0x04
    Vision::Screen *over;                            // +0x08
    ShadowGroup *group = nullptr;                    // +0x0c
    ShadowPirates *pirates = nullptr;                // +0x10
    ShadowGator *gator = nullptr;                    // +0x14
    ShadowInterface *iface = nullptr;                // +0x18
    Cannon *fuse = nullptr;                          // +0x1c (the state's +0x7c0)
    std::vector<const char *> nice;                  // +0x20
    Vision::Sound catch_snd;                         // +0x30 (never named: silent)
};

extern int shadow_max_visits;                        // 0x5dac80
extern int shadow_k;                                 // 0x53d380 (set by start; never read)

}  // namespace Game
