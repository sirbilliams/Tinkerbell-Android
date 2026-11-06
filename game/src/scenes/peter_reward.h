// Peter's two visits after an area's games, reconstructed from "Tinker Bell.exe" (both tasks, made by the area
// scenes, run while the scene waits for them):
//   PeterCapReward (vtable 0x516fa8, 0x104 bytes, ctor 0x4bf280, dtor 0x4bf3b0, setup 0x4bf480, capStep 0x4bf990,
//                  run v10 0x4bfa60): every game won twice: in the dark Peter comes, the cap flies to the player.
//   PeterPrompt    (vtable 0x516fdc, 0x114 bytes, ctor 0x4bfea0, dtor 0x4bff80, setup 0x4c00c0, notify2 v9 0x4c0040,
//                  run v10 0x4c03c0): all five map segments: Peter tells to open the map (modal until it is).
// Both bring their own Peter ("peterPan" from the "Peter_Char" pack) when the scene gives none.
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"

namespace Game {

class SceneCharacter;
class TinkerbellRaytraceScene;
class DustLift;

class PeterCapReward : public Vision::Task {
public:
    // 0x4bf280: the scene's Peter (or null), the scene, the area (its spot 0 for the camera), the lift
    PeterCapReward(SceneCharacter *peter, TinkerbellRaytraceScene *scene, int area, DustLift *lift);
    ~PeterCapReward() override;                      // 0x4bf3b0
    void setup();                                    // 0x4bf480
    bool capStep();                                  // 0x4bf990: along the curve for a second
    void run() override;                             // v10 0x4bfa60
    SceneCharacter *own = nullptr;                   // +0xc0
    SceneCharacter *peter;                           // +0xc4
    Vision::Ras *pack = nullptr;                     // +0xc8
    DustLift *lift;                                  // +0xcc
    TinkerbellRaytraceScene *scene;                  // +0xd0
    Raytrace::RayTrace *rt;                          // +0xd4
    int area;                                        // +0xd8
    float cam[3] = {}, cam_deg = 0;                  // +0xdc..+0xe8: the camera before
    float peter_pos[3] = {};                         // +0xec..+0xf4: Peter's place before
    Raytrace::Sprite3D *cap = nullptr;               // +0xf8 "reward cap"
    uint32_t t0 = 0;                                 // +0xfc
    Raytrace::Curve3 *curve = nullptr;               // +0x100
};

class PeterPrompt : public Vision::Task {
public:
    PeterPrompt(SceneCharacter *peter, TinkerbellRaytraceScene *scene, int area, DustLift *lift);   // 0x4bfea0
    ~PeterPrompt() override;                         // 0x4bff80
    void setup();                                    // 0x4c00c0
    void notify2(Vision::Service *s) override;       // v9 0x4c0040: the map icon clicked: to the treasure map
    void run() override;                             // v10 0x4c03c0
    Vision::Mouse mouse;                             // +0xc0
    SceneCharacter *own = nullptr;                   // +0xf4
    SceneCharacter *peter;                           // +0xf8
    Vision::Ras *pack = nullptr;                     // +0xfc
    TinkerbellRaytraceScene *scene;                  // +0x100
    Raytrace::RayTrace *rt;                          // +0x104
    int area;                                        // +0x108
    uint32_t said = 0;                               // +0x10c
    DustLift *lift;                                  // +0x110
};

}  // namespace Game
