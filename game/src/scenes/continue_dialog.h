// ContinueDialog (vtable 0x517900, ctor 0x4e5330, dtor 0x4e5590), reconstructed from "Tinker Bell.exe": the "play
// again?" sign a mini game puts up when the player did not win: Peter asks, the sign and the X glow under the
// mouse, a click on the sign is "no" (+0x6f0), on the X "yes" (+0x6f1); Peter answers and the task finishes (its
// parent reads the two flags in its notify2).
//   activate v6 0x4e5c20, deactivate v7 0x4e5c90, notify2 v9 0x4e5ce0, run v10 0x4e5d70,
//   setAsk 0x4e5720, setYes 0x4e5860, setNo 0x4e59a0, setIdle 0x4e5ae0
#pragma once
#include "../vision/engine.h"
#include "../vision/sound.h"
#include <string>

namespace Game {

class ContinueDialog : public Vision::Task {
public:
    ContinueDialog(const std::string &sign, const std::string &sign_glow, const std::string &x_glow);   // 0x4e5330
    ~ContinueDialog() override;                      // 0x4e5590
    void activate() override;                        // v6 0x4e5c20
    void deactivate() override;                      // v7 0x4e5c90
    void notify2(Vision::Service *s) override;       // v9 0x4e5ce0
    void run() override;                             // v10 0x4e5d70
    void setAsk(const std::string &s) { ask = s; }   // 0x4e5720
    void setYes(const std::string &s) { yes_line = s; }   // 0x4e5860
    void setNo(const std::string &s) { no_line = s; }     // 0x4e59a0
    void setIdle(const std::string &s) { idle = s; }      // 0x4e5ae0
    Vision::Screen screen;                           // +0xc0
    Vision::Mouse mouse;                             // +0x13c
    Vision::Image sign, sign_glow, x_glow;           // +0x170 +0x218 +0x2c0
    Vision::Sound click;                             // +0x368 "GLOB_FX_GSclick"
    int f6a8 = 7;                                    // +0x6a8
    std::string ask = "glob_vo_petreplay";           // +0x6b0
    std::string yes_line, no_line, idle;             // +0x6c0 +0x6d0 +0x6e0
    bool no = false;                                 // +0x6f0: the sign clicked
    bool yes = false;                                // +0x6f1: the X clicked
};

}  // namespace Game
