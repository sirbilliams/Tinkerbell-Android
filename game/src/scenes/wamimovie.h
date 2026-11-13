// The treasure scene's three films, reconstructed from "Tinker Bell.exe":
//   WamiMovie (vtable 0x515984, ctor 0x4933b0, the scene's +0x29d0): Peter and Wendy find the door, Hook and Smee
//     walk in and dare the player to open it. load 0x493710, enter 0x493e40, skip 0x493f50, run 0x493ff0,
//     chars 0x495100 (hands its four characters to WamiMovie2), mouseOn 0x4950f0.
//   WamiMovie2 (vtable 0x5159b4, ctor 0x495140, +0x3e20): after the door opens: the crocodile's clock scares Hook
//     and Smee off, Peter thanks the player with Tink's bell and flies off with Wendy. start 0x495260, skip
//     0x4953a0, link 0x495460, run 0x495470.
//   Movie2Ending (vtable 0x515a14, +0x588 of WamiMovie2): Peter waits for a click on him, then flies up with the
//     camera into the clouds; the sign-in room. start 0x496e50, run 0x496f20, notify2 0x4971a0, steps 0x4972a0 /
//     0x497340.
//   SmoothRunner (vtable 0x5159e4, +0x8b0 of WamiMovie2): a character moved along one axis by a Tween. run 0x497400.
//   WamiMovie3 (vtable 0x515a54, ctor 0x4974f0, +0x4ae0): the door opens: the camera sways while "you win" plays.
//     start 0x4975e0, skip 0x497600, run 0x497620.
//   BellBehindPixieDust (vtable 0x515a84, ctor 0x4978c0, +0x9c0 of WamiMovie2): the bell shown behind a puff of
//     dust. run 0x497a20.
#pragma once
#include "wamichar.h"
#include "acts.h"
#include "voacts.h"
#include "../raytrace/mover.h"
#include "../vision/animation.h"
#include "../vision/sound.h"

namespace Game {

class WamiMovie : public Vision::Task {
public:
    explicit WamiMovie(Raytrace::RayTrace *rt);      // 0x4933b0
    void load();                                     // 0x493710
    void enter();                                    // 0x493e40
    void skip();                                     // 0x493f50
    void run() override;                             // 0x493ff0
    void chars(WamiChar **p, WamiChar **w, WamiBaseChar **h, WamiBaseChar **s) {   // 0x495100
        *p = &peter; *w = &wendy; *h = &hook; *s = &smee;
    }
    bool mouseOn() { return mouse.active(); }        // 0x4950f0 (never activated)
    void playWait(const char *line);                 // (inlined: play, then wait while it plays)

    Raytrace::RayTrace *rt;                          // +0xc0
    WamiChar peter;                                  // +0xc4 "peter"
    WamiChar wendy;                                  // +0x36c "wendy"
    ComeNear near;                                   // +0x614
    Raytrace::CameraTween tween;                     // +0x710
    bool running = false;                            // +0x818
    WamiBaseChar hook;                               // +0x81c "hook"
    Raytrace::LinearTween hookTween;                 // +0xa38
    WamiBaseChar smee;                               // +0xa78 "smee"
    Raytrace::LinearTween smeeTween;                 // +0xc98
    bool agreed = false;                             // +0xcd8
    Inactivity inact1, inact2;                       // +0xcdc +0x107c (never started)
    Vision::Mouse mouse;                             // +0x141c
};

class Movie2Ending : public Vision::Task {
public:
    explicit Movie2Ending(Raytrace::RayTrace *rt);
    void start();                                    // 0x496e50
    void run() override;                             // 0x496f20
    void notify2(Vision::Service *s) override;       // 0x4971a0
    void handIdle();                                 // 0x4972a0 (state 4)
    void rising();                                   // 0x497340 (state 5)
    Raytrace::RayTrace *rt;                          // +0xc0
    WamiChar *peter = nullptr;                       // +0xc4
    RepeatLine repeat;                               // +0xc8 "WAMI_VO_PEThandid"
    Vision::Mouse mouse;                             // +0x19c
    int state = 0;                                   // +0x1d0: 0 waiting for the click, 1 "go", 2/5 flying up, 6 fading
    Raytrace::Tween peterY;                          // +0x1d8
    Raytrace::CameraTween cam;                       // +0x218
    bool star_said = false;                          // +0x320
};

class SmoothRunner : public Vision::Task {
public:
    SmoothRunner() : Task(nullptr) {}
    void run() override;                             // 0x497400
    Raytrace::RayTraceCharacter *ch = nullptr;       // +0xc0
    Raytrace::Tween *tw = nullptr;                   // +0xc4
    int axis = 0;                                    // +0xc8: 0 x, 1 y, 2 z
};

class BellBehindPixieDust : public Vision::Task {
public:
    BellBehindPixieDust();                           // 0x4978c0
    void run() override;                             // 0x497a20
    Vision::Screen screen;                           // +0xc0
    Vision::Image bell;                              // +0x140 "WAMI_AM_bell_01"
    Vision::Animation puff;                          // +0x1e8 "GLOB_BU_BAGpffsel_"
};

class WamiMovie2 : public Vision::Task {
public:
    explicit WamiMovie2(Raytrace::RayTrace *rt);     // 0x495140
    ~WamiMovie2() override;
    void start();                                    // 0x495260
    void skip();                                     // 0x4953a0
    void link() { ending.peter = peter; }            // 0x495460
    void run() override;                             // 0x495470
    void playWait(const char *line);
    void waitQueued(WamiBaseChar *c, const char *line);   // (inlined: its sayOn line said and played)
    void waitUntil(WamiBaseChar *c, const char *anim);    // (inlined: until it shows the animation)

    Raytrace::RayTrace *rt;                          // +0xc0
    WamiChar *peter = nullptr, *wendy = nullptr;     // +0xc4 +0xc8 (WamiMovie's)
    WamiBaseChar *hook = nullptr, *smee = nullptr;   // +0xcc +0xd0
    bool running = false;                            // +0xd4
    bool once = false;                               // +0xd5
    Raytrace::CameraTween tween;                     // +0xd8
    Raytrace::LinearTween hookX, hookZ, smeeX;       // +0x1e0 +0x200 +0x220
    Vision::Sound *tnk;                              // +0x240 "WAMI_FX_TNKtalk" (embedded)
    Movie2Ending ending;                             // +0x588
    SmoothRunner runner;                             // +0x8b0
    Raytrace::Tween wendyY;                          // +0x980
    BellBehindPixieDust bell;                        // +0x9c0
};

class WamiMovie3 : public Vision::Task {
public:
    explicit WamiMovie3(Raytrace::RayTrace *rt);     // 0x4974f0
    void start() { reset(); activate(); running = true; }   // 0x4975e0
    void skip();                                     // 0x497600
    void run() override;                             // 0x497620
    Raytrace::RayTrace *rt;                          // +0xc0
    bool running = false;                            // +0xc4
    Raytrace::CameraTween cam;                       // +0xc8
    VoOneAfterAnother uwin;                          // +0x1d0 "WAMI_VO_ALLuwin"
    VoOneAfterAnother spare;                         // +0x2a4 (never used)
};

}  // namespace Game
