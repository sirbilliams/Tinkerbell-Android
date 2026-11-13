// Shadow Boxing, reconstructed from "Tinker Bell.exe" (see shadowboxing.h). x87 sums as in the 3D code: doubles on
// the FPU, floats where the exe stores.
#include "shadowboxing.h"
#include "skull.h"
#include "state.h"
#include "tink3d.h"
#include "cannon.h"
#include "voiceover.h"
#include "tb.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

int shadow_max_visits = 0;                           // 0x5dac80
int shadow_k = 0;                                    // 0x53d380

static const uint32_t kHidden = 7000;                // *0x53d354: ms an animal stays down at least

static const char *abb(int type) {                   // 0x53d4a8..0x53d4b8
    switch (type) {
    case 1: return "RAB";
    case 2: return "MON";
    case 3: return "TUR";
    case 4: return "SQU";
    case 5: return "DUC";
    default: return "";                              // port: the exe leaves its buffer as it was
    }
}

static const char *phase_name(int phase) {
    switch (phase) {
    case 2: return "idle";                           // 0x53bee4
    case 3: return "hide";                           // 0x53d4d4
    default: return "emerge";                        // 0x53d4cc
    }
}

// 0x42b500 (on the RayTrace): the eye value that puts the camera at height y.
static double eye_for(float y) { return ((double)y - 480.0f) * 0.487805f; }   // 0x513d48, 0x513d44

// ---------------------------------------------------------------- ShadowCursor
ShadowCursor::ShadowCursor()                         // 0x480f10
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {}   // 0x411b90

ShadowCursor::~ShadowCursor() {                      // 0x480fa0
    if (loaded_) {
        Graphic::unload();                           // 0x412020
        inactive->unload();                          // v16
        delete inactive;
        active_->unload();
        delete active_;
        loaded_ = false;
    }
    inactive = active_ = nullptr;
}

void ShadowCursor::load() {                          // v15 0x481060
    state = 0;
    inactive = Gorilla::gnew<Animation>("SHBX_CU_netinactiv_");   // 0x4243c0 + 0x404c20
    inactive->load();
    inactive->mode = 1;                              // +0x104
    active_ = Gorilla::gnew<Animation>("SHBX_CU_netactiv_");
    active_->load();
    active_->mode = 1;
    last = frame_time();
    loaded_ = true;
    misses = 0;
}

void ShadowCursor::unload() {                        // v16 0x481150
    Graphic::unload();
    inactive->unload();
    delete inactive;
    inactive = nullptr;
    active_->unload();
    delete active_;
    active_ = nullptr;
    loaded_ = false;
}

// v14 0x4811a0: a frame every 60 ms, at the cursor's spot.
void ShadowCursor::draw() {
    uint32_t now = frame_time();
    uint32_t dt = now - last;
    Animation *a = state == 1 ? active_ : inactive;  // port: state 2 (never set) draws the exe's stale register
    if (dt >= 0x3c) {
        last = now;
        a->step();                                   // 0x403d60
        if (state == 2 && a->current() >= a->count() - 1) state = 0;
    }
    a->x = x;                                        // +0x58
    a->y = y;
    a->draw();                                       // v14
}

// 0x481230: a click on no animal counts a miss (a click on one clears them); the tenth: the skunk's help.
void ShadowCursor::miss() {
    if (state == 1) misses = 0;
    if (state == 0) misses++;
    if (misses >= 10) {
        VoiceOver::get().play("SHBX_VO_SKNhelp1");   // 0x4c9400
        misses = 0;
    }
}

void ShadowCursor::setActive(bool b) {               // 0x481280
    if (state == 2) return;
    if (b) {
        if (state == 1) return;
        state = 1;
        active_->setFrame(0);                        // 0x403f20
    } else {
        if (state == 0) return;
        state = 0;
        inactive->setFrame(0);
    }
}

// ---------------------------------------------------------------- ShadowAnimal
ShadowAnimal::ShadowAnimal() { t0 = frame_time() - kHidden; }   // 0x4812d0

void ShadowAnimal::detach() {                        // 0x481310
    if (rock) rock->detach(att->name.c_str());       // 0x42f560
}

void ShadowAnimal::makePrefix() {                    // 0x481330
    snprintf(buf, sizeof buf, "shbx_ca_%s%s_", abb(type), phase_name(phase));   // 0x53d4bc
}

// 0x481420: the rock, the first free "<animal><nn>" (from 01) attached to it with the current phase's animation
// (n + 100 deep), off; down long enough to come up at once.
void ShadowAnimal::setup(int n, int t, Raytrace::RayTrace *r) {
    rt = r;
    rock_no = n;
    type = t;
    char rn[0x20];
    snprintf(rn, sizeof rn, "AA_shbx_am_rock%1.1d_01", rock_no);   // 0x53d4e4
    rock = rt->findSprite(rn);                       // 0x42dc70
    const char *animal = "";
    switch (type) {
    case 1: animal = "bunny"; break;                 // 0x53d4a0
    case 2: animal = "monkey"; break;
    case 3: animal = "turtle"; break;
    case 4: animal = "squirrel"; break;
    case 5: animal = "duck"; break;
    default: break;
    }
    char name[0x28];
    int i = 1;
    for (;;) {
        snprintf(name, sizeof name, "%s%2.2d", animal, i);   // 0x53d4dc
        att = rt->findAttached(name);                // 0x42dd00
        if (!att) break;
        i++;
    }
    makePrefix();
    rock->attach(name, buf, i + 100);                // 0x42f390
    att = rt->findAttached(name);
    att->on = false;                                 // +0x17c
    t0 = frame_time() - kHidden;
}

// 0x4816b0: every `period` ms a frame; at an animation's end: emerged, it idles (looping); hidden, it is off.
void ShadowAnimal::step() {
    uint32_t now = frame_time();
    if (now - last < period) return;
    Animation *g = static_cast<Animation *>(att->g);   // +0x150
    if (g->current() >= g->count() - 1) {
        if (phase == 1) {
            phase = 2;
            makePrefix();
            att->setPrefix(buf);                     // 0x42e4f0
            static_cast<Animation *>(att->g)->mode = 1;   // +0x104
        } else if (phase == 3) {
            att->on = false;
            up = false;
            return;
        }
    }
    last = now;
    static_cast<Animation *>(att->g)->step();
}

// ---------------------------------------------------------------- ShadowGroup
ShadowGroup::ShadowGroup(Raytrace::RayTrace *r) : rt(r), last(frame_time()) {}

ShadowGroup::~ShadowGroup() {                        // 0x481810: each animal's attachment off its rock
    for (ShadowAnimal &a : animals) a.detach();
}

void ShadowGroup::add(int rock_no, int type) {       // 0x481870
    animals[count].setup(rock_no, type, rt);
    count++;
}

// 0x4818b0: every `interval` ms: with no game on, every animal up hides; the animals step; now and then (the
// longer since the last update, the likelier) one that is down and has been for 7 s comes up.
void ShadowGroup::update() {
    uint32_t now = frame_time();
    uint32_t dt = now - last;
    if (dt < interval) return;
    if (!skull_scene().shadow_on) {                  // 0x5df040
        for (int i = 0; i < count; i++) {
            ShadowAnimal &a = animals[i];
            if (!a.up) continue;
            a.t0 = frame_time();
            if (a.phase == 3) continue;
            a.phase = 3;
            a.makePrefix();
            a.att->setPrefix(a.buf);
            static_cast<Animation *>(a.att->g)->mode = 0;
        }
    }
    last = now;
    for (int i = 0; i < count; i++) {
        ShadowAnimal &a = animals[i];
        if (!a.up) continue;
        uint32_t t = frame_time();
        if (t - a.dist_t > 1000) {
            a.dist = (float)rt->spriteDist(a.rock);  // 0x42e3d0
            a.dist_t = t;
        }
        a.step();
    }
    if (!((uint32_t)vision_random(1000) < dt + 100)) return;   // 0x425d40 (its own object)
    int i = vision_random(count);
    int tries = 0;
    for (; tries < count; tries++) {
        if (!animals[i].up) break;
        i++;
        if (i >= count) i = 0;
    }
    if (tries >= count) return;
    ShadowAnimal &a = animals[i];
    uint32_t t = frame_time();
    if (t - a.t0 < kHidden) return;
    a.last = t;
    a.phase = 1;
    a.makePrefix();
    a.att->setPrefix(a.buf);
    Animation *g = static_cast<Animation *>(a.att->g);
    g->mode = 0;
    a.att->on = true;
    g->setFrame(0);                                  // 0x403f20
    a.up = true;
}

// ---------------------------------------------------------------- ShadowPirates
static const char *const kPirateFx[6] = {"shbx_fx_pirrowin", "shbx_fx_piridle", "COCO_VO_PIRyell1",
                                         "COCO_VO_PIRyell4", "shbx_fx_pirrowout", "shbx_fx_scream"};   // *0x53d364..
static const char *const kGatorFx[3] = {"shbx_fx_tictictoc", "shbx_fx_flyup", "shbx_fx_flydw_01"};   // *0x53d358..

ShadowPirates::ShadowPirates(Raytrace::RayTrace *r) : rt(r) {
    for (Sound *&s : snd) s = new Sound("");         // 0x437290 x6
}

ShadowPirates::~ShadowPirates() {                    // 0x481b30
    teardown();
    for (Sound *&s : snd) { delete s; s = nullptr; }
}

void ShadowPirates::teardown() {
    if (!made) return;
    on = false;
    made = false;
    rt->removeSprite("pirates");                     // 0x42da60
    for (Sound *s : snd) s->close();                 // 0x41fc50
}

// 0x481bd0: the boat at (1538, 769), 200 %, hidden; its sounds opened.
void ShadowPirates::start() {
    if (made) return;
    last = frame_time();
    on = false;
    made = true;
    rt->addSprite("pirates", "SHBX_CA_PIRrowin_", 0);   // 0x42d9d0
    spr = rt->findSprite("pirates");
    spr->load();                                     // 0x42ef20
    spr->scale = 200.0f;                             // +0x84
    spr->x = 1538.0f;                                // 0x44c04000
    spr->place();                                    // 0x42f7f0
    spr->z = 769.0f;                                 // 0x44404000
    spr->mode = 1;                                   // +0x88
    if (spr->anim) spr->anim->visible = false;       // +0x70 +0x7c
    state = 1;
    for (int i = 0; i < 6; i++) {
        snd[i]->setName(kPirateFx[i]);               // 0x424400
        snd[i]->open();                              // 0x41f9d0(0)
    }
    f6b4 = 7;                                        // +0x6b4 (port: the exe's store lands in snd[1]'s fields)
}

// 0x481ca0: the state's animation (rowing loops, the scream once), 50 ms a frame, 100 lower than its place.
void ShadowPirates::anim() {
    int m = 0;
    switch (state) {
    case 1: spr->setAnim("SHBX_CA_PIRrowin_"); m = 1; break;   // 0x42efb0
    case 2: spr->setAnim("SHBX_CA_PIRidle_"); m = 1; break;
    case 3: spr->setAnim("SHBX_CA_PIRscream_"); m = 0; break;
    case 4: spr->setAnim("SHBX_CA_PIRrowout_"); m = 1; break;
    default: break;                                  // port: the exe passes its argument's slot
    }
    spr->frame_ms = 0x32;                            // +0xac
    spr->anim->mode = m;                             // +0x104
    spr->place();
    spr->y = (float)((double)spr->y - 100.0f);       // 0x513da0
}

// The rowing out (with its sound), towards (2600, -260).
static void row_out(ShadowPirates *p) {
    p->tx = 2600.0f;                                 // 0x45228000
    p->tz = -260.0f;                                 // 0xc3820000
}

// 0x481d50: with the game on, a visit (at most the skill's number, 15 s apart, 1 in 200 a frame): the boat rows in
// from (771, 1139) to (1138, 769) with the skunk's warning. The game off, or 30 s on: it rows out. Then it moves
// (each axis' step shrunk by tenths to 10 or less); in, it idles (lines now and then, the skunk's "croc!"); out, it
// is gone.
void ShadowPirates::update() {
    uint32_t now = frame_time();
    bool game_on = skull_scene().shadow_on;          // 0x5df040
    if (!on) {
        if (game_on) {
            if (visits >= shadow_max_visits) return;  // 0x5dac80
            if (now - last <= 15000) return;
            if ((uint32_t)vision_random(1000) >= 5) return;   // (its own object +0x18)
            visits++;
            t_on = frame_time();
            snd[0]->play();                          // 0x41fd50 "pirrowin"
            VoiceOver::get().play("SHBX_VO_SKNpirate");   // 0x4cca20 on the skunk
            on = true;
            state = 1;
            anim();
            spr->anim->visible = true;
            spr->x = 771.0f;                         // 0x4440c000
            spr->z = 1139.0f;                        // 0x448e6000
            tx = 1138.0f;                            // 0x448e4000
            tz = 769.0f;                             // 0x44404000
            return;
        }
    }
    if (!game_on && state != 4) {
        snd[4]->play();                              // "pirrowout"
        state = 4;
        anim();
        row_out(this);
        snd[1]->stop();                              // 0x41fe70 "piridle"
    }
    if (on && now - t_on > 30000 && state != 4) {
        VoiceOver::get().play("SHBX_VO_SKNgone1");   // 0x4cca20 on the skunk (0x5de7d4)
        snd[4]->play();
        spr->setAnim("SHBX_CA_PIRrowout_");
        state = 4;
        spr->frame_ms = 0x32;
        spr->anim->mode = 1;
        spr->place();
        spr->y = (float)((double)spr->y - 100.0f);
        row_out(this);
        snd[1]->stop();
    }
    last = now;
    float dx = (float)((double)tx - spr->x);
    uint32_t fdt = now - frame_t;
    bool move = false;
    float dz = (float)((double)tz - spr->z);
    float d2 = (float)((double)dz * dz + (double)dx * dx);
    if (state == 1) {
        if (d2 < 400.0f) {                           // 0x514f04
            state = 2;
            snd[1]->play();
            next = frame_time() + 5000;
            anim();
        } else {
            move = true;
        }
    }
    if (state == 4) {
        if (d2 < 400.0f) {
            on = false;
            if (spr->anim) spr->anim->visible = false;
        } else {
            move = true;
        }
    }
    if (move) {
        double vx = dx;
        if (fabs(vx) > 10.0)                         // 0x515238
            do vx *= 0.9f; while (fabs(vx) > 10.0);  // 0x5154a0
        double vz = dz;
        if (fabs(vz) > 10.0)
            do vz *= 0.9f; while (fabs(vz) > 10.0);
        spr->x = (float)(vx + spr->x);
        spr->z = (float)(vz + spr->z);
    }
    if (fdt < 0x32) return;
    frame_t = now;
    if (spr->anim->current() < spr->anim->count() - 1) return;
    if (state == 2) {
        if ((uint32_t)vision_random(1000) < 100) VoiceOver::get().play("SHBX_VO_SKNcroc");
        for (Sound *s : snd)
            if (s->isPlaying()) return;              // 0x41fec0
        if (!(frame_time() > next)) return;
        switch (vision_random(3)) {                  // 0x425d40 (state +8)
        case 0: snd[1]->play(); break;
        case 1: snd[2]->play(); break;
        case 2: snd[3]->play(); break;
        default: break;
        }
        next = frame_time() + 5000;
    } else if (state == 3) {
        state = 2;
        spr->setAnim("SHBX_CA_PIRidle_");
        spr->frame_ms = 0x32;
        spr->anim->mode = 1;
        spr->place();
        spr->y = (float)((double)spr->y - 100.0f);
    }
}

// ---------------------------------------------------------------- ShadowGator
ShadowGator::ShadowGator(Raytrace::RayTrace *r) : rt(r) {
    for (Sound *&s : snd) s = new Sound("");         // 0x437290 x3
}

ShadowGator::~ShadowGator() {                        // 0x4821d0
    for (Sound *&s : snd) { delete s; s = nullptr; }
}

// 0x4821f0: an empty base at (1575, 925), 300 %, with the crocodile's eyes on it (looping); its sounds.
void ShadowGator::make() {
    if (made) return;
    made = true;
    state = 0;
    base = rt->addSprite("gator_base", "empty", 1);  // 0x42d9d0
    base->load();
    base->scale = 300.0f;                            // 0x43960000
    base->x = 1575.0f;                               // 0x44c4e000
    base->place();
    base->y = (float)((double)base->y - 50.0f);      // 0x515190
    base->z = 925.0f;                                // 0x44674000
    base->mode = 1;
    base->anim->visible = true;
    base->anim->mode = 1;
    att = base->attach("gator", "SHBX_CA_TICeyeid_", 100);   // 0x42f390
    att->on = true;
    static_cast<Animation *>(att->g)->mode = 1;
    for (int i = 0; i < 3; i++) {
        snd[i]->setName(kGatorFx[i]);
        if (i == 0) snd[i]->type = 6;                // +0x340: the ticking loops
        snd[i]->open();
    }
}

void ShadowGator::eyes() {                           // 0x482300
    Animation *g = static_cast<Animation *>(att->g);
    if (g->current() < g->count() - 1) return;
    state = 0;
    att->setPrefix("SHBX_CA_TICeyeid_");
    static_cast<Animation *>(att->g)->mode = 1;
}

// (inlined twice) at each animation's end: up, it snaps (the ticking and the snap's sound); snapped, it goes down;
// down, the eyes again.
void ShadowGator::step() {
    Animation *g = static_cast<Animation *>(att->g);
    switch (state) {
    case 1:
        if (g->current() < g->count() - 1) return;
        state = 2;
        snd[0]->play();
        snd[1]->play();                              // +0x35c "flyup"
        att->setPrefix("SHBX_CA_TICsnap_");
        static_cast<Animation *>(att->g)->mode = 0;
        return;
    case 2:
        if (g->current() < g->count() - 1) return;
        state = 3;
        snd[2]->play();                              // +0x6a4 "flydw_01"
        snd[0]->stop();
        att->setPrefix("SHBX_CA_TICflydw_");
        static_cast<Animation *>(att->g)->mode = 0;
        return;
    case 3:
        eyes();
        return;
    default: return;
    }
}

// ---------------------------------------------------------------- ShadowInterface
ShadowInterface::ShadowInterface()                   // 0x483740
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    setZ(0x1e);                                      // 0x411e70
}

ShadowInterface::~ShadowInterface() {                // 0x483810
    for (Image *&b : boxes) { delete b; b = nullptr; }
    delete duc; delete mon; delete rab; delete squ; delete tur; delete bar;
    duc = mon = rab = squ = tur = bar = nullptr;
}

static Image *iface_image(const char *n) {
    Image *i = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);   // 0x4243c0 (0xa8), 0x4179e0
    i->load();                                       // v15
    return i;
}

void ShadowInterface::load() {                       // v15 0x4838a0
    count = 0;
    for (int i = 0; i < 8; i++) {
        char n[0x20];
        snprintf(n, sizeof n, "SHBX_IF_boxtar%1.1d_01", i + 1);   // 0x53d71c
        boxes[i] = iface_image(n);
    }
    duc = iface_image("SHBX_IF_DUC_01");
    mon = iface_image("SHBX_IF_MON_01");
    rab = iface_image("SHBX_IF_RAB_01");
    squ = iface_image("SHBX_IF_SQU_01");
    tur = iface_image("SHBX_IF_TUR_01");
    bar = iface_image("SHBX_IF_interfacebar_01");
}

void ShadowInterface::unload() {                     // v16 0x483b20
    Graphic::unload();                               // 0x412020
    for (Image *&b : boxes) {
        if (!b) continue;
        b->unload();                                 // v16
        delete b;                                    // v3(1)
        b = nullptr;
    }
    for (Image **p : {&duc, &mon, &rab, &squ, &tur, &bar}) {
        if (!*p) continue;
        (*p)->unload();
        delete *p;
        *p = nullptr;
    }
}

// v14 0x483c50: the bar, then each catch's icon at its box's spot.
void ShadowInterface::draw() {
    bar->draw();                                     // v14
    for (int i = 0; i < count; i++) {
        Image *g = nullptr;
        switch (types[i]) {
        case 1: g = rab; break;
        case 2: g = mon; break;
        case 3: g = tur; break;
        case 4: g = squ; break;
        case 5: g = duc; break;
        default: break;                              // port: the exe draws its stale register (never reached)
        }
        if (!g) continue;
        g->x = boxes[i]->x;                          // +0x58
        g->y = boxes[i]->y;
        g->draw();
    }
}

// ---------------------------------------------------------------- ShadowBoxing
ShadowBoxing::ShadowBoxing(Raytrace::RayTrace *r, Screen *o) : rt(r), over(o), catch_snd("") {   // 0x482360
    nice.push_back("SHBX_VO_SKNnice1");              // 0x430dd0
    nice.push_back("SHBX_VO_SKNnice2");
    nice.push_back("SHBX_VO_SKNnice3");
    nice.push_back("SHBX_VO_SKNnice4");
}

ShadowBoxing::~ShadowBoxing() {                      // 0x482590 (port: teardown frees them; kept for safety)
    delete iface;
    delete group;
    delete pirates;
    delete gator;
}

// 0x482600: the five animals at their rocks, the bar (hidden) in the scene's layer, the pirates' boat, the
// crocodile, the fuse.
void ShadowBoxing::setup() {
    group = new ShadowGroup(rt);                     // 0x4f34f3 (0x2d4)
    group->add(2, 5);                                // (inlined) the duck
    group->add(3, 1);                                // 0x481870: the bunny
    group->add(1, 3);                                // the turtle
    group->add(4, 2);                                // the monkey
    group->add(5, 4);                                // the squirrel
    iface = Gorilla::gnew<ShadowInterface>();        // 0x4243c0 (0xf8)
    iface->load();                                   // v15
    over->layer(0)->add(iface);                      // 0x41ece0, 0x41e930
    iface->visible = false;                          // +0x7c
    pirates = new ShadowPirates(rt);                 // 0x4f34f3 (0x13e4)
    pirates->start();                                // 0x481bd0
    gator = new ShadowGator(rt);                     // 0x4f34f3 (0x9ec)
    gator->make();                                   // 0x4821f0
    fuse = &Game::cannon();                          // state +0x7c0
}

// 0x482830: the bar, the animals, the boat, the crocodile (its sprite asked for as "gator": the exe's; the base
// stays).
void ShadowBoxing::teardown() {
    iface->unload();                                 // v16
    delete iface;
    iface = nullptr;
    delete group;                                    // 0x481810
    group = nullptr;
    pirates->teardown();
    delete pirates;
    pirates = nullptr;
    for (Sound *s : gator->snd) s->close();
    rt->removeSprite("gator");                       // 0x42da60
    delete gator;
    gator = nullptr;
}

void ShadowBoxing::gatorDust() {                     // 0x482910
    if (gator->state != 0) return;
    gator->state = 1;
    gator->att->setPrefix("SHBX_CA_TICflyup_");
    static_cast<Animation *>(gator->att->g)->mode = 0;
}

int ShadowBoxing::count() { return iface->count; }  // 0x482950

// 0x482960: the game on (its tune, its bit), the fuse lit by the skill and "Info" (with the pirates' visits),
// the bar shown empty.
void ShadowBoxing::start() {
    Skull_Scene &sc = skull_scene();
    sc.shadow_on = true;                             // 0x5df040
    sc.music(0);                                     // 0x4894c0
    sc.bits |= 1;                                    // 0x5df03c
    on = 1;
    State &st = Game::state();
    int info = st.saves.gameInfo("Shadow Boxing Game");   // 0x4e9d70 (*0x543514)
    int skill;
    if (!st.saves.getSkill(&skill)) skill = 1;       // 0x4e97a0
    if (info == 0) {
        switch (skill) {
        case 1: fuse->start(0xb4); shadow_max_visits = 1; shadow_k = 8; break;   // 0x4e3a50
        case 2: fuse->start(0x78); shadow_max_visits = 2; shadow_k = 6; break;
        case 3: fuse->start(0x5a); shadow_max_visits = 2; shadow_k = 5; break;
        default: break;
        }
    } else {
        switch (skill) {
        case 1: fuse->start(0x78); shadow_max_visits = 2; shadow_k = 6; break;
        case 2: fuse->start(0x5a); shadow_max_visits = 2; shadow_k = 5; break;
        case 3: fuse->start(0x3c); shadow_max_visits = 3; shadow_k = 4; break;
        default: break;
        }
    }
    fuse->visible = true;                            // +0x7c
    iface->count = 0;                                // +0xd0
    iface->visible = true;
    st.last_game = 3;                                // +0x79c
    pirates->last = frame_time();                    // +0x10
    sc.inactivity2.start();                          // 0x4c15f0 (0x5dec9c)
}

// 0x482ab0: off, the animals come and go, the crocodile, the boat (its visits counted afresh); on, the game.
void ShadowBoxing::update() {
    if (on == 1) {
        playing();                                   // 0x482c20
        return;
    }
    if (on != 0) return;
    group->update();                                 // 0x4818b0
    gator->step();
    pirates->visits = 0;                             // +0x13e0
    if (pirates->on) pirates->update();              // 0x481d50
}

// 0x482c20: the fuse out: lost (the game's end flagged for the scene's run). Else the crocodile snapping at the
// idling boat sends it off (screaming); while the boat idles or screams, every animal up hides.
void ShadowBoxing::playing() {
    group->update();
    if (getenv("TB_SHADOW_AUTO") && count() < 8)     // test aid (not in the game): every animal up is caught
        for (int i = 0; i < group->count; i++)
            if (group->animals[i].up && group->animals[i].phase == 2) {
                click(group->animals[i].att);
                if (count() >= 8) {
                    skull_scene().shadowEnd();       // (as the scene's click would; the frame ends there)
                    return;
                }
                break;
            }
    if (!fuse->running()) {                          // 0x4e3b80
        if (getenv("TB_DEBUG_SKULL")) tb_log("skull: the fuse is out (%d caught, %u)", count(), frame_time());
        Skull_Scene &sc = skull_scene();
        State &st = Game::state();
        sc.bits |= 2;
        st.last_result = 1;                          // +0x7a0
        st.hover_up = false;                         // +0x75
        st.hover_down = false;                       // +0x76
        st.shadow_ended = true;                      // +0x7a5
        sc.inactivity2.stop();                       // 0x4c4690
        sc.shadow_on = false;
        sc.music(1);
        on = 0;
        fuse->hide();                                // 0x4e3bf0
        iface->visible = false;
        st.hover_up = false;
        st.hover_down = false;
        st.shadow_ended = true;
        sc.inactivity2.stop();
        return;
    }
    if (pirates->on) {
        if (gator->state == 2 && pirates->state == 2) {
            VoiceOver::get().play("SHBX_VO_SKNgone2");   // 0x4cca20 on the skunk
            pirates->snd[4]->play();                 // "pirrowout"
            pirates->snd[5]->play();                 // "scream"
            pirates->state = 4;
            pirates->anim();
            row_out(pirates);
            pirates->snd[1]->stop();
        }
        if (pirates->state == 3 || pirates->state == 2) {
            for (int i = 0; i < group->count; i++) {
                ShadowAnimal &a = group->animals[i];
                if (!a.up) continue;
                a.t0 = frame_time();
                if (a.phase == 3) continue;
                a.phase = 3;
                a.makePrefix();
                a.att->setPrefix(a.buf);
                static_cast<Animation *>(a.att->g)->mode = 0;
            }
        }
    }
    pirates->update();
    gator->step();
}

// 0x482fa0: an animal up under the net is caught: it hides, its type on the bar (eight at most), the skunk's line
// for the count (half way, one more, or now and then a "nice").
void ShadowBoxing::click(Attachment *a) {
    if (!a) return;
    ShadowAnimal *hit = nullptr;
    for (int i = 0; i < group->count; i++)
        if (group->animals[i].up && group->animals[i].att == a) { hit = &group->animals[i]; break; }
    if (!hit) return;
    hit->t0 = frame_time();
    if (hit->phase != 3) {
        hit->phase = 3;
        hit->makePrefix();
        hit->att->setPrefix(hit->buf);
        static_cast<Animation *>(hit->att->g)->mode = 0;
    }
    int type = 0;
    for (int i = 0; i < group->count; i++)
        if (group->animals[i].up && group->animals[i].att == a) { type = group->animals[i].type; break; }
    if (iface->count < 8) iface->types[iface->count++] = type;
    catch_snd.play();                                // +0x30 (never opened)
    switch (iface->count) {                          // 0x483294
    case 4: VoiceOver::get().play("SHBX_VO_SKNhalfwy"); break;   // 0x4c9400
    case 7: VoiceOver::get().play("SHBX_VO_SKN1more"); break;
    case 1: case 3: case 5: case 6:
        VoiceOver::get().play(nice.at((size_t)vision_random((int)nice.size())));
        break;
    default: break;
    }
}

bool ShadowBoxing::wantNet(Attachment *a) {          // 0x4832b0
    for (int i = 0; i < group->count; i++)
        if (group->animals[i].up && group->animals[i].att == a) return true;
    return false;
}

// 0x4832f0: Tink's help. With the boat in: to the crocodile (by way of a spot over the water), pointing at it.
// Else the rock nearest her (not the last caught kind): she flies 10 short of it a little to the side, at its
// height, then up and points at it.
void ShadowBoxing::hint() {
    Tinkerbell3D *tink = skull_scene().tink;         // 0x5dd3cc
    if (pirates->state == 3 || pirates->state == 2) {
        tink->pushBasic(2, 2174.24f, 500.0f, 1518.55f);   // 0x4507e3d7, 0x44bdd19a
        tink->pushBasic(2, 1556.62f, 500.0f, 769.0f);     // 0x44c293d7
        tink->point(1556.62f, 0.1f, 769.0f);         // 0x4db7b0
        return;
    }
    int best_i = 0;
    float best = 2.0e8f;                             // 0x4d3ebc20
    for (int i = 0; i < group->count; i++) {
        ShadowAnimal &an = group->animals[i];
        if (iface->count != 0 && iface->types[iface->count - 1] == an.type) continue;
        float rx = an.rock->x, rz = an.rock->z;
        float dx = (float)((double)tink->getX() - rx);   // 0x4cc9b0
        double dz = (double)tink->getZ() - rz;       // 0x4cc9d0
        double d2 = dz * dz + (double)dx * dx;
        if (d2 < best) {
            best = (float)d2;
            best_i = i;
        }
    }
    Sprite3D *rock = group->animals[best_i].rock;
    float rx = rock->x, rz = rock->z;
    float k = (float)((double)rock->scale * rock->y * 0.01f);   // 0x513d4c
    float dx = (float)((double)rx - rt->camx);       // 0x42b470
    double dz = (double)rz - rt->camz;               // 0x42b540
    float dist = (float)(sqrt(dz * dz + (double)dx * dx) - 10.0);   // 0x515238
    float ang = (float)(-atan2(dz, (double)dx) - (double)3.14159274f * 0.0444444f);   // 0x515450, 0x5154a4
    float nx = (float)(rt->camx + cos((double)ang) * dist);
    float nz = (float)(rt->camz - sin((double)ang) * dist);
    tink->pushBasic(2, nx, k, nz);                   // 0x4de020
    tink->pushBasic(3, 600.0f, 10000.0f, 0.0f);
    tink->point(rx, k, rz);
}

// 0x483590: the nearest rock to the camera (not the last caught kind): the eye eased towards its height, the
// faster the nearer.
void ShadowBoxing::hover() {
    float C[3];
    rt->camPos(C);                                   // 0x42b490
    int best_i = 0;
    float best = FLT_MAX;                            // 0x7f7fffff
    for (int i = 0; i < group->count; i++) {
        ShadowAnimal &an = group->animals[i];
        if (iface->count != 0 && iface->types[iface->count - 1] == an.type) continue;
        float P[3] = {an.rock->x, 0.1f, an.rock->z};
        double dx = (double)P[0] - C[0], dy = (double)P[1] - C[1], dz = (double)P[2] - C[2];
        double d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= (double)best * best) {             // 0x425da0
            best_i = i;
            best = (float)sqrt(d2);                  // 0x425d50
        }
    }
    Sprite3D *rock = group->animals[best_i].rock;
    float k = (float)((double)rock->scale * rock->y * 0.01f);
    float t = (float)eye_for(k);                     // 0x42b500
    double diff = (double)t - rt->eyef;              // 0x42b480
    double f = (double)best * 0.00666667f;           // 0x5154a8
    if (f < 1.0f) f = 1.0f;                          // 0x513710
    float v = (float)(diff / f);
    if (v > 0.0f) rt->raise(v);                      // 0x42b130
    else rt->lower(-v);                              // 0x42b2d0
}

}  // namespace Game
