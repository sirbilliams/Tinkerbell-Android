// Echo Echo, the rabbit's puzzle game at Skull Rock (played in the scene itself on the instruments' shelf; the
// game is the scene's +0x43bc), reconstructed from "Tinker Bell.exe". The cave echoes a few animal sounds (thunder,
// a rattlesnake, a cow, a bird, a horse); the player answers each with its instrument on the shelf (the drum, the
// maracas, the horn, the flute, the coconuts), in order. Seven rounds right win the map segment; a timed game
// (played before) ends when its fuse burns down.
//   EchoInstrument  (0x1c bytes, no vtable; play 0x48a920, playing 0x48a9c0, dtor 0x48ab10)
//   EchoInstruments (0x28 bytes; ctor 0x48a9d0, update 0x48ab30): the five, the shelf, the one picked
//   EchoPlayer      (0x48 bytes; dtor 0x489e10, pop 0x48abf0): the echoes still to play, one after another
//   EchoEcho        (vtable 0x5155b4, 0x4b0 bytes; ctor 0x4896d0, dtor 0x4897b0, cursor 0x489960, start 0x489980,
//                    skip 0x489bd0, setup 0x489c00, teardown 0x489d80, mouse 0x489ed0, makeRound 0x489fc0,
//                    check 0x48a1e0, run v10 0x48a370, hint 0x48a8d0)
#pragma once
#include "../vision/engine.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"
#include "acts.h"
#include <deque>
#include <vector>

namespace Game {

class Cannon;
class Tinkerbell3D;
class EchoEcho;

struct EchoInstrument {
    void play(const char *name);                     // 0x48a920: a fresh sound of that name, played
    bool playing();                                  // 0x48a9c0
    ~EchoInstrument();                               // 0x48ab10
    Vision::Sound *snd = nullptr;                    // +0x00
    const char *name;                                // +0x04 "echo drum"..: its attachment on the shelf
    const char *anim;                                // +0x08 "echo_ga_puzdrum_": under the mouse
    const char *idle;                                // +0x0c "echo_ga_puzdrumid_"
    const char *fx;                                  // +0x10 "echo_fx_drum"
    bool on = false;                                 // +0x14: its animation showing
    Raytrace::Attachment *att = nullptr;             // +0x18
};

struct EchoInstruments {
    EchoInstruments(Raytrace::RayTrace *rt, EchoEcho *echo);   // 0x48a9d0
    void update();                                   // 0x48ab30: the one under the mouse animates (the hand)
    EchoInstrument *inst[5] = {};                    // +0x00: drum, maracas, horn, flute, coconuts
    int picked = -1;                                 // +0x14
    Raytrace::Sprite3D *shelf = nullptr;             // +0x18 "AA_ECHO_GA_puzshelf_01"
    Raytrace::RayTrace *rt;                          // +0x1c
    EchoEcho *echo;                                  // +0x20
    bool enabled = false;                            // +0x24: the player's turn
};

struct EchoPlayer {
    ~EchoPlayer();                                   // 0x489e10
    void play(const char *name);                     // (EchoInstrument's 0x48a920 on its head)
    Vision::Sound *snd = nullptr;                    // +0x00
    const char *names[5] = {"echo_fx_thunder", "echo_fx_rattlesnk", "echo_fx_cow", "echo_fx_bird",
                            "echo_fx_horse"};        // +0x04..+0x14
    std::deque<int> queue;                           // +0x18
};

class EchoEcho : public Vision::Task {
public:
    explicit EchoEcho(Raytrace::RayTrace *rt);       // 0x4896d0
    ~EchoEcho() override;                            // 0x4897b0
    Vision::Graphic *cursor(Vision::Graphic *def);   // 0x489960: the hand over an instrument
    void start();                                    // 0x489980
    void skip();                                     // 0x489bd0: the skip key
    void setup();                                    // 0x489c00 (enter)
    void teardown();                                 // 0x489d80 (leave)
    void mouse();                                    // 0x489ed0 (the scene's mouse, while on)
    void makeRound();                                // 0x489fc0
    void check();                                    // 0x48a1e0
    void run() override;                             // v10 0x48a370
    void hint(Tinkerbell3D *tink);                   // 0x48a8d0
    bool busy();                                     // (inlined twice in run): a line, an echo or an instrument
    Raytrace::RayTrace *rt;                          // +0xc0
    int state = 0;                                   // +0xc4: 0 the player's turn, 1 a new round, 2 won, 3 out of
                                                     // time, 4 over
    std::vector<int> seq;                            // +0xc8: the echoes still to answer
    int length = 0;                                  // +0xd8: the longest round
    int rounds = 0;                                  // +0xdc: rounds right
    EchoPlayer *player = nullptr;                    // +0xe0
    EchoInstruments *instr = nullptr;                // +0xe4
    Cannon *fuse = nullptr;                          // +0xe8 (the state's +0x7c0)
    bool hand = false;                               // +0xec
    int secs = 0;                                    // +0xf0
    Inactivity inactivity;                           // +0xf4 "ECHO_VO_RABprompt"
    Raytrace::CameraMover mover;                     // +0x494
    bool timed = false;                              // +0x4ac
};

}  // namespace Game
