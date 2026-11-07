// The Coconuts Away game, reconstructed from "Tinker Bell.exe" (see coconut.h).
#include "coconut.h"
#include "continue_dialog.h"
#include "cannon.h"
#include "state.h"
#include "toolbar.h"
#include "raytrace_scene.h"
#include "voiceover.h"
#include "../../../platform/dinput.h"
#include "tb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0
static void dbg(const char *m) { if (getenv("TB_DEBUG_COCONUT")) tb_log("coconut: %s (t=%u)", m, frame_time()); }

// 0x537f80: the boats' top speed (set by the skill; the exe's .data holds the value before the first game)
static float &g_speed() {
    static float v;
    static bool init = false;
    if (!init) {
        init = true;
        uint32_t u = tb_exe_u32(0x537f80);
        memcpy(&v, &u, sizeof v);
    }
    return v;
}

static Sound &pir_sink() {                           // 0x5b6da8 (a static Sound, made at start-up)
    static Sound *s = new Sound("COCO_VO_PIRsink");
    return *s;
}
static VoList *g_taunts = nullptr;                   // 0x5b9544: made by the first coconut's load
static SoundCycle *g_yells = nullptr;                // 0x5b9548: made by the first boat group

Coconut_Scene &coconut_scene() {                     // the static at 0x5b70f0
    static Coconut_Scene s;
    return s;
}

static Screen &scene_screen() { return coconut_scene().screen; }   // 0x5b722c / 0x5b7230: its w, h

// ---------------------------------------------------------------- SoundCycle
void SoundCycle::openAll() {                         // 0x4cc450
    for (size_t i = 0; i < sounds.size(); i++) sounds.at(i)->open();
}

void SoundCycle::closeAll() {                        // 0x4cc490
    for (size_t i = 0; i < sounds.size(); i++) {
        sounds.at(i)->stop();
        sounds.at(i)->close();
    }
}

void SoundCycle::playNext() {                        // 0x4cc4f0
    index++;
    if (index == (int)sounds.size()) index = 0;
    sounds.at(index)->stop();                        // 0x41fe70
    sounds.at(index)->play();
}

// ---------------------------------------------------------------- CocoGround
// v18 0x44a4f0: the coconuts and the boats move with the water; now and then a rowing boat's pirate yells.
void CocoGround::update() {
    ScrollGround::update();
    Coconut_Scene &sc = coconut_scene();
    float fdy = (float)dy, fdx = (float)dx;
    sc.mine.move(fdx, fdy);                          // 0x5b8618
    sc.its.move(fdx, fdy);                           // 0x5b8630
    BoatGroup *g = sc.boats;                         // 0x5b8614
    if (!g->running) return;
    for (size_t i = 0; i < g->boats.size(); i++) g->boats.at(i)->update(fdx, fdy);
    if (frame_time() <= g->next_yell) return;
    for (size_t i = 0; i < g->boats.size(); i++) {
        Boat *b = g->boats.at(i);
        if (!b->visible || !b->rowing()) continue;
        g_yells->playNext();                         // 0x4cc4f0
        g->next_yell = frame_time() + 7000;          // 0x1b58
        return;
    }
}

// ---------------------------------------------------------------- CocoCursor
CocoCursor::CocoCursor() : Graphic(GRAPHIC_ARGS), active("COCO_CU_active_"), notactive("COCO_CU_notactive_") {}

void CocoCursor::draw() {                            // v14 0x44a6e0
    if (!cur) return;
    cur->x = x;
    cur->y = y;
    cur->draw();
}

void CocoCursor::load() {                            // v15 0x44a630
    active.load();
    active.play(0, 1);
    notactive.load();
    notactive.play(0, 1);
    hand = Game::state().cursor_active;              // state +0x60
    static_cast<Animation *>(hand)->play(0, 1);
    cur = &notactive;
    normal = Game::state().cursor;                   // state +0x5c
}

void CocoCursor::unload() {                          // v16 0x44a6a0 (the base's unload is not called)
    active.unload();
    notactive.unload();
    hand = nullptr;
    normal = nullptr;
}

// ---------------------------------------------------------------- NumberShow
void NumberShow::load() {                            // v15 0x44cd90
    x = 0;
    y = 0;
    setZ(5);
    value = 0;
}

// v14 0x44cdd0: the tens (when there are any) left of the middle, the units right of it (or in the middle).
void NumberShow::draw() {
    State &st = Game::state();
    int ones = value % 10, tens = value / 10;
    if (tens) {
        Image *t = small ? st.num_small[tens] : st.num_big[tens];   // +0x34 / +0xc
        t->x = x - t->ow / 2 - 1;                    // +0x3c
        t->y = y;
        t->draw();
    }
    Image *o = small ? st.num_small[ones] : st.num_big[ones];
    if (!tens) o->x = x;
    else o->x = o->ow / 2 + x + 1;
    o->y = y;
    o->draw();
}

// ---------------------------------------------------------------- CocoScore
CocoScore::CocoScore() : Graphic(GRAPHIC_ARGS), bar("COCO_IF_interface_01", 0, 0, 0, 0, 0, 0, true) {   // 0x44a1d0
    bar.load();                                      // v15
    player.load();
    player.small = true;                             // +0x1dc (the exe stores a pointer: non-zero)
    player.x = -285;                                 // 0xfffffee3
    player.y = 0;
    rabbit.load();
    rabbit.small = true;
    rabbit.x = 285;                                  // 0x11d
    rabbit.y = 0;
    player.value = 0;
    rabbit.value = 0;
}

CocoScore::~CocoScore() {                            // 0x44a3a0
    bar.unload();
    player.unload();
    rabbit.unload();
}

void CocoScore::draw() {                             // v14 0x44a4b0
    bar.draw();
    player.draw();
    rabbit.draw();
}

// ---------------------------------------------------------------- Boat
static Animation *boat_anim(char n, const char *what, int hitmode) {
    std::string name = std::string("COCO_GA_PIR") + n + what;   // 0x44cf20, 0x44d1b0
    Animation *a = Gorilla::gnew<Animation>(name.c_str());   // 0x4243c0(0x118), 0x404c20
    a->load();                                       // v15
    a->setHit(hitmode, true);                        // v6
    a->setScale(65.0);                               // v5 (0x40504000 00000000)
    return a;
}

// 0x44b7a0: the three boats' rowing (hit-tested), sinking and floating-wreck animations; a new boat in its lane.
Boat::Boat(int l) : Graphic(GRAPHIC_ARGS), lane(l) {
    for (char c = '1'; c <= '3'; c++) row.push_back(boat_anim(c, "row_", 1));
    for (char c = '1'; c <= '3'; c++) sink.push_back(boat_anim(c, "sink_", 0));
    for (char c = '1'; c <= '3'; c++) water.push_back(boat_anim(c, "water_", 0));
    (void)vision_random((int)row.size());            // a pick the exe does not use
    reset();                                         // 0x44c1a0
}

Boat::~Boat() {                                      // 0x44bd70
    for (Animation *a : row) delete a;
    for (Animation *a : sink) delete a;
    for (Animation *a : water) delete a;
    cur = nullptr;
}

bool Boat::rowing() const {
    for (Animation *a : row) if (a == cur) return true;
    return false;
}

// 0x44bf80: under the point it sinks (one of the sinking animations, at its place).
bool Boat::hitAt(int px, int py) {
    if (!cur->hit(px, py)) return false;             // v11
    Animation *s = sink.at(vision_random((int)sink.size()));
    s->x = cur->x;
    s->y = cur->y;
    s->setFrame(0);
    s->play(0, 0);
    cur = s;
    return true;
}

// 0x44c030: a sunk boat leaves a floating wreck; a rowing one moves at its own speed besides the water's; off the
// bottom it starts again.
void Boat::update(float fdx, float fdy) {
    if (!cur) return;
    bool sinking = false;
    for (Animation *a : sink) if (a == cur) sinking = true;
    double ddy = fdy;
    if (sinking && cur->current() + 1 == cur->count()) {
        Animation *w = water.at(vision_random((int)water.size()));
        w->x = cur->x;
        w->y = cur->y;
        w->setFrame(0);
        w->play(0, 1);
        cur = w;
    } else if (rowing()) {
        ddy = (float)(fdy + speed);
    }
    Animation *c = cur;
    if ((double)(c->h / 2) + 240.0 < (double)fy) {   // 0x513f1c
        reset();
        return;
    }
    fy = (float)(ddy + fy);
    c->x = ftol((double)c->x + fdx);
    c->y = ftol(fy);
}

// 0x44c1a0: a rowing animation at random; somewhere in its fifth of the water's width; a random speed between 0.8
// and the skill's; a random height above the screen.
void Boat::reset() {
    cur = row.at(vision_random((int)row.size()));
    cur->setFrame(0);
    int fifth = coconut_scene().ground.img1.w / 5;   // 0x5b815c
    int half = row.at(0)->w / 2;                     // +0x64
    int r = vision_random(fifth - half) + half;
    int pct = vision_random(101);                    // 0x65
    float f = (float)((double)pct * 0.01f);          // 0x513d4c
    speed = (float)((g_speed() - 0.8f) * (double)f + 0.8f);   // 0x51452c
    cur->x = lane * fifth - coconut_scene().ground.img1.w / 2 + r;
    int h = row.at(0)->h;                            // +0x68
    int sh = scene_screen().h;                       // 0x5b7230
    int r2 = vision_random(sh);
    fy = (float)-((double)(sh + h) * 0.5f + (double)r2);
    cur->y = ftol(fy);
    cur->play(0, 1);
    visible = true;
}

// ---------------------------------------------------------------- BoatGroup
// 0x44c370: five boats, one per lane; the pirates' yells (made once); the first yell in 7 s.
BoatGroup::BoatGroup() {
    for (int i = 0; i < 5; i++) boats.push_back(Gorilla::gnew<Boat>(i));   // 0x4243c0(0xe0)
    if (!g_yells) g_yells = new SoundCycle();        // 0x4f34f3(0x14), 0x4cc220
    if (g_yells->sounds.empty()) {
        g_yells->add("COCO_VO_PIRyell1");
        g_yells->add("COCO_VO_PIRyell2");
        g_yells->add("COCO_VO_PIRyell3");
        g_yells->add("COCO_VO_PIRyell4");
    }
    g_yells->openAll();                              // 0x4cc450
    next_yell = frame_time() + 7000;
}

BoatGroup::~BoatGroup() {                            // 0x44c4c0
    for (Boat *b : boats) delete b;
    boats.clear();
    g_yells->closeAll();                             // 0x4cc490
}

Boat *BoatGroup::target() {                          // 0x44c580
    std::vector<Boat *> v;
    for (size_t i = 0; i < boats.size(); i++)
        if (boats.at(i)->visible && boats.at(i)->rowing()) v.push_back(boats.at(i));
    if (v.empty()) return nullptr;
    return v.at(vision_random((int)v.size()));
}

// ---------------------------------------------------------------- CocoItem
CocoItem::CocoItem() : coconut("COCO_GA_coconut_"), splash("COCO_GA_splash_") {}   // 0x44c700

// 0x44c770: both hidden at depth 80; the pirates' taunts (made once).
void CocoItem::load() {
    coconut.load();
    coconut.setZ(80);
    coconut.visible = false;
    splash.load();
    splash.setZ(80);
    splash.visible = false;
    cur = &coconut;
    if (!g_taunts) g_taunts = new VoList();          // 0x4f34f3(0x34), 0x4cc080
    if (g_taunts->lines.empty()) {
        g_taunts->add("COCO_VO_PIRtaunt1");
        g_taunts->add("COCO_VO_PIRtaunt2");
        g_taunts->add("COCO_VO_PIRtaunt3");
    }
}

void CocoItem::addTo(Screen *s) {                    // 0x44c850
    s->layer(0)->add(&coconut);
    s->layer(0)->add(&splash);
}

// 0x44c890: off the bottom it is gone; after two seconds in the air it splashes (the miss sound even when it sank
// a boat); a splash that has played out ends it, and a player's coconut that sank nothing gets a pirate's taunt.
void CocoItem::update(float fdx, float fdy, int kind) {
    if (!active) return;
    uint32_t now = frame_time();
    if ((double)(cur->h / 2) + 240.0 < (double)cur->y) {
        active = false;
        cur->visible = false;
        return;
    }
    if (cur != &splash) {
        if (now - t0 > 2000) {                       // 0x7d0
            coconut_scene().sounds[3].play();        // 0x5b7c24: miss
            cur->visible = false;
            splash.x = cur->x;
            splash.y = cur->y;
            cur = &splash;
            splash.play(0, 0);
            cur->visible = true;
            return;
        }
    } else if (splash.current() + 1 == splash.count()) {
        active = false;
        cur->visible = false;
        if (!hit && kind == 1 && !g_taunts->anyPlaying()) g_taunts->playNext();
        hit = false;
        return;
    }
    cur->x = ftol((double)cur->x + fdx);
    cur->y = ftol((double)cur->y + fdy);
}

// 0x44cb70: a coconut down for more than two seconds sinks the first rowing boat under it (a point to its thrower,
// at most 99; the player's sometimes gets a pirate's cry); then each moves on.
void CocoGroup::move(float fdx, float fdy) {
    Coconut_Scene &sc = coconut_scene();
    for (size_t i = 0; i < items.size(); i++) {
        CocoItem *it = items.at(i);
        if (it->active && frame_time() - it->t0 > 2000) {
            int bx = it->cur->x, by = it->cur->y;
            BoatGroup *g = sc.boats;                 // 0x5b8614
            for (size_t j = 0; j < g->boats.size(); j++) {
                if (!g->boats.at(j)->hitAt(bx, by)) continue;
                it->hit = true;
                sc.sounds[2].play();                 // 0x5b78dc: boathit
                if (kind == 1) {
                    int &v = sc.score->player.value; // 0x5b8520 +0x1d8
                    if (++v > 99) v = 99;
                    if (!pir_sink().isPlaying() && vision_random(2) != 0) pir_sink().play();   // state +8
                } else {
                    int &v = sc.score->rabbit.value; // +0x278
                    if (++v > 99) v = 99;
                }
                break;
            }
        }
        items.at(i)->update(fdx, fdy, kind);
    }
}

// ---------------------------------------------------------------- Rabbit
static Animation *rabbit_anim(const char *fmt, const char *who) {
    char n[0x28];
    snprintf(n, sizeof n, fmt, who);                 // 0x4f396f
    return Gorilla::gnew<Animation>(n);              // 0x4243c0(0x118)
}

Rabbit::Rabbit(int ch, const char *who) : Graphic(GRAPHIC_ARGS), chance(ch) {   // 0x44a710
    idle = rabbit_anim("COCO_CA_%sFLYidle_", who);
    fly_no = rabbit_anim("COCO_CA_%sFLYno_", who);
    fly_so = rabbit_anim("COCO_CA_%sFLYso_", who);
    fly_ea = rabbit_anim("COCO_CA_%sFLYea_", who);
    fly_we = rabbit_anim("COCO_CA_%sFLYwe_", who);
    str_no = rabbit_anim("COCO_CA_%sSTRno_", who);
    str_so = rabbit_anim("COCO_CA_%sSTRso_", who);
    str_ea = rabbit_anim("COCO_CA_%sSTRea_", who);
    str_we = rabbit_anim("COCO_CA_%sSTRwe_", who);
    thrw = rabbit_anim("COCO_CA_%sTHROW_", who);
}

Rabbit::~Rabbit() {                                  // 0x44aa90
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we, str_no, str_so, str_ea, str_we, thrw}) delete a;
}

// v15 0x44abb0: at the middle, far target, landed; the flying ones loop.
void Rabbit::load() {
    x = 0;
    y = 0;
    tx = ty = -999999;                               // 0xfff0bdc1
    phase = 0;
    dir = 0;
    anim_t = frame_time();
    step_t = frame_time();
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we}) {
        a->load();
        a->mode = 1;                                 // +0x104
        a->setHit(1, false);
    }
    for (Animation *a : {str_no, str_so, str_ea, str_we, thrw}) {
        a->load();
        a->setHit(1, false);
    }
}

void Rabbit::unload() {                              // v16 0x44ad30
    Graphic::unload();
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we, str_no, str_so, str_ea, str_we, thrw}) a->unload();
}

Animation *Rabbit::strAnim() {
    switch (dir) {
    case 1: return str_no;
    case 2: return str_so;
    case 3: return str_ea;
    case 4: return str_we;
    }
    return nullptr;
}

Animation *Rabbit::flyAnim() {
    switch (dir) {
    case 1: return fly_no;
    case 2: return fly_so;
    case 3: return fly_ea;
    case 4: return fly_we;
    }
    return nullptr;
}

Animation *Rabbit::current() {                       // the tables 0x44b72c / 0x448618
    switch (phase) {
    case 0: case 4: case 6: return idle;
    case 1: case 3: return strAnim();
    case 2: return flyAnim();
    case 5: return thrw;
    }
    return nullptr;
}

// 0x44adb0: over a rowing boat where it will be (its speed ahead, a quarter of the distance more) when all of the
// rabbit fits on the screen; it will throw there.
void Rabbit::aim() {
    Boat *b = coconut_scene().boats->target();       // 0x44c580
    if (!b) return;
    int bx = 0, by = 0;                              // port: the exe reads two stale locals when the boat has no animation
    if (b->cur) {
        bx = b->cur->x;
        by = b->cur->y;
    }
    tx = bx;
    double ddx = bx - x, ddy = by - y;
    double dist = sqrt(ddy * ddy + ddx * ddx);
    int nty = ftol((double)b->speed * 175.0 + dist * 0.25 + by);   // 0x514778, 0x51477c
    ty = nty;
    int w2 = scene_screen().w / 2, h2 = scene_screen().h / 2;   // 0x5b722c / 0x5b7230
    auto in = [](int v, int lim) { return v < lim && v > -lim; };
    if (in(tx - 30, w2) && in(nty, h2) && in(tx + 30, w2) && in(nty, h2) && in(tx, w2) && in(nty - 100, h2) &&
        in(tx, w2) && in(nty + 60, h2)) {
        throws = true;
        phase = 1;
    } else {
        throws = false;
        phase = 0;
    }
}

// 0x44af00: by chance after a boat, else to a random spot (throwing three times in four); off, facing the way.
void Rabbit::choose() {
    if ((unsigned)vision_random(100) < (unsigned)chance) {   // +0xc0
        aim();
    } else {
        int w = scene_screen().w - 30, h = scene_screen().h - 60;
        tx = vision_random(w) - w / 2;
        ty = vision_random(h) - h / 2;
        throws = vision_random(4) != 0;
        phase = 1;
    }
    step_t = frame_time();
    if (phase != 1) return;
    int ddx = tx - x, ddy = ty - y;
    if (abs(ddx) > abs(ddy)) dir = ddx < 0 ? 4 : 3;
    else dir = ddy < 0 ? 1 : 2;
    Animation *a = strAnim();
    if (a) a->setFrame(0);                           // port: the exe calls it on null for no direction (never)
}

// 0x44b090: every 30 ms an eighth of the way (half that while taking off, at least a pixel); close enough (under 2
// across and 5 down a step), it lands.
bool Rabbit::step() {
    uint32_t now = frame_time();
    if (now - step_t < 30) return false;
    step_t = now;
    if (phase == 3) {
        if (strAnim()->current() != 0) return false;
        phase = 0;
        idle->setFrame(0);
        return true;
    }
    int ex = tx - x, ey = ty - y;
    int mx = ex / 8, my = ey / 8;
    if (phase == 1) {
        mx /= 2;
        my /= 2;
    }
    if (mx == 0 && ex != 0) mx = ex / abs(ex);
    if (my == 0 && ey != 0) my = ey / abs(ey);
    if (abs(mx) < 2 && abs(my) < 5) {
        phase = 3;
        Animation *a = strAnim();
        a->setFrame(a->count() - 1);
    }
    x += mx;
    y += my;
    return false;
}

// 0x44b240: the animation by the phase; landed, it throws a coconut of its own (the exe looks the free one up by the
// last throw's time instead of a counter, so only its first throw of a game finds one) or chooses again.
void Rabbit::animate() {
    switch (phase) {
    case 0:
    case 6:
        idle->step();
        return;
    case 1: {
        Animation *a = strAnim();
        if (a->current() >= a->count() - 1) {
            phase = 2;
            Animation *f = flyAnim();
            if (f) f->setFrame(0);
        } else {
            a->step();
        }
        return;
    }
    case 2:
        flyAnim()->step();
        return;
    case 3: {
        Animation *a = strAnim();
        if (a->current() != 0) {
            a->setFrame(a->current() - 1);
            return;
        }
        if (!throws) {
            phase = 4;
            return;
        }
        int bx = x, by = y - 70;                     // 0x46
        phase = 5;
        uint32_t now = frame_time();
        CocoGroup &g = coconut_scene().its;          // 0x5b8630
        uint32_t old = g.t;
        if (now - old >= 750) {                      // 0x2ee
            g.t = now;
            uint32_t idx = old;                      // the exe's index (the time, not a counter)
            if (idx < g.items.size() && !g.items.at(idx)->active) {
                // port: when that item is in the air the exe loops on it for ever; here it is skipped
                coconut_scene().sounds[0].play();    // 0x5b724c
                coconut_scene().sounds[1].play();    // 0x5b7594
                CocoItem *it = g.items.at(idx);
                it->cur = &it->coconut;
                it->coconut.fps = (int)((uint32_t)(it->coconut.count() * 1000) / 2000u);
                it->coconut.x = bx;
                it->coconut.y = by;
                it->coconut.setFrame(0);
                it->coconut.play(0, 0);
                it->active = true;
                it->coconut.visible = true;
                it->t0 = frame_time();
            }
        }
        thrw->setFrame(0);
        return;
    }
    case 5:
        if (thrw->current() >= thrw->count() - 1) {
            phase = 4;
            idle->setFrame(0);
        } else {
            thrw->step();
        }
        return;
    }
}

// v14 0x44b5f0: drawn by the phase; every 30 ms: idle, one time in ten it chooses; flying, a step; choosing, choose.
void Rabbit::draw() {
    if (!visible) return;
    Animation *a = current();
    a->x = x;
    a->y = y;
    a->draw();
    uint32_t now = frame_time();
    if (now - anim_t < 30) return;
    anim_t = now;
    if (phase == 0) {
        if ((unsigned)vision_random(1000) < 100) phase = 4;   // 0x3e8, 0x64
    } else if (phase == 2) {
        step();
    } else if (phase == 4) {
        choose();
    }
    animate();
}

// ---------------------------------------------------------------- Coconut_Scene
Coconut_Scene::Coconut_Scene()                       // 0x449410
    : Game2D_Scene("coconut"),                       // 0x537fc8
      rabbit(70, "rab"),                             // 0x46
      music(nullptr),
      guide("COCO_VO_PETgreet1", "COCO_VO_PETgreet2") {
    mine.kind = 1;
    its.kind = 2;
    for (int i = 0; i < 3; i++) mine.items.push_back(new CocoItem());   // 0x4f34f3(0x240)
    for (int i = 0; i < 3; i++) its.items.push_back(new CocoItem());
    keyboard.setClient(this);
    mouse.setClient(this);
    pir_sink();                                      // the static's constructor
}

Coconut_Scene::~Coconut_Scene() {}                   // 0x4497a0

// v18 0x449a80
void Coconut_Scene::load2() {
    music.setName("COCO_MU_music");
    music.mode = 2;                                  // +0x18a0
    music.type = 6;                                  // +0x18a8
    static const char *const names[4] = {"coco_fx_cocofall", "coco_fx_throw", "coco_fx_boathit", "coco_fx_miss"};   // 0x537f70
    for (int i = 0; i < 4; i++) sounds[i].setName(names[i]);
    inactivity.add("COCO_VO_PETprompt", false);
    endvo.setWins("COCO_VO_PETtbwins_01", "COCO_VO_PETtbwins_02", "COCO_VO_PETtbwins_03");
    endvo.setLoses("COCO_VO_PETtblose_01", "COCO_VO_PETtblose_02", "COCO_VO_PETtblose_03");
    endvo.setTie("COCO_VO_PETtie_01");
    endvo.setWinChallenge("COCO_VO_PETwinsch_01", "COCO_VO_PETwinsch_02", "COCO_VO_PETwinsch_03");
    endvo.setAward("COCO_VO_PETaward");
    endvo.setLoseChallenge("COCO_VO_PETlosech_01", "COCO_VO_PETlosech_02", "COCO_VO_PETlosech_03");
    endvo.setTie2("COCO_VO_PETtie_02");
}

// v19 0x449b70: the rabbit's badge; the "coconuts_away" pack; the cursor; the score bar; the water; the rabbit
// (hidden); five boats (depth 90, hidden); the coconuts; the cannon; the end scoreboard; the music; Peter (unless he
// is already loaded); the sounds; input.
void Coconut_Scene::enter() {
    Game2D_Scene::enter();
    badge->detach();
    delete badge;
    badge = Gorilla::gnew<PlayerBadge>("GLOB_IF_RABheadsm_01");
    badge->setZ(-1000);
    screen.layer(0)->add(badge);
    pack = ras_open("coconuts_away", nullptr, &data_device());
    pack->load();
    ras_set_current(pack);
    screen.activate();
    screen.loadAll();
    cursor.load();                                   // v15
    set_cursor(&cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    hide_cursor();
    score = Gorilla::gnew<CocoScore>();              // 0x4243c0(0x280)
    score->setZ(-1000);
    screen.layer(0)->add(score);
    ground.load();
    screen.layer(0)->add(&ground);
    rabbit.load();
    rabbit.visible = false;                          // +0x14b4
    screen.layer(0)->add(&rabbit);
    boats = new BoatGroup();                         // 0x4f34f3(0x1c)
    for (size_t i = 0; i < boats->boats.size(); i++) {
        boats->boats.at(i)->setZ(90);                // 0x5a
        screen.layer(0)->add(boats->boats.at(i));
    }
    for (size_t i = 0; i < boats->boats.size(); i++) boats->boats.at(i)->visible = false;
    boats->running = false;
    for (size_t i = 0; i < mine.items.size(); i++) mine.items.at(i)->load();
    if (!pir_sink().isOpen()) pir_sink().open();     // 0x41f9c0 (+0x334)
    for (size_t i = 0; i < mine.items.size(); i++) mine.items.at(i)->addTo(&screen);
    for (size_t i = 0; i < its.items.size(); i++) its.items.at(i)->load();
    if (!pir_sink().isOpen()) pir_sink().open();
    for (size_t i = 0; i < its.items.size(); i++) its.items.at(i)->addTo(&screen);
    cannon = &Game::cannon();                        // state +0x7c0
    screen.layer(0)->add(cannon);
    cannon->visible = true;
    board = Gorilla::gnew<Scoreboard>("COCO_IF_scorescr_01");
    board->setZ(-1000);
    board->visible = false;
    screen.layer(0)->add(board);
    music.close();
    music.open();
    if (!Game::state().guide_loaded) {               // +0x7b8
        guide.load(false);
        guide.setZ(-950);
        guide.Task::activate();                      // +0x1950 v6
        screen.layer(0)->add(&guide);
    }
    for (int i = 0; i < 4; i++) sounds[i].open();
    input_on = false;
    mouse.activate();
    keyboard.activate();
    reset();                                         // 0x421110
    activate();
}

// 0x448850: by the skill: the cannon's seconds and the boats' top speed; this game recorded; the rabbit back in
// the middle; new boats; no coconuts in the air; the fuse lit.
void Coconut_Scene::setup() {
    int skill;
    if (!Game::state().saves.getSkill(&skill)) skill = 1;
    Game::state().last_game = 2;                     // +0x79c
    switch (skill) {
    case 1: secs = 90; g_speed() = 1.6f; break;
    case 2: secs = 90; g_speed() = 2.2f; break;
    case 3: secs = 60; g_speed() = 3.0f; break;
    }
    rab = &rabbit;                                   // +0x1520
    rabbit.visible = true;
    escaped = false;
    score->player.value = 0;
    score->rabbit.value = 0;
    rab->x = 0;
    rab->y = 0;
    rab->tx = 0;
    rab->ty = 0;
    rab->phase = 0;
    rab->dir = 0;
    rab->visible = true;
    for (size_t i = 0; i < boats->boats.size(); i++) boats->boats.at(i)->reset();
    boats->running = true;
    for (CocoGroup *g : {&mine, &its})
        for (size_t i = 0; i < g->items.size(); i++) {
            g->items.at(i)->active = false;          // +0x23c
            if (g->items.at(i)->cur) g->items.at(i)->cur->visible = false;
        }
    cannon->start(secs);                             // 0x4e3a50
}

// 0x449340: the rabbit away; how it ended (state +0x7a0), nothing when skipped.
void Coconut_Scene::result() {
    rab->visible = false;
    if (escaped) return;
    State &st = Game::state();
    if (score->player.value > score->rabbit.value) {
        int info = st.saves.gameInfo("Coconuts Away Game");   // *0x543524
        if (info == 0) st.last_result = 2;
        else if (info == 1) st.last_result = 3;
        else st.last_result = 4;
    } else if (score->rabbit.value > score->player.value) {
        st.last_result = 1;
    } else if (score->rabbit.value == score->player.value) {
        st.last_result = 5;
    }
}

// v10 0x448a20
void Coconut_Scene::run() {
    wait(1);
    hide_cursor();
    input_on = false;
    clouds->reset();
    dbg("run");
    while (guide.Task::active()) wait(0);            // +0x1950 v5
    music.play();
    setup();                                         // 0x448850
    input_on = true;
    cursor.cur = &cursor.active;                     // +0x1428 = +0x11f0
    set_cursor(&cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    show_cursor();
    while (clouds->fall()) wait(30);
    rab->anim_t = frame_time();
    rab->step_t = frame_time();
    rab->phase = 4;
    cannon->start(secs);
    inactivity.start();                              // v12
    mine.t = 0;                                      // +0x1528
    playing = true;
    dbg("play");
    while (cannon->timeLeft() && !escaped) {
        if (map->hit(g_mouse.x, g_mouse.y)) cursor.cur = cursor.hand;   // v11
        else if (frame_time() - mine.t > 750) cursor.cur = &cursor.active;   // 0x2ee
        else cursor.cur = &cursor.notactive;
        wait(0);
    }
    dbg("play over");
    input_on = false;
    rab->phase = 6;
    hide_cursor();
    if (!escaped) {
        inactivity.stop();                           // v13
        clouds->cover();
        while (clouds->rise()) wait(30);
        for (size_t i = 0; i < boats->boats.size(); i++) boats->boats.at(i)->visible = false;
        boats->running = false;
        board->s2 = score->rabbit.value;
        board->s1 = score->player.value;
        board->visible = true;
        result();                                    // 0x449340
        endvo.playEnd();
        while (endvo.playing()) wait(0);
    }
    if (escaped) {
        fader->visible = true;
        fader->start(false, 2000);
        hide_cursor();
        while (fader->busy()) wait(0);
    }
    show_cursor();
    cursor.cur = cursor.normal;                      // +0x1424
    if (score->player.value > score->rabbit.value || escaped) {
        change_scene("Mermaid");                     // 0x537fe0
    } else {
        dialog = Gorilla::gnew<ContinueDialog>("glob_if_contgame_01", "glob_if_signglow_01", "glob_if_xglow_01");
        dialog->setAsk("coco_vo_petbegcon_01");
        dialog->setYes("coco_vo_petbegyes_01");
        dialog->setNo("coco_vo_petbegno_01");
        dialog->setIdle("coco_vo_petbegidl_01");
        keyboard.deactivate();                       // v7
        call(dialog);
        wait(1);
    }
    finish();
}

// v9 0x4487a0: the dialog's answer: again (the run restarted in place) or off to the lagoon.
void Coconut_Scene::notify2(Service *s) {
    if (current_scene() != this) return;
    if (s == &mouse) {
        onMouse();
        return;
    }
    if (s == &keyboard) {
        onKey();
        return;
    }
    if (s == dialog) {
        if (dialog->yes) {                           // +0x6f1
            coconut_scene().Task::reset();           // 0x421110 on the static
            board->visible = false;
        } else if (dialog->no) {                     // +0x6f0
            change_scene("Mermaid");
        }
        dialog = nullptr;
        mouse.activate();
        keyboard.activate();
    }
}

// 0x448340: the map icon; a click (three a second at most, while the fuse burns, not on the rabbit) drops a coconut.
void Coconut_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (!input_on) cursor.cur = cursor.normal;
    if (!playing) return;
    if (map->hit(g_mouse.x, g_mouse.y)) {
        cursor.cur = cursor.hand;
        if (mouse_down() && map->click(g_mouse.x, g_mouse.y, true) == 2) change_scene("map");
        return;
    }
    if (!mouse_down()) return;
    int mx = g_mouse.x, my = g_mouse.y;
    Animation *a = rab->current();
    a->x = rab->x;
    a->y = rab->y;
    if (a->hit(mx, my)) return;
    if (!Game::cannon().timeLeft()) return;          // state +0x7c0
    uint32_t now = frame_time();
    if (now - mine.t < 750) return;
    mine.t = now;
    for (size_t i = 0; i < mine.items.size(); i++) {
        CocoItem *it = mine.items.at(i);
        if (it->active) continue;
        sounds[0].play();                            // 0x5b724c cocofall
        sounds[1].play();                            // 0x5b7594 throw
        it->cur = &it->coconut;
        it->coconut.fps = (int)((uint32_t)(it->coconut.count() * 1000) / 2000u);
        it->coconut.x = mx;
        it->coconut.y = my;
        it->coconut.setFrame(0);
        it->coconut.play(0, 0);
        it->active = true;
        it->coconut.visible = true;
        it->t0 = frame_time();
        return;
    }
}

// 0x448670: as the mud pie game's.
void Coconut_Scene::onKey() {
    State &st = Game::state();
    if (!playing) {
        if ((int)g_key.vk == st.skip_key) guide.stop();
        return;
    }
    ground.setMult(2);
    if (g_key.down) {
        if ((int)g_key.vk == st.skip_key) {
            st.last_game = 5;
            escaped = true;
        } else if (st.key((int)g_key.vk)) {
            return;
        }
    }
    if (key_down(0x26)) ground.setMult(4);
    if (key_down(0x28)) ground.setMult(1);
    if (key_down(0x27)) ground.setDir(2);
    if (key_down(0x25)) ground.setDir(1);
    if (!key_down(0x25) && !key_down(0x27)) ground.setDir(0);
    if (key_down(0x25) && key_down(0x27)) ground.setDir(0);
}

// v20 0x44a060
void Coconut_Scene::leave() {
    VoiceOver::get().stop();
    for (int i = 0; i < 4; i++) sounds[i].close();
    Game2D_Scene::leave();
    ground.unload();
    cursor.unload();
    rabbit.unload();
    delete boats;
    boats = nullptr;
    delete score;
    score = nullptr;
    rab = nullptr;
    guide.unload();
    delete board;
    board = nullptr;
    if (cannon) cannon->detach();
    ras_release(pack);
    pack = nullptr;
    screen.deactivate();
    screen.unloadAll();
    screen.clear();
    deactivate();
    keyboard.deactivate();
    mouse.deactivate();
    music.close();
    inactivity.stop();                               // v13
    tasks_finish_all();                              // 0x4210d0
    set_cursor(Game::state().cursor, true);
    Game::state().guide_loaded = false;
}

}  // namespace Game
