// Raytrace_Scene and MovementInputTask, reconstructed from "Tinker Bell.exe" (see raytrace_scene.h).
#include "raytrace_scene.h"
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

}  // namespace Game
