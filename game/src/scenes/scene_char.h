// SceneCharacter (vtables 0x517388 / 0x517358 / 0x517344, ctor 0x4ccad0, dtor 0x43e3a0), reconstructed from
// "Tinker Bell.exe": a character standing in a Neverland area (the Lost Boys in their hideout, Peter...). Like the
// tutorial's TutorialCharacter it is a RayTraceCharacter with a task that plays its idles, a MagpieCharacter that
// talks (a mouth attachment, talking gestures) and a mouse service (sprinkled with dust it floats). It also
// blinks (an eyes attachment shown for a frame now and then while it talks) and cheers when one of a list of
// lines plays (the "win" lines, shared by everyone in the scene).
//   init 0x4cce80, release 0x4ccd80, run 0x4cd020, notify2 0x4ce770, ready 0x4ce040, mouth 0x4ccdd0, rest 0x4cce30,
//   blink 0x4cd3f0, play 0x4cd550, the state steps 0x4cd9b0 (1) / 0x4cdac0 (3) / 0x4cd930 (5, 7) / 0x4cd6e0 (6) /
//   0x4cd830 (8), setters 0x4ccd20 / 0x4cd650 / 0x4ce2d0 / 0x4ce850 / 0x4ce880.
// The states (+0x1dc): 0 to the root then 1, 1 idles at random, 2 to the talking root, 3 talking (gestures), 4
// waiting for a line (+0x24c) to end, 5 and 7 back to the root, 6 cheering once the win line ends, 8 flying.
#pragma once
#include "../raytrace/character.h"
#include "voiceover.h"
#include "../vision/engine.h"
#include <string>
#include <vector>

namespace Game {

class DustLift;

class SceneCharacter : public Raytrace::RayTraceCharacter, public Vision::Task, public Vision::MagpieCharacter {
public:
    // `lift` is the scene's +0x2740 (the DustLift pointer, made by the scene's open later)
    SceneCharacter(const char *name, Raytrace::RayTrace *rt, DustLift **lift);   // 0x4ccad0
    ~SceneCharacter() override;                      // 0x43e3a0
    void init() override;                            // RayTraceCharacter v1 0x4cce80
    void release() override;                         // RayTraceCharacter v2 0x4ccd80
    void run() override;                             // Task v10 0x4cd020
    void notify2(Vision::Service *s) override;       // Task v9 0x4ce770: a click with dust on the cursor
    bool ready() override;                           // Magpie +4 0x4ce040
    void mouth(int letter) override;                 // Magpie +8 0x4ccdd0
    void rest() override;                            // Magpie +0xc 0x4cce30

    void setLip(const char *name, const char *prefix, int z) { lip_name = name; mouth_prefix = prefix; lip_z = z; }   // 0x4ccd20
    void setIdles(const char *r, const char *i1, const char *i2, const char *i3) {   // 0x4cd650
        root = r; idle1 = i1; idle2 = i2; idle3 = i3;
    }
    // 0x4ce2d0: the talking root and three gestures; a gesture that is one of the game's talking loops
    // ("GLOB_CA_<who>talk<n>_") swings back and forth and holds, any other plays once.
    void setTalks(const char *rt_, const char *t1, const char *t2, const char *t3);
    void setFlyIdle(const char *s) { flyidle = s; }  // 0x4ce850
    void setWave(const char *anim, const std::vector<std::string> *lines) { wave_anim = anim; wave_lines = lines; }   // 0x4ce880
    void play(const char *anim, int mode);           // 0x4cd550: unless already showing it so; scale 126
    bool animDone();                                 // (0x4cfed0, TutorialCharacter's: shared code)
    void blink();                                    // 0x4cd3f0
    void stepIdle();                                 // 0x4cd9b0 (state 1)
    void stepTalk();                                 // 0x4cdac0 (state 3)
    void stepBack();                                 // 0x4cd930 (states 5 and 7)
    void stepWave();                                 // 0x4cd6e0 (state 6)
    void stepFly();                                  // 0x4cd830 (state 8)

    Raytrace::Attachment *mouth_att = nullptr;       // +0x114
    Raytrace::Attachment *eyes_att = nullptr;        // +0x118
    std::string eyes;                                // +0x11c: the blink attachment ("RAC_eyes"...)
    std::string lip_name, mouth_prefix;              // +0x128 +0x134
    int lip_z = 0;                                   // +0x140
    std::string root, idle1, idle2, idle3;           // +0x144 +0x150 +0x15c +0x168
    std::string roottalk, talk[3];                   // +0x174 +0x180 +0x18c +0x198
    std::string after_wait;                          // +0x1a4: played when the awaited line (state 4) ends
    // +0x1b0: a byte the ctor fills from its first argument (the low byte of the name pointer); never read.
    // +0x1b4..+0x1bc: zeroed, never read.
    std::string wave_anim;                           // +0x1c0
    const std::vector<std::string> *wave_lines = nullptr;   // +0x1cc
    std::string flyidle;                             // +0x1d0
    int state = 0;                                   // +0x1dc
    // +0x1e0: a random number object (0x425d10); its draws are the game's rand() (0x425d40) like everyone's
    Vision::Mouse mouse;                             // +0x1e4
    Vision::Keyboard keys;                           // +0x218
    std::string wait_line;                           // +0x24c: the line state 4 waits for
    std::string wave_line;                           // +0x258: the win line heard
    DustLift **lift;                                 // +0x264
    struct { bool once = false; int n = 0; } gesture[3];   // +0x268.. (+0x26c.. zeroed by a looping play, unread)
    SceneCharacter *partner = nullptr;               // +0x280: talks along (the raccoon twins)
    uint32_t blink_at = 0;                           // +0x284
    bool keep_flying = false;                        // +0x288
};

}  // namespace Game
