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

namespace Game {

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
