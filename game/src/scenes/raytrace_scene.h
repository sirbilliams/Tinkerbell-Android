// Raytrace_Scene (vtable 0x5162f4): the base of every 3D scene (London, the Neverland areas), reconstructed from
// "Tinker Bell.exe". It owns the RayTrace screen (+0x140), the three packs (<name>_walls / _sprites / _ground),
// the flying physics (v29) and a MovementInputTask (vtable 0x5162c4) that reads the arrow keys each frame.
// Work in progress: TinkerbellRaytraceScene (0x516380, Tink, the toolbar) and London_Scene's own run are not in
// yet; London is entered as a plain Raytrace_Scene for now.
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"
#include "../vision/faderect.h"
#include "../raytrace/mover.h"

namespace Game {

class Raytrace_Scene;
class Tinkerbell3D;

// The scene's path planner (+0xf8, 0x4e9280..): obstacles made from the sprite boxes (+0x110 list, 0x4aa880) and
// the waypoints of the last path (+0x118 count, +0x120 array). A path is straight, or bends once around an
// obstacle in the way.
struct PathPlanner {
    struct Obstacle {                                // 0x48 bytes (0x4e9070)
        std::string name;                            // +0x08
        float r = 0, r2 = 0;                         // +0x30 +0x34: radius (0.6 of the given size) and its square
        float x = 0, y = 0, z = 0;                   // +0x38 +0x3c +0x40
        float h = 0;                                 // +0x44: height
        bool blocks(const float seg[6], bool from_start) const;   // 0x4e8de0
    };
    struct Point { float x, y, z; };
    float a[3] = {}, b[3] = {};                      // +0x00 +0x0c
    std::vector<Obstacle> obstacles;                 // +0x18
    std::vector<Point> points;                       // +0x20.. (count, capacity, array)
    void add(const char *name, float x, float y, float z, float size, float h);   // 0x4e9070
    bool blocked(const float to[3]);                 // 0x4e91e0: from the last waypoint
    bool findPath(float ax, float ay, float az, float bx, float by, float bz);   // 0x4e9280
    void straight(float ax, float ay, float az, float bx, float by, float bz);   // 0x4e9380
};

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
    void notify2(Vision::Service *s) override;       // v9 0x4a9690: the mouse -> v25, the keyboard -> v26
    virtual void spriteFrames();                     // v22 0x4a9a70: the sprites' animation frames
    virtual void forceNone();                        // v23 0x4aa950
    virtual void forceOff();                         // v24 0x4aa980
    virtual void onMouse() {}                        // v25 (0x4ef760 in the base): a mouse event, or a frame with a key held
    virtual bool onKey() { return false; }           // v26 0x4a9680
    virtual void open(const char *name);             // v27 0x4a9ca0
    virtual void initPhysics();                      // v28 0x4a96d0
    virtual void physics();                          // v29 0x4a9760
    virtual void cursorUpdate() {}                   // v30 (0x4aa100 returns the cursor; the scenes override)
    virtual bool isFlying();                         // v31 0x4a9a50: eye above 5
    void buildObstacles();                           // 0x4aa880

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
    PathPlanner planner;                             // +0xf8
    Vision::Keyboard keys;                           // +0x12f4
    Vision::Mouse mouse;                             // +0x1328
    Vision::Screen over1, over2;                     // +0x14e0 +0x155c: drawn over the RayTrace
    Vision::Image cursor_img;                        // +0x15d8 "glob_cu_normal_01"
};

// Global3DTask (vtable 0x516414, 0x4abfd0): every frame unless the game is paused (state +0x64): the sprites'
// frames, the cursor, Tink, the hover.
class Global3DTask : public Vision::Task {
public:
    explicit Global3DTask(class TinkerbellRaytraceScene *s) : scene(s) {}
    void run() override;
    class TinkerbellRaytraceScene *scene;            // +0xc0
};

// TinkerbellRaytraceScene (vtable 0x516380): a Raytrace_Scene with Tink in it.
// port: so far Tink, the Global3DTask and the hover; the toolbar, help bell, cloud banks and 0x4befe0 come next.
class TinkerbellRaytraceScene : public Raytrace_Scene {
public:
    explicit TinkerbellRaytraceScene(const char *name) : Raytrace_Scene(name) {}
    ~TinkerbellRaytraceScene() override;
    void open(const char *name) override;            // v27 0x4ab0f0
    void leave() override;                           // v20 0x4ab470
    void forceNone() override;                       // v23 0x4abf30
    void forceOff() override;                        // v24 0x4abf50
    virtual void hover();                            // v32 0x443820: the eye eases down (or up to 200)
    void unforce() { forced_off = false; }           // v34 0x4abf70
    void allowUpDown() { force[4] = force[5] = 0; }  // 0x4aa9b0 (Raytrace_Scene's)
    bool flyThroughClouds(bool second);              // 0x4ab920: up through the cloud bank; true while flying
    bool stateKeys();                                // 0x4ec5f0 (the state's, on this scene): Enter, Shift, Ctrl
    Tinkerbell3D *tink = nullptr;                    // +0x2744
    Global3DTask *global_task = nullptr;             // +0x2758
    uint32_t last_hover = 0;                         // +0x2754
    Vision::Ras *tinkpack = nullptr, *global3d = nullptr;   // +0x17d0 +0x17d4
    bool forced_off = false;                         // +0x17c8: the last of v23 / v24 was v24
    bool busy = false;                               // +0x28e0: a scene's intro runs (London: the fade-in)
    Vision::FadeRect dim;                            // +0x2838: full screen, depth -1000
    Raytrace::Sprite3D *cloud1 = nullptr, *cloud2 = nullptr;   // +0x2748 +0x274c (the same sprite)
    bool flying = false;                             // +0x2750
    Raytrace::CameraTween tween;                     // +0x16c0
};

}  // namespace Game
