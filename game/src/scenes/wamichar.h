// The characters of the treasure scene ("treasure", WAMI_*), reconstructed from "Tinker Bell.exe":
//   WamiBaseChar (vtables 0x5174cc / Task 0x517488 / Magpie 0x5174b8, ctor 0x4d0190, 0x21c bytes): Hook and Smee. A
//     RayTraceCharacter with a task that steps its animation every +0x164 ms and plays a queue of animations
//     (queueAnim), a queue of lines each said once its animation shows (sayOn), a mouth per line (setInt: the mouth
//     number), a resting mouth on some idles and eyes that blink.
//       start 0x4d0460, release 0x4d05a0, setMouth 0x4d0430, setEyes 0x4d08a0, blink 0x4d0990, playAnim 0x4d0a80,
//       run 0x4d1120, animDone 0x4d12a0, queueAnim 0x4d13d0, animStep 0x4d1510, sayOn 0x4d1800, speak 0x4d1970,
//       setInt 0x4d1bd0, mouth 0x4d1c80, rest 0x4d1d50, ready 0x4d1d80, isQueued 0x4d1da0.
//   WamiChar (vtables 0x517520 / 0x5174f0 / 0x5174dc, ctor 0x4d2170): Peter and Wendy. Adds a pack, a root with
//     three idles and the talking root with three gestures (as SceneCharacter's).
//       init 0x4d2280, release 0x4d2320, run 0x4d2360, setIdles 0x4d2560, setTalks 0x4d25f0, playKeep 0x4d2a70,
//       idleStep 0x4d2ab0, talkStep 0x4d2bf0, ready 0x4d2f30, rest 0x4d30a0.
// The states (+0x178): 0 to the idle then 1, 1 idle, 2 the queue, 3 back to the root then 4, 4 idling at random,
// 5 to the talking root then 6, 6 talking (gestures).
#pragma once
#include "../raytrace/character.h"
#include "voiceover.h"
#include "../vision/engine.h"
#include "../vision/sound.h"
#include <deque>
#include <map>
#include <string>

namespace Game {

class WamiBaseChar : public Raytrace::RayTraceCharacter, public Vision::Task, public Vision::MagpieCharacter {
public:
    WamiBaseChar(const char *name, Raytrace::RayTrace *rt);   // 0x4d0190
    ~WamiBaseChar() override;
    void release() override;                         // v2 0x4d05a0
    void run() override;                             // Task 0x4d1120
    bool ready() override;                           // Magpie 0x4d1d80
    void mouth(int letter) override;                 // Magpie 0x4d1c80
    void rest() override;                            // Magpie 0x4d1d50

    void start(const char *anim, int mode);          // 0x4d0460: the first animation, the mouths, the task
    void setMouth(const char *name, int i);          // 0x4d0430
    void setEyes(const char *anim, const char *eyes) { eyes_by_anim[anim] = eyes; }   // 0x4d08a0
    void setInt(const char *line, int v) { mouth_by_line[line] = v; mouth_idx = v; }   // 0x4d1bd0
    void blink();                                    // 0x4d0990
    void playAnim(const char *anim, int mode);       // 0x4d0a80
    bool animDone();                                 // 0x4d12a0
    // 0x4d13d0: played in turn; dir -1 from the end, synced: its frames by the clock, nowait: no waiting for the
    // previous one's end
    void queueAnim(const char *anim, int mode, int dir, bool synced, bool nowait);
    void animStep();                                 // 0x4d1510 (state 2)
    void sayOn(const char *anim, const char *line);  // 0x4d1800: the line once the animation shows
    void speak();                                    // 0x4d1970
    bool isQueued(const char *line);                 // 0x4d1da0: a sayOn line not said yet
    bool animIs(const char *anim);                   // (the inlined sprite +0x70 name compare, 0x4243f0 + 0x423c60)

    struct Queued {                                  // 0x18 bytes
        std::string anim;                            // +0x00
        int mode = 0, dir = 1;                       // +0x0c +0x10
        bool synced = false, nowait = false;         // +0x14 +0x15
    };
    struct Line { std::string anim, line; };         // 0x18 bytes

    std::string mouths[4];                           // +0x114: the mouth attachments' names
    float scale = 130.0f;                            // +0x144
    std::string pack_name;                           // +0x148 (never set: no pack)
    Vision::Ras *pack = nullptr;                     // +0x154
    std::string idle;                                // +0x158 (never set)
    uint32_t frame_ms = 0x53;                        // +0x164
    Raytrace::Attachment *mouth_att[4] = {};         // +0x168
    int state = 0;                                   // +0x178
    std::deque<Queued> queue;                        // +0x17c
    std::deque<Line> lines;                          // +0x1ac
    std::map<std::string, int> mouth_by_line;        // +0x1dc
    int mouth_idx = 0;                               // +0x1ec
    bool synced = false;                             // +0x1f0
    uint32_t synced_t0 = 0;                          // +0x1f4
    Raytrace::Attachment *rest_att = nullptr;        // +0x1f8
    std::map<std::string, std::string> eyes_by_anim; // +0x1fc
    Raytrace::Attachment *eyes_att = nullptr;        // +0x20c
    uint32_t blink_at = 0;                           // +0x210
    int holds = 0, max_holds = 0;                    // +0x214 +0x218: "WAMI_CA_WENready_" held at frame 9

    static Vision::Sound &clock();                   // the static at 0x631238 "WAMI_FX_CLOCK" (0x4d0150)
};

class WamiChar : public WamiBaseChar {
public:
    WamiChar(const char *name, Raytrace::RayTrace *rt);   // 0x4d2170
    void init() override;                            // v1 0x4d2280
    void release() override;                         // v2 0x4d2320
    void run() override;                             // 0x4d2360
    bool ready() override;                           // 0x4d2f30
    void rest() override;                            // 0x4d30a0
    void setIdles(const char *r, const char *i1, const char *i2, const char *i3) {   // 0x4d2560
        root = r; idle1 = i1; idle2 = i2; idle3 = i3;
    }
    void setTalks(const char *rt_, const char *t1, const char *t2, const char *t3);   // 0x4d25f0
    void playKeep(const char *anim, int mode);       // 0x4d2a70: keeping the state
    void idleStep();                                 // 0x4d2ab0
    void talkStep();                                 // 0x4d2bf0

    std::string pack2;                               // +0x21c (never set: no pack)
    Vision::Ras *pack2_ras = nullptr;                // +0x228
    std::string root, idle1, idle2, idle3;           // +0x22c +0x238 +0x244 +0x250
    std::string roottalk, talk[3];                   // +0x25c +0x268 +0x274 +0x280
    struct { bool once = false; int n = 0; } gesture[3];   // +0x28c.. (n zeroed by a looping play, unread)
    uint32_t wendy_idle_at = 0;                      // +0x2a4
};

}  // namespace Game
