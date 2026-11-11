// The pick-a-game room (scene "pickagame", the static at 0x62a5b0, vtable 0x5166c8, ctor 0x4ae6a0, dtor 0x4ae7f0),
// reconstructed from "Tinker Bell.exe" (namespace PickAGame in its RTTI). The fifteen games' buttons: one goes to its
// area, set to that game's spot; the return button goes back to the map. Opened from the map with the treasure.
//   notify2 v9 0x4ae900 (mouse 0x4af280), load2 v18 0x4ae950, enter v19 0x4ae970, leave v20 0x4af070
//   GameButton       vtable 0x516770 (ctor 0x4af450, dtor 0x4af580, load 0x4af650, unload 0x4af6c0, draw 0x4af720,
//                    hit 0x4afa60)
//   ReturnButton     (ctor 0x4af800, dtor 0x4af940, draw 0x4afa40)
//   PickAGameCursor  vtable 0x516724 (draw 0x4af7d0): draws the game's cursor it points to
#pragma once
#include "acts.h"
#include "../vision/engine.h"
#include "../vision/sound.h"
#include <vector>

namespace Game {

struct GameDesc {                                    // the statics 0x62a448.. (set by 0x4ae270..0x4ae5f0)
    const char *idle, *rov, *line, *scene;
    int area, game;
};

class GameButton : public Vision::Graphic {          // 0x310 bytes
public:
    explicit GameButton(const GameDesc &d);          // 0x4af450
    ~GameButton() override;                          // 0x4af580
    bool hit(int x, int y) override;                 // v11 0x4afa60
    void draw() override;                            // v14 0x4af720
    void load() override;                            // v15 0x4af650
    void unload() override;                          // v16 0x4af6c0
    Vision::Image idle, rov;                         // +0x98 +0x140
    bool lit = false;                                // +0x1e8
    const char *scene;                               // +0x1ec
    bool opened = false;                             // +0x1f0
    int area, game;                                  // +0x1f4 +0x1f8
    PauseOver pause;                                 // +0x1fc
};

class ReturnButton : public Vision::Graphic {        // 0x598 bytes
public:
    ReturnButton();                                  // 0x4af800
    ~ReturnButton() override;                        // 0x4af940
    bool hit(int x, int y) override;                 // v11 0x4afa60
    void draw() override;                            // v14 0x4afa40: lit only
    Vision::Image img;                               // +0x98 "GLOB_BU_return_01" (its visible: lit)
    Vision::Sound click;                             // +0x140 "GLOB_FX_butclick"
    PauseOver pause;
};

class PickAGameCursor : public Vision::Graphic {
public:
    PickAGameCursor();
    void draw() override;                            // v14 0x4af7d0
    Vision::Graphic *cur = nullptr;                  // +0x98
};

class PickAGame_Scene : public Vision::Scene {
public:
    PickAGame_Scene();                               // 0x4ae6a0
    ~PickAGame_Scene() override;                     // 0x4ae7f0
    void notify2(Vision::Service *s) override;       // v9 0x4ae900
    void load2() override;                           // v18 0x4ae950
    void enter() override;                           // v19 0x4ae970
    void leave() override;                           // v20 0x4af070
    void onMouse();                                  // 0x4af280
    Vision::Screen screen;                           // +0xcc
    Vision::Mouse mouse;                             // +0x148
    Vision::Keyboard keyboard;                       // +0x17c
    Vision::Ras *pack = nullptr;                     // +0x1b0
    Vision::Image bg;                                // +0x1b8 "GLOB_BG_pickgame_01"
    std::vector<GameButton *> buttons;               // +0x260
    ReturnButton *ret = nullptr;                     // +0x270
    PickAGameCursor cursor;                          // +0x278
    Vision::Sound *music = nullptr;                  // +0x318
};

PickAGame_Scene &pickagame_scene();                  // the static at 0x62a5b0

}  // namespace Game
