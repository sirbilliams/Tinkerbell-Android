// AmbientSpriteTask and AmbientSprite, reconstructed from "Tinker Bell.exe" (see ambient.h).
#include "ambient.h"
#include "toolbar.h"
#include "../vision/database.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include <math.h>
#include <stdlib.h>

namespace Game {

using Raytrace::Sprite3D;

static const float kPi = 3.14159274f;                // 0x517694
static Vision::Sound *g_happy = nullptr;             // 0x6315a0 "glob_fx_tnkhappy" (made once, kept)
static int g_count = 0;                              // 0x631598

// 0x4df720: a new one every 2..4 s.
void AmbientSpriteTask::run() {
    wait(1);
    for (;;) {
        wait(Vision::vision_random(2000) + 2000);    // a local random object (0x425d10 / 0x425d40)
        bool dusty = Vision::vision_random(8) == 0;
        start(Gorilla::gnew<AmbientSprite>(rt, scene, dusty ? bag : nullptr));   // 0x4243c0(0x110), 0x420ef0
    }
}

// 0x4df8d0
AmbientSprite::AmbientSprite(Raytrace::RayTrace *r, const std::string &scene, PixieDustBag *b)
    : Task(nullptr), rt(r), bag(b) {
    if (!g_happy) {
        g_happy = new Vision::Sound("glob_fx_tnkhappy");   // 0x420210
        g_happy->open();                             // 0x41f9d0(0)
    }
    Vision::Database db("ambient_sprites", "dat", "");   // 0x4e6680 (the exe caches it by name)
    db.load();
    Vision::Database::Section *sec = db.section(scene, true);   // 0x4e4fb0
    if (sec->entries.empty()) {
        // the exe throws "No ambient sprites for this scene"
        Vision::vision_log("No ambient sprites for this scene");
        abort();
    }
    name = "ambient_sprite_" + std::to_string(g_count++);   // an ostringstream: "ambient_sprite_" << n
    int idx = Vision::vision_random((int)sec->entries.size());   // +0x28
    const Vision::Database::Entry &e = sec->entries[idx];
    Sprite3D *s = rt->addSprite(name.c_str(), e.key.c_str(), 1);   // 0x42d9d0 (0x425240: the key)
    s->load();                                       // 0x42ef20
    if (bag) s->attach("dust", "glob_am_ambspark_", 0);   // 0x42f390
    float cam[3];
    rt->camPos(cam);                                 // 0x42b490
    float ang = (float)((double)rt->angf - kPi * 0.25f);   // 0x51477c
    if (e.value == "air") {
        int h = Vision::vision_random(700) + 700;    // 0x2bc
        s->y = (float)h;
    } else {
        s->place();                                  // 0x42f7f0
    }
    int d = Vision::vision_random(10000) + 700;      // 0x2710
    s->x = (float)(cos(ang) * d + cam[0]);
    s->z = (float)(sin(ang) * d + cam[2]);
    speed = 10.0f;
    speed = (float)((double)Vision::vision_random((int)(speed * 0.5f)) + speed);   // ftol, then fild qword
    mouse.setClient(this);
}

AmbientSprite::~AmbientSprite() { rt->removeSprite(name.c_str()); }   // 0x4dff30

// 0x4dfff0: around the camera (a quarter turn from its bearing) by `speed` a frame while within 90 degrees of
// the view.
void AmbientSprite::run() {
    wait(1);
    if (bag) mouse.activate();
    for (;;) {
        float limit = kPi * 0.5f;
        Sprite3D *s = rt->findSprite(name.c_str());  // 0x42dc70
        if (s) {
            double rel = atan2((double)s->z - rt->camz, (double)s->x - rt->camx) - rt->angf;   // 0x42cf30
            if (rel <= -kPi) rel += kPi + kPi;       // 0x513d28
            else if (!(rel < kPi)) rel -= kPi + kPi;
            if (fabs(rel) > limit) break;
        }
        s = rt->findSprite(name.c_str());
        if (!s) break;                               // port: gone with its scene (left for the map) before this task
        double a = atan2((double)s->z - rt->camz, (double)s->x - rt->camx) + (double)(kPi * 0.5f);
        s->x = (float)(cos(a) * speed + s->x);
        s->z = (float)(sin(a) * speed + s->z);
        wait(0);
    }
    finish();
}

// 0x4e0170: clicked: the dust into the bag, the sparkle off, Tink's happy sound.
void AmbientSprite::notify2(Vision::Service *) {
    if (!(Vision::g_mouse.ldown || Vision::g_mouse.rdown)) return;   // 0x5a6e30
    Sprite3D *s = rt->findSprite(name.c_str());
    if (!s || !rt->hitSprite(s, Vision::g_mouse.x, Vision::g_mouse.y)) return;   // 0x42c7b0
    bag->refillRandom();                             // 0x4f0670
    s->detach("dust");                               // 0x42f560
    g_happy->play();                                 // 0x41fd50
    mouse.deactivate();
}

}  // namespace Game
