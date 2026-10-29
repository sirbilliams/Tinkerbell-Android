// The pixie dust lift (0x594 bytes, no vtable; ctor 0x4befe0, TinkerbellRaytraceScene +0x2740), reconstructed from
// "Tinker Bell.exe": a sprite sprinkled with dust floats up, hangs and sinks back. Fifty slots; Global3DTask runs
// update (0x4bf070) every frame.
//   add 0x4bf0b0, rising 0x4bf160, floating 0x4bf1b0, sinking 0x4bf200, slot step 0x4beea0, release 0x4bf030.
#pragma once
#include "../raytrace/raytrace.h"
#include <vector>

namespace Game {

class DustLift {
public:
    struct Slot {                                    // 0x1c bytes (ctor 0x4bee80)
        Raytrace::Sprite3D *sprite = nullptr;        // +0x00
        int state = 3;                               // +0x04: 0 rising, 1 floating, 2 sinking, 3 free
        uint32_t elapsed = 0;                        // +0x08
        float y0 = 0, y1 = 0;                        // +0x0c +0x10: from, to
        bool saved_flag = false;                     // +0x14: the sprite's box flag before
        int height = 0;                              // +0x18
        void step(uint32_t dt, Raytrace::RayTrace *rt);   // 0x4beea0
    };
    explicit DustLift(Raytrace::RayTrace *rt);       // 0x4befe0
    ~DustLift();                                     // 0x4bf030
    void update();                                   // 0x4bf070
    void add(Raytrace::Sprite3D *s, int height);     // 0x4bf0b0
    bool rising(Raytrace::Sprite3D *s) const { const Slot *t = find(s); return t && t->state == 0; }    // 0x4bf160
    bool floating(Raytrace::Sprite3D *s) const { const Slot *t = find(s); return t && t->state == 1; }  // 0x4bf1b0
    bool sinking(Raytrace::Sprite3D *s) const { const Slot *t = find(s); return t && t->state == 2; }   // 0x4bf200
    std::vector<Raytrace::Sprite3D *> excluded;      // +0x588..+0x590: never lifted (nothing adds to it here)

    // port: +0x0 is the time of the last update, but neither the ctor nor anything else ever writes it (the
    // object comes from operator new); taken as 0, so every update sees a huge gap and steps by the 200 ms cap.
    uint32_t last = 0;                               // +0x00
    uint32_t min_gap = 30;                           // +0x04
    Slot slots[50];                                  // +0x08
    Raytrace::RayTrace *rt;                          // +0x580
private:
    const Slot *find(Raytrace::Sprite3D *s) const;
};

}  // namespace Game
