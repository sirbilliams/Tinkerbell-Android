// Game "acts" the sign-in room needs, reconstructed from "Tinker Bell.exe" (the voice-over acts are in voacts.h):
//   FadeRectangle  vtable 0x517930: a full-screen fade over 2 s   ctor 0x4e6100, start 0x4e61f0, draw 0x4e6260
//   Inactivity     vtable 0x513fd4 (Act): after `interval` ms without a click or key, plays the next of its lines
//                  ctor 0x4c4590, add 0x4c46e0, interval 0x4c4850, start 0x4c15f0 (shared with DelayedVO),
//                  stop 0x4c4690, notify2 0x4c4860, run 0x4c48c0, isPlaying 0x4c4aa0
//   PauseOver      vtable 0x5160a8 (Act): the mouse resting on a graphic for `delay` ms plays a line once
//                  ctor 0x4c58e0, set 0x4c5970, start 0x4c59c0, stop 0x4c59f0, run 0x4c5a60
//   RepeatLine     vtable 0x517118 (Act): every `ms` of silence its line   ctor 0x4c4af0, start 0x4c4b80, run 0x4c4ba0
//   PromptLine     vtable 0x517154 (Act): the same, a click restarts the wait   ctor 0x4c4c70, start 0x4c4d20,
//                  stop 0x4c4d50, run 0x4c4d70, notify2 0x4c4e40, isPlaying 0x4c4e60
//   ComeNear       vtable 0x517044 (Act): until the camera comes near (z <= 4184) its two lines in turn every 10 s
//                  ctor 0x4c1360, run 0x4c1410
#pragma once
#include "../vision/act.h"
#include "../vision/faderect.h"
#include "../raytrace/raytrace.h"
#include "voacts.h"
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

class RepeatLine : public Act {
public:
    RepeatLine(const char *line, uint32_t ms);       // 0x4c4af0
    void start() override;                           // +0x30 0x4c4b80
    void stop() override { deactivate(); }           // +0x34 0x4c1400
    void run() override;                             // 0x4c4ba0
    uint32_t next = 0;                               // +0xc0
    uint32_t ms;                                     // +0xc4
    std::string line;                                // +0xc8
};

class PromptLine : public Act {
public:
    PromptLine(const char *line, uint32_t ms);       // 0x4c4c70
    void start() override;                           // +0x30 0x4c4d20
    void stop() override;                            // +0x34 0x4c4d50
    void run() override;                             // 0x4c4d70 (as 0x4c4ba0)
    void notify2(Service *s) override;               // +0x24 0x4c4e40
    bool isPlaying();                                // 0x4c4e60
    uint32_t next = 0;                               // +0xc0
    uint32_t ms;                                     // +0xc4
    std::string line;                                // +0xc8
    Mouse mouse;                                     // +0xd4
};

class ComeNear : public Act {
public:
    ComeNear(Raytrace::RayTrace *rt, const char *l1, const char *l2);   // 0x4c1360
    void start() override { reset(); activate(); }   // (0x4c19e0, the shared start)
    void stop() override { deactivate(); }           // +0x34 0x4c1400
    void run() override;                             // 0x4c1410
    Raytrace::RayTrace *rt;                          // +0xc0
    VoList lines;                                    // +0xc4
    uint32_t next = 0;                               // +0xf8
};

}  // namespace Game
