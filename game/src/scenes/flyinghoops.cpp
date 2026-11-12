// Flying Hoops, reconstructed from "Tinker Bell.exe" (see flyinghoops.h).
#include "flyinghoops.h"
#include "indianvillage.h"
#include "state.h"
#include "cannon.h"
#include "voiceover.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static Raytrace::RayTrace &vrt() { return village_scene().rt; }   // 0x5c8cc0 (the static's +0x140)

FlyingHoopsGame::FlyingHoopsGame()                   // 0x4611e0
    : Task(nullptr), reminder("FLYH_LY_shiftcnt_01", "FLYH_VO_PETupdown") {
    inactivity.add("FLYH_VO_RACprompt", false);      // 0x4c46e0
    nice.push_back("FLYH_VO_RACnice1");
    nice.push_back("FLYH_VO_RACnice2");
    nice.push_back("FLYH_VO_RACnice3");
    nice.push_back("FLYH_VO_RACnice4");
}

// 0x460da0: the time and how many fires smoke, by skill (a game won before: harder).
void FlyingHoopsGame::setup() {
    State &st = Game::state();
    int info = st.saves.gameInfo("Flying Hoops Game");   // 0x4e9d70 (*0x543528)
    int skill = 0;                                   // port: the exe's local is uninitialised when the save's
                                                     // skill is out of range (then no case is taken)
    if (!st.saves.getSkill(&skill)) skill = 1;       // 0x4e97a0
    if (info == 0) {
        switch (skill) {
        case 1: secs = 120; level = 1; break;
        case 2: secs = 90; level = 2; break;
        case 3: secs = 60; level = 4; break;
        default: break;
        }
    } else {
        switch (skill) {
        case 1: secs = 90; level = 2; break;
        case 2: secs = 60; level = 3; break;
        case 3: secs = 45; level = 5; break;
        default: break;
        }
    }
}

// 0x4604c0: the camp fires ("flyh_ga_hopfire_..."), as many kept as `level` (the others dropped at random), each
// with a smoke pole over it and the smoke ring on top.
void FlyingHoopsGame::smoke() {
    Raytrace::RayTrace &rt = vrt();
    static const std::string fire = "flyh_ga_hopfire_";   // 0x5c8b50
    std::vector<Sprite3D *> fires;
    for (int i = 0; i < (int)rt.sprites.size(); i++) {   // 0x42a120, 0x42a0e0
        Sprite3D *s = rt.sprites[i];
        if (s->prefix.compare(0, fire.size(), fire) == 0) fires.push_back(s);   // 0x461c50
    }
    while (fires.size() > (size_t)level) {
        int r = vision_random((int)fires.size());    // 0x425d40
        fires.erase(fires.begin() + r);
    }
    for (int i = 0; i < (int)fires.size(); i++) {
        char n[64];
        snprintf(n, sizeof n, "smokepole%2.2d", i);
        Sprite3D *p = rt.addSprite(n, "FLYH_GA_smpole_", 0);   // 0x42d9d0
        p->load();                                   // 0x42ef20
        p->frame_ms = 100;                           // +0xac
        p->mode = 1;                                 // +0x88
        Sprite3D *f = fires[i];
        p->x = f->x;
        p->z = f->z;
        p->y = f->y;
        p->y = (float)((double)f->scale * f->y * 0.01f + p->y);   // 0x513d4c
        int hh = (int)((double)p->anim->frame(0)->oh * 0.5f);   // 0x403ec0 (+0x40), ftol
        p->scale = 100.0f;                           // +0x84
        p->y = (float)((double)hh + p->y);
        snprintf(n, sizeof n, "smokehoop%2.2d", i);
        Sprite3D *h = rt.addSprite(n, "FLYH_GA_HOPsmoke_", 0);
        h->load();
        h->mode = 1;
        h->x = f->x;
        h->y = 0;
        h->z = f->z;
        h->y = (float)((double)p->scale * p->y * 0.01f);
        int hh2 = h->anim->frame(0)->oh;
        h->scale = 100.0f;
        h->y = (float)((double)hh2 + h->y);
    }
}

// 0x4608b0: at each hoop pole and teepee a white hoop (numbered from 1) and a red one, hidden, in the same place.
void FlyingHoopsGame::hoops() {
    Raytrace::RayTrace &rt = vrt();
    int n = 0;
    for (int i = 0; i < (int)rt.sprites.size(); i++) {   // (the count read again each time: the hoops are added)
        std::string p = rt.sprites[i]->prefix;       // (copied to a buffer)
        bool pole = false;
        if (!strcasecmp(p.c_str(), "FLYH_GA_HOPpole1_")) pole = true;   // 0x423840
        if (!strcasecmp(p.c_str(), "FLYH_GA_HOPpole2_")) pole = true;
        if (!strcasecmp(p.c_str(), "FLYH_GA_HOPpole3_")) pole = true;
        if (strcasecmp(p.c_str(), "INDV_AM_teepee_") != 0 && !pole) continue;
        Sprite3D *pl = rt.sprites[i];
        n++;
        char name[64];
        snprintf(name, sizeof name, "whitehoop%2.2d", n);
        rt.addSprite(name, "FLYH_GA_hoopwhte_", 0);
        Sprite3D *w = rt.findSprite(name);
        w->load();
        w->x = pl->x;
        w->y = (float)((double)pl->scale * pl->y * 0.02f);   // 0x514db8
        int hh = (int)((double)w->anim->frame(0)->oh * 0.5f);
        w->y = (float)(((double)hh + w->y) * 0.99f);  // 0x514db4
        w->mode = 0;
        w->z = pl->z;
        w->scale = 100.0f;
        snprintf(name, sizeof name, "redhoop%2.2d", n);
        rt.addSprite(name, "FLYH_GA_hoopred_", 0);
        Sprite3D *r = rt.findSprite(name);
        r->load();
        r->x = w->x;
        r->y = w->y;
        r->z = w->z;
        r->scale = w->scale;
        if (r->anim) r->anim->visible = false;
    }
}

// The camera's point for the hoops (0x42b470, 0x42b4e0, 0x42b540): its x, its eye in world height, its z.
static void cam_point(Raytrace::RayTrace &rt, float *x, float *y, float *z) {
    *x = rt.camx;
    *y = (float)((double)rt.eyef * 2.05f + 480.0f);  // 0x513d9c, 0x513d48
    *z = rt.camz;
}

static double dist2(float cx, float cy, float cz, const Sprite3D *s) {
    double dx = (double)cx - s->x, dy = (double)cy - s->y, dz = (double)cz - s->z;
    return (dz * dz + dy * dy) + dx * dx;
}

// 0x460ac0: a white hoop within 200 flown through forward turns red (backward: the raccoon's "the other way");
// moving within 200 of a smoke ring: "not through the smoke". The raccoon's line as the count goes up.
void FlyingHoopsGame::update() {
    Raytrace::RayTrace &rt = vrt();
    IndianVillage_Scene &sc = village_scene();
    float cx, cy, cz;
    cam_point(rt, &cx, &cy, &cz);
    for (int i = 0; i < 10; i++) {
        char n[64];
        snprintf(n, sizeof n, "whitehoop%2.2d", i + 1);
        Sprite3D *w = rt.findSprite(n);
        if (done[i]) continue;
        if (!(dist2(cx, cy, cz, w) < 40000.0f)) continue;   // 0x514dbc
        if (sc.fwd) {                                // 0x5ca204 (the static's +0x1684)
            if (w->anim) w->anim->visible = false;
            snprintf(n, sizeof n, "redhoop%2.2d", i + 1);
            rt.findSprite(n)->anim->visible = true;
            done[i] = true;
            sc.fx[0]->play();                        // 0x5ce7e8 "FLYH_FX_hoopgood"
        } else if (sc.back) {                        // 0x5ca205
            VoiceOver::get().play("FLYH_VO_RACbackno");
        }
    }
    for (int i = 0; i < level; i++) {
        char n[64];
        snprintf(n, sizeof n, "smokehoop%2.2d", i);
        Sprite3D *s = rt.findSprite(n);
        if (!(dist2(cx, cy, cz, s) < 40000.0f)) continue;
        if (sc.fwd || sc.back) VoiceOver::get().play("FLYH_VO_RACsmoke");
    }
    passed = 0;
    for (bool d : done)
        if (d) passed++;
    if (passed != last) {
        switch (passed) {                            // the table 0x460d74
        case 1: case 3: case 7: case 8:
            VoiceOver::get().play(nice.at(vision_random((int)nice.size())));   // 0x425d40
            break;
        case 5: VoiceOver::get().play("FLYH_VO_RAChalfwy_y"); break;
        case 9: VoiceOver::get().play("FLYH_VO_RAC1more_y"); break;
        default: break;
        }
    }
    last = passed;
}

// 0x460e70: the signs and hot regions off, the hoops all white, the game on (its tune), the raccoon's prompts.
void FlyingHoopsGame::start() {
    IndianVillage_Scene &sc = village_scene();
    Raytrace::RayTrace &rt = vrt();
    sc.signsOn(false);                               // 0x463270
    sc.hotsOn(false);                                // 0x468b30
    for (int i = 0; i < 10; i++) {
        done[i] = false;
        char n[64];
        snprintf(n, sizeof n, "redhoop%2.2d", i + 1);
        Sprite3D *r = rt.findSprite(n);
        if (r->anim) r->anim->visible = false;
        snprintf(n, sizeof n, "whitehoop%2.2d", i + 1);
        rt.findSprite(n)->anim->visible = true;
    }
    on = true;
    reset();                                         // 0x421110
    activate();                                      // +0x18
    sc.music(0);                                     // 0x466010
    Game::state().last_game = 3;                     // +0x79c
    inactivity.start();                              // v12
    last = 0;
}

// 0x460f50: all ten passed: won ("Info" 0: the sign's star, 1: its cap); else lost.
void FlyingHoopsGame::results() {
    IndianVillage_Scene &sc = village_scene();
    State &st = Game::state();
    sc.signsOn(true);
    sc.hotsOn(true);
    bits |= 1;
    if (passed != 10) {
        bits |= 2;
        st.last_result = 1;                          // +0x7a0
        return;
    }
    switch (st.saves.gameInfo("Flying Hoops Game")) {
    case 0:
        bits |= 4;
        sc.flyh.star->on = true;                     // 0x5cd188 (+0x17c)
        st.last_result = 2;
        break;
    case 1:
        bits |= 8;
        sc.flyh.cap->on = true;                      // 0x5cd184
        st.last_result = 3;
        break;
    case 2: st.last_result = 4; break;
    default: break;
    }
}

void FlyingHoopsGame::skip() {                       // 0x461040
    if (!on) return;
    Game::cannon().stop();                           // 0x4e3bc0
    Game::state().last_game = 5;
}

// 0x461070 (the scene's hover while the game is on and the eye drifts): the eye eases toward the nearest hoop's
// height (one passed counts 5 times as far), faster the nearer it is.
void FlyingHoopsGame::hover() {
    Raytrace::RayTrace &rt = vrt();
    Sprite3D *near = nullptr;
    float best = 1e8f;                               // 0x4cbebc20
    float cx, cy, cz;
    cam_point(rt, &cx, &cy, &cz);
    for (int i = 0; i < 10; i++) {
        char n[64];
        snprintf(n, sizeof n, "whitehoop%2.2d", i + 1);
        Sprite3D *w = rt.findSprite(n);
        double d = dist2(cx, cy, cz, w);
        if (done[i]) d *= 25.0f;                     // 0x514484
        if (d < best) {
            best = (float)d;
            near = w;
        }
    }
    if (!near) return;                               // port: the exe reads the null sprite
    float ty = (float)(((double)near->y - 480.0f) * 0.48780489f);   // 0x42b500
    double t = (double)ty - rt.eyef;                 // 0x42b480
    double d = sqrt((double)best) * 0.006666666666666667;   // 0x514dc0
    if (d < 1.0f) d = 1.0f;
    float v = (float)(t / d);
    if (v > 0.0f) rt.raise(v);                       // 0x42b130
    else rt.lower(-v);                               // 0x42b2d0
}

// 0x4619e0: the nearest white hoop not passed yet (none once all ten are).
bool FlyingHoopsGame::nextHoop(Sprite3D **out) {
    if (passed == 10) return false;
    Raytrace::RayTrace &rt = vrt();
    std::vector<Sprite3D *> left;
    for (int i = 0; i < 10; i++) {
        if (done[i]) continue;
        char n[64];
        snprintf(n, sizeof n, "whitehoop%2.2d", i + 1);   // 0x423ae0
        left.push_back(rt.findSprite(n));
    }
    std::vector<float> d(left.size());
    for (size_t i = 0; i < left.size(); i++) d[i] = (float)rt.spriteDist(left[i]);   // 0x42e3d0
    if (d.empty()) return false;                     // port: the exe's min_element on an empty list throws
    size_t m = 0;
    for (size_t i = 1; i < d.size(); i++)
        if (d[i] < d[m]) m = i;
    *out = left[m];
    return true;
}

// v10 0x4616c0: the "Shift / Ctrl" reminder, then the fuse; each time the game is started (start), every frame its
// update until the fuse runs out or all ten are passed; the results (unless skipped); a fade to black, the
// camera back at the area's spot 1, a fade back in; the end flag for the scene (state +0x7a7).
void FlyingHoopsGame::run() {
    wait(1);
    reminder.start();                                // 0x4c6220
    while (reminder.active()) wait(0);
    Cannon &fuse = Game::cannon();                   // 0x5cfee8
    fuse.visible = true;
    fuse.start(secs);                                // 0x4e3a50
    IndianVillage_Scene &sc = village_scene();
    State &st = Game::state();
    for (;;) {
        wait(50);
        if (!on) continue;
        while (fuse.running()) {                     // 0x4e3b80
            wait(0);
            if (getenv("TB_HOOPS_AUTO")) {           // test aid (not in the game): into the nearest hoop, forward
                Sprite3D *h = nullptr;
                if (nextHoop(&h))
                    sc.rt.setCamera(h->x, (float)(((double)h->y - 480.0f) / 2.05f), h->z, (float)sc.rt.angleDeg());
                sc.fwd = true;
            }
            update();
            if (passed == 10) break;
        }
        sc.music(2);                                 // 0x466010
        on = false;
        fuse.stop();
        fuse.hide();                                 // 0x4e3bf0
        if (st.last_game != 5) results();            // 0x460f50
        t0 = frame_time();
        sc.busy = true;                              // 0x5cb460 (the static's +0x28e0)
        hide_cursor();                               // 0x4119e0
        for (;;) {
            wait(30);
            uint32_t dt = frame_time() - t0;
            if (dt < 2000) {
                sc.dim.setLevel((float)((2000.0f - (double)(int64_t)dt) * 0.0005f));   // 0x5cb3b8, 0x415f20
                continue;
            }
            if (dt < 3000) {
                st.setCameraToSpot(1, 1, &sc.rt);    // 0x4ec140
                st.hover_up = false;                 // +0x75
                st.hover_down = false;               // +0x76
                sc.forceNone();                      // 0x4abf30
                sc.rt.setCamera(sc.rt.camx, 0.1f, sc.rt.camz, (float)sc.rt.angleDeg());   // 0x42a570
                sc.dim.setLevel(0);
                // (falls through: the level below is negative until 3000 ms; the exe's)
            }
            if (dt < 5000) {
                sc.dim.setLevel((float)(((double)(int64_t)dt - 3000.0f) * 0.0005f));
                continue;
            }
            sc.dim.setLevel(1.0f);
            sc.busy = false;
            show_cursor();                           // 0x4119d0
            st.hover_down = false;
            st.hover_up = false;
            st.hoops_ended = true;                   // +0x7a7
            on = false;
            inactivity.stop();                       // v13
            reminder.stop();                         // 0x4c6260
            break;
        }
    }
}

}  // namespace Game
