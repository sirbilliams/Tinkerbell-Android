// The 3D scenes' corner buttons, reconstructed from "Tinker Bell.exe" (members of TinkerbellRaytraceScene):
//   HelpBellIcon     vtable 0x51799c (+0x1fe8)  ctor 0x4e6df0, dtor 0x4e6f00, hit v11 0x4e7210, draw v14 0x4e6ff0,
//                    load v15 0x4e7080, unload v16 0x4e7120, click 0x4e7180
//   NeverlandMapIcon vtable 0x517bf0 (+0x19a0)  ctor 0x4ef8e0, hit v11 0x4efba0, draw v14 0x4efbe0,
//                    load v15 0x4efae0, unload v16 0x4efb50, click 0x4efc90
//   PixieDustBag     vtable 0x517c3c (+0x1998, new'd by the scene's open)  ctor 0x4efd50, dtor 0x4f0000,
//                    hit v11 0x4f0170, draw v14 0x4f01d0, has 0x4f02d0, click 0x4f02f0, sprinkle 0x4f0480,
//                    refills 0x4f0570 / 0x4f05f0 / 0x4f0670 / 0x4f0770, refund 0x4f07b0, spend 0x4f0860,
//                    cost 0x4f08c0, regen 0x4f08f0 (every frame from Global3DTask)
// Each one plays a line of Peter's when the mouse rests on it for a second (PauseOver).
#pragma once
#include "acts.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"

namespace Game {

class HelpBellIcon : public Graphic {
public:
    HelpBellIcon();                                  // 0x4e6df0
    ~HelpBellIcon() override;                        // 0x4e6f00
    bool hit(int x, int y) override;                 // v11 0x4e7210
    void draw() override;                            // v14 0x4e6ff0
    void load() override;                            // v15 0x4e7080
    void unload() override;                          // v16 0x4e7120
    bool click();                                    // 0x4e7180: a click on it rings it
    Image idle, rov;                                 // +0x98 +0x140 (their +0x7c flags say which shows)
    Animation sel;                                   // +0x1e8 "GLOB_BU_BELsel_"
    Sound ring;                                      // +0x300 "GLOB_FX_BELring"
    PauseOver pause;                                 // +0x648 "GLOB_VO_PETbellpo"
};

class NeverlandMapIcon : public Graphic {
public:
    NeverlandMapIcon();                              // 0x4ef8e0
    ~NeverlandMapIcon() override;
    bool hit(int x, int y) override;                 // v11 0x4efba0
    void draw() override;                            // v14 0x4efbe0
    void load() override;                            // v15 0x4efae0
    void unload() override;                          // v16 0x4efb50
    int click(int x, int y, bool play);              // 0x4efc90: 0 missed, 1 played its line, 2 too soon
    Image idle, rov;                                 // +0x98 +0x140
    Sound vo;                                        // +0x1e8 "DEMO_VO_PETmap1"
    uint32_t last = 0;                               // +0x534
    PauseOver pause;                                 // +0x538 "GLOB_VO_PETmap"
};

class PixieDustBag : public Graphic {
public:
    PixieDustBag();                                  // 0x4efd50
    ~PixieDustBag() override;                        // 0x4f0000
    bool hit(int x, int y) override;                 // v11 0x4f0170
    void draw() override;                            // v14 0x4f01d0
    bool has(int n) const { return *dust >= n; }     // 0x4f02d0
    bool click();                                    // 0x4f02f0: dust on / off the cursor (or "fill the bag")
    // 0x4f0480: with dust on the cursor and the button down: what was sprinkled (the sprite, and the
    // attachment), the throw sound, the dust off the cursor, the game cursor back.
    void sprinkle(Raytrace::RayTrace *rt, Raytrace::Sprite3D **sprite, Raytrace::Attachment **att);
    void refillSmall();                              // 0x4f0570: +15 / +12 / +10 by skill
    void refillLarge();                              // 0x4f05f0: +30 / +24 / +20
    void refillRandom();                             // 0x4f0670: +random(5) + 4 / 3 / 2
    void fill();                                     // 0x4f0770: +50
    void refund();                                   // 0x4f07b0: +2 / +3 / +4 (the cost of a throw)
    void spend();                                    // 0x4f0860: the puff, -4 / -5 / -6
    static int cost2();                              // 0x4f08c0: 4 / 5 / 6
    void regen(uint32_t unknown);                    // 0x4f08f0: +2 every 15 / 17 / 19 s
    Image *level[5] = {};                            // +0x98: the bag from empty to full (embedded, 0xa8 each)
    AnimationHelper *dustin = nullptr;               // +0x3e0 "glob_if_dustin_": dust going in
    Animation puff;                                  // +0x3e8 "glob_bu_bagpffsel_" (its +0x7c: playing)
    uint32_t regen_time = 0;                         // +0x500
    uint32_t puff_time = 0;                          // +0x508
    int *dust;                                       // +0x50c: the state's +0x3c8
    PauseOver pause;                                 // +0x510 "GLOB_VO_PETdstbag"
private:
    void add(int n);                                 // the inlined clamp to 0..50 (0x4efd30)
    int levelIndex() const;                          // (dust + 4) / 10, at most 4
};

}  // namespace Game
