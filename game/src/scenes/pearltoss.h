// Pearl Toss, Cubby's game in Mermaid Cove (played in the scene itself; the monitor is the scene's +0x2900),
// reconstructed from "Tinker Bell.exe". The two mermaids on their rocks toss pearls at the coconuts on five palm
// trees: a coconut hit drops onto its tree's foot and lies there a while; a click on it (the basket cursor) catches
// it, else it falls and breaks. Some tosses miss on purpose and fly past the tree. Eight catches before the fuse
// burns down win.
//   PearlTossMonitor (vtable 0x51498c, 0x1ba8 bytes; ctor 0x4546b0, setup 0x454ba0, kinds 0x454ef0, start 0x454830,
//                     run v10 0x455050, notify2 v9 0x4554c0, stop 0x455610, hint 0x455950)
//   PearlToss        (vtable 0x5148fc, 0x220 bytes; ctor 0x453760, start 0x453a70, run v10 0x453b90, notify2 v9
//                     0x454180, idle 0x454430); its path (+0x110): toTree 0x452db0, pastTree 0x453190
//   ClappingMermaid  (vtable 0x51492c, the toss's +0x158: run v10 0x455c40, stop 0x455be0)
//   MermaidTask      (vtable 0x51495c, two of them in the static pair at 0x5c3b60 (ctor 0x454570): run v10 0x455d40)
//   PearlInterface   (the monitor's +0xbb8, a Screen; ctor 0x4535c0): the catches on a bar.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"
#include "acts.h"
#include <vector>

namespace Game {

// A thrown pearl's way, point by point (embedded in PearlToss at +0x110, 0x3c bytes).
struct PearlPath {
    explicit PearlPath(Raytrace::RayTrace *r) : rt(r) {}
    void sprite();                                   // (the head of both): "PERL_GA_pearl_" found, else made
    void toTree(Raytrace::Sprite3D *from, Raytrace::Sprite3D *to);     // 0x452db0: 19 points, rising to the tree
    void pastTree(Raytrace::Sprite3D *from, Raytrace::Sprite3D *to);   // 0x453190: 39 points, over and past it
    Raytrace::RayTrace *rt;                          // +0x00
    Raytrace::Sprite3D *pearl = nullptr;             // +0x04
    std::vector<int> xs, zs, ys;                     // +0x08 +0x18 +0x28
    unsigned index = 0;                              // +0x38
};

// (vtable 0x51492c) the tossing mermaid's throw played out; then her idle back.
class ClappingMermaid : public Vision::Task {
public:
    ClappingMermaid() : Task(nullptr) {}
    void run() override;                             // v10 0x455c40
    void stop();                                     // 0x455be0
    Raytrace::Sprite3D *toss = nullptr;              // +0xc0
    Raytrace::Sprite3D *idle = nullptr;              // +0xc4
};

// (vtable 0x51495c) a mermaid claps once her idle shows again (a catch).
class MermaidTask : public Vision::Task {
public:
    MermaidTask() : Task(nullptr) {}
    void run() override;                             // v10 0x455d40
    Raytrace::Sprite3D *idle = nullptr;              // +0xc0 (the scene's +0x28e8 / +0x28ec)
    Raytrace::Sprite3D *toss = nullptr;              // +0xc4
    Raytrace::Sprite3D *clap = nullptr;              // +0xc8
    Vision::Sound *snd = nullptr;                    // +0xcc "PERL_FX_MERclap"
};
MermaidTask &mermaid_a();                            // 0x5c3b60
MermaidTask &mermaid_b();                            // 0x5c3c30

// One toss (five of them, one per tree).
class PearlToss : public Vision::Task {
public:
    PearlToss();                                     // 0x453760
    // 0x453a70: the tree, the mermaid's toss and idle sprites, the tree's coconut and its breaking
    void start(Raytrace::Sprite3D *tree, Raytrace::Sprite3D *toss, Raytrace::Sprite3D *idle,
               Raytrace::Attachment *coco, Raytrace::Attachment *broken);
    void run() override;                             // v10 0x453b90
    void notify2(Vision::Service *s) override;       // v9 0x454180
    bool idle();                                     // 0x454430: no coconut of this toss under the mouse
    bool under();                                    // (inlined in both): its coconut under the mouse (a column)
    Raytrace::RayTrace *rt;                          // +0xc0
    Raytrace::Sprite3D *tree = nullptr;              // +0xc4
    Raytrace::Sprite3D *toss = nullptr;              // +0xc8
    Raytrace::Sprite3D *idle_spr = nullptr;          // +0xcc
    Raytrace::Attachment *coco = nullptr;            // +0xd0 "SC<n>"
    Raytrace::Attachment *broken = nullptr;          // +0xd4 "FC<n>"
    uint32_t t0 = 0;                                 // +0xd8
    Vision::Mouse mouse;                             // +0xdc
    PearlPath path;                                  // +0x110
    int hold = 0;                                    // +0x14c: ms the coconut lies there
    int info = 0;                                    // +0x150: the game won before
    bool miss = false;                               // +0x154: a toss past the tree
    ClappingMermaid clap;                            // +0x158
    static std::vector<PearlToss *> &all();          // 0x5c3038
};

// (the monitor's +0xbb8) a Screen: the bar and eight coconuts, shown one by one at the eight targets' spots.
class PearlInterface : public Vision::Screen {
public:
    PearlInterface();                                // 0x4535c0
    Vision::Image *targets[8] = {};                  // +0x80 (0xa8 each; not drawn: their spots)
    Vision::Image *catches[8] = {};                  // +0x5c0 (0xa8 each)
    Vision::Image bar;                               // +0xb00 "PERL_IF_interfacebar_01"
    int count = 0;                                   // +0xba8 (the monitor's +0x1760)
};

class PearlTossMonitor : public Vision::Task {
public:
    explicit PearlTossMonitor(Raytrace::RayTrace *rt);   // 0x4546b0
    void setup();                                    // 0x454ba0: the mermaids' toss and clap sprites, the trees
    void kinds(int pearls, int misses);              // 0x454ef0: the tosses to come, shuffled
    void start();                                    // 0x454830
    void run() override;                             // v10 0x455050
    void notify2(Vision::Service *s) override;       // v9 0x4554c0
    void stop();                                     // 0x455610
    bool hint(Raytrace::Attachment **out);           // 0x455950: the nearest coconut lying there
    Raytrace::RayTrace *rt;                          // +0xc0
    Raytrace::Sprite3D *toss_a = nullptr, *toss_b = nullptr;   // +0xc4 +0xc8
    Raytrace::Sprite3D *clap_a = nullptr, *clap_b = nullptr;   // +0xcc +0xd0
    Raytrace::Sprite3D *trees[5] = {};               // +0xd4
    Raytrace::Attachment *cocos[5] = {};             // +0xe8 "SC0".. "PERL_GA_coconutb_"
    Raytrace::Attachment *brokens[5] = {};           // +0xfc "FC0".. "PERL_GA_cocobreakb_"
    PearlToss tosses[5];                             // +0x110
    uint32_t t0 = 0;                                 // +0xbb0: the last toss
    unsigned shots = 0;                              // +0xbb4
    PearlInterface iface;                            // +0xbb8
    int info = 0;                                    // +0x1768
    uint32_t interval = 0;                           // +0x176c
    std::vector<int> kind;                           // +0x1774: 0 a pearl, 1 a miss
    std::vector<int> free_;                          // +0x1784: the trees with no toss on
    Vision::Mouse mouse;                             // +0x1790
    Vision::Keyboard keys;                           // +0x17c4
    int no_catch = 0;                                // +0x17f8: tosses since the last catch
    Inactivity inactivity;                           // +0x17fc "PERL_VO_CUBprompt"
    Raytrace::Sprite3D *side_toss = nullptr;         // +0x1b9c
    Raytrace::Sprite3D *side_idle = nullptr;         // +0x1ba0
};

}  // namespace Game
