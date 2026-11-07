// The 2D game scenes' base (the mini games: mud pie...), reconstructed from "Tinker Bell.exe":
//   Game2D_Scene  vtable 0x5165d0, ctor 0x4acf40, dtor 0x4acfb0 (0x437240), enter v19 0x4acfd0, leave v20 0x4ad270
//   CloudBank     vtables 0x516538 (draw 0x4acdc0, load 0x4acd70, unload 0x4acda0, dtor 0x4accf0) and 0x516584
//                 (load 0x4ace90, dtor 0x4ace10): "glob_ly_cloudbank_01" sliding down over the screen or away
//                 (0x4acec0 / 0x4acee0, 20 px a call), 0x4acf00 resets it
//   PlayerBadge   vtable 0x51662c, ctor 0x4ad330, dtor 0x4ad910, draw v14 0x4ada10: the player's frame, location,
//                 character and a head icon (the toolbar's corner)
// The scene keeps a screen (+0xcc) with the map icon, the cloud bank and the badge, and an overlay screen (+0x158,
// depth -10000) with the fader. Entering also loads the "global_2d" pack and its number images (State 0x4eca30).
#pragma once
#include "../vision/engine.h"
#include <vector>

// The Graphic base as the 2D game classes construct it (0x411b90: hidden mask -1, the colour key 0xffff00ff).
#define GRAPHIC_ARGS 0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu

namespace Game {

inline int ftol(double v) { return (int)v; }          // 0x4f3948 (truncation)

class NeverlandMapIcon;
class SceneFader;

class CloudBank : public Vision::Graphic {
public:
    CloudBank();                                     // (inlined in the enter)
    ~CloudBank() override;                           // 0x4ace10
    void draw() override;                            // v14 0x4acdc0
    void load() override;                            // v15 0x4ace90 (0x516538's 0x4acd70: hidden, y -1000)
    void unload() override;                          // v16 0x4acda0
    bool rise();                                     // 0x4acec0: up 20 while y <= -50 (else mode 2, false)
    bool fall();                                     // 0x4acee0: down 20 while y <= 1000 (else mode 2, false)
    void reset();                                    // 0x4acf00
    void cover();                                    // 0x4acf20: above the screen (y -1000), shown, mode 0
    Vision::Image image;                             // +0x98
    bool shown = false;                              // +0x140
    int mode = 2;                                    // +0x148
};

class PlayerBadge : public Vision::Graphic {
public:
    explicit PlayerBadge(const char *head);          // 0x4ad330 (null: "GLOB_IF_PETheadsm_01")
    ~PlayerBadge() override;                         // 0x4ad910
    void draw() override;                            // v14 0x4ada10
    Vision::Image *frame = nullptr;                  // +0x98 (by "Frame Color")
    Vision::Image *location = nullptr;               // +0x9c (by "Location")
    Vision::Image *character = nullptr;              // +0xa0 (by "Character")
    Vision::Image *head = nullptr;                   // +0xa4
};

// ScrollGround (vtable 0x516450, ctor 0x4ac0e0, load 0x4ac2c0, unload 0x4ac350, update v18 0x4ac3e0, draw 0x4ac550):
// two ground images scrolling down (the up and down keys set the speed); left and right slide it.
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

// Scoreboard (vtable 0x516678, ctor 0x4ada80, 0x168 bytes): a game's two final scores.
class Scoreboard : public Vision::Graphic {          // 0x4ada80
public:
    explicit Scoreboard(const char *bg);
    ~Scoreboard() override;                          // 0x4add80
    void draw() override;                            // v14 0x4adf70
    Vision::Image bg;                                // +0x98
    std::vector<Vision::Image *> digits, slots;      // +0x140 +0x150
    int s1 = 0, s2 = 0;                              // +0x160 +0x164
};

class Game2D_Scene : public Vision::Scene {
public:
    explicit Game2D_Scene(const char *name);         // 0x4acf40
    ~Game2D_Scene() override;
    void enter() override;                           // v19 0x4acfd0
    void leave() override;                           // v20 0x4ad270
    Vision::Screen screen;                           // +0xcc
    NeverlandMapIcon *map = nullptr;                 // +0x148
    CloudBank *clouds = nullptr;                     // +0x14c
    PlayerBadge *badge = nullptr;                    // +0x150
    SceneFader *fader = nullptr;                     // +0x154
    Vision::Screen *overlay = nullptr;               // +0x158
};

}  // namespace Game
