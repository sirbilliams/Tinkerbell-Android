// Tinkerbell3D and her commands, reconstructed from "Tinker Bell.exe" (see tink3d.h). x87 precision kept as in
// the 3D engine: values on the FPU are doubles, stores to float locals and fields round.
#include "tink3d.h"
#include "voiceover.h"
#include "raytrace_scene.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "tb.h"

namespace Game {
using Raytrace::Sprite3D;

static const char *const kRoot = "GLOB_CA_TNKroot_";  // 0x54294c
static const float kDeg = 57.29582595825195f;       // 0x5144f4
static int g_command_id = 0;                         // 0x542928

static int rnd(int n) { return Vision::vision_random(n); }   // (state +8)->0x425d40

static double wrap360(double a) {                     // the < 0 / > 360 pair the code repeats
    if (a < 0.0) a += 360.0f;
    if (!(a <= 360.0f)) a -= 360.0f;
    return a;
}

// 0x425d80 on (Tink, camera): the angle from Tink to the camera, degrees, wrapped once.
static double to_camera(Tinkerbell3D *t) {
    Raytrace::RayTrace *rt = t->rt;
    return wrap360(atan2((double)rt->camz - t->z, (double)rt->camx - t->x) * kDeg);
}

TinkCommand::TinkCommand() : id(g_command_id++) {}

// ---------------------------------------------------------------- Basic
TinkCmdBasic::TinkCmdBasic(Raytrace_Scene *s, int t, float px, float py, float pz)   // 0x4de020
    : type(t), x(px), y(py), z(pz), scene(s) {
    fly = new Vision::Sound("GLOB_FX_TNKfly1");      // 0x420210
    if (type == 2) waypoint = -1;
}
TinkCmdBasic::~TinkCmdBasic() { delete fly; }

bool TinkCmdBasic::update(uint32_t, Tinkerbell3D *t) {   // 0x4dd3b0
    switch (type) {
    case 0: {
        t->setAnim(kRoot, nullptr, 1, 1);
        Vision::Animation *a = t->sprite()->anim;
        return a->current() < a->count() - 1;
    }
    case 1: {
        double d = to_camera(t) - t->heading;
        if (d < -180.0f) d += 360.0f;                // 0x513de4
        if (!(d <= 180.0f)) d -= 360.0f;
        int ad = abs((int)d);
        if (!((float)ad > 10.0f)) return false;      // 0x517548
        double b = to_camera(t);
        float bf = (float)b;
        if (!(b < 360.0f)) bf = (float)(b - 360.0f);
        if (bf < 0.0f) bf = (float)((double)bf + 360.0f);
        t->turnToward(bf);
        return true;
    }
    case 2:
        return flyTo(t);
    case 3: {
        if (!started) {
            started = true;
            t0 = Vision::frame_time();
            return true;
        }
        float r = x;
        float dx = (float)((double)t->x - t->rt->camx);
        double dz = (double)t->z - t->rt->camz;
        double h;
        if ((double)r * r < dz * dz + (double)dx * dx) {
            if ((double)(int64_t)(Vision::frame_time() - t0) < y) return true;
            h = t->facing();
            if (!(h < 360.0f)) h -= 360.0f;
        } else {
            double b = to_camera(t);
            float bf = (float)b;
            h = b;
            if (!(bf < 360.0f)) h = (double)bf - 360.0f;
        }
        if (h < 0.0) h += 360.0f;
        t->heading = (float)h;
        started = false;
        return false;
    }
    case 4:
        return false;
    }
    return true;                                     // port: the exe returns a stray byte for types above 4
}

void TinkCmdBasic::step(Tinkerbell3D *t) {           // 0x4ddb70
    static const char *const idle[8] = {             // jump table 0x4ddd6c
        "GLOB_CA_TNKidleso_", "GLOB_CA_TNKidlese_", "GLOB_CA_TNKidleea_", "GLOB_CA_TNKidlene_",
        "GLOB_CA_TNKidleno_", "GLOB_CA_TNKidlenw_", "GLOB_CA_TNKidlewe_", "GLOB_CA_TNKidlesw_" };
    if (type >= 1 && type <= 2) {
        double d = ((double)t->heading - to_camera(t)) + 22.5;   // 0x517680
        while (d < 0.0) d += 360.0;
        while (!(d < 360.0)) d -= 360.0;
        unsigned i = (unsigned)(int)(d * 0.0221606642f);   // 0x517678
        if (i < 8) t->setAnim(idle[i], nullptr, 1, 1);
    } else if (started) {
        t->setAnim(kRoot, nullptr, 1, 1);
    }
}

// 0x4dd710: the path to (x, y, z); straight when the planned one leads out of the room or near the camera.
void TinkCmdBasic::startPath(Tinkerbell3D *t) {
    waypoint = 1;
    PathPlanner &pp = scene->planner;
    bool ok = pp.findPath(t->x, t->y, t->z, x, y, z);
    bool good = true;
    for (size_t i = 0; i < pp.points.size(); i++) {
        float px = pp.points[i].x, pz = pp.points[i].z;
        if (!t->rt->canSlide(px, pz)) { good = false; break; }
        float dx = (float)((double)px - t->rt->camx);
        double dz = (double)pz - t->rt->camz;
        if (dz * dz + (double)dx * dx < 90000.0f) { good = false; break; }   // 0x5175c8
    }
    if (!good || !ok) pp.straight(t->x, t->y, t->z, x, y, z);   // 0x4e9380
    if (getenv("TB_DEBUG_TINK"))
        for (size_t i = 0; i < pp.points.size(); i++) tb_log("tink: path %zu %.0f,%.0f,%.0f%s", i, pp.points[i].x, pp.points[i].y, pp.points[i].z, good ? "" : " (straight)");
    // 0x425da0: the fly sound when the target is more than 750 away (y counts a tenth)
    double ex = (double)t->x - x, ey = (double)(float)(t->y * 0.100000001f) - (float)(y * 0.100000001f);
    double ez = (double)t->z - z;
    if ((double)750.0f * 750.0f < ez * ez + ey * ey + ex * ex) {
        fly->open();
        fly->play();
    }
}

// 0x4dd8f0: fly towards the current waypoint at four times the distance per second (at least 4 a second), facing
// it while it is more than 100 away; the next one within 10.
bool TinkCmdBasic::flyTo(Tinkerbell3D *t) {
    if (waypoint == -1) startPath(t);
    PathPlanner &pp = scene->planner;
    const PathPlanner::Point &p = pp.points[(size_t)waypoint];
    float dx = (float)((double)p.x - t->x);
    float dy = (float)((double)p.y - t->y);
    float dz = (float)((double)p.z - t->z);
    float sqz = (float)((double)dz * dz);
    float sqx = (float)((double)dx * dx);
    if ((double)sqx + sqz > 10000.0f) {
        double h = atan2((double)p.z - t->z, (double)p.x - t->x) * 57.067508697509766f;   // 0x517650 (sic)
        float hf = (float)h;
        if (!(h <= 360.0f)) hf = (float)((double)hf - 360.0f);
        if (hf < 0.0f) hf = (float)((double)hf + 360.0f);
        t->turnToward(hf);
    }
    double vx = (double)dx * 4.0f, vy = (double)dy * 4.0f, vz = (double)dz * 4.0f;   // 0x5132a0
    double s = vz * vz + vy * vy + vx * vx;
    if (s < 16.0f) {                                 // 0x513d3c
        if (s != 0.0)
            do {
                vx += vx; vy += vy; vz += vz;
                s = vz * vz + vy * vy + vx * vx;
            } while (s < 16.0f);
    }
    t->vx = (float)vx;
    t->vy = (float)vy;
    t->vz = (float)vz;
    if ((double)dy * dy + sqx + sqz < 100.0f) {      // 0x513da0
        waypoint++;
        if (waypoint >= (int)pp.points.size()) return false;
    }
    return true;
}

// ---------------------------------------------------------------- Animation
TinkCmdAnimation::TinkCmdAnimation(const char *p, const char *sound) : prefix(p) {   // 0x4dc530
    if (sound) {
        snd = new Vision::Sound(sound);
        snd->open();                                 // 0x41f9d0(0)
    }
}
TinkCmdAnimation::~TinkCmdAnimation() { delete snd; }

bool TinkCmdAnimation::update(uint32_t, Tinkerbell3D *t) {   // 0x4dc660
    if (!started) {
        double d = to_camera(t) - t->heading;
        if (d < -180.0f) d += 360.0f;
        if (!(d <= 180.0f)) d -= 360.0f;
        int ad = abs((int)d);
        if ((float)ad > 10.0f) {
            double b = to_camera(t);
            float bf = (float)b;
            if (!(b < 360.0f)) bf = (float)(b - 360.0f);
            if (bf < 0.0f) bf = (float)((double)bf + 360.0f);
            t->turnToward(bf);
            return true;
        }
        started = true;
        if (snd) snd->play();
        t->setAnim(prefix, nullptr, 1, 1);
        return true;
    }
    Vision::Animation *a = t->sprite()->anim;
    if (a->current() < a->count() - 1) return true;
    double h = to_camera(t);
    float hf = (float)h;
    if (!(hf < 360.0f)) h = (double)hf - 360.0f;
    if (h < 0.0) h += 360.0f;
    t->heading = (float)h;
    return false;
}

// ---------------------------------------------------------------- Point
TinkCmdPoint::TinkCmdPoint(float px, float py, float pz) : x(px), y(py), z(pz) {   // 0x4db7b0 (part)
    fly = new Vision::Sound("GLOB_FX_TNKfly1");      // 0x420210
    fly->open();                                     // 0x41f9d0(0)
}
TinkCmdPoint::~TinkCmdPoint() { delete fly; }        // 0x4dce60 -> 0x4dce80

bool TinkCmdPoint::update(uint32_t, Tinkerbell3D *t) {   // 0x4dcef0
    if (!started) {
        double d = to_camera(t) - t->heading;
        if (d < -180.0f) d += 360.0f;                // 0x513de4
        if (!(d <= 180.0f)) d -= 360.0f;             // 0x513da4
        int ad = abs((int)d);                        // 0x4f3948
        if ((float)ad > 10.0f) {                     // 0x517548
            double b = to_camera(t);
            float bf = (float)b;
            if (!(b < 360.0f)) bf = (float)(b - 360.0f);
            if (bf < 0.0f) bf = (float)((double)bf + 360.0f);
            t->turnToward(bf);                       // 0x4dc3d0
            return true;
        }
        fly->play();                                 // 0x41fd50
        started = true;
        pointAnim(t);
        return true;
    }
    Vision::Animation *a = t->sprite()->anim;
    if (a->current() < a->count() - 1) return true;
    double h = atan2((double)t->rt->camz - t->z, (double)t->rt->camx - t->x) * kDeg;
    float hf = (float)h;
    if (h < 0.0) { h += 360.0f; hf = (float)h; }
    if (!(h <= 360.0f)) { h -= 360.0f; hf = (float)h; }
    if (!(hf < 360.0f)) h = (double)hf - 360.0f;
    if (h < 0.0) h += 360.0f;
    t->heading = (float)h;
    return false;
}

// 0x4dd1d0: straight up or down when the point is above or below her (within 100 across, more than 50 up or
// down), else east or west by the direction to it.
void TinkCmdPoint::pointAnim(Tinkerbell3D *t) {
    float dx = (float)((double)x - t->x), dy = (float)((double)y - t->y), dz = (float)((double)z - t->z);   // 0x425eb0
    if (sqrt((double)dz * dz + (double)dx * dx) < 100.0 && !(fabs((double)dy) <= 50.0)) {   // 0x5134c0, 0x517660
        t->setAnim(dy < 0.0f ? "GLOB_CA_TNKptdw_" : "GLOB_CA_TNKptup_", nullptr, 1, 1);
        return;
    }
    double a = atan2((double)dz, (double)dx) * 57.29582595825195f;   // 0x51765c
    if (a < 270.0f && !(a < 90.0f)) t->setAnim("GLOB_CA_TNKptwe_", nullptr, 1, 1);   // 0x517658, 0x517654
    else t->setAnim("GLOB_CA_TNKptea_", nullptr, 1, 1);
}

// ---------------------------------------------------------------- Tinkerbell3D
void Tinkerbell3D::point(float px, float py, float pz) { queue.push_back(new TinkCmdPoint(px, py, pz)); }   // 0x4db7b0
Tinkerbell3D::Tinkerbell3D(Raytrace::RayTrace *r, Raytrace_Scene *s)   // 0x4da4a0
    : RayTraceCharacter(r, "tinkerbell3D"), scene(s) {}

Tinkerbell3D::~Tinkerbell3D() {                      // 0x4da590
    for (TinkCommand *c : queue) delete c;
    queue.clear();
}

int Tinkerbell3D::pushBasic(int type, float px, float py, float pz) {   // 0x4de020
    TinkCmdBasic *c = new TinkCmdBasic(scene, type, px, py, pz);
    queue.push_back(c);
    return c->id;
}

void Tinkerbell3D::init() {                          // v1 0x4da790
    RayTraceCharacter::init();
    hit = 0;
    setAnim(kRoot, nullptr, 1, 1);
    float px = (float)(cos((double)rt->angf) * 300.0 + rt->camx);   // 0x5144b8
    float pz = (float)(sin((double)rt->angf) * 300.0 + rt->camz);
    setX(px);
    setY((float)((double)rt->eyef + 375.0f));        // 0x517560
    setZ(pz);
    dust_x = x;
    dust_y = y;
    dust_z = z;
    noBox();
    initDust();
    last = Vision::frame_time();
    last_dust = Vision::frame_time();
    vz = vy = vx = 0;
    pushBasic(1, 0, 0, 0);
    last_idle = Vision::frame_time();
}

void Tinkerbell3D::wave() { push(new TinkCmdAnimation("GLOB_CA_TNKwave_", nullptr)); }   // 0x4dbd90
void Tinkerbell3D::say(const char *line) { Vision::VoiceOver::get().play(line); }        // 0x4cca20 -> 0x4c9400

void Tinkerbell3D::release() {                       // v2 0x4da8b0
    RayTraceCharacter::release();
    char n[40];
    for (int i = 1; i <= 35; i++) {
        snprintf(n, sizeof n, "traildust%2.2d", i);
        rt->removeSprite(n);
    }
    for (Sprite3D *&d : dust_) d = nullptr;
}

void Tinkerbell3D::initDust() {                      // 0x4da980
    char n[40], p[40];
    for (int i = 1; i <= 35; i++) {
        snprintf(n, sizeof n, "traildust%2.2d", i);                 // 0x542960
        snprintf(p, sizeof p, "GLOB_AM_TNKdust%1.1d_", rnd(3) + 1);   // 0x542970
        Sprite3D *s = rt->addSprite(n, p, 0);
        dust_[i - 1] = s;
        s->load();
        s->bheight = 0;
        s->anim->mode = 0;
        s->anim->visible = false;
    }
}

double Tinkerbell3D::facing() {                      // 0x4db110
    return wrap360(atan2((double)rt->camz - z, (double)rt->camx - x) * kDeg);
}

bool Tinkerbell3D::turnToward(float deg) {           // 0x4dc3d0
    double h = deg;
    while (!(h <= 360.0f)) h -= 360.0f;
    while (h < 0.0) h += 360.0f;
    double d = h - heading;
    if (!(d <= 180.0f)) d -= 360.0f;                 // 0x513da4
    if (d < -180.0f) d += 360.0f;
    int ad = abs((int)d);
    if (!((float)ad > 10.0f)) return true;
    if (!(d <= 15.0f)) d = 15.0f;                    // 0x5144a8
    else if (d < -15.0f) d = -15.0f;                 // 0x514490
    d += heading;
    while (!(d <= 360.0f)) d -= 360.0f;
    while (d < 0.0) d += 360.0f;
    heading = (float)d;
    return false;
}

// 0x4db3a0: each shown dust puff rises 0.04 a ms and goes after its last frame (or within 100 of the camera).
void Tinkerbell3D::dust(uint32_t dt) {
    float rise = (float)((double)(int64_t)dt * -0.0399999991f);   // 0x5175d0
    float cx = rt->camx, cz = rt->camz;
    for (int i = 0; i < 35; i++) {
        Sprite3D *s = dust_[i];
        if (!s || !s->anim || !s->anim->visible) continue;
        double ex = (double)s->x - cx, ez = (double)s->z - cz;
        if (ez * ez + ex * ex < 10000.0f) { s->anim->visible = false; continue; }   // 0x5175d4
        if (s->anim->current() < s->anim->count() - 1) s->y = (float)((double)rise + s->y);
        else s->anim->visible = false;
    }
}

// 0x4db1a0: every 250 ms, a dust puff for each 25 flown since the last ones, scattered by up to 12.
void Tinkerbell3D::moreDust() {
    uint32_t now = Vision::frame_time();
    if (now - last_dust < 0xfa) return;
    last_dust = now;
    float ex = (float)((double)x - dust_x);
    float ey = (float)((double)y - dust_y);
    float ez = (float)((double)z - dust_z);
    double len = sqrt((double)ez * ez + (double)ey * ey + (double)ex * ex);
    int n = 1 - (int)(len * -0.0399999991f);
    float nf = (float)n;
    for (int k = 0; k < n; k++) {
        float f = (float)((double)(k + 1) / nf);
        float px = (float)(((double)rnd(12) + (double)f * ex) + dust_x - 6.0f);   // 0x51451c
        float py = (float)(((double)rnd(12) + (double)f * ey) + dust_y - 20.0f - 6.0f);   // 0x5150f8
        double pz = ((double)rnd(12) + (double)f * ez) + dust_z - 6.0f;
        for (int i = 0; i < 35; i++) {
            Sprite3D *s = dust_[i];
            if (s && s->anim && s->anim->visible) continue;
            s->x = px;
            s->y = py;
            s->z = (float)pz;
            s->anim->setFrame(0);
            s->anim->visible = true;
            break;
        }
    }
    dust_x = x;
    dust_y = y;
    dust_z = z;
}

// 0x4db470: velocity capped at 420 a second, applied, then damped by 0.3% a ms.
void Tinkerbell3D::move(uint32_t dt) {
    float dtf = (float)(int64_t)dt;
    float sec = (float)((double)dtf * 0.00100000005f);   // 0x514344
    double s = (double)vx * vx + (double)vy * vy + (double)vz * vz;
    if (!(s <= 176400.0f)) {                         // 0x5175e8
        double k = 420.0 / sqrt(s);                  // 0x5175e0
        vx = (float)(k * vx);
        vy = (float)(k * vy);
        vz = (float)(k * vz);
    }
    setX((float)((double)x + (double)sec * vx));
    setY((float)((double)sec * vy + y));
    setZ((float)((double)sec * vz + z));
    double f = dt < 200 ? (double)dtf : 200.0f;      // 0x514340
    double damp = 1.0f - f * 0.00300000003f;         // 0x5175d8
    vx = (float)(damp * vx);
    vy = (float)(damp * vy);
    vz = (float)(damp * vz);
}

// 0x4dad40: somewhere ahead of the camera (300-400 out, 500 more while moving forward), in the room and not
// within 300 of the camera, widening the search; fly there, then face the camera.
void Tinkerbell3D::wander() {
    int r = rnd(100) + 300;
    int spread = (int)((double)r * 0.400000006f);    // 0x5175cc
    if (scene->fwd) r += 500;
    float rf = (float)r;
    float cx = (float)(cos((double)rt->angf) * rf + rt->camx);
    float cz = (float)(sin((double)rt->angf) * rf + rt->camz);
    float px, py, pz;
    bool ok;
    do {
        float sf = (float)spread;
        px = (float)(((double)rnd(spread * 2) + cx) - sf);
        float ry = (float)rnd(300);
        py = (float)(((double)rt->eyef * 2.04999995f + 480.0f) + ry - 150.0f);   // 0x42b4e0, 0x5142e8
        if (py < 0.0f) py = 0.0f;
        pz = (float)(((double)rnd(spread * 2) + cz) - sf);
        if (!rt->canSlide(px, pz)) ok = false;
        else {
            float ddx = (float)((double)px - rt->camx);
            double ddz = (double)pz - rt->camz;
            ok = !(ddz * ddz + (double)ddx * ddx < 90000.0f);
        }
        if (spread < 2000) spread = spread * 11 / 10;
    } while (!ok);
    if (getenv("TB_DEBUG_TINK")) tb_log("tink: wander to %.0f,%.0f,%.0f (cam %.0f,%.0f eye %.0f)", px, py, pz, rt->camx, rt->camz, rt->eyef);
    pushBasic(2, px, py, pz);
    pushBasic(1, 0, 0, 0);
}

// 0x4daa20: one of six idles.
void Tinkerbell3D::randomIdle() {
    switch (rnd(6)) {
    case 0: push(new TinkCmdAnimation("GLOB_CA_TNKloop_", "GLOB_FX_TNKfly1")); break;   // Loop 0x5175bc
    case 1: push(new TinkCmdAnimation("GLOB_CA_TNKidle3_", "GLOB_FX_TNKyawn")); break;  // Idle3
    case 2: push(new TinkCmdAnimation("GLOB_CA_TNKidle4_", nullptr)); break;           // Idle4
    case 3: push(new TinkCmdAnimation("GLOB_CA_TNKidle6_", nullptr)); break;           // Idle6
    case 4: push(new TinkCmdAnimation("GLOB_CA_TNKidle7_", nullptr)); break;           // Idle7
    case 5: push(new TinkCmdAnimation("GLOB_CA_TNKidle9_", nullptr)); break;           // Idle9
    }
}

// 0x4daf30: every frame (20 ms at least): the front command, else the root loop facing the camera; when idle
// (the camera still, or 1 in 20) a wander or, every 15 s, an idle; then move and the dust.
void Tinkerbell3D::update() {
    uint32_t now = Vision::frame_time();
    uint32_t dt = now - last;
    if (dt < 0x14) return;
    if (!visible) { dust(dt); return; }
    bool root = true;
    if (!queue.empty()) {
        if (!queue.front()->update(dt, this)) {
            if (!queue.empty()) {
                delete queue.front();
                queue.pop_front();
            }
        }
        if (!queue.empty()) {
            queue.front()->step(this);
            root = false;
        }
    }
    if (root) {
        setAnim(kRoot, nullptr, 1, 1);
        double h = facing();
        if (!(h < 360.0f)) h -= 360.0f;
        if (h < 0.0) h += 360.0f;
        heading = (float)h;
    }
    if (queue.empty()) {
        int r = rnd(20);
        uint32_t still = rt->idleTime();
        if ((still == 0 || r == 0) && wanders) {
            if (Vision::frame_time() - last_idle > 0x3a98) {
                randomIdle();
                last_idle = Vision::frame_time();
            } else {
                wander();
            }
        }
    }
    if (dt > 200) dt = 200;
    last = now;
    move(dt);
    dust(dt);
    if (!queue.empty()) moreDust();
}

}  // namespace Game
