// Raytrace_Scene (vtable 0x5162f4): the base of every 3D scene (London, the Neverland areas), reconstructed from
// "Tinker Bell.exe". It owns the RayTrace screen (+0x140), the three packs (<name>_walls / _sprites / _ground),
// the flying physics (v29) and a MovementInputTask (vtable 0x5162c4) that reads the arrow keys each frame.
// Work in progress: TinkerbellRaytraceScene (0x516380, Tink, the toolbar) and London_Scene's own run are not in
// yet; London is entered as a plain Raytrace_Scene for now.
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"

namespace Game {

class Raytrace_Scene;

// 0x4aaa30: every frame, the keys into the scene's six flags (+0xe0..+0xf4 can force each off (1) or on (2)),
// opposite keys cancel, the physics step (v29), and v25 + the activity time when anything was held.
class MovementInputTask : public Vision::Task {
public:
    explicit MovementInputTask(Raytrace_Scene *s) : scene(s) {}
    void run() override;
    Raytrace_Scene *scene;                          // +0xc0
};

class Raytrace_Scene : public Vision::Scene {
public:
    explicit Raytrace_Scene(const char *name);       // 0x4a9d.. (ctor ~0x4aa240)
    ~Raytrace_Scene() override;
    void run() override;                             // v10 0x4a9be0
    void leave() override;                           // v20 0x4aa110
    virtual void spriteFrames();                     // v22 0x4a9a70: the sprites' animation frames
    virtual void forceNone();                        // v23 0x4aa950
    virtual void forceOff();                         // v24 0x4aa980
    virtual void onMove() {}                         // v25 (0x4ef760 in the base: nothing to port yet)
    virtual void open(const char *name);             // v27 0x4a9ca0
    virtual void initPhysics();                      // v28 0x4a96d0
    virtual void physics();                          // v29 0x4a9760
    virtual void cursorUpdate() {}                   // v30 (0x4aa100 returns the cursor; the scenes override)
    virtual bool isFlying();                         // v31 0x4a9a50: eye above 5

    Raytrace::RayTrace rt;                           // +0x140
    Vision::Ras *walls = nullptr, *sprites = nullptr, *ground = nullptr;   // +0x130 +0x134 (+0x4e8 of rt)
    MovementInputTask *input = nullptr;              // +0x12c
    float cam_x = 3000.1f, cam_eye = 0.1f, cam_z = 3000.1f, cam_deg = 90.1f;   // +0xcc..+0xd8
    uint32_t opened = 0, active_time = 0;            // +0xdc
    int force[6] = {};                               // +0xe0..+0xf4: fwd, back, left, right, up, down
    bool paused = false;                             // +0x128
    // +0x1684..: the keys held: forward, back, left, right, up (Shift), down (Ctrl)
    bool fwd = false, back = false, left = false, right = false, up = false, down = false;
    float speed = 0, max_speed = 400, vspeed = 0, max_vspeed = 200, turn_rate = 40;   // +0x168c..+0x169c
    uint32_t last_step = 0;                          // +0x16a0
    float accel = 400, brake = 400, up_accel = 700, down_accel = 500, gravity = 100;  // +0x16a4..+0x16b4
    float friction = 8, friction_flying = 6;         // +0x16b8 +0x16bc
};

// "london" (London_Scene@LondonScene, vtable 0x516a3c). port: for now only the 3D street itself: the
// TinkerbellRaytraceScene layer (0x4ab0f0) and London's enter (0x4b5800) / run (0x4b5300) come next.
class London_Scene : public Raytrace_Scene {
public:
    London_Scene() : Raytrace_Scene("london") {}
    void enter() override;
};

}  // namespace Game
