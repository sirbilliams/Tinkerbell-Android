// PeterGuide (vtables 0x5173e0 / 0x5173b0 / 0x51739c, ctor 0x4ce910, dtor 0x4cea70), reconstructed from
// "Tinker Bell.exe": Peter seen from above in a mini game's corner ("PET_Top_Down"): he rises in (20 px every
// 30 ms from 500 down), says the game's greeting (the first time's, else the other), and flies off to the right
// once the line ends; he blinks and mouths the line meanwhile.
//   load 0x4ceb60, unload v16 0x4cec80, draw v14 0x4ced00, run v10 0x4cee00, stop 0x4cf060,
//   ready 0x4cf0f0, mouth 0x4cf0c0, rest 0x4cf0e0
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "voiceover.h"

namespace Game {

class PeterGuide : public Vision::Graphic, public Vision::Task, public Vision::MagpieCharacter {
public:
    PeterGuide(const char *first, const char *again);   // 0x4ce910
    ~PeterGuide() override;                          // 0x4cea70
    void load(bool replay);                          // 0x4ceb60 (replay: the "again" line even the first time)
    void unload() override;                          // v16 0x4cec80
    void draw() override;                            // v14 0x4ced00
    void run() override;                             // Task v10 0x4cee00
    void stop();                                     // 0x4cf060
    bool ready() override { return idle.visible; }  // 0x4cf0f0
    void mouth(int letter) override;                 // 0x4cf0c0
    void rest() override { mouth_a.visible = false; }   // 0x4cf0e0
    // each one's +0x7c (visible) is the guide's own "shown" flag for it (+0x1dc +0x2f4 +0x40c +0x524)
    Vision::Animation enter_a, idle, exit_a, mouth_a;   // +0x160 +0x278 +0x390 +0x4a8
    const char *first, *again;                       // +0x5c0 +0x5c4
    bool replay = false;                             // +0x5c8
    Vision::Ras *pack = nullptr;                     // +0x5cc
    uint32_t blink_at = 0;                           // +0x5d0
    bool blinked = false;                            // +0x5d4
    Vision::Image eyes;                              // +0x5d8 "PEV_eyes01"
};

}  // namespace Game
