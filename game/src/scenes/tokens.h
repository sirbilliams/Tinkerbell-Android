// The areas' puzzle-piece display and the character "hot regions", reconstructed from "Tinker Bell.exe":
//   TokenInterface (vtable 0x517c88, 0x168 bytes, ctor 0x4f0960, dtor 0x4f0c00, load v15 0x4f0d10, unload v16
//     0x4f0e10, draw v14 0x4f0e60, add 0x4f0ec0, show 0x4f1040): a backdrop and up to five coloured pieces, drawn
//     once any piece is won.
//   HotRegion (vtable 0x5170e4, 0xf4 bytes, ctor 0x4c3b30, dtor 0x4c3be0, run v10 0x4c4200, won 0x4c3bf0): coming
//     within 600 of a game's sign and staying 1.5 s, its character calls out to play.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../raytrace/raytrace.h"
#include <string>

namespace Game {

class TokenInterface : public Vision::Graphic {
public:
    explicit TokenInterface(const char *prefix);     // 0x4f0960
    ~TokenInterface() override;                      // 0x4f0c00
    void add(const char *name, const char *prefix);  // 0x4f0ec0
    void show(const char *name);                     // 0x4f1040: that piece won
    void load() override;                            // v15 0x4f0d10
    void unload() override;                          // v16 0x4f0e10
    void draw() override;                            // v14 0x4f0e60
    Vision::Animation *back = nullptr;               // +0x98
    std::string prefix;                              // +0x9c
    Vision::Animation *pieces[5] = {};               // +0xac
    std::string names[5], prefixes[5];               // +0xc0 +0x110
    int count = 0;                                   // +0x160
};

class HotRegion : public Vision::Task {
public:
    // 0x4c3b30: around the sprite's spot; `game`'s "Info" picks the line: 0 never played (a puzzle game with all
    // five pieces won: the second), 1 won, 2 played
    HotRegion(Raytrace::RayTrace *rt, const char *game, Raytrace::Sprite3D *spot, const char *never,
              const char *never_done, const char *won, const char *played);
    void run() override;                             // v10 0x4c4200
    int won();                                       // 0x4c3bf0: the saved pieces won in a puzzle game's area
    bool inside();                                   // (the run's repeated test; the distance kept in +0xd8)
    bool on = false;                                 // +0xc0
    Raytrace::RayTrace *rt;                          // +0xc4
    float last = 999999.9f;                          // +0xc8 (0x497423fe)
    float x, z;                                      // +0xcc +0xd0
    float radius = 600.0f;                           // +0xd4
    float dist = 0;                                  // +0xd8
    const char *game;                                // +0xdc
    const char *lines[4];                            // +0xe0..+0xec
    int ticks = 0;                                   // +0xf0
};

}  // namespace Game
