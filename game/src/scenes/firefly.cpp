// The Firefly game, reconstructed from "Tinker Bell.exe" (see firefly.h).
#include "firefly.h"
#include "continue_dialog.h"
#include "cannon.h"
#include "state.h"
#include "toolbar.h"
#include "raytrace_scene.h"
#include "voiceover.h"
#include "../../../platform/dinput.h"
#include "tb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;

static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0

Firefly_Scene &firefly_scene() {                     // the static at 0x5d91b0
    static Firefly_Scene s;
    return s;
}
static Firefly_Scene &S() { return firefly_scene(); }

// 0x5dac74 / 0x5dac78: the swarms' speed and how many turtles walk, by the skill
static float g_speed = 0;
static int g_turtles = 0;

static Animation *new_anim(const char *n) {
    return Gorilla::gnew<Animation>(n);              // 0x4243c0(0x118)
}

// ---------------------------------------------------------------- FireflyCursor
FireflyCursor::FireflyCursor() : Graphic(GRAPHIC_ARGS), hand_anim("glob_cu_active_") {   // 0x47bda0
    static const char *const k[7] = {"emp", "16", "26", "36", "46", "56", "ful"};
    char buf[64];
    for (int i = 0; i < 7; i++) {
        snprintf(buf, sizeof buf, "dcbl_cu_%sjarID_", k[i]);
        idle[i] = new_anim(buf);
    }
    for (int i = 0; i < 7; i++) {
        snprintf(buf, sizeof buf, "dcbl_cu_%sjar_", k[i]);
        ready[i] = new_anim(buf);
    }
}

FireflyCursor::~FireflyCursor() {                    // 0x47c140
    for (int i = 0; i < 7; i++) {
        delete idle[i];
        delete ready[i];
        idle[i] = ready[i] = nullptr;
    }
}

void FireflyCursor::load() {                         // 0x47c200
    hand = false;
    near = false;
    count = 0;
    t2 = frame_time();
    t = frame_time();
    for (int i = 0; i < 7; i++) {
        idle[i]->load();
        ready[i]->load();
        idle[i]->mode = 1;
        ready[i]->mode = 1;
    }
    hand_anim.load();
    hand_anim.mode = 1;                              // +0x1d4
}

void FireflyCursor::unload() {                       // 0x47c290
    Graphic::unload();
    for (int i = 0; i < 7; i++) {
        idle[i]->unload();
        ready[i]->unload();
    }
    hand_anim.unload();
}

// v14 0x47c2d0: the hand over the map icon; else the jar with its fireflies, glowing when it can catch or light
void FireflyCursor::draw() {
    Animation *a = hand ? &hand_anim : near ? ready[count] : idle[count];
    uint32_t now = frame_time();
    if (now - t > 50) {                              // 0x32
        t = now;
        a->step();
    }
    a->x = x;
    a->y = y;
    a->draw();
}

// ---------------------------------------------------------------- Turtle
Turtle::Turtle() : Graphic(GRAPHIC_ARGS), sound(nullptr) {   // 0x47e7a0
    vx = 0;
    off = new_anim("dcbl_ga_turtoff_");
    on = new_anim("dcbl_ga_turton_");
}

Turtle::~Turtle() {
    delete off;
    delete on;
}

void Turtle::load() {                                // 0x47e9c0
    off->load();
    off->mode = 1;
    on->load();
    on->mode = 1;
    t = frame_time();
    active = false;
    lit = false;
    sound.setName("dcbl_fx_turtle");                 // *0x53cc48
    sound.open();
}

void Turtle::unload() {                              // 0x47ea40
    Graphic::unload();
    sound.close();
    off->unload();
    on->unload();
    lit = false;
}

// v14 0x47ea80: walks right a step every 100 ms until its front reaches a target area; gone off the screen
void Turtle::draw() {
    if (!active) return;
    Animation *a = lit ? on : off;
    uint32_t now = frame_time();
    if (now - t > 100) {                             // 0x64
        x += vx;
        a->step();
        t = now;
    }
    a->x = x;
    a->y = y;
    a->draw();
    if (vx) {
        BackgroundScroller *sc = S().scroller;
        int px = off->w / 2 + x;                     // +0x64
        if (sc->target1.hit(px, y) || sc->target2.hit(px, y)) vx = 0;
    }
    if (y > 240 || x >= 320 || x <= -320) {          // 0xf0, 0x140
        active = false;
        lit = false;
        sound.stop();                                // 0x41fe70
    }
}

// ---------------------------------------------------------------- Swarm
Swarm::Swarm(int k) : Graphic(GRAPHIC_ARGS), n(k), sound(nullptr) {   // 0x47eb90
    if (k == 1) anim = new_anim("dcbl_ga_swarmsm_");
    else if (k == 2) anim = new_anim("dcbl_ga_swarmmd_");
    else if (k == 3) anim = new_anim("dcbl_ga_swarmlg_");
}

Swarm::~Swarm() { delete anim; }                     // 0x47ed00

void Swarm::load() {                                 // 0x47edb0
    anim->load();
    anim->mode = 1;
    active = false;
    t = frame_time();
    sound.setName("dcbl_fx_swarm");                  // *0x53cc4c
    sound.open();
}

void Swarm::unload() {                               // 0x47ee00
    Graphic::unload();
    anim->unload();
    active = false;
    sound.close();
}

bool Swarm::near(int px, int py) {                   // 0x47ee30
    int dx = x - px, dy = y - py;
    return dy * dy + dx * dx < 3600;                 // 0xe10
}

// v14 0x47ee60: drifts (the time test's start is never moved on: every draw after the first 50 ms) and leaves
void Swarm::draw() {
    if (frame_time() - t > 50) {
        x += vx;
        y += vy;
        anim->step();
    }
    anim->x = x;
    anim->y = y;
    anim->draw();
    if (vx > 0 && x > 350) active = false;           // 0x15e
    if (vx < 0 && x < -350) active = false;
    if (y > 300) active = false;                     // 0x12c
    if (!active) sound.stop();
}

// 0x47ef10: half the time in from the top, else from a side
void Swarm::start() {
    if (vision_random(1000) < 500) {
        x = vision_random(600) - 300;
        vx = vision_random(2);
        if (vision_random(1000) < 500) vx = -vx;
        y = -240 - vision_random(50);
        vy = vision_random(3);
    } else {
        vx = vision_random(3) + 1;
        if (vision_random(1000) < 500) {
            x = 350;
            vx = -vx;
        } else {
            x = -350;
        }
        y = -vision_random(240);
        vy = vision_random(6) - 3;
    }
    anim->x = x;
    anim->y = y;
    vx = ftol((double)vx * (double)g_speed);
    vy = ftol((double)vy * (double)g_speed);
}

// ---------------------------------------------------------------- TurtleGroup
TurtleGroup::~TurtleGroup() {                        // 0x47fdd0
    for (Turtle *t : turtles) {
        t->unload();
        delete t;
    }
}

// 0x47ff20: the nearest unlit turtle on its way (within 200, on the screen's middle)
bool TurtleGroup::nearest(int px, int py, int *ox, int *oy) {
    int best = 1000000000;
    bool found = false;
    for (Turtle *t : turtles) {
        if (!t->active || t->lit) continue;
        int dx = t->x - px, dy = t->y - py;
        int d2 = dy * dy + dx * dx;
        if (d2 >= best || d2 >= 40000) continue;     // 0x9c40
        best = d2;                                   // (before the place test, as the exe)
        if (t->x <= -200 || t->x >= 200 || t->y <= -200 || t->y >= 150) continue;
        *ox = t->x;
        *oy = t->y;
        found = true;
    }
    return found;
}

// 0x480100: the first idle turtle of those the skill allows starts above the screen, clear of the others, at the
// left or the middle, moved right past the target areas
void TurtleGroup::spawn() {
    for (int k = 0; k < g_turtles; k++) {
        Turtle *t = turtles.at(k);
        if (t->active) continue;
        bool clash;
        do {
            t->y = vision_random(480) - 720;         // state +8: 0x1e0, 0x2d0
            clash = false;
            for (Turtle *o : turtles) {
                if (!o->active) continue;
                int h = o->off->h;                   // +0x68
                if (o->y - h < t->y && o->y + h > t->y) {
                    clash = true;
                    break;
                }
            }
        } while (clash);
        t->x = vision_random(2) ? -320 : 0;          // 0xfffffec0
        BackgroundScroller *sc = S().scroller;
        while (sc->target1.hit(t->x, t->y) || sc->target2.hit(t->x, t->y)) {
            t->x++;
            if (t->x >= 640) return;                 // 0x280
        }
        t->x += t->off->w / 2;
        t->active = true;
        t->sound.play();
        t->vx = 1;
        return;
    }
}

// ---------------------------------------------------------------- BackgroundScroller
BackgroundScroller::BackgroundScroller(const char *a, const char *b)   // 0x47f040
    : ScrollGround(a, b),
      target1("DCBL_BG_target1_01", 0, 0, 0, 0, 0, 0, true),
      target2("DCBL_BG_target1_02", 0, 0, 0, 0, 0, 0, true) {
    static const int ns[5] = {1, 1, 2, 2, 3};
    for (int i = 0; i < 5; i++) swarms[i] = Gorilla::gnew<Swarm>(ns[i]);   // 0x4243c0(0x400)
}

BackgroundScroller::~BackgroundScroller() {
    for (int i = 0; i < 5; i++) delete swarms[i];
}

void BackgroundScroller::load() {                    // 0x47f300
    ScrollGround::load();
    target1.load();
    target1.visible = true;
    target1.setHit(1, true);
    target2.load();
    target2.visible = true;
    target2.setHit(1, true);
    char buf[64];
    for (int i = 0; i < 6; i++) {
        snprintf(buf, sizeof buf, "dcbl_ga_bkalmp_%2.2d", i + 1);
        lampA[i] = Gorilla::gnew<Image>(buf, 0, 0, 0, 0, 0, 0, true);
        lampA[i]->load();
        litA[i] = false;
        snprintf(buf, sizeof buf, "dcbl_ga_bkblmp_%2.2d", i + 1);
        lampB[i] = Gorilla::gnew<Image>(buf, 0, 0, 0, 0, 0, 0, true);
        lampB[i]->load();
        litB[i] = false;
    }
    for (int i = 0; i < 5; i++) swarms[i]->load();
    group = new TurtleGroup();                       // 0x4f34f3(0x10)
    for (int i = 0; i < 5; i++) {
        Turtle *t = Gorilla::gnew<Turtle>();         // 0x4243c0(0x3f8)
        group->turtles.push_back(t);
        t->load();
    }
}

void BackgroundScroller::unload() {                  // 0x47f5d0
    ScrollGround::unload();
    target1.unload();
    target2.unload();
    for (int i = 0; i < 6; i++) {
        if (lampA[i]) lampA[i]->unload();
        delete lampA[i];
        lampA[i] = nullptr;
        if (lampB[i]) lampB[i]->unload();
        delete lampB[i];
        lampB[i] = nullptr;
    }
    for (int i = 0; i < 5; i++) swarms[i]->unload();
    delete group;
    group = nullptr;
}

// v18 0x47f680: the ground's scroll; the target areas and lamps ride on its two images (a lamp scrolled far away
// goes out); the swarms and turtles move with it
void BackgroundScroller::update() {
    ScrollGround::update();
    target1.x = img1.x;
    target1.y = img1.y;
    target2.x = img2.x;
    target2.y = img2.y;
    for (int i = 0; i < 6; i++) {
        lampA[i]->x = lampA[i]->ox + img1.x;
        lampA[i]->y = lampA[i]->oy + img1.y;
        if (lampA[i]->y > 400 || lampA[i]->y < -400) litA[i] = false;
        lampB[i]->x = lampB[i]->ox + img2.x;
        lampB[i]->y = lampB[i]->oy + img2.y;
        if (lampB[i]->y > 400 || lampB[i]->y < -400) litB[i] = false;
    }
    for (int i = 0; i < 5; i++) {
        swarms[i]->x += dx;
        swarms[i]->y += dy;
    }
    for (Turtle *t : group->turtles) {
        t->x += dx;
        t->y += dy;
    }
}

void BackgroundScroller::draw() {                    // 0x47f7f0
    ScrollGround::draw();
    for (int i = 0; i < 6; i++) {
        if (litA[i]) lampA[i]->draw();
        if (litB[i]) lampB[i]->draw();
    }
    for (int i = 0; i < 5; i++)
        if (swarms[i]->active) swarms[i]->draw();
    for (Turtle *t : group->turtles) t->draw();
}

static bool lamp_near(Image *l, int px, int py) { return abs(px - l->x) < 25 && abs(py - l->y) < 50; }   // 0x19, 0x32

// 0x47f890: an unlit lamp or turtle's lantern there
bool BackgroundScroller::canLight(int px, int py) {
    for (int i = 0; i < 6; i++) {
        if (!litA[i] && lamp_near(lampA[i], px, py)) return true;
        if (!litB[i] && lamp_near(lampB[i], px, py)) return true;
    }
    for (Turtle *t : group->turtles) {
        if (t->lit || !t->active) continue;
        if ((t->lit ? t->on : t->off)->hit(px, py)) return true;
    }
    return false;
}

// 0x47f9b0: as canLight, and lights it
bool BackgroundScroller::light(int px, int py) {
    for (int i = 0; i < 6; i++) {
        if (!litA[i] && lamp_near(lampA[i], px, py)) {
            litA[i] = true;
            return true;
        }
        if (!litB[i] && lamp_near(lampB[i], px, py)) {
            litB[i] = true;
            return true;
        }
    }
    for (Turtle *t : group->turtles) {
        if (t->lit || !t->active) continue;
        if ((t->lit ? t->on : t->off)->hit(px, py)) {
            t->lit = true;
            return true;
        }
    }
    return false;
}

// 0x47fb20: the swarm there caught: its fireflies
int BackgroundScroller::catchAt(int px, int py) {
    for (int i = 0; i < 5; i++) {
        Swarm *s = swarms[i];
        if (!s->active) continue;
        int ddx = s->x - px, ddy = s->y - py;
        if (ddy * ddy + ddx * ddx >= 3600) continue;
        s->active = false;
        s->sound.stop();
        return s->n;
    }
    return 0;
}

// 0x47fbc0: the nearest swarm within 200 that is in the screen's middle
bool BackgroundScroller::nearestSwarm(int px, int py, int *ox, int *oy) {
    int best = 1000000000;
    bool found = false;
    for (int i = 0; i < 5; i++) {
        Swarm *s = swarms[i];
        if (!s->active) continue;
        int ddx = s->x - px, ddy = s->y - py;
        int d2 = ddy * ddy + ddx * ddx;
        if (d2 >= best || d2 >= 40000) continue;
        best = d2;                                   // (before the place test, as the exe)
        if (s->x <= -200 || s->x >= 200 || s->y <= -200 || s->y >= 150) continue;
        *ox = s->x;
        *oy = s->y;
        found = true;
    }
    return found;
}

// 0x47fc70: the nearest unlit lamp, then the turtles' answer. The exe returns only what the turtles' search says
// (a lamp found leaves its place in ox, oy but returns false unless a turtle is found too): kept.
bool BackgroundScroller::nearestLight(int px, int py, int *ox, int *oy) {
    int best = 1000000000;
    auto look = [&](Image *l) {
        int ddx = l->x - px, ddy = l->y - py;
        int d2 = ddy * ddy + ddx * ddx;
        if (d2 >= best || d2 >= 40000) return;
        best = d2;
        if (l->x <= -200 || l->x >= 200 || l->y <= -200 || l->y >= 150) return;
        *ox = l->x;
        *oy = l->y;
    };
    for (int i = 0; i < 6; i++)
        if (!litA[i]) look(lampA[i]);
    for (int i = 0; i < 6; i++)
        if (!litB[i]) look(lampB[i]);
    return group->nearest(px, py, ox, oy);
}

// ---------------------------------------------------------------- FFInterface
FFInterface::FFInterface()                           // 0x480430
    : Graphic(GRAPHIC_ARGS),
      bar("DCBL_IF_interface_01", 0, 0, 0, 0, 0, 0, true),
      dark("DCBL_IF_darklight_01", 0, 0, 0, 0, 0, 0, true),
      lit("DCBL_IF_litlight_01", 0, 0, 0, 0, 0, 0, true) {
    p = s = 0;
    bar.load();
    dark.load();
    lit.load();
    char buf[64];
    for (int i = 9; i < 17; i++) {
        snprintf(buf, sizeof buf, "DCBL_IF_target_%2.2d", i - 8);
        Image *a = Gorilla::gnew<Image>(buf, 0, 0, 0, 0, 0, 0, true);
        a->load();
        mine.push_back(a);
        snprintf(buf, sizeof buf, "DCBL_IF_target_%2.2d", i);
        Image *b = Gorilla::gnew<Image>(buf, 0, 0, 0, 0, 0, 0, true);
        b->load();
        his.push_back(b);
    }
}

FFInterface::~FFInterface() {                        // 0x480820
    bar.unload();
    dark.unload();
    lit.unload();
    for (Image *i : mine) { i->unload(); delete i; }
    for (Image *i : his) { i->unload(); delete i; }
}

// v14 0x480a50: the bar, then eight lights a side, lit for each point
void FFInterface::draw() {
    bar.draw();
    auto row = [&](std::vector<Image *> &v, int n) {
        for (int i = 0; i < n; i++) {
            lit.x = v.at(i)->ox;
            lit.y = v.at(i)->oy;
            lit.draw();
        }
        for (int i = n; i < 8; i++) {
            dark.x = v.at(i)->ox;
            dark.y = v.at(i)->oy;
            dark.draw();
        }
    };
    row(mine, p);
    row(his, s);
}

// ---------------------------------------------------------------- PeterControlScript
void PeterControlScript::setState(int st) {          // 0x47e000
    counter = 0;
    state = st;
    if (st == 0 || st == 1 || st == 3) dir = 666;
}

// v0 0x47e030: as Color the Clouds' Peter, without the throw
void PeterControlScript::animate() {
    Animation *str = nullptr, *fly = nullptr;
    switch (dir) {
    case 0: str = r->find("start_north"); fly = r->find("fly_north"); break;
    case 2: str = r->find("start_east"); fly = r->find("fly_east"); break;
    case 4: str = r->find("start_south"); fly = r->find("fly_south"); break;
    case 6: str = r->find("start_west"); fly = r->find("fly_west"); break;
    case 666: fly = str = r->find("fly_idle"); break;
    }
    if (cur == fly) {
        cur->step();
        lastdir = dir;
        return;
    }
    if (cur == str) {
        cur->step();
        if (cur->current() >= cur->count() - 1) {
            cur = fly;
            fly->setFrame(0);
            lastdir = dir;
        }
        r->select(cur, true);
        return;
    }
    if (cur == r->find("fly_idle")) {
        cur = str;
        str->setFrame(0);
        r->select(cur, true);
        return;
    }
    const char *back = nullptr;
    if (cur == r->find("fly_north")) back = "start_north";
    else if (cur == r->find("fly_south")) back = "start_south";
    else if (cur == r->find("fly_east")) back = "start_east";
    else if (cur == r->find("fly_west")) back = "start_west";
    if (back) {
        cur = r->find(back);
        cur->setFrame(cur->count() - 1);
    } else if (cur->current() != 0) {
        cur->setFrame(cur->current() - 1);
    } else if (dir == 666) {
        cur = r->find("fly_idle");
        cur->setFrame(0);
    } else {
        cur = str;
        str->setFrame(0);
    }
    r->select(cur, true);
}

static void head_for(PeterControlScript *a) {        // (inlined) the way to tx, ty and off there
    int dx = a->tx - a->r->x, dy = a->ty - a->r->y;
    if (abs(dx) > abs(dy)) a->dir = dx > 0 ? 2 : 6;
    else a->dir = dy > 0 ? 4 : 0;
    a->state = 2;
    a->counter = 0;
}

static void random_target(PeterControlScript *a) {   // (inlined) somewhere near him
    a->tx = vision_random(200) + a->r->x - 100;
    if (a->tx < -250) a->tx += 100;
    if (a->tx > 250) a->tx -= 100;
    a->ty = vision_random(200) + a->r->y - 100;
    if (a->ty < -100) a->ty += 100;
    if (a->ty > 200) a->ty -= 100;
}

// 0x47e230: catch a swarm he is on; full, off to a light; else now and then after a swarm, or anywhere
void PeterControlScript::decide() {
    if (stop) return;
    BackgroundScroller *sc = S().scroller;
    int n = sc->catchAt(r->x, r->y);
    if (n) {
        count += n;
        if (count > 6) count = 6;
    }
    if (count >= 6) {
        state = 3;
        counter = 0;
        dir = 666;
        return;
    }
    if (vision_random(1000) < 500) {                 // 0x1f4
        int x, y;
        if (sc->nearestSwarm(r->x, r->y, &x, &y)) {
            tx = x;
            ty = y;
            head_for(this);
        }
        return;
    }
    if (vision_random(1000) >= 100) return;
    random_target(this);
    head_for(this);
}

// 0x47e460: full, lighting what he reaches; else catching swarms on the way; a tenth of the way (four more) a step
void PeterControlScript::move() {
    BackgroundScroller *sc = S().scroller;
    if (count >= 6) {
        if (sc->light(r->x, r->y)) {
            count = 0;
            S().iface->s++;
            state = 1;
            counter = 0;
            dir = 666;
            return;
        }
    } else {
        for (int i = 0; i < 5; i++) {
            Swarm *s = sc->swarms[i];
            if (!s->active || !s->near(r->x, r->y)) continue;
            s->active = false;
            s->sound.stop();
            if (s->n) {
                count += s->n;
                if (count > 6) count = 6;
            }
            break;
        }
        if (count >= 6) {
            state = 3;
            counter = 0;
            dir = 666;
            return;
        }
    }
    int dx = tx - r->x, dy = ty - r->y;
    auto stepof = [](int d) {
        int st = d / 10;
        if (d > st) st += 4;
        else if (d < st) {
            st -= 4;
            if (d > st) st += 4;
        }
        return st;
    };
    r->x += stepof(dx);
    r->y += stepof(dy);
    if (abs(dx) < 10 && abs(dy) < 10) {
        state = count >= 6 ? 3 : 1;
        counter = 0;
        dir = 666;
    }
}

// 0x47e610: full: a light where he is, or (mostly) off to the nearest one, or anywhere
void PeterControlScript::home() {
    BackgroundScroller *sc = S().scroller;
    if (sc->light(r->x, r->y)) {
        count = 0;
        S().iface->s++;
        state = 1;
        counter = 0;
        dir = 666;
        return;
    }
    if (vision_random(10000) >= 7000) return;        // 0x2710, 0x1b58
    int x, y;
    if (sc->nearestLight(r->x, r->y, &x, &y)) {
        tx = x;
        ty = y + 30;                                 // 0x1e
    } else {
        random_target(this);
    }
    head_for(this);
}

// ---------------------------------------------------------------- Firefly_Scene
Firefly_Scene::Firefly_Scene()                       // 0x47db70
    : Game2D_Scene("firefly"),
      guide("DCBL_VO_PETgreet1", "DCBL_VO_PETgreet2"),
      music(nullptr) {
    keyboard.setClient(this);
    mouse.setClient(this);
    nice.push_back("DCBL_VO_PETnice1");
    nice.push_back("DCBL_VO_PETnice2");
    nice.push_back("DCBL_VO_PETnice3");
    nice.push_back("DCBL_VO_PETnice4");
}

Firefly_Scene::~Firefly_Scene() {}                   // 0x47dda0

void Firefly_Scene::load2() {                        // v18 0x47d430
    music.setName("DCBL_MU_music");
    music.mode = 2;                                  // +0x193c
    music.type = 6;                                  // +0x1944
    static const char *const names[3] = {"dcbl_fx_lantlit", "dcbl_fx_gdrop", "dcbl_fx_cuclicky"};   // 0x53cc50
    for (int i = 0; i < 3; i++) sounds[i].setName(names[i]);
    inactivity.add("DCBL_VO_PETprompt", false);
    endvo.setWins("DCBL_VO_PETtbwins_01", "DCBL_VO_PETtbwins_02", "DCBL_VO_PETtbwins_03");
    endvo.setLoses("DCBL_VO_PETtblose_01", "DCBL_VO_PETtblose_02", "DCBL_VO_PETtblose_03");
    endvo.setTie("DCBL_VO_PETtie_01");
    endvo.setWinChallenge("DCBL_VO_PETwinsch_01", "DCBL_VO_PETwinsch_02", "DCBL_VO_PETwinsch_03");
    endvo.setAward("DCBL_VO_PETaward");
    endvo.setLoseChallenge("DCBL_VO_PETlosech_01", "DCBL_VO_PETlosech_02", "DCBL_VO_PETlosech_03");
    endvo.setTie2("DCBL_VO_PETtie_02");
}

// v19 0x47d6b0
void Firefly_Scene::enter() {
    Game2D_Scene::enter();
    over = false;
    mouse.activate();
    keyboard.activate();
    pack = ras_open(name(), nullptr, &data_device());   // "firefly"
    pack->load();
    ras_set_current(pack);
    if (previous != this) {                          // +0xc4
        guide.load(false);
        guide.setZ(-950);                            // 0xfffffc4a
        screen.layer(0)->add(&guide);
        guide.Task::activate();                      // +0xc40 v6
    }
    cursor = Gorilla::gnew<FireflyCursor>();         // 0x4243c0(0x1f8)
    cursor->load();
    cursor->visible = true;
    set_cursor(cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    hide_cursor();
    screen.activate();
    screen.loadAll();
    board = Gorilla::gnew<Scoreboard>("DCBL_IF_scorescr_01");
    board->setZ(-1000);
    board->visible = false;
    screen.layer(0)->add(board);
    reset();                                         // 0x421110
    activate();
    playing = false;
    iface = Gorilla::gnew<FFInterface>();            // 0x4243c0(0x2b8)
    iface->setZ(-1000);
    screen.layer(0)->add(iface);
    music.open();
}

// 0x47d520: Peter's flight and take-offs; his thinking
void Firefly_Scene::setupPeter() {
    peter_r = Gorilla::gnew<TopDownCharacterRenderer>();
    peter_r->add("fly_idle", "dcbl_ca_petflyidle_");
    peter_r->add("fly_north", "dcbl_ca_petflyno_");
    peter_r->add("fly_south", "dcbl_ca_petflyso_");
    peter_r->add("fly_east", "dcbl_ca_petflyea_");
    peter_r->add("fly_west", "dcbl_ca_petflywe_");
    peter_r->add("start_north", "dcbl_ca_petstrno_");
    peter_r->add("start_south", "dcbl_ca_petstrso_");
    peter_r->add("start_east", "dcbl_ca_petstrea_");
    peter_r->add("start_west", "dcbl_ca_petstrwe_");
    peter_r->load();
    screen.layer(0)->add(peter_r);
    ai = new PeterControlScript();                   // 0x4f34f3(0x30)
    ai->count = 0;
    ai->r = peter_r;
    ai->state = 0;
    ai->stop = false;
}

// 0x47d8f0: the ground with its lamps, swarms and turtles; Peter; the cannon; no points; the sounds
void Firefly_Scene::setup() {
    scroller = Gorilla::gnew<BackgroundScroller>("DCBL_BG_ground1_01", "DCBL_BG_ground1_02");   // (0x418)
    scroller->load();
    screen.layer(0)->add(scroller);
    setupPeter();                                    // 0x47d520
    Cannon &cn = Game::cannon();
    screen.layer(0)->add(&cn);
    cn.visible = true;                               // state +0x83c
    iface->p = 0;
    iface->s = 0;
    scroller->visible = true;
    for (int i = 0; i < 3; i++) sounds[i].open();
}

// 0x47c5a0: the hand over the map icon; else the jar glows over a light it can fill, or a swarm it can catch
void Firefly_Scene::cursorUpdate() {
    if (map->hit(g_mouse.x, g_mouse.y)) {
        cursor->hand = true;
        cursor->near = false;
        return;
    }
    cursor->hand = false;
    if (cursor->count >= 6) {
        cursor->near = scroller->canLight(g_mouse.x, g_mouse.y);
        return;
    }
    for (int i = 0; i < 5; i++) {
        Swarm *s = scroller->swarms[i];
        if (s->active && s->near(g_mouse.x, g_mouse.y)) {
            cursor->near = true;
            return;
        }
    }
    cursor->near = false;
}

// v10 0x47c930
void Firefly_Scene::run() {
    State &st = Game::state();
    wait(1);
    hide_cursor();
    escaped = false;
    clouds->reset();
    while (guide.Task::active()) wait(0);            // +0xc40 v5
    int skill;
    if (!st.saves.getSkill(&skill)) skill = 1;
    switch (skill) {
    case 1: secs = 90; g_speed = 1.0f; g_turtles = 4; break;
    case 2: secs = 90; g_speed = 1.5f; g_turtles = 3; break;
    case 3: secs = 60; g_speed = 2.0f; g_turtles = 2; break;
    }
    Game::cannon().start(secs);                      // 0x4e3a50
    setup();                                         // 0x47d8f0
    music.play();
    show_cursor();
    while (clouds->fall()) wait(30);
    inactivity.start();                              // v12
    playing = true;
    if (getenv("TB_DEBUG_FIREFLY")) tb_log("firefly: play (t=%u)", frame_time());
    while (!escaped) {
        wait(30);
        cursorUpdate();                              // 0x47c5a0
        if (vision_random(1000) < 100) {             // the scene's random +0x1c4
            for (int i = 0; i < 5; i++) {
                Swarm *s = scroller->swarms[i];
                if (s->active) continue;
                s->start();
                s->active = true;
                s->sound.play();
                break;
            }
        }
        scroller->group->spawn();                    // 0x480100
        if (!Game::cannon().running()) break;
        if (iface->p >= 8 || iface->s >= 8) break;
        PeterControlScript *a = ai;
        if (a->state) a->animate();
        switch (a->state) {
        case 0:
            a->count = 0;
            a->dir = 666;
            a->lastdir = 666;
            a->cur = peter_r->find("fly_idle");
            a->setState(1);
            break;
        case 1: a->decide(); break;
        case 2: a->move(); break;
        case 3: a->home(); break;
        }
    }
    if (getenv("TB_DEBUG_FIREFLY"))
        tb_log("firefly: over: escaped %d p %d s %d", escaped, iface->p, iface->s);
    hide_cursor();
    inactivity.stop();                               // v13
    ai->stop = true;
    ai->setState(1);
    if (escaped) {
        fader->visible = true;
        fader->start(false, 2000);
        while (fader->busy()) wait(0);
    } else {
        clouds->cover();
        while (clouds->rise()) wait(30);
    }
    for (int i = 0; i < 5; i++) scroller->swarms[i]->sound.stop();
    Game::cannon().detach();
    set_cursor(st.cursor, false);
    over = true;
    if (escaped) {
        change_scene("Skull");
        finish();
        return;
    }
    board->s2 = iface->s;
    board->s1 = iface->p;
    board->visible = true;
    wait(3000);
    if (iface->p > iface->s) {
        int info = st.saves.gameInfo("Firefly Game");   // *0x54351c
        if (info == 0) st.last_result = 2;
        else if (info == 1) st.last_result = 3;
        else st.last_result = 4;
    } else if (iface->p == iface->s) {
        st.last_result = 5;
    } else if (iface->p < iface->s) {
        st.last_result = 1;
    }
    wait(3000);
    endvo.playEnd();
    while (endvo.playing()) wait(0);
    if (escaped || iface->p > iface->s) {
        change_scene("Skull");
    } else {
        dialog = Gorilla::gnew<ContinueDialog>("glob_if_contgame_01", "glob_if_signglow_01", "glob_if_xglow_01");
        dialog->setAsk("dcbl_vo_petbegcon_01");
        dialog->setYes("dcbl_vo_petbegyes_01");
        dialog->setNo("dcbl_vo_petbegno_01");
        dialog->setIdle("dcbl_vo_petbegidl_01");
        keyboard.deactivate();                       // v7
        show_cursor();
        call(dialog);
        wait(1);
    }
    finish();
}

void Firefly_Scene::notify2(Service *s) {            // v9 0x47c880
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
            board->visible = false;
            again();                                 // 0x41dde0
        } else if (dialog->no) {                     // +0x6f0
            change_scene("Skull");
        }
        dialog = nullptr;
        mouse.activate();
        keyboard.activate();
    }
}

// 0x47c350: a full jar lights what is clicked (a point; every other one, a word from Peter); else a click catches
// the swarm there. Moving over a light with a full jar, the drop sound.
void Firefly_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (!playing) return;
    int mx = g_mouse.x, my = g_mouse.y;
    if (!(g_mouse.ldown || g_mouse.rdown)) {         // 0x5a6e30
        if (cursor->count >= 6 && scroller->canLight(mx, my)) sounds[1].play();   // +0x514 gdrop
        return;
    }
    int r = map->click(mx, my, true);
    if (r) {
        if (r == 2) change_scene("map");
        return;
    }
    if (cursor->count >= 6) {
        if (!scroller->light(mx, my)) return;
        iface->p++;
        cursor->count = 0;
        sounds[0].play();                            // lantlit
        if (iface->p % 2 == 1) {
            int k = vision_random((int)nice.size());   // state +8
            VoiceOver::get().play(nice.at(k));
        }
        return;
    }
    int n = 0;
    for (int i = 0; i < 5; i++) {
        Swarm *s = scroller->swarms[i];
        if (!s->active) continue;
        int dx = s->x - mx, dy = s->y - my;
        if (dy * dy + dx * dx >= 3600) continue;
        s->active = false;
        s->sound.stop();
        n = s->n;
        break;
    }
    if (cursor->count < 6) {
        cursor->count += n;
        sounds[2].play();                            // +0x85c cuclicky
    }
    if (cursor->count > 6) cursor->count = 6;
}

// 0x47c6c0: Escape skips Peter's welcome, or the game; the arrows scroll and slide the ground
void Firefly_Scene::onKey() {
    State &st = Game::state();
    if (!playing) {
        if ((int)g_key.vk == st.skip_key) guide.stop();
        return;
    }
    scroller->setMult(2);
    up = down = left = right = false;
    if (g_key.down) {                                // 0x5a6dd8
        if ((int)g_key.vk == st.skip_key) {
            escaped = true;
            st.last_game = 5;
        }
        if (st.key((int)g_key.vk)) return;
    }
    if (over) return;
    if (key_down(0x26) && !over) {
        scroller->setMult(4);
        up = true;
    }
    if (key_down(0x28) && !over) {
        scroller->setMult(1);
        down = true;
    }
    if (key_down(0x27) && !over) {
        scroller->setDir(2);
        right = true;
    }
    if (key_down(0x25) && !over) {
        scroller->setDir(1);
        left = true;
    }
    if (left && right) left = right = false;
    if (up && down) {
        up = down = false;
        scroller->setMult(2);
    }
    if (!left && !right) scroller->setDir(0);
}

// v20 0x47d9f0
void Firefly_Scene::leave() {
    VoiceOver::get().stop();
    music.stop();
    music.close();
    delete board;
    board = nullptr;
    Game2D_Scene::leave();
    guide.unload();
    for (int i = 0; i < 3; i++) sounds[i].close();
    Game::cannon().detach();
    deactivate();
    delete iface;
    iface = nullptr;
    if (scroller) scroller->unload();
    delete scroller;
    scroller = nullptr;
    delete ai;
    ai = nullptr;
    delete peter_r;
    peter_r = nullptr;
    // port: the cursor goes back to the game's before it is unloaded (and is deleted, the exe keeps it)
    set_cursor(Game::state().cursor, true);
    if (cursor) cursor->unload();
    delete cursor;
    cursor = nullptr;
    mouse.deactivate();
    keyboard.deactivate();
    screen.deactivate();
    screen.unloadAll();
    ras_release(pack);
    pack = nullptr;
    tasks_finish_all();                              // 0x4210d0
    set_cursor(Game::state().cursor, true);
    Game::state().guide_loaded = false;
}

}  // namespace Game
