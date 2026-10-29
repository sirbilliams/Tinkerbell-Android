// Game "acts" the sign-in room needs, reconstructed from "Tinker Bell.exe" (the voice-over acts are in voacts.h):
//   FadeRectangle  vtable 0x517930: a full-screen fade over 2 s   ctor 0x4e6100, start 0x4e61f0, draw 0x4e6260
//   Inactivity     vtable 0x513fd4 (Act): after `interval` ms without a click or key, plays the next of its lines
//                  ctor 0x4c4590, add 0x4c46e0, interval 0x4c4850, start 0x4c15f0 (shared with DelayedVO),
//                  stop 0x4c4690, notify2 0x4c4860, run 0x4c48c0, isPlaying 0x4c4aa0
//   PauseOver      vtable 0x5160a8 (Act): the mouse resting on a graphic for `delay` ms plays a line once
//                  ctor 0x4c58e0, set 0x4c5970, start 0x4c59c0, stop 0x4c59f0, run 0x4c5a60
#pragma once
#include "../vision/act.h"
#include "../vision/faderect.h"
#include <string>
#include <vector>

namespace Game {
using namespace Vision;

class FadeRectangle : public Graphic {
public:
    FadeRectangle();                                 // 0x4e6100
    void start();                                    // 0x4e61f0: from black, over 2 s
    void fadeOut();                                  // 0x4e6220: to black, over 2 s
    void draw() override;                            // 0x4e6260
    int mode = 0;                                    // +0x98: 0 fading in, 1 fading out, 2 done
    FadeRect rect;                                   // +0xa0 (640x480)
    uint32_t start_time = 0;                         // +0x148
};

class Inactivity : public Act {
public:
    Inactivity();                                    // 0x4c4590
    void add(const char *line, bool flag);           // 0x4c46e0
    void setInterval(uint32_t ms) { interval = ms; } // 0x4c4850
    void start() override;                           // +0x30 0x4c15f0
    void stop() override;                            // +0x34 0x4c4690
    void notify2(Service *s) override;               // +0x24 0x4c4860: input restarts the wait; stops its line
    void run() override;                             // 0x4c48c0
    bool isPlaying();                                // 0x4c4aa0: its last line is still playing
    uint32_t last = 0;                               // +0xc0
    Mouse mouse;                                     // +0xc4
    Keyboard kbd;                                    // +0xf8
    std::vector<std::string> lines;                  // +0x150 (a list in the exe)
    int index = 0;                                   // +0x15c
    uint32_t interval = 30000;                       // +0x160
    bool once = false;                               // +0x164: stop after playing one line
    Act *instruction = nullptr;                      // +0x168: started for an "I" line (port: no scene ported so far has one)
    std::string current;                             // +0x394
};

class PauseOver : public Act {
public:
    PauseOver();                                     // 0x4c58e0
    void set(Graphic *owner, const char *line, uint32_t delay);   // 0x4c5970
    void start() override;                           // +0x30 0x4c59c0
    void stop() override;                            // +0x34 0x4c59f0
    void run() override;                             // 0x4c5a60
    void cancel();                                   // 0x4c5be0: its line stopped if playing; not again this visit
    Graphic *owner = nullptr;                        // +0xc0
    std::string line;                                // +0xc4
    Mouse mouse;                                     // +0xd0
    uint32_t over_since = 0;                         // +0x104
    uint32_t delay = 0;                              // +0x108
    int played = 0;                                  // +0x10c
};

}  // namespace Game
