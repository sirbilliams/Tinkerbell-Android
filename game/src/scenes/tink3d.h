// Tinkerbell3D (vtable 0x517550, ctor 0x4da4a0) and her commands, reconstructed from "Tinker Bell.exe": Tink
// flying near the camera in the 3D scenes. A queue of commands (an MSVC deque at +0x54, count +0x7c) drives her;
// when it runs dry she wanders near the view or plays an idle, leaving a trail of dust sprites.
#pragma once
#include "../raytrace/character.h"
#include "../vision/sound.h"
#include <deque>

namespace Game {

class Raytrace_Scene;
class Tinkerbell3D;

// Tinkerbell3DCommand (vtable 0x517604): +4 an id from the counter 0x542928.
struct TinkCommand {
    TinkCommand();
    virtual ~TinkCommand() {}
    virtual bool update(uint32_t dt, Tinkerbell3D *t) = 0;   // v1: false when finished
    virtual void step(Tinkerbell3D *) {}            // v2 (0x402b80: nothing)
    int id;                                          // +0x04
};

// Tinkerbell3DCommandBasic (0x51766c, made by 0x4de020): 0 play the root loop once, 1 face the camera, 2 fly to
// (x, y, z) along the scene's path, 3 wait until the camera is within x or y ms pass, 4 nothing.
struct TinkCmdBasic : TinkCommand {
    TinkCmdBasic(Raytrace_Scene *s, int type, float x, float y, float z);   // 0x4de020
    ~TinkCmdBasic() override;
    bool update(uint32_t dt, Tinkerbell3D *t) override;   // 0x4dd3b0
    void step(Tinkerbell3D *t) override;             // 0x4ddb70: the idle facing the right way
    void startPath(Tinkerbell3D *t);                 // 0x4dd710
    bool flyTo(Tinkerbell3D *t);                     // 0x4dd8f0
    int type;                                        // +0x08
    float x, y, z;                                   // +0x0c +0x10 +0x14
    Raytrace_Scene *scene;                           // +0x18
    int waypoint = 0;                                // +0x1c (-1 for type 2 until the path is made)
    bool started = false;                            // +0x20
    uint32_t t0 = 0;                                 // +0x24
    Vision::Sound *fly = nullptr;                    // +0x28 "GLOB_FX_TNKfly1"
};

// Tinkerbell3DCommandAnimation (0x517624, ctor 0x4dc530) and the idles built on it: turn to the camera, then
// play <prefix> (with its sound) to the last frame.
struct TinkCmdAnimation : TinkCommand {
    TinkCmdAnimation(const char *prefix, const char *sound);   // 0x4dc530
    ~TinkCmdAnimation() override;
    bool update(uint32_t dt, Tinkerbell3D *t) override;   // 0x4dc660
    const char *prefix;                              // +0x08
    bool started = false;                            // +0x0c
    Vision::Sound *snd = nullptr;                    // +0x10
};

// Tinkerbell3DCommandPoint (0x5175f4, made by 0x4db7b0): turn to the camera, then point at (x, y, z) (up, down,
// east or west) to the animation's end.
struct TinkCmdPoint : TinkCommand {
    TinkCmdPoint(float x, float y, float z);         // 0x4db7b0 (part)
    ~TinkCmdPoint() override;                        // 0x4dce60
    bool update(uint32_t dt, Tinkerbell3D *t) override;   // 0x4dcef0
    void pointAnim(Tinkerbell3D *t);                 // 0x4dd1d0
    float x, y, z;                                   // +0x08 +0x0c +0x10
    bool started = false;                            // +0x14
    Vision::Sound *fly = nullptr;                    // +0x18 "GLOB_FX_TNKfly1"
};

class Tinkerbell3D : public Raytrace::RayTraceCharacter {
public:
    Tinkerbell3D(Raytrace::RayTrace *rt, Raytrace_Scene *scene);   // 0x4da4a0
    ~Tinkerbell3D() override;                        // 0x4da590
    void init() override;                            // v1 0x4da790
    void release() override;                         // v2 0x4da8b0
    void update();                                   // 0x4daf30: every frame (from Global3DTask)
    void push(TinkCommand *c) { queue.push_back(c); }   // 0x4de400 / 0x4de160
    int pushBasic(int type, float x, float y, float z);   // 0x4de020
    void point(float x, float y, float z);           // 0x4db7b0: a TinkCmdPoint queued
    double facing();                                 // 0x4db110: the heading to the camera (degrees)
    bool turnToward(float deg);                      // 0x4dc3d0: up to 15 degrees; true when within 10
    bool idle() const { return queue.empty(); }      // 0x4de010 (on +0x50): no command left
    void wave();                                     // 0x4dbd90: a Tinkerbell3DCommandWave ("GLOB_CA_TNKwave_")
    void say(const char *line);                      // 0x4cca20: plays a voice line (or her sound) as no one
    void moreDust();                                 // 0x4db1a0: new dust along the way flown
    bool wanders = true;                             // +0x13c
    float heading = 270.0f;                          // +0x140
    float vx = 0, vy = 0, vz = 0;                    // +0x124 +0x128 +0x12c
    Raytrace_Scene *scene;                           // +0x84 / +0x88
private:
    void initDust();                                 // 0x4da980
    void dust(uint32_t dt);                          // 0x4db3a0: the dust rises and goes
    void move(uint32_t dt);                          // 0x4db470
    void wander();                                   // 0x4dad40
    void randomIdle();                               // 0x4daa20
    std::deque<TinkCommand *> queue;                 // +0x54..+0x7c
    uint32_t last = 0, last_dust = 0;                // +0x8c +0x90
    Raytrace::Sprite3D *dust_[35] = {};              // +0x98
    float dust_x = 0, dust_y = 0, dust_z = 0;        // +0x130 +0x134 +0x138
    uint32_t last_idle = 0;                          // +0x1ac
};

}  // namespace Game
