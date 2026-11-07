// The Coconuts Away game (scene "coconut", the static at 0x5b70f0, vtable 0x514554, ctor 0x449410, dtor 0x4497a0),
// reconstructed from "Tinker Bell.exe". In the mermaids' lagoon, seen from above: pirate rowboats come down the
// water; the player clicks to drop coconuts on them, the rabbit flies about doing the same; a coconut lands after
// two seconds (a splash, or the boat sinks). The cannon's fuse is the clock; the higher score wins.
//   notify2 v9 0x4487a0 (mouse 0x448340, keys 0x448670), run v10 0x448a20, load2 v18 0x449a80, enter v19 0x449b70,
//   leave v20 0x44a060, setup 0x448850, result 0x449340
// Its pieces:
//   CocoGround   vtable 0x514648 (update v18 0x44a4f0): the water; moves the coconuts and the boats with it
//   CocoCursor   vtable 0x5145fc (load 0x44a630, unload 0x44a6a0, draw 0x44a6e0)
//   CocoScore    vtable 0x514698 (ctor 0x44a1d0, 0x280 bytes, draw 0x44a4b0): the bar with the two counts
//   NumberShow   (draw 0x44cdd0, load 0x44cd90): a count drawn with the state's number images
//   Rabbit       vtable 0x514730 (ctor 0x44a710, load 0x44abb0, unload 0x44ad30, draw 0x44b5f0): flies to a boat
//                (aim 0x44adb0) or a random spot (choose 0x44af00), lands (step 0x44b090, animate 0x44b240) and
//                throws
//   CocoItem     (ctor 0x44c700, 0x240 bytes, load 0x44c770, add 0x44c850, update 0x44c890): a coconut and its
//                splash; CocoGroup (+0x1528 the player's, +0x1540 the rabbit's; move 0x44cb70, dtor 0x44c9e0)
//   Boat         vtable 0x514784 (ctor 0x44b7a0, 0xe0 bytes, dtor 0x44bd70, hit 0x44bf80, update 0x44c030, reset
//                0x44c1a0, draw 0x44bf70); BoatGroup (ctor 0x44c370, dtor 0x44c4c0, target 0x44c580)
//   SoundCycle   (ctor 0x4cc220, add 0x4cc240, openAll 0x4cc450, closeAll 0x4cc490, playNext 0x4cc4f0)
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

// A list of plain sounds played one after another (0x14 bytes).
class SoundCycle {
public:
    void add(const char *name) { sounds.push_back(new Vision::Sound(name)); }   // 0x4cc240
    void openAll();                                  // 0x4cc450
    void closeAll();                                 // 0x4cc490
    void playNext();                                 // 0x4cc4f0
    std::vector<Vision::Sound *> sounds;             // +4
    int index = -1;                                  // +0x10
};

class CocoGround : public ScrollGround {             // vtable 0x514648
public:
    CocoGround() : ScrollGround("COCO_BG_ground1_01", "COCO_BG_ground2_01") {}
    void update() override;                          // v18 0x44a4f0
};

class CocoCursor : public Vision::Graphic {          // vtable 0x5145fc
public:
    CocoCursor();
    void draw() override;                            // v14 0x44a6e0
    void load() override;                            // v15 0x44a630
    void unload() override;                          // v16 0x44a6a0
    Vision::Animation active, notactive;             // +0x98 +0x1b0
    Vision::Graphic *hand = nullptr, *normal = nullptr;   // +0x2c8 +0x2cc
    Vision::Graphic *cur = nullptr;                  // +0x2d0
};

class NumberShow : public Vision::Graphic {
public:
    NumberShow() : Graphic(GRAPHIC_ARGS) {}
    void draw() override;                            // v14 0x44cdd0
    void load() override;                            // v15 0x44cd90
    int value = 0;                                   // +0x98
    bool small = false;                              // +0x9c
};

class CocoScore : public Vision::Graphic {           // 0x44a1d0
public:
    CocoScore();
    ~CocoScore() override;                           // 0x44a3a0
    void draw() override;                            // v14 0x44a4b0
    Vision::Image bar;                               // +0x98 "COCO_IF_interface_01"
    NumberShow player, rabbit;                       // +0x140 +0x1e0 (their values at +0x1d8 +0x278)
};

class Boat : public Vision::Graphic {                // 0x44b7a0
public:
    explicit Boat(int lane);
    ~Boat() override;                                // 0x44bd70
    void draw() override { cur->draw(); }            // v14 0x44bf70
    bool hitAt(int x, int y);                        // 0x44bf80: sinks when hit
    void update(float dx, float dy);                 // 0x44c030
    void reset();                                    // 0x44c1a0: a new boat above the screen in its lane
    bool rowing() const;                             // (inlined: cur is one of the rowing animations)
    float fy = 0;                                    // +0x98
    std::vector<Vision::Animation *> row, sink, water;   // +0x9c +0xac +0xbc
    Vision::Animation *cur = nullptr;                // +0xcc
    int lane;                                        // +0xd0
    float speed = 0;                                 // +0xd4
};

class BoatGroup {                                    // 0x44c370 (0x1c bytes)
public:
    BoatGroup();
    ~BoatGroup();                                    // 0x44c4c0
    Boat *target();                                  // 0x44c580: a rowing boat on screen, at random
    std::vector<Boat *> boats;                       // +0
    uint32_t next_yell;                              // +0x14
    bool running = false;                            // +0x18
};

class CocoItem {                                     // 0x44c700 (0x240 bytes)
public:
    CocoItem();
    void load();                                     // 0x44c770
    void addTo(Vision::Screen *s);                   // 0x44c850
    void update(float dx, float dy, int kind);       // 0x44c890
    bool hit = false;                                // +0: it sank a boat
    uint32_t t0 = 0;                                 // +4
    Vision::Animation coconut, splash;               // +8 +0x120
    Vision::Animation *cur = nullptr;                // +0x238
    bool active = false;                             // +0x23c
};

struct CocoGroup {                                   // +0x1528 / +0x1540 in the scene
    uint32_t t = 0;                                  // +0: the last throw
    int kind;                                        // +4: 1 the player's, 2 the rabbit's
    std::vector<CocoItem *> items;                   // +8
    void move(float dx, float dy);                   // 0x44cb70
};

class Rabbit : public Vision::Graphic {              // 0x44a710
public:
    Rabbit(int chance, const char *who);
    ~Rabbit() override;                              // 0x44aa90
    void draw() override;                            // v14 0x44b5f0
    void load() override;                            // v15 0x44abb0
    void unload() override;                          // v16 0x44ad30
    void aim();                                      // 0x44adb0
    void choose();                                   // 0x44af00
    bool step();                                     // 0x44b090
    void animate();                                  // 0x44b240
    Vision::Animation *strAnim();
    Vision::Animation *flyAnim();
    Vision::Animation *current();
    Vision::Animation *fly_no, *fly_so, *fly_ea, *fly_we;   // +0x98..+0xa4
    Vision::Animation *str_no, *str_so, *str_ea, *str_we;   // +0xa8..+0xb4
    Vision::Animation *idle, *thrw;                  // +0xb8 +0xbc
    int tx = 0, ty = 0;                              // +0xc4 +0xc8
    int phase = 0;                                   // +0xcc: 0 idle, 1 off, 2 flying, 3 landing, 4 choosing, 5 throwing, 6 done
    int dir = 0;                                     // +0xd0
    uint32_t anim_t = 0, step_t = 0;                 // +0xd4 +0xd8
    int chance;                                      // +0xdc: of a hundred, to go after a boat
    bool throws = false;                             // +0xe0
};

class Coconut_Scene : public Game2D_Scene {
public:
    Coconut_Scene();                                 // 0x449410
    ~Coconut_Scene() override;                       // 0x4497a0
    void notify2(Vision::Service *s) override;       // v9 0x4487a0
    void run() override;                             // v10 0x448a20
    void load2() override;                           // v18 0x449a80
    void enter() override;                           // v19 0x449b70
    void leave() override;                           // v20 0x44a060
    void onMouse();                                  // 0x448340
    void onKey();                                    // 0x448670
    void setup();                                    // 0x448850
    void result();                                   // 0x449340

    struct SoundSlot : Vision::Sound { SoundSlot() : Sound(nullptr) {} };
    SoundSlot sounds[4];                             // +0x15c: cocofall, throw, boathit, miss
    bool input_on = false;                           // +0xe7c
    Vision::Mouse mouse;                             // +0xe80
    Vision::Keyboard keyboard;                       // +0xeb4
    ContinueDialog *dialog = nullptr;                // +0xee8
    CocoGround ground;                               // +0xef0
    CocoCursor cursor;                               // +0x1158
    CocoScore *score = nullptr;                      // +0x1430
    Rabbit rabbit;                                   // +0x1438
    Rabbit *rab = nullptr;                           // +0x1520
    BoatGroup *boats = nullptr;                      // +0x1524
    CocoGroup mine, its;                             // +0x1528 +0x1540
    Scoreboard *board = nullptr;                     // +0x1558
    Vision::Ras *pack = nullptr;                     // +0x155c "coconuts_away"
    int secs = 0;                                    // +0x1560
    bool escaped = false;                            // +0x1564
    Vision::Sound music;                             // +0x1568
    Cannon *cannon = nullptr;                        // +0x18b0
    PeterGuide guide;                                // +0x18b8
    Inactivity inactivity;                           // +0x1f38
    GameEndVo endvo;                                 // +0x22d8
    bool playing = false;                            // +0x2448
};

Coconut_Scene &coconut_scene();                      // the static at 0x5b70f0

}  // namespace Game
