// The pixie dust lift, reconstructed from "Tinker Bell.exe" (see dustlift.h).
#include "dustlift.h"
#include "../vision/engine.h"

namespace Game {

using Raytrace::Sprite3D;

DustLift::DustLift(Raytrace::RayTrace *r) : rt(r) {}   // 0x4befe0 (+0x584 gets the low byte of rt: unused)

DustLift::~DustLift() { excluded.clear(); }          // 0x4bf030

// 0x4beea0: up for 3 s, hold 5 s, down for 3 s (the time stepped by at most 200 ms); undone when the sprite's
// box would swallow the camera.
void DustLift::Slot::step(uint32_t dt, Raytrace::RayTrace *rt) {
    uint32_t d = dt;
    if (d > 200) d = 200;
    elapsed += d;
    uint32_t dur;
    switch (state) {
    case 0: dur = 3000; break;
    case 1: dur = 5000; break;
    case 2: dur = 3000; break;
    default: dur = dt; break;
    }
    double t = (double)(uint64_t)elapsed / (int32_t)dur;   // fild qword / fidiv
    if (!(t <= 1.0f)) t = 1.0f;                      // 0x513710
    float saved = sprite->y;
    switch (state) {
    case 0: sprite->y = (float)(((double)y1 - y0) * t + y0); break;
    case 1: sprite->y = y1; break;
    case 2: sprite->y = (float)(((double)y1 - y0) * (1.0f - t) + y0); break;
    default: break;
    }
    if (rt->cameraInBox(sprite)) {                   // 0x429710
        elapsed -= d;
        sprite->y = saved;
        return;
    }
    if (elapsed < dur) return;
    elapsed = 0;
    switch (state) {
    case 0: state = 1; break;
    case 1: state = 2; break;
    case 2: state = 3; y1 = y0; break;
    default: break;
    }
}

void DustLift::update() {                            // 0x4bf070
    uint32_t dt = Vision::frame_time() - last;
    if (dt < min_gap) return;
    for (Slot &s : slots)
        if (s.state != 3) s.step(dt, rt);
}

// 0x4bf0b0: only a sprite that can be hit (+0x90), not excluded and not already lifting; the first free slot.
void DustLift::add(Sprite3D *s, int height) {
    if (!s->f90) return;
    for (Sprite3D *e : excluded)
        if (e == s) return;
    for (const Slot &t : slots)
        if (t.state != 3 && t.sprite == s) return;
    for (Slot &t : slots) {
        if (t.state != 3) continue;
        t.sprite = s;
        t.y0 = s->y;
        t.height = height;
        t.y1 = (float)((double)height + t.y0);
        t.state = 0;
        t.elapsed = 0;
        t.saved_flag = s->box_flag;
        s->box_flag = true;
        return;
    }
}

const DustLift::Slot *DustLift::find(Sprite3D *s) const {   // the loop of 0x4bf160 / 0x4bf1b0 / 0x4bf200
    for (const Slot &t : slots)
        if (t.state != 3 && t.sprite == s) return &t;
    return nullptr;
}

}  // namespace Game
