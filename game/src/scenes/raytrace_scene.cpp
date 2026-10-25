// Raytrace_Scene and MovementInputTask, reconstructed from "Tinker Bell.exe" (see raytrace_scene.h).
#include "raytrace_scene.h"
#include "tink3d.h"
#include "state.h"
#include <string.h>
#include <math.h>
#include <string>
#include "tb.h"
#include "../../../platform/dinput.h"

namespace Game {
using namespace Vision;

static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0 (GetAsyncKeyState's top bit)

// ---------------------------------------------------------------- MovementInputTask
void MovementInputTask::run() {                      // 0x4aaa30
    wait(1);
    for (;;) {
        wait(0);
        Raytrace_Scene *s = scene;
        s->fwd = s->back = s->left = s->right = s->up = s->down = false;
        if (key_down(0x27)) s->right = true;         // VK_RIGHT
        if (key_down(0x25)) s->left = true;          // VK_LEFT
        if (key_down(0x26)) s->fwd = true;           // VK_UP
        if (key_down(0x28)) s->back = true;          // VK_DOWN
        if (key_down(0x10)) s->up = true;            // VK_SHIFT
        if (key_down(0x11)) s->down = true;          // VK_CONTROL
        bool *k[6] = { &s->fwd, &s->back, &s->left, &s->right, &s->up, &s->down };
        for (int i = 0; i < 6; i++) {
            if (s->force[i] == 1) *k[i] = false;
            if (s->force[i] == 2) *k[i] = true;
        }
        if (s->left && s->right) s->left = s->right = false;
        if (s->fwd && s->back) s->fwd = s->back = false;
        if (s->up && s->down) s->up = s->down = false;
        s->physics();                                // v29
        if (s->fwd || s->back || s->up || s->down || s->left || s->right) {
            s->onMove();                             // v25
            s->active_time = frame_time();
        }
    }
}

// ---------------------------------------------------------------- Raytrace_Scene
Raytrace_Scene::Raytrace_Scene(const char *name) : Scene(name) {}
Raytrace_Scene::~Raytrace_Scene() {                 // port: quitting inside the scene frees its packs too
    delete input;
    if (walls) ras_close(walls);
    if (sprites) ras_close(sprites);
    if (ground) ras_close(ground);
}

void Raytrace_Scene::initPhysics() {                 // v28 0x4a96d0
    paused = false;
    speed = 0;
    max_speed = 400.0f;
    vspeed = 0;
    max_vspeed = 200.0f;
    last_step = real_time();
    turn_rate = 40.0f;
    accel = 400.0f;
    brake = 400.0f;
    up_accel = 700.0f;
    down_accel = 500.0f;
    gravity = 100.0f;
    friction = 8.0f;
    friction_flying = 6.0f;
}

bool Raytrace_Scene::isFlying() { return rt.eyef > 5.0f; }   // v31 0x4a9a50 (0x514c84)

// v29 0x4a9760: one step of the flying physics, at most every 20 ms (dt capped at 200 ms).
void Raytrace_Scene::physics() {
    if (paused) { speed = 0; vspeed = 0; return; }
    uint32_t now = real_time();
    uint32_t ms = now - last_step;
    if (ms < 20) return;
    if (ms > 200) ms = 200;
    last_step = now;
    float dt = (float)((double)(int64_t)ms * 0.00100000005f);   // 0x513294
    if (!(speed < 0.0f)) rt.moveForward((float)((double)dt * speed));
    else rt.moveBack((float)-((double)dt * speed));
    if (!(vspeed < 0.0f)) {
        rt.raise((float)((double)dt * vspeed));
    } else {
        float h0 = rt.eyef;
        float d = (float)-((double)dt * vspeed);
        rt.lower(d);
        double moved = fabs((double)rt.eyef - h0);
        if (!(fabs((double)d * 0.25f) <= moved) && !(fabs((double)d) <= 1.0)) vspeed = 0;   // landed
    }
    if (right) rt.turn((float)-((double)dt * turn_rate));
    if (left) rt.turn((float)((double)dt * turn_rate));
    float fr = isFlying() ? friction_flying : friction;
    if (!fwd && !back) {
        speed = (float)((double)speed + -((double)fr * dt * speed));
        if (!((double)max_speed * 0.100000001f <= fabs((double)speed))) speed = 0;   // 0x5162a0
    }
    if (fwd) speed = (float)((double)dt * accel + speed);
    if (back) speed = (float)((double)speed - (double)dt * brake);
    if (speed > max_speed) speed = max_speed;
    if (-max_speed > speed) speed = -max_speed;
    if (!up && !down) vspeed = 0;
    if (up) vspeed = (float)((double)dt * up_accel + vspeed);
    if (down) vspeed = (float)((double)vspeed - (double)dt * down_accel);
    vspeed = (float)((double)vspeed - (double)dt * gravity);
    if (vspeed > max_vspeed) vspeed = max_vspeed;
    if (-max_vspeed > vspeed) vspeed = -max_vspeed;
}

// v22 0x4a9a70: each sprite's frame: mode 0 the first, 1 stepped every frame_ms, 2 by the angle it is seen from.
void Raytrace_Scene::spriteFrames() {
    uint32_t now = real_time();
    for (size_t i = 0; i < rt.sprites.size(); i++) {
        Raytrace::Sprite3D *s = rt.sprites[i];
        if (s->mode == 0) s->anim->setFrame(0);
        else if (s->mode == 1) {
            if (s->frame_ms <= now - s->last) { s->anim->step(); s->last = now; }
        } else if (s->mode == 2) {
            double dx = (double)s->x - rt.camx;
            double dz = (double)s->z - rt.camz;
            double a = atan2(dz, dx) * 57.295827908797776;   // 0x5162b8
            int n = s->anim->count();
            double f = ((double)s->f8c + a) - 90.0 - 360.0 / n * 0.5;   // 0x5162b0, 0x5162a8, 0x513d10
            while (f < 0.0) f += 360.0;
            while (!(f < 360.0)) f -= 360.0;
            s->anim->setFrame((int)f * n / 361);
        }
        uint32_t t = frame_time();                   // 0x42f270: attached animations
        for (Raytrace::Attachment *a = s->attached; a; a = a->next) {
            if (a->period == -1 || (uint32_t)a->period > t - a->last) continue;
            if (Animation *an = dynamic_cast<Animation *>(a->g)) an->step();
            a->last = t;
        }
    }
}

void Raytrace_Scene::forceNone() { for (int &f : force) f = 0; }   // v23
void Raytrace_Scene::forceOff() { for (int &f : force) f = 1; }    // v24

// v27 0x4a9ca0: the packs, the RayTrace (walls, sprites, ground), the input task and the camera.
void Raytrace_Scene::open(const char *nm) {
    if (!nm) { vision_log("Raytrace_Scene::open() NEEDS NAME"); return; }
    opened = frame_time();                           // +0xdc
    forceNone();                                     // +0xe0..+0xf4 cleared
    // port: the mouse/keyboard/joystick services (+0x12c0/+0x12f4/+0x1328) are global in this port, and the
    // 0x158-byte graphic 0x4a9410 (+0x124) is not ported yet.
    initPhysics();                                   // v28
    input = new MovementInputTask(this);             // +0x12c
    input->activate();
    std::string n(nm);
    walls = ras_open((n + "_walls").c_str(), "", &data_device());
    walls->load();
    ras_set_current(walls);
    sprites = ras_open((n + "_sprites").c_str(), "", &data_device());
    sprites->load();
    activate();
    reset();                                         // 0x421110
    rt.activate();
    ras_set_current(walls);
    rt.load(nm);                                     // 0x4281a0
    ras_set_current(sprites);
    rt.loadSprites(nm, sprites);                     // 0x4aa570
    ground = ras_open((n + "_ground").c_str(), "", &data_device());   // 0x42bc60
    ground->load();
    ras_set_current(ground);
    rt.loadGround(("data\\" + n + ".grn").c_str());  // 0x42cc80
    rt.loadAll();
    tb_input_set_arrows(true);                       // port: the pad as the arrow keys (platform/dinput.h)
    rt.setCamera(cam_x, cam_eye, cam_z, cam_deg);    // 0x42a570
    buildObstacles();                                // 0x4aa880
}

// ---------------------------------------------------------------- the path planner
// 0x4aa880: an obstacle for each box: its centre, size = twice the larger of its x/z extents, its y extent.
void Raytrace_Scene::buildObstacles() {
    planner.points.clear();                          // 0x4e8c20
    planner.obstacles.clear();                       // 0x4e9040
    for (Raytrace::Box *b : rt.boxes) {
        float cx = (float)(((double)b->minx + b->maxx) * 0.5f);
        float cy = (float)(((double)b->y0 + b->y1) * 0.5f);
        float cz = (float)(((double)b->minz + b->maxz) * 0.5f);
        float hy = (float)fabs((double)b->y1 - b->y0);
        float dz = (float)fabs((double)b->maxz - b->minz);
        double dx = fabs((double)b->maxx - b->minx);
        double m = (double)dz < dx ? dx : (double)dz;
        planner.add(b->name.c_str(), cx, cy, cz, (float)(m + m), hy);
    }
}

void PathPlanner::add(const char *n, float px, float py, float pz, float size, float hh) {   // 0x4e9070
    Obstacle o;
    o.name = n;
    float r = (float)((double)size * 0.600000024f);  // 0x517b74
    o.r = r;
    o.r2 = (float)((double)r * r);
    o.x = px; o.y = py; o.z = pz;
    o.h = hh;
    obstacles.push_back(o);
}

// 0x4e8de0: does the segment seg = (start, end) pass through the obstacle (its circle and height band)?
bool PathPlanner::Obstacle::blocks(const float seg[6], bool from_start) const {
    float ax = seg[0], ay = seg[1], az = seg[2], bx = seg[3], by = seg[4], bz = seg[5];
    if (from_start) {
        double ex = (double)ax - x, ez = (double)az - z;
        if (r2 > ez * ez + ex * ex && h > fabs((double)ay - y)) return true;
    }
    {
        double ex = (double)bx - x, ez = (double)bz - z;
        if (r2 > ez * ez + ex * ex && h > fabs((double)by - y)) return true;
    }
    double ox = (double)x - ax, oz = (double)z - az;
    float dist = (float)sqrt(oz * oz + ox * ox);
    float dx = (float)((double)bx - ax);
    float dy = (float)((double)by - ay);
    float dz = (float)((double)bz - az);
    float len = (float)sqrt((double)dz * dz + (double)dx * dx);
    if (len == 0.0f) {
        if (dy == 0.0f) return false;
        float step = (float)((double)dy * 0.0500000007f);   // 0x517b78
        double sx = (double)x - x, sz = (double)z - z;   // (sic: the obstacle against itself)
        float d2 = (float)(sz * sz + sx * sx);
        double yy = ay;
        for (int i = 0; i < 20; i++) {
            yy += step;
            if (r2 > d2 && h > fabs(yy - y)) return true;
        }
        return false;
    }
    double t = 1.0f;
    if (!(dist <= len)) t = (double)dist / len;
    double px = t * dx + ax - x;
    double pz = t * dz + az - z;
    if (!(r2 > pz * pz + px * px)) return false;
    return h > fabs(t * dy + ay - y);
}

bool PathPlanner::blocked(const float to[3]) {       // 0x4e91e0
    const Point &l = points.back();
    float seg[6] = { l.x, l.y, l.z, to[0], to[1], to[2] };
    for (const Obstacle &o : obstacles)
        if (o.blocks(seg, false)) return true;
    return false;
}

// 0x4e9280: from a to b, around one obstacle by a corner at the middle turned 45 degrees.
bool PathPlanner::findPath(float ax, float ay, float az, float bx, float by, float bz) {
    a[0] = ax; a[1] = ay; a[2] = az;
    b[0] = bx; b[1] = by; b[2] = bz;
    points.clear();
    points.push_back(Point{ ax, ay, az });
    if (blocked(b)) {
        double dx = (double)b[0] - a[0];
        double dz = (double)b[2] - a[2];
        float cz = (float)((dz + dx) * 0.5f + az);
        float cx = (float)((dx * 0.5f + ax) - dz * 0.5f);
        points.push_back(Point{ cx, ay, cz });
        points.push_back(Point{ bx, by, bz });
        return true;
    }
    points.push_back(Point{ b[0], b[1], b[2] });
    return true;
}

void PathPlanner::straight(float ax, float ay, float az, float bx, float by, float bz) {   // 0x4e9380
    points.clear();
    points.push_back(Point{ ax, ay, az });
    points.push_back(Point{ bx, by, bz });
}

// ---------------------------------------------------------------- TinkerbellRaytraceScene
void Global3DTask::run() {                           // 0x4abfd0
    wait(1);
    for (;;) {
        if (!Game::state().paused3d) {
            scene->spriteFrames();                   // v22
            scene->cursorUpdate();                   // v30
            // port: 0x4bf070 (the +0x2740 object, 0x4befe0) is not ported yet
            if (scene->tink) scene->tink->update();  // 0x4daf30
            scene->hover();                          // v32
            // port: 0x4f08f0 (the toolbar) is not ported yet
        }
        wait(0);
    }
}

TinkerbellRaytraceScene::~TinkerbellRaytraceScene() {
    delete global_task;
    delete tink;
}

// v27 0x4ab0f0 (part): the 3D scene, then the global_3d and tinkerbell3D packs, Tink and the frame task.
void TinkerbellRaytraceScene::open(const char *nm) {
    Game::state().hover_up = false;                        // +0x75
    Raytrace_Scene::open(nm);
    global3d = ras_open("global_3d", "", &data_device());   // +0x17d4
    global3d->load();
    ras_set_current(global3d);
    // port: the toolbar (+0x17d8, +0x19a0, 0x4efd50 +0x1998, +0x1fe8), 0x4befe0 (+0x2740), the help bell and the
    // cloud banks are not ported yet
    tinkpack = ras_open("tinkerbell3D", "", &data_device());   // +0x17d0
    tinkpack->load();
    ras_set_current(tinkpack);
    tink = new Tinkerbell3D(&rt, this);              // 0x4da4a0
    tink->init();                                    // v1
    reset();                                         // 0x421110
    global_task = new Global3DTask(this);
    global_task->activate();
}

void TinkerbellRaytraceScene::leave() {              // v20 0x4ab470 (part)
    if (global_task) { global_task->deactivate(); delete global_task; global_task = nullptr; }
    if (tink) { tink->release(); delete tink; tink = nullptr; }
    if (tinkpack) ras_close(tinkpack);
    tinkpack = nullptr;
    if (global3d) ras_close(global3d);
    global3d = nullptr;
    Raytrace_Scene::leave();
}

// v32 0x443820: every 30 ms (at most 100): the eye sinks by eye * dt / 1000 (no more than half of it) while above 1,
// or, with state +0x75, eases to 200; state +0x76 holds it.
void TinkerbellRaytraceScene::hover() {
    float e = rt.eyef;
    uint32_t now = frame_time();
    uint32_t ms = now - last_hover;
    if (ms < 0x1e) return;
    if (ms > 100) ms = 100;
    last_hover = now;
    if (!Game::state().hover_up) {
        if (Game::state().hover_down) return;
        float d = (float)((double)(int64_t)ms * e * 0.00100000005f);
        if (!(e > 1.0f)) return;
        if (e < d) d = (float)((double)e * 0.5f);
        rt.lower(d);
    } else {
        float v = (float)((200.0f - (double)e) * (int32_t)ms * 0.00100000005f);
        if (v > 0.0f) rt.raise(v);
        else rt.lower(-v);
    }
}

void Raytrace_Scene::run() {                         // v10 0x4a9be0
    wait(1);
    for (;;) {
        wait(0);
        cursorUpdate();                              // v30
        spriteFrames();                              // v22
    }
}

void Raytrace_Scene::leave() {                       // v20 0x4aa110
    tb_input_set_arrows(false);
    if (input) { input->deactivate(); delete input; input = nullptr; }
    rt.unloadAll();
    rt.clear();
    rt.deactivate();
    rt.release();                                    // 0x429980
    if (walls) ras_close(walls);
    walls = nullptr;
    if (sprites) ras_close(sprites);
    sprites = nullptr;
    if (ground) ras_close(ground);
    ground = nullptr;
    deactivate();
}

// ---------------------------------------------------------------- London
void London_Scene::enter() {                         // part of 0x4b5800 (-> 0x4ab0f0 -> 0x4a9ca0)
    open(name());
}

void London_Scene::run() {                           // port: until 0x4b5300 is in, the frame loop only
    wait(1);
    for (;;) {
        wait(0);
        cursorUpdate();                              // v30
    }
}

}  // namespace Game
