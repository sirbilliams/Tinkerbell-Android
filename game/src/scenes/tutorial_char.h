// TutorialCharacter (vtables 0x517474 / 0x517444 / 0x517430, ctor 0x4cf150, dtor 0x4442e0), reconstructed from
// "Tinker Bell.exe": a Lost Boy (or Peter) standing in the Lost Boys' hideout during the tutorial. A
// RayTraceCharacter with a task that plays its idles, a MagpieCharacter that talks (mouth attachment and a
// talking loop) and a mouse service: sprinkled with pixie dust it floats in its flying idle.
//   init 0x4cf4a0, release 0x4cf370, run 0x4cf5d0, notify2 0x4cff30, ready 0x4cfbd0, mouth 0x4cf3b0, rest 0x4cf410,
//   play 0x4cf940, animDone 0x4cfed0, flying 0x4cfb40, settle 0x4cfa10, setters 0x4cf310 / 0x4cf9a0 / 0x4cfea0 /
//   0x4d0030 / 0x4d0060, wave 0x4d00b0, toRoot 0x4d00d0, atRoot 0x4d00f0.
// The states (+0x188): 0 root once then 1, 1 idles at random, 2/3 back to the root, 4 waiting for the root to end
// before talking, 5 talking, 6 waving, 7 back to 0, 8 flying with dust.
#pragma once
#include "../raytrace/character.h"
#include "voiceover.h"
#include "../vision/engine.h"
#include <string>

namespace Game {

class DustLift;

class TutorialCharacter : public Raytrace::RayTraceCharacter, public Vision::Task, public Vision::MagpieCharacter {
public:
    // `lift` is the scene's +0x2740 (the DustLift pointer, made by the scene's open later)
    TutorialCharacter(const char *name, Raytrace::RayTrace *rt, DustLift **lift);   // 0x4cf150
    ~TutorialCharacter() override;                   // 0x4442e0
    void init() override;                            // RayTraceCharacter v1 0x4cf4a0
    void release() override;                         // RayTraceCharacter v2 0x4cf370
    void run() override;                             // Task v10 0x4cf5d0
    void notify2(Vision::Service *s) override;       // Task v9 0x4cff30: a click with dust on the cursor
    bool ready() override;                           // Magpie +4 0x4cfbd0
    void mouth(int letter) override;                 // Magpie +8 0x4cf3b0
    void rest() override;                            // Magpie +0xc 0x4cf410

    void setLip(const char *name, const char *prefix, int z) { lip_name = name; mouth_prefix = prefix; lip_z = z; }   // 0x4cf310
    void setIdles(const char *r, const char *i1, const char *i2) { root = r; idle1 = i1; idle2 = i2; }   // 0x4cf9a0
    void setRootTalk(const char *s) { roottalk = s; }                // 0x4cfea0
    void setFlyIdle(const char *s) { flyidle = s; }                  // 0x4d0030
    void setWave(const char *anim, const char *line) { wave_anim = anim; wave_line = line; }   // 0x4d0060
    void wave() { wave_pending = true; state = 6; }  // 0x4d00b0
    void toRoot() { root_pending = true; state = 2; }   // 0x4d00d0
    bool atRoot() const { return state == 3; }       // 0x4d00f0
    void play(const char *anim, int mode);           // 0x4cf940: the animation from its first frame, scale 130
    bool animDone();                                 // 0x4cfed0: at the last frame (or back at the first)
    bool stillFlying();                              // 0x4cfb40
    void settle();                                   // 0x4cfa10: after the dust, back to the root

    Raytrace::Attachment *mouth_att = nullptr;       // +0x114
    std::string lip_name, mouth_prefix;              // +0x118 +0x124
    int lip_z = 0;                                   // +0x130
    std::string root, idle1, idle2, roottalk;        // +0x134 +0x140 +0x14c +0x158
    std::string wave_line, wave_anim, flyidle;       // +0x164 +0x170 +0x17c
    int state = 0;                                   // +0x188
    Vision::Mouse mouse;                             // +0x190
    Vision::Keyboard keys;                           // +0x1c4
    DustLift **lift;                                 // +0x1f8
    TutorialCharacter *partner = nullptr;            // +0x1fc: talks along (the raccoon twins)
    bool wave_pending = false;                       // +0x200
    bool root_pending = false;                       // +0x201
    bool keep_flying = false;                        // +0x202: stays in the flying idle (Peter until he lands)
    const char *talk_anim = nullptr;                 // +0x204: the next talk's animation (instead of the root talk)
};

}  // namespace Game
