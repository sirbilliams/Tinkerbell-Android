// Cannon (vtable 0x517898, ctor 0x4e3550, at the game state +0x7c0), reconstructed from "Tinker Bell.exe": the
// timer graphic of the 3D games: a cannon whose fuse burns down one piece per 3 seconds, a spark running along
// it, and a puff of smoke with a bang when the time is up.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"

namespace Game {

class Cannon : public Vision::Graphic {
public:
    Cannon();                                        // 0x4e3550
    void load() override;                            // v15 0x4e3780
    void unload() override;                          // v16 0x4e3830
    void draw() override;                            // v14 0x4e38a0
    void start(int secs);                            // 0x4e3a50: the fuse lit for `secs` seconds
    void setTime(int secs);                          // 0x4e3ae0: the length shown, not lit
    void restart();                                  // 0x4e3b30
    bool running() const { return lit; }             // 0x4e3b80
    bool timeLeft();                                 // 0x4e3b90
    void stop();                                     // 0x4e3bc0
    void hide();                                     // 0x4e3bf0
    int remaining();                                 // 0x4e3c20 (ms)
    Vision::Image cannon, bar;                       // +0x98 +0x140
    Vision::Animation smoke, fuse, spark;            // +0x1e8 +0x308 +0x420
    uint32_t smoke_last = 0, spark_last = 0;         // +0x300 +0x538
    Vision::Sound fuse_snd, blast;                   // +0x53c +0x884
    bool lit = false;                                // +0xbcc
    uint32_t t0 = 0;                                 // +0xbd0
    int secs = 0;                                    // +0xbd4
    bool blasted = false;                            // +0xbd8
    int pieces = 0;                                  // +0xbdc: the fuse pieces still shown
};

Cannon &cannon();                                    // the game state's (+0x7c0)

}  // namespace Game
