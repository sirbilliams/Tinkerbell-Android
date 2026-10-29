// Raytrace_Scene and MovementInputTask, reconstructed from "Tinker Bell.exe" (see raytrace_scene.h).
#include "raytrace_scene.h"
#include "tink3d.h"
#include "state.h"
#include "voacts.h"
#include "cannon.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
            s->onMouse();                            // v25
            s->active_time = frame_time();
        }
    }
}

// ---------------------------------------------------------------- Raytrace_Scene
Raytrace_Scene::Raytrace_Scene(const char *name)   // 0x4aa240
    : Scene(name), over1(0, 0, 0, 0, 0, true, true), over2(0, 0, 0, 0, 0, true, true),
      cursor_img("glob_cu_normal_01", 0, 0, 0, 0, 0, 0, true) {
    keys.setClient(this);                            // 0x41f290
    mouse.setClient(this);
}

void Raytrace_Scene::notify2(Service *s) {          // v9 0x4a9690
    if (current_scene() != this) return;
    if (s == &mouse) onMouse();                      // v25
    if (s == &keys) onKey();                         // v26
}
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
    mouse.activate();                                // +0x1328, +0x12f4 (port: the joystick +0x12c0 is the pad,
    keys.activate();                                 // read as keys by platform/dinput)
    // port: the 0x158-byte graphic 0x4a9410 (+0x124) is not ported yet.
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
    over1.activate();                                // +0x665 = 1 (port: no use found yet)
    over1.loadAll();
    over2.activate();
    over2.loadAll();
    cursor_img.load();                               // v15
    set_cursor(&cursor_img, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
    show_cursor();                                   // 0x4119d0
    // port: the cursor hot spot (0x5a6930/34) = 0 (the Image draws from its corner already)
    tb_input_set_arrows(true);                       // port: the pad as the arrow keys (platform/dinput.h)
    if (const char *c = getenv("TB_RT_CAM"))         // desktop test aid (not in the game): "x,z,degrees,eye"
        sscanf(c, "%f,%f,%f,%f", &cam_x, &cam_z, &cam_deg, &cam_eye);
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
            scene->lift->update();                   // 0x4bf070
            if (scene->tink) scene->tink->update();  // 0x4daf30
            scene->hover();                          // v32
            scene->bag->regen(0);                    // 0x4f08f0 (port: its unused stack argument as 0)
        }
        wait(30);
    }
}

TinkerbellRaytraceScene::TinkerbellRaytraceScene(const char *nm)   // 0x4aadb0
    : Raytrace_Scene(nm), pixie("GLOB_CU_pixiedust_", 50) {}

TinkerbellRaytraceScene::~TinkerbellRaytraceScene() {
    delete global_task;
    delete tink;
    delete bag;
    delete lift;
}

// v30 0x4ab6b0: the hand cursor over the map icon, the bag or the bell (when shown); the dust cursor while dusting.
Vision::Graphic *TinkerbellRaytraceScene::cursorUpdate() {
    Vision::Graphic *hand = Game::state().cursor_active;   // +0x60
    Vision::Graphic *c = nullptr;
    if (map.visible && map.hit(g_mouse.x, g_mouse.y)) c = hand;
    if (Game::state().dusting) c = &pixie;
    else if (bag->visible && bag->hit(g_mouse.x, g_mouse.y)) c = hand;
    if (bell.visible && bell.hit(g_mouse.x, g_mouse.y)) return hand;
    return c;
}

// v27 0x4ab0f0 (part): the 3D scene, then the global_3d and tinkerbell3D packs, Tink and the frame task.
void TinkerbellRaytraceScene::open(const char *nm) {
    Game::state().hover_up = false;                        // +0x75
    busy = false;                                    // +0x28e0
    Raytrace_Scene::open(nm);
    global3d = ras_open("global_3d", "", &data_device());   // +0x17d4
    global3d->load();
    ras_set_current(global3d);
    pixie.load();                                    // +0x17d8 v15
    pixie.visible = false;                           // +0x1854
    pixie.start(true);                               // 0x4e34c0(1)
    map.load();                                      // +0x19a0 v15
    over1.layer(0)->add(&map);
    bag = new PixieDustBag();                        // 0x4efd50 (+0x1998)
    over1.layer(0)->add(bag);
    bell.load();                                     // +0x1fe8 v15
    over1.layer(0)->add(&bell);
    // +0x16b4 = 0 (gravity) and +0x12b0 = 975: the exe's order
    gravity = 0;                                     // +0x16b4
    rt.pick_max = 975.0f;                            // +0x12b0 = 0x516378
    over1.layer(0)->add(&Game::cannon());            // state +0x7c0
    Game::cannon().hide();                           // 0x4e3bf0
    lift = new DustLift(&rt);                        // 0x4befe0 (+0x2740)
    // port: the joystick service (+0x12c0) is the pad, read as keys (platform/dinput)
    g_mouse.x = 0;                                   // 0x5a6e60 / 0x5a6e7c
    g_mouse.y = 0;
    dim.x = 0; dim.y = 0; dim.z = -1000; dim.w = 640; dim.h = 480;   // +0x2890..+0x28a0
    over1.layer(0)->add(&dim);
    tinkpack = ras_open("tinkerbell3D", "", &data_device());   // +0x17d0
    tinkpack->load();
    ras_set_current(tinkpack);
    tink = new Tinkerbell3D(&rt, this);              // 0x4da4a0
    tink->init();                                    // v1
    reset();                                         // 0x421110
    global_task = new Global3DTask(this);
    global_task->activate();
    cloud1 = rt.addSprite("GLOB_LY_cloudbank_", "GLOB_LY_cloudbank_", 1);   // 0x42d9d0
    cloud1->load();
    cloud1->anim->visible = false;
    cloud1->scale = 100.0f;
    cloud2 = rt.findSprite("GLOB_LY_cloudbank_");
    cloud2->load();
    cloud2->anim->visible = false;
    cloud2->scale = 100.0f;
    show_cursor();                                   // 0x4119d0
    // port: +0x2831 = 0 (a flag of scenes not ported yet)
    if (strcasecmp(name(), "treasure") != 0) Game::state().last_scene = name();   // 0x423c60, state +0x1830
}

// 0x4ab760: the camera put up at 475 above the cloud bank (150 ahead at height 1500) and brought down to the ground
// in 2 s, the hover held and the input off; true while flying.
bool TinkerbellRaytraceScene::arriveThroughClouds(bool second) {
    if (!flying) {
        float X = rt.camx, Z = rt.camz, A = (float)rt.angleDeg();   // 0x42b470, 0x42b540, 0x42b560
        double ang = (double)3.14159274f * A * 0.00555555569f;     // 0x516374, 0x513d90
        float px = (float)(cos(ang) * 150.0 + X);    // 0x516440
        float pz = (float)(sin(ang) * 150.0 + Z);
        cloud1->setPos(px, 1500.0f, pz);             // 0x42f740
        cloud2->setPos(px, 1500.0f, pz);
        if (second) cloud2->anim->visible = true;
        else cloud1->anim->visible = true;
        rt.setCamera(X, 475.0f, Z, A);               // 0x42a570
        tween.start(&rt, X, 0.1f, Z, A, 2000.0);     // 0x4df3d0
        Game::state().hover_down = true;             // +0x76
        paused = true;                               // +0x128
        flying = true;
        return true;
    }
    if (tween.step()) return flying;                 // 0x4df4d0
    cloud1->anim->visible = false;
    cloud2->anim->visible = false;
    Game::state().hover_down = false;
    paused = false;
    flying = false;
    return false;
}

// 0x4ab920: the cloud bank 150 ahead at height 1500, the camera rising to 475 in 2 s, the hover held, input off.
bool TinkerbellRaytraceScene::flyThroughClouds(bool second) {
    if (!flying) {
        float X = rt.camx, Z = rt.camz, A = (float)rt.angleDeg();
        double ang = (double)3.14159274f * A * 0.00555555569f;   // 0x516374, 0x513d90
        float px = (float)(cos(ang) * 150.0 + X);    // 0x516440
        float pz = (float)(sin(ang) * 150.0 + Z);
        cloud1->setPos(px, 1500.0f, pz);             // 0x44bb8000
        cloud2->setPos(px, 1500.0f, pz);
        if (second) cloud2->anim->visible = true;
        else cloud1->anim->visible = true;
        tween.start(&rt, X, 475.0f, Z, A, 2000.0);   // 0x4df3d0
        Game::state().hover_down = true;             // +0x76
        paused = true;                               // +0x128
        Game::state().hover_up = false;
        flying = true;
        return true;
    }
    if (tween.step()) return flying;                 // 0x4df4d0
    flying = false;
    return false;
}

// 0x4ec5f0 (state +0x77: Enter was let go): Enter toggles the hover up (the eye to 200, up/down keys off), Shift
// or Ctrl holds the eye where it is.
bool TinkerbellRaytraceScene::stateKeys() {
    State &st = Game::state();
    if (g_key.vk == 0x0d && g_key.up && !st.enter_released) st.enter_released = true;
    if (!g_key.down) return false;
    if (!st.hover_up) {
        if (g_key.vk == 0x0d && st.enter_released && forced_off != true) {   // 0x4abf80
            st.hover_up = true;
            st.hover_down = false;
            force[4] = force[5] = 1;                 // 0x4aa9c0
            st.enter_released = false;
            return true;
        }
        if (!st.hover_down) {
            bool ok = force[4] != 1 && force[5] != 1;   // 0x4aa9e0
            if ((g_key.vk == 0x10 || g_key.vk == 0x11) && ok) {
                st.hover_down = true;
                return true;
            }
        }
    } else if (g_key.vk == 0x0d && st.enter_released) {
        st.hover_up = false;
        st.hover_down = false;
        allowUpDown();                               // 0x4aa9b0
        st.enter_released = false;
        return true;
    }
    return false;
}

void TinkerbellRaytraceScene::forceNone() { Raytrace_Scene::forceNone(); forced_off = false; }   // v23 0x4abf30
void TinkerbellRaytraceScene::forceOff() { Raytrace_Scene::forceOff(); forced_off = true; }      // v24 0x4abf50

void TinkerbellRaytraceScene::leave() {              // v20 0x4ab470 (part)
    cloud1 = cloud2 = nullptr;                       // port: 0x42f2b0 unloads them; RayTrace::release frees them
    Game::cannon().detach();                         // 0x412200
    dim.detach();
    if (global_task) { global_task->deactivate(); delete global_task; global_task = nullptr; }
    pixie.detach();                                  // 0x412200
    pixie.unload();                                  // v16
    delete bag;                                      // v3(1) (its dtor takes it off the screen)
    bag = nullptr;
    map.unload();                                    // v16 (it stays in over1; the next open adds it again)
    bell.unload();
    if (tink) { tink->release(); delete tink; tink = nullptr; }
    if (tinkpack) ras_close(tinkpack);
    tinkpack = nullptr;
    if (global3d) ras_close(global3d);
    global3d = nullptr;
    delete lift;                                     // 0x4bf030 + free
    lift = nullptr;
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
    cursor_img.unload();                             // v16
    if (g_cursor_graphic == &cursor_img) g_cursor_graphic = nullptr;   // port: the exe leaves it pointing here
    mouse.deactivate();
    keys.deactivate();
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
    over1.clear();                                   // 0x41ebc0
    over1.deactivate();
    over1.unloadAll();                               // 0x41ec00
    over2.clear();
    over2.deactivate();
    over2.unloadAll();
    deactivate();
}

}  // namespace Game
