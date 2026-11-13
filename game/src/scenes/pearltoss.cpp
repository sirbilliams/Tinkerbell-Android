// Pearl Toss, reconstructed from "Tinker Bell.exe" (see pearltoss.h).
#include "pearltoss.h"
#include "mermaid.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "voiceover.h"
#include "cannon.h"
#include "shuffle.h"
#include <stdio.h>
#include <stdlib.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static Animation *anim_of(Attachment *a) { return static_cast<Animation *>(a->g); }
static bool cur_vis() { return g_cursor_graphic ? g_cursor_graphic->visible : false; }   // (each set_cursor's)

static const char *const kGame = "Pearl Toss Game";  // *0x54350c

// The statics around the game (0x44d360..0x44d4ba, 0x4544e0..0x4546ae).
static Animation &basket() { static Animation a("PERL_CU_basket_"); return a; }   // 0x5c3048: the cursor over a coconut
static Image &inbasket() {                           // 0x5b9550: the game's cursor
    static Image i("PERL_CU_inbasket_01", 0, 0, 0, 0, 0, 0, true);
    return i;
}
// 0x5b9600: perl_fx_cocofall, perl_fx_cocobreak, perl_fx_getcoco, perl_fx_pearlhit_01 (named by the first toss)
static Sound &coco_fx(int i) {
    static Sound *s[4] = {new Sound(""), new Sound(""), new Sound(""), new Sound("")};   // port: never freed
    return *s[i];
}
static Sound &toss_snd() { static Sound *s = new Sound("PERL_FX_MERtoss"); return *s; }   // 0x5c3180
static Sound &clap_snd(int i) {                      // 0x5c34c8 / 0x5c3818
    static Sound *s[2] = {new Sound("PERL_FX_MERclap"), new Sound("PERL_FX_MERclap")};
    return *s[i];
}
static int help_count = 0;                           // 0x5c3d00: clicks beside the coconuts

MermaidTask &mermaid_a() { static MermaidTask *m = new MermaidTask(); return *m; }   // 0x5c3b60
MermaidTask &mermaid_b() { static MermaidTask *m = new MermaidTask(); return *m; }   // 0x5c3c30

std::vector<PearlToss *> &PearlToss::all() {         // 0x5c3038
    static std::vector<PearlToss *> *v = new std::vector<PearlToss *>();
    return *v;
}

// ---------------------------------------------------------------- the pearl's way
// The heights: up to the throw's peak and down again (doubles of float constants, the exe's table on the stack).
static const double kArc[19] = {0.0, (double)0.05f, (double)0.1f, (double)0.15f, (double)0.2f, (double)0.25f,
                                (double)0.3f, (double)0.35f, (double)0.4f, (double)0.36f, (double)0.32f,
                                (double)0.28f, (double)0.24f, (double)0.2f, (double)0.16f, (double)0.12f,
                                (double)0.08f, (double)0.04f, 0.0};

void PearlPath::sprite() {
    pearl = rt->findSprite("PERL_GA_pearl_");        // 0x42dc70
    if (pearl) return;
    pearl = rt->addSprite("PERL_GA_pearl_", "PERL_GA_pearl_", 1);   // 0x42d9d0
    pearl->load();                                   // 0x42ef20
    pearl->bheight = 20.0f;                          // +0xa8
    pearl->radius = 20.0f;                           // +0xa4
    pearl->mode = 0;                                 // +0x88
    rt->makeBox(pearl, true);                        // 0x429580
    if (pearl->anim) pearl->anim->visible = false;
}

// 0x452db0: from the mermaid to the tree in 20 steps (19 points): straight in x, in the height by the sizes
// (the target's scale and height against the thrower's), with the arc added to z (the exe's).
void PearlPath::toTree(Sprite3D *from, Sprite3D *to) {
    sprite();
    double x0 = from->x, z0 = from->z;
    double h = (double)(from->anim->oh / 2);         // +0x70 +0x40
    double dx = ((double)to->x - x0) * 0.05;         // 0x5148e8
    double dz = ((double)to->z - z0) * 0.05;
    double D = (double)to->scale * to->y * 1.5 * 0.01 - (double)from->scale * h * 0.015;   // 0x513b68, 0x513560, 0x5148e0
    double dy = D * 0.05;
    xs.clear();
    zs.clear();
    ys.clear();
    for (int i = 1; i - 1 < 19; i++) {
        double di = i;
        xs.push_back((int)(di * dx + x0));          // 0x4f3948 (ftol)
        zs.push_back((int)(di * dz + D * kArc[i - 1] * 0.2 + z0));   // 0x5148d8
        ys.push_back((int)(di * dy + h));
    }
    index = 0;
}

// 0x453190: the same line on to twice the way (39 points), straight in z, the arc in the height: up for 20 points
// (the 20th reads past the 19 heights: the exe's next stack slot, dx), then the first 19 heights backwards.
void PearlPath::pastTree(Sprite3D *from, Sprite3D *to) {
    sprite();
    double x0 = from->x, z0 = from->z;
    double h = (double)(from->anim->oh / 2);
    double dx = ((double)to->x - x0) * 0.05;
    double dz = ((double)to->z - z0) * 0.05;
    double D = (double)to->scale * to->y * 2.5 * 0.01 - (double)from->scale * h * 0.015;   // 0x5148f0
    double dy = D * 0.05;
    xs.clear();
    ys.clear();
    zs.clear();
    for (int i = 1; i - 1 < 39; i++) {
        double di = i;
        xs.push_back((int)(di * dx + x0));
        if (i - 1 < 20) {
            double arc = i - 1 < 19 ? kArc[i - 1] : dx;   // port: the exe's read past the table
            ys.push_back((int)(di * dy + D * arc * 0.2 + h));
        }
        zs.push_back((int)(di * dz + z0));
    }
    for (int k = 18; k >= 0; k--) ys.push_back(ys.at(k));   // 0x455f10 (the check), 0x430dd0
    index = 0;
}

// ---------------------------------------------------------------- the mermaids
void ClappingMermaid::stop() {                       // 0x455be0
    if (!active()) return;
    deactivate();
    if (!idle) return;
    if (toss->anim) toss->anim->visible = false;
    toss->anim->setFrame(0);
    idle->anim->setFrame(0);
    idle->anim->visible = true;
}

// v10 0x455c40: the throw to its end, then the idle again.
void ClappingMermaid::run() {
    wait(1);
    while (toss->anim->step()) wait(0);              // 0x403d60
    if (active()) {
        deactivate();
        if (idle) {
            if (toss->anim) toss->anim->visible = false;
            toss->anim->setFrame(0);
            idle->anim->setFrame(0);
            idle->anim->visible = true;
        }
    }
    finish();
}

// v10 0x455d40: once her idle shows, a clap with its sound, then the idle again.
void MermaidTask::run() {
    wait(1);
    while (!(idle->anim && idle->anim->visible)) wait(0);
    if (idle->anim) idle->anim->visible = false;
    clap->anim->visible = true;
    clap->anim->setFrame(0);
    snd->play();                                     // 0x41fd50
    while (clap->anim->step()) wait(0);
    if (active()) {
        deactivate();
        idle->anim->visible = true;
        if (clap->anim) clap->anim->visible = false;
        snd->stop();                                 // 0x41fe70
    }
    finish();
}

// ---------------------------------------------------------------- a toss
// port: the toss's RayTrace is the scene's (the exe writes the static scene's address); set by the monitor
PearlToss::PearlToss() : Task(nullptr), rt(nullptr), path(nullptr) {   // 0x453760
    mouse.setClient(this);                           // 0x41f290
    if (all().empty()) {
        coco_fx(0).setName("perl_fx_cocofall");      // 0x424400
        coco_fx(1).setName("perl_fx_cocobreak");
        coco_fx(2).setName("perl_fx_getcoco");
        coco_fx(3).setName("perl_fx_pearlhit_01");
    }
    all().push_back(this);                           // 0x5c3038
}

// 0x453a70
void PearlToss::start(Sprite3D *t, Sprite3D *ts, Sprite3D *id, Attachment *c, Attachment *b) {
    tree = t;
    toss = ts;
    idle_spr = id;
    coco = c;
    broken = b;
    reset();                                         // 0x421110
    activate();                                      // v6
    path.toTree(ts, t);                              // 0x452db0
    switch (Game::state().saves.gameInfo(kGame)) {   // 0x4e9d70
    case 0: info = 0; break;
    case 1: case 2: info = 1; break;
    default: break;
    }
    switch (Game::state().skill) {                   // +0x68
    case 1: hold = info == 0 ? 9000 : 7000; break;
    case 2: hold = info == 0 ? 5000 : 3000; break;
    case 3: hold = info == 0 ? 4000 : 2000; break;
    default: break;
    }
    miss = false;
    Game::state().last_game = 3;                     // +0x79c
}

// v10 0x453b90: the mermaid free (her throw at its start, no clap on), she throws (the sound at the 14th frame),
// the pearl flies; a pearl knocks the coconut down: it lies a while (the mouse watched), then falls and breaks.
// A miss: the throw played out, then the end. A catch (notify2) leaves the task off where it waits.
void PearlToss::run() {
    wait(1);
    for (;;) {
        if (toss->anim->current() == 0) {            // 0x403f00
            MermaidTask &m = idle_spr == mermaid_a().idle ? mermaid_a() : mermaid_b();   // 0x5c3c20
            if (!m.active()) break;
        }
        wait(0);
    }
    if (idle_spr->anim) idle_spr->anim->visible = false;
    toss->anim->setFrame(0);                         // 0x403f20
    toss->anim->visible = true;
    for (;;) {
        wait(0);
        while (toss->anim->step() && toss->anim->current() < 14) wait(0);
        toss_snd().play();
        clap.toss = toss;                            // +0x218
        clap.idle = idle_spr;                        // +0x21c
        clap.activate();                             // v6
        wait(0);
        for (;;) {
            bool on;
            if (path.index < path.xs.size()) {
                unsigned i = path.index;
                path.pearl->x = (float)path.xs.at(i);
                path.pearl->z = (float)path.zs.at(i);
                path.pearl->y = (float)path.ys.at(i);
                path.index++;
                path.pearl->anim->visible = true;
                on = true;
            } else {
                if (path.pearl->anim) path.pearl->anim->visible = false;
                on = false;
            }
            if (!on) break;
            wait(0);
        }
        if (miss) {
            while (clap.active()) wait(0);
            finish();                                // 0x420fe0
            return;
        }
        coco_fx(3).play();                           // perl_fx_pearlhit_01
        anim_of(coco)->setFrame(0);
        coco->on = true;                             // +0x17c
        mouse.activate();                            // v6
        t0 = frame_time();                           // +0xd8
        while (anim_of(coco)->step() && anim_of(coco)->current() < 3) wait(0x43);
        while (frame_time() - t0 < (uint32_t)hold) wait(0x43);
        coco_fx(0).play();                           // perl_fx_cocofall
        coco->on = false;
        anim_of(broken)->setFrame(0);
        broken->on = true;
        while (anim_of(broken)->step()) {
            wait(0x43);
            if (anim_of(broken)->current() == 12) {
                coco_fx(1).play();                   // perl_fx_cocobreak
                mouse.deactivate();                  // v7
            }
        }
        broken->on = false;
        if (!active()) continue;
        if (!miss) {
            coco->on = false;
            broken->on = false;
        }
        mouse.deactivate();
        deactivate();                                // v7: the task waits at the loop's top, off
        if (path.pearl && path.pearl->anim) path.pearl->anim->visible = false;
        if (clap.active()) {
            clap.deactivate();
            if (clap.idle) {
                if (clap.toss->anim) clap.toss->anim->visible = false;
                clap.toss->anim->setFrame(0);
                clap.idle->anim->setFrame(0);
                clap.idle->anim->visible = true;
            }
        }
    }
}

// (inlined in notify2 and idle) the coconut (or its breaking, before the 12th frame) under the mouse, tried at the
// mouse and up to 80 pixels below it.
bool PearlToss::under() {
    for (int y = g_mouse.y; y > g_mouse.y - 100; y -= 20) {
        Attachment *a = rt->pickAttached(g_mouse.x, y, false);   // 0x42c950
        if (a == coco) return true;
        if (a == broken && anim_of(broken)->current() < 12) return true;
    }
    return false;
}

bool PearlToss::idle() {                             // 0x454430
    if (!mouse.active()) return true;
    rt->pick_max = Game::state().skill == 1 ? 1500.0f : 1000.0f;   // +0x1170
    return !under();
}

// v9 0x454180: the basket over this toss's coconut (the game's cursor when no other toss has one under the
// mouse); clicked, it is caught: the monitor told, the toss off, the mermaids clap. Clicks beside: every third,
// Cubby's help line.
void PearlToss::notify2(Service *s) {
    if (s != &mouse) return;
    State &st = Game::state();
    rt->pick_max = st.skill == 1 ? 1500.0f : 1000.0f;   // +0x1170
    bool others = true;
    std::vector<PearlToss *> &v = all();
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i] == this) continue;
        if (!v.at(i)->idle()) { others = false; break; }   // 0x455ea0
    }
    if (others) set_cursor(&inbasket(), cur_vis());  // 0x4119b0
    if (!under()) {
        if (mouse_down() && ++help_count == 3) {
            help_count = 0;
            vo_play("PERL_VO_CUBhelp1");             // 0x4c9400
        }
        return;
    }
    set_cursor(&basket(), cur_vis());
    if (!mouse_down()) return;
    coco_fx(2).play();                               // perl_fx_getcoco
    notifyClient();                                  // 0x41f1f0: the monitor counts it
    if (active()) {
        if (!miss) {
            coco->on = false;
            broken->on = false;
        }
        mouse.deactivate();
        deactivate();
        if (path.pearl && path.pearl->anim) path.pearl->anim->visible = false;
        if (clap.active()) {
            clap.deactivate();
            if (clap.idle) {
                if (clap.toss->anim) clap.toss->anim->visible = false;
                clap.toss->anim->setFrame(0);
                clap.idle->anim->setFrame(0);
                clap.idle->anim->visible = true;
            }
        }
    }
    set_cursor(&inbasket(), cur_vis());
    if (!mermaid_a().active()) {
        mermaid_a().reset();
        mermaid_a().activate();
    }
    if (!mermaid_b().active()) {
        mermaid_b().reset();
        mermaid_b().activate();
    }
}

// ---------------------------------------------------------------- the interface
// 0x4535c0. port: the images are made with their names (the exe names its embedded ones after making them).
PearlInterface::PearlInterface()
    : Screen(0, 0, 0, 0, 0, true, true), bar("PERL_IF_interfacebar_01", 0, 0, 0, 0, 0, 0, true) {   // 0x41eea0, 0x4179e0
    layer(0)->add(&bar);                             // 0x41ece0, 0x41e930
    static const char *const tn[8] = {"PERL_IF_cocotargeta_01", "PERL_IF_cocotargetb_01", "PERL_IF_cocotargetc_01",
                                      "PERL_IF_cocotargetd_01", "PERL_IF_cocotargete_01", "PERL_IF_cocotargetf_01",
                                      "PERL_IF_cocotargetg_01", "PERL_IF_cocotargeth_01"};
    for (int i = 0; i < 8; i++) targets[i] = new Image(tn[i], 0, 0, 0, 0, 0, 0, true);   // 0x453740 + 0x424400
    for (int i = 0; i < 8; i++) {
        catches[i] = new Image("PERL_IF_coconut_01", 0, 0, 0, 0, 0, 0, true);
        layer(0)->add(catches[i]);
    }
}

// ---------------------------------------------------------------- the monitor
// 0x4546b0. port: the RayTrace is given (the exe writes the static scene's address).
PearlTossMonitor::PearlTossMonitor(Raytrace::RayTrace *r) : Task(nullptr), rt(r) {
    for (PearlToss &t : tosses) {
        t.rt = r;
        t.path.rt = r;
        t.setClient(this);                           // 0x41f290
    }
    mouse.setClient(this);
    keys.setClient(this);
    inactivity.add("PERL_VO_CUBprompt", false);      // 0x4c46e0
}

// 0x454ba0: once a visit (the toss sprites not made yet): the mermaids' toss and clap sprites at their rocks (x,
// then the height, then z: the exe's), the mermaid pair's sprites, the trees and their coconuts, the cursors. The
// fuse shown.
void PearlTossMonitor::setup() {
    toss_a = rt->findSprite("PERL_GA_MERtossa_");    // 0x42dc70
    if (toss_a) {
        Game::cannon().visible = true;               // state +0x83c
        return;
    }
    Mermaid_Scene &sc = mermaid_scene();
    auto make = [this](const char *n, const char *p, float x, float z) {
        Sprite3D *s = rt->addSprite(n, p, 1);        // 0x42d9d0
        s->load();                                   // 0x42ef20
        s->x = x;
        s->place();                                  // 0x42f7f0
        s->z = z;
        s->anim->mode = 0;                           // +0x104
        s->mode = 3;                                 // +0x88
        if (s->anim) s->anim->visible = false;
        return s;
    };
    toss_a = make("PERL_GA_MERtossa_", "PERL_GA_MERtossa_", 2989.0f, 5250.0f);   // 0x453ad000, 0x45a41000
    toss_b = make("PERL_GA_MERtossb_", "PERL_GA_MERtossa_", 2689.0f, 5350.0f);   // 0x45281000, 0x45a73000
    clap_a = make("PERL_GA_MERclapa_", "PERL_GA_MERclapa_", 2989.0f, 5250.0f);
    clap_b = make("PERL_GA_MERclapb_", "PERL_GA_MERclapa_", 2689.0f, 5350.0f);
    mermaid_a().toss = toss_a;                       // 0x5c3c24
    mermaid_a().idle = sc.idle_a;                    // 0x5c3c20 (the scene's +0x28e8)
    mermaid_a().clap = clap_a;
    mermaid_a().snd = &clap_snd(0);                  // 0x5c34c8
    mermaid_b().idle = sc.idle_b;                    // +0x28ec
    mermaid_b().toss = toss_b;
    mermaid_b().clap = clap_b;
    mermaid_b().snd = &clap_snd(1);                  // 0x5c3818
    static const char *const tn[5] = {"AC_perl_ga_treeb_01", "AB_perl_ga_treeb_01", "AE_perl_ga_treeb_01",
                                      "AD_perl_ga_treeb_01", "AH_perl_ga_treeb_01"};   // 0x539160
    static const char *const sc_[5] = {"SC0", "SC1", "SC2", "SC3", "SC4"};   // 0x5391b0
    static const char *const fc[5] = {"FC0", "FC1", "FC2", "FC3", "FC4"};    // 0x539200
    for (int i = 0; i < 5; i++) {
        trees[i] = rt->findSprite(tn[i]);
        trees[i]->load();
        cocos[i] = trees[i]->attach(sc_[i], "PERL_GA_coconutb_", -1);   // 0x42f390, 0x42e4a0
        anim_of(cocos[i])->mode = 0;                 // +0x104
        cocos[i]->on = false;
        brokens[i] = trees[i]->attach(fc[i], "PERL_GA_cocobreakb_", -1);
        anim_of(brokens[i])->mode = 0;
        brokens[i]->on = false;
    }
    basket().load();                                 // v15
    basket().play(0, 1);                             // 0x403be0
    inbasket().load();                               // 0x416f80
    help_count = 0;
    Game::cannon().visible = true;
}

// 0x454ef0: `pearls` pearls and `misses` misses, shuffled 50 to 69 times.
void PearlTossMonitor::kinds(int pearls, int misses) {
    kind.clear();
    for (int i = pearls; i > 0; i--) kind.push_back(0);   // 0x430dd0
    for (int i = misses; i > 0; i--) kind.push_back(1);
    int r = vision_random(20) + 50;                  // 0x425d10, 0x425d40
    for (; r > 0; r--) ms_shuffle(kind);             // (inlined)
}

// 0x454830: the sprites, the interface (the eight coconuts hidden at the targets' spots), the basket cursor, the
// pace and the fuse by skill (and a win before), the trees free, the sounds, Cubby's prompt.
void PearlTossMonitor::start() {
    State &st = Game::state();
    setup();
    shots = 0;                                       // +0xbb4
    reset();
    activate();
    mouse.activate();                                // +0x1790 v6
    keys.activate();                                 // +0x17c4 v6
    iface.loadAll();                                 // 0x41ec40
    iface.activate();                                // v6
    for (int i = 0; i < 8; i++) {
        iface.targets[i]->load();                    // v15
        iface.catches[i]->x = iface.targets[i]->x;   // +0x598
        iface.catches[i]->y = iface.targets[i]->y;   // +0x59c
        iface.catches[i]->visible = false;           // +0x5bc
    }
    iface.count = 0;                                 // +0x1760
    set_cursor(&inbasket(), cur_vis());
    switch (st.saves.gameInfo(kGame)) {
    case 0: info = 0; break;
    case 1: case 2: info = 1; break;
    default: break;
    }
    uint32_t ms = 0;
    switch (st.skill) {
    case 1:
        if (info == 0) { interval = 6000; ms = 150000; kinds(20, 5); }
        else { interval = 5000; ms = 120000; kinds(18, 6); }
        break;
    case 2:
        if (info == 0) { interval = 5714; ms = 120000; kinds(15, 6); }
        else { interval = 4500; ms = 90000; kinds(13, 7); }
        break;
    case 3:
        if (info == 0) { interval = 5000; ms = 90000; kinds(12, 6); }
        else { interval = 3750; ms = 60000; kinds(10, 6); }
        break;
    default: break;                                  // port: the exe leaves the time unset (no such skill)
    }
    Game::cannon().start((int)(ms / 1000));          // 0x4e3a50
    t0 = frame_time() - interval;
    free_.clear();                                   // 0x455ed0
    for (int i = 0; i < 5; i++)
        if (!tosses[i].active()) free_.push_back(i);
    for (size_t j = 0; j < free_.size(); j++) ms_shuffle(free_);
    st.last_game = 3;                                // +0x79c
    for (int i = 0; i < 4; i++) coco_fx(i).open();   // 0x41f9d0
    no_catch = 0;                                    // +0x17f8
    toss_snd().open();
    clap_snd(0).open();
    clap_snd(1).open();
    inactivity.start();                              // +0x30
    st.pearl_ended = false;                          // +0x7a4
}

// v10 0x455050: while the fuse burns and fewer than eight are caught: every `interval` a toss from either mermaid
// at a free tree (the next kind: a pearl, or a miss; Cubby's help after five tosses with no catch); every 100 ms
// the free trees again, shuffled. Eight caught: the claps waited for. Then the end.
void PearlTossMonitor::run() {
    wait(1);
    Mermaid_Scene &sc = mermaid_scene();
    while (Game::cannon().running() && iface.count != 8) {   // 0x4e3b80
        if (frame_time() - t0 >= interval && free_.size() && shots + 1 < kind.size()) {
            if ((unsigned)vision_random(1000) < 500) {    // the state's +8
                side_toss = toss_a;
                side_idle = sc.idle_a;                   // 0x5bcc08
            } else {
                side_toss = toss_b;
                side_idle = sc.idle_b;
            }
            int k = kind.at(shots++);                    // 0x4560d0
            int slot = free_.at(0);                      // 0x455f10
            if (k == 0) {
                tosses[slot].start(trees[slot], side_toss, side_idle, cocos[slot], brokens[slot]);   // 0x453a70
                if (no_catch++ == 4) vo_play("PERL_VO_CUBhelp2");   // 0x4c9400
            } else {
                PearlToss &p = tosses[slot];
                p.tree = trees[slot];
                p.toss = side_toss;
                p.idle_spr = side_idle;
                p.reset();
                p.activate();
                p.path.pastTree(side_toss, trees[slot]);   // 0x453190
                p.miss = true;
            }
            t0 = frame_time();
        }
        wait(100);
        free_.clear();                               // 0x4d87f0
        for (int i = 0; i < 5; i++)
            if (!tosses[i].active()) free_.push_back(i);   // 0x4561b0
        for (size_t j = 0; j < free_.size(); j++) ms_shuffle(free_);   // 0x456430
    }
    while (iface.count == 8 && (mermaid_a().active() || mermaid_b().active())) wait(0);
    stop();                                          // 0x455610
    finish();
}

// v9 0x4554c0: the skip key ends the game unplayed; a toss's catch: the next coconut on the bar, Cubby's line
// (halfway, one more, or one of four nice ones).
void PearlTossMonitor::notify2(Service *s) {
    State &st = Game::state();
    if (s == &keys && g_key.down && (int)g_key.vk == st.skip_key) {   // 0x5a6dd8, 0x5a6d9c, +0x74
        st.last_game = 5;
        stop();
    }
    for (int i = 0; i < 5; i++) {
        if (s != &tosses[i]) continue;
        // port: a ninth catch (before the run notices the eighth) would show the exe's next image, the bar
        if (iface.count < 8) iface.catches[iface.count]->visible = true;   // +0x1178 + n * 0xa8 (+0x7c)
        else iface.bar.visible = true;
        iface.count++;
        if (iface.count > 8) iface.count = 8;
        no_catch = 0;
        if (iface.count == 4) vo_play("PERL_VO_CUBhalfwy");
        else if (iface.count == 7) vo_play("PERL_VO_CUB1more");
        else switch (vision_random(4)) {             // the state's +8
            case 0: vo_play("PERL_VO_CUBnice1"); break;
            case 1: vo_play("PERL_VO_CUBnice2"); break;
            case 2: vo_play("PERL_VO_CUBnice3"); break;
            case 3: vo_play("PERL_VO_CUBnice4"); break;
            default: break;
            }
    }
}

// 0x455610: every toss off, the interface gone, the fuse out and hidden, the pearl ended (the scene's run fades
// back), the sounds closed, the mermaids back on their rocks; eight caught: the sign's star (or cap) by a win
// before, and the result.
void PearlTossMonitor::stop() {
    if (!active()) return;
    State &st = Game::state();
    Mermaid_Scene &sc = mermaid_scene();
    for (PearlToss &p : tosses) {
        if (!p.active()) continue;
        if (!p.miss) {
            p.coco->on = false;
            p.broken->on = false;
        }
        p.mouse.deactivate();
        p.deactivate();
        if (p.path.pearl && p.path.pearl->anim) p.path.pearl->anim->visible = false;
        p.clap.stop();                               // 0x455be0
    }
    deactivate();
    mouse.deactivate();
    keys.deactivate();
    set_cursor(st.cursor, cur_vis());                // +0x5c
    Game::cannon().stop();                           // 0x4e3bc0
    Game::cannon().hide();                           // 0x4e3bf0
    for (Image *t : iface.targets) t->unload();      // v16
    iface.unloadAll();                               // 0x41ec00
    iface.deactivate();
    st.pearl_ended = true;                           // +0x7a4
    st.hover_up = false;                             // +0x75
    st.hover_down = false;                           // +0x76
    sc.fadeOutBegin();                               // 0x4abab0
    inactivity.stop();                               // +0x34
    for (int i = 0; i < 4; i++) coco_fx(i).close();  // 0x41fc50
    toss_snd().close();
    clap_snd(0).close();
    clap_snd(1).close();
    sc.idle_a->anim->visible = true;                 // 0x5bcc08
    sc.idle_b->anim->visible = true;
    toss_a->anim->setFrame(0);
    if (toss_a->anim) toss_a->anim->visible = false;
    toss_b->anim->setFrame(0);
    if (toss_b->anim) toss_b->anim->visible = false;
    if (clap_a->anim) clap_a->anim->visible = false;
    if (clap_b->anim) clap_b->anim->visible = false;
    for (MermaidTask *m : {&mermaid_a(), &mermaid_b()}) {
        if (!m->active()) continue;
        m->deactivate();
        m->idle->anim->visible = true;
        if (m->clap->anim) m->clap->anim->visible = false;
        m->snd->stop();
    }
    sc.tink->wanders = true;                         // 0x5bca64 +0x13c
    if (st.last_game == 5) return;
    if (iface.count != 8) {
        st.last_result = 1;                          // +0x7a0
        return;
    }
    switch (st.saves.gameInfo(kGame)) {
    case 0:
        sc.pearl_sign->showAttached("pearlGameStar");   // 0x42f690
        st.last_result = 2;
        break;
    case 1:
        sc.pearl_sign->showAttached("pearlGameCap");
        st.last_result = 3;
        break;
    case 2: st.last_result = 4; break;
    default: break;
    }
}

// 0x455950: of the coconuts lying at the trees' feet, the nearest to the camera (its picture's x and z).
bool PearlTossMonitor::hint(Attachment **out) {
    std::vector<Attachment *> on;
    for (Attachment *c : cocos)
        if (c->on) on.push_back(c);                  // 0x4561b0
    if (on.empty()) return false;
    std::vector<float> d(on.size(), 0.0f);           // 0x4cc570
    for (size_t i = 0; i < on.size(); i++)
        d[i] = (float)rt->floorDist((float)on.at(i)->g->x, (float)on.at(i)->g->z);   // 0x42e400
    size_t m = 0;
    for (size_t i = 1; i < d.size(); i++)
        if (d[i] < d[m]) m = i;
    *out = on.at(m);
    return true;
}

}  // namespace Game
