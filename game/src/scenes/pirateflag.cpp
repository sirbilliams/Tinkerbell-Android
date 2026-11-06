// Pirate Flag Tag, reconstructed from "Tinker Bell.exe" (see pirateflag.h).
#include "pirateflag.h"
#include "lost.h"
#include "state.h"
#include "cannon.h"
#include "voiceover.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace Game {

using Raytrace::Sprite3D;
using Vision::Animation;

static int g_level = 0;                              // 0x5b6d94
static float g_high_chance = 0;                      // 0x5b6d98
static const char *const kGame = "Pirate Flag Tag Game";   // *0x543530

// ---------------------------------------------------------------- GhostPath

void GhostPath::pickTarget(bool toEnd) {             // 0x447f10
    float X, Y, Z;
    for (;;) {
        bool ok = true;
        float R = (float)Vision::vision_random((int)(2000.0f / (double)g_level * 6.0f));   // 0x5144d0, 0x51451c
        double a = (double)Vision::vision_random(0x168) * 0.0174532924f;   // 0x514518
        float A = (float)a;
        X = (float)(cos(a) * R + 3350.0);            // 0x514510
        uint32_t c = (uint32_t)Vision::vision_random(1000);
        Y = c < (uint32_t)(int)(g_high_chance * 1000.0f) ? 350.0f : 0.0f;   // 0x513db0
        Z = (float)(sin((double)A) * R + 3350.0);
        if (rt->pointInBox(X, Z)) ok = false;        // 0x429920
        if (rt->cameraNear(X, Z, 100.0f)) ok = false;   // 0x4298e0
        double dx = (double)X - 3350.0f, dz = (double)Z - 3350.0f;   // 0x514508
        if ((double)2000.0f * 2000.0f < dx * dx + dz * dz) continue;
        if (ok) break;
    }
    if (toEnd) {
        to[0] = X; to[1] = Y; to[2] = Z;
    } else {
        from[0] = cur[0] = X;
        from[1] = cur[1] = Y;
        from[2] = cur[2] = Z;
    }
    double dx = (double)to[0] - from[0], dy = (double)to[1] - from[1], dz = (double)to[2] - from[2];
    delta[0] = (float)dx; delta[1] = (float)dy; delta[2] = (float)dz;
    duration = (int)(sqrt(dz * dz + dy * dy + dx * dx) / 0.25f);   // 0x5144cc
    elapsed = 0;
    last = Vision::frame_time();
}

// ---------------------------------------------------------------- Ghost

// 0x446810: "ghost_1".."ghost_4" (Foxy, Rabbit, a raccoon, the skunk under the sheets).
Ghost::Ghost(const char *n, Raytrace::RayTrace *r) : rt(r) {
    strncpy(name, n, sizeof name - 1);
    // port: the exe passes two arguments to the three-argument 0x42d9d0; the third is the caller's saved edi
    // (the game object, non-zero): a hit mask
    rt->addSprite(name, "PIRT_CA_GSTwalkid_", 1);
    sprite = rt->findSprite(name);
    sprite->preload("PIRT_CA_GSTwalkno_");          // 0x42edd0
    sprite->preload("PIRT_CA_GSTwalkea_");
    sprite->preload("PIRT_CA_GSTwalkwe_");
    sprite->preload("PIRT_CA_GSTwalkso_");
    sprite->preload("PIRT_CA_GSTcaught_");
    sprite->load();
    sprite->scale = 200.0f;
    sprite->place();
    yoff = sprite->y;
    sprite->mode = 1;
    sprite->radius = 250.0f;                         // +0xa4
    sprite->bheight = 400.0f;                        // +0xa8
    rt->makeBox(sprite, false);                      // 0x429580
    path = new GhostPath();                          // 0x4f34f3(0x40)
    path->rt = rt;
    path->pickTarget(false);
    path->pickTarget(true);
    state = 0;
    last = Vision::frame_time();
    static const struct { const char *ghost, *awww, *t1, *t2, *t3; } lines[4] = {
        {"ghost_1", "PIRT_VO_FOXawww", "PIRT_VO_FOXtaunt1", "PIRT_VO_FOXtaunt2", "PIRT_VO_FOXtaunt3"},
        {"ghost_2", "PIRT_VO_RABawww", "PIRT_VO_RABtaunt1", "PIRT_VO_RABtaunt2", "PIRT_VO_RABtaunt3"},
        {"ghost_3", "PIRT_VO_RACawww_y", "PIRT_VO_RACtaunt1", "PIRT_VO_RACtaunt2", "PIRT_VO_RACtaunt3"},
        {"ghost_4", "PIRT_VO_SKNawww", "PIRT_VO_SKNtaunt1", "PIRT_VO_SKNtaunt2", "PIRT_VO_SKNtaunt3"}};
    for (const auto &l : lines) {
        if (strcmp(name, l.ghost) != 0) continue;    // 0x423c60
        awww = l.awww;
        taunts.add(l.t1);
        taunts.add(l.t2);
        taunts.add(l.t3);
        break;
    }
}

Ghost::~Ghost() {                                    // 0x446b20
    delete path;
    path = nullptr;
    rt->removeSprite(name);                          // 0x42da60
}

// The step each phase inlines (0x4474bb..): every 30 ms. Placed: pushed off any box it stands in, then off.
void Ghost::tick() {
    uint32_t now = Vision::frame_time();
    dt = now - last;
    if (dt < 0x1e) return;
    last = now;
    switch (state) {
    case 0:
        while (rt->boxHits(sprite)) {                // 0x429830
            path->pickTarget(false);
            sprite->x = path->from[0];
            sprite->y = path->from[1] + yoff;
            sprite->z = path->from[2];
        }
        path->pickTarget(true);
        state = 1;
        break;
    case 1: walk(); break;
    case 2: idle(); break;
    case 3: sprite->setAnim("PIRT_CA_GSTcaught_"); break;   // 0x42efb0
    default: break;
    }
}

// 0x446ba0: along the path (every 30 ms of its own clock); in the camera's box or another box it turns back to
// where it was and looks for a new way; at the end it stands (state 0).
void Ghost::walk() {
    bool done = false;
    GhostPath *p = path;
    uint32_t now = Vision::frame_time();
    uint32_t d = now - p->last;
    if (d >= 0x1e) {
        p->last = now;
        p->elapsed += (int)d;
        double t = (double)(uint32_t)p->elapsed / (double)p->duration;   // fild qword / fidiv
        if (!(t < 1.0f)) { t = 1.0f; done = true; }  // 0x513710
        p->cur[0] = (float)(t * p->delta[0] + p->from[0]);
        p->cur[1] = (float)(t * p->delta[1] + p->from[1]);
        p->cur[2] = (float)(t * p->delta[2] + p->from[2]);
    }
    float sx = sprite->x, sy = sprite->y, sz = sprite->z;
    sprite->x = path->cur[0];
    sprite->y = path->cur[1];
    sprite->z = path->cur[2];
    sprite->y = (float)((double)sprite->y + yoff);
    if (rt->cameraInBox(sprite) || rt->boxHits(sprite)) {   // 0x429710, 0x429830
        path->from[0] = sx;
        path->from[1] = (float)((double)sy - yoff);
        path->from[2] = sz;
        newPath();
    } else if (done) {
        path->from[0] = path->to[0];
        path->from[1] = path->to[1];
        path->from[2] = path->to[2];
        state = 0;
    }
    face();
}

// 0x446d00: from where it is, up to five new ends whose start and first fiftieth are free; none: it idles.
void Ghost::newPath() {
    GhostPath *p = path;
    p->cur[0] = p->from[0];
    p->cur[1] = p->from[1];
    p->cur[2] = p->from[2];
    bool ok = false;
    int tries = 0;
    while (tries < 5) {
        ok = true;
        tries++;
        path->pickTarget(true);
        float st[3] = {(float)(path->delta[0] * 0.02), (float)(path->delta[1] * 0.02),
                       (float)(path->delta[2] * 0.02)};   // 0x5144d8
        bool hit = false;
        for (int i = 0; i < 2; i++) {
            sprite->x = (float)((double)i * st[0] + path->from[0]);
            sprite->y = (float)((double)i * st[1] + path->from[1] + yoff);
            sprite->z = (float)((double)i * st[2] + path->from[2]);
            if (rt->cameraInBox(sprite) || rt->boxHits(sprite)) { hit = true; break; }
        }
        if (!hit) break;
        ok = false;
    }
    sprite->x = path->from[0];
    sprite->y = (float)((double)yoff + path->from[1]);
    sprite->z = path->from[2];
    if (!ok) {
        state = 2;
        return;
    }
    double dx = (double)p->to[0] - p->from[0], dy = (double)p->to[1] - p->from[1], dz = (double)p->to[2] - p->from[2];
    p->delta[0] = (float)dx; p->delta[1] = (float)dy; p->delta[2] = (float)dz;
    p->duration = (int)(sqrt(dz * dz + dy * dy + dx * dx) / 0.25f);
    p->elapsed = 0;
    p->last = Vision::frame_time();
}

// 0x446e80: standing; within 600 of the camera, or 1 in 50 each step, it moves on: a wait set by the hint ends
// first (resuming the old way), else past it a new end.
void Ghost::idle() {
    sprite->setAnim("PIRT_CA_GSTwalkid_");
    double dx = (double)sprite->x - rt->camx, dz = (double)sprite->z - rt->camz;
    bool go = dz * dz + dx * dx < 360000.0f;         // 0x5144e0
    if ((uint32_t)Vision::vision_random(1000) < 0x14) go = true;
    if (!go) return;
    uint32_t w = wait;
    if (Vision::frame_time() > w && w != 0) {
        wait = 0;
        path->last = Vision::frame_time();
        state = 1;
        return;
    }
    if (Vision::frame_time() > wait) {
        path->pickTarget(true);
        state = 1;
    }
}

// 0x446f50: the heading relative to the view, 0..360: east below 45 and from 315, south to 135, west to 225,
// north to 315.
void Ghost::face() {
    double h = atan2((double)path->delta[2], (double)path->delta[0]);
    double deg = (h - ((double)rt->angf - 1.5707963267948966)) * 57.2957802f + 180.0f;   // 0x5144f8, 0x5144f4
    while (deg < 0.0f) deg += 360.0f;                // 0x513de8
    while (deg > 360.0f) deg -= 360.0f;
    if (!(deg < 315.0f) || deg < 45.0f) sprite->setAnim("PIRT_CA_GSTwalkea_");
    else if (deg < 135.0f) sprite->setAnim("PIRT_CA_GSTwalkso_");
    else if (deg < 225.0f) sprite->setAnim("PIRT_CA_GSTwalkwe_");
    else sprite->setAnim("PIRT_CA_GSTwalkno_");
}

// ---------------------------------------------------------------- FlagCursor

FlagCursor::FlagCursor()                             // 0x442e70
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      wave("PIRT_CU_flagwave_"), flag("PIRT_CU_flag_") {}

void FlagCursor::load() {                            // v15 0x442fc0: both looping
    wave.load();
    wave.play(0, 1);                                 // 0x403be0
    flag.load();
    flag.play(0, 1);
}

void FlagCursor::unload() {                          // v16 0x443000
    Graphic::unload();
    wave.unload();
    flag.unload();
}

void FlagCursor::draw() {                            // v14 0x443030
    if (!visible) return;
    Animation &a = waving ? wave : flag;
    a.x = x;
    a.y = y;
    a.draw();
}

// ---------------------------------------------------------------- PirateFlagGame

PirateFlagGame::PirateFlagGame(Raytrace::RayTrace *r) : rt(r) {   // 0x447050
    last = Vision::frame_time();
}

PirateFlagGame::~PirateFlagGame() {                  // 0x447090
    for (Ghost *&g : ghosts) { delete g; g = nullptr; }
    delete flag;                                     // port: the exe leaves it to cleanup
    flag = nullptr;
}

// 0x4470c0: by the game's "Info" (never won / won) and the skill (1 when not saved): the cannon's time, the
// level and the chance of a high spot.
void PirateFlagGame::start() {
    Lost_Scene &sc = lost_scene();
    sc.flag_on = true;                               // 0x5af934
    phase = 2;
    loaded = false;
    State &st = state();
    int info = st.saves.gameInfo(kGame);
    int skill = 0;
    int s = st.saves.getSkill(&skill) ? skill : 1;
    if (info == 0) {
        switch (s) {
        case 1: secs = 0x78; g_level = 6; g_high_chance = 0.1f; break;
        case 2: secs = 0x5a; g_level = 5; g_high_chance = 0.2f; break;
        case 3: secs = 0x3c; g_level = 4; g_high_chance = 0.4f; break;
        default: break;
        }
    } else {
        switch (s) {
        case 1: secs = 0x5a; g_level = 5; g_high_chance = 0.3f; break;
        case 2: secs = 0x3c; g_level = 4; g_high_chance = 0.4f; break;
        case 3: secs = 0x2d; g_level = 3; g_high_chance = 0.6f; break;
        default: break;
        }
    }
    st.last_game = 3;
}

void PirateFlagGame::stop() {                        // 0x447240
    Game::cannon().hide();
    if (phase == 3) {
        phase = 4;
        ended = Vision::frame_time();
    }
}

// 0x447270: the pirates' tune, the "pirate_flag" pack, the flag cursor and the four ghosts.
void PirateFlagGame::load() {
    Lost_Scene &sc = lost_scene();
    sc.playMusic(3);
    pack = Vision::ras_open("pirate_flag", "", &Vision::data_device());   // 0x419b90
    pack->load();                                    // 0x41aa20
    Vision::Ras *prev = Vision::ras_set_current(pack);
    flag = new FlagCursor();                         // 0x4243c0(0x2d0)
    flag->load();                                    // v15
    for (int i = 0; i < 4; i++) {
        char n[16];
        snprintf(n, sizeof n, "ghost_%1.1d", i + 1);   // 0x537f58
        ghosts[i] = new Ghost(n, rt);                // 0x4f34f3(0x88)
    }
    Vision::ras_set_current(prev);
    t0 = Vision::frame_time();
    phase = 1;
    loaded = true;
    taunted = Vision::frame_time();
    prompted = false;
}

void PirateFlagGame::tickGhosts() {
    for (Ghost *g : ghosts) g->tick();
}

// 0x4473a0: the scene busy; 2 s fading to black, then at the ghosts' spot 2 s fading in; then the game.
void PirateFlagGame::intro() {
    Lost_Scene &sc = lost_scene();
    sc.busy = true;                                  // 0x5aedd8
    sc.signsOn(false);
    sc.hotOn(false);
    uint32_t e = Vision::frame_time() - t0;
    if (e > 4000) {
        sc.dim.setLevel(1.0f);                       // 0x5aed30
        phase = 3;
        sc.playMusic(0);
        sc.busy = false;
        Game::cannon().visible = true;               // state +0x83c
        Game::cannon().start(secs);
        flag->visible = true;
        sc.skunk.hide();                             // 0x4cca40
        sc.raccoon1.hide();
        sc.raccoon2.hide();
        return;
    }
    if (e > 2000) {
        sc.rt.setCamera(4991.26f, 0.1f, 2696.38f, 151.636f);   // 0x42a570
        sc.tokens->visible = false;
        tickGhosts();
        sc.dim.setLevel((float)((double)(e - 2000) * 0.0005f));   // 0x514504
        return;
    }
    sc.dim.setLevel((float)((2000.0f - (double)e) * 0.0005f));   // 0x514500
    flag->visible = false;
}

// 0x447860: the ghosts; every 5 s one at random (not tagged, no line playing) taunts; under 20 s left, Cubby's
// prompt once.
void PirateFlagGame::play() {
    tickGhosts();
    State &st = state();
    if (Vision::frame_time() > taunted + 5000) {
        Ghost *g = ghosts[Vision::vision_random(4)];
        if (g->state != 3 && !st.vo_busy()) g->taunts.playNext();   // +0x560, 0x4cc0e0
        taunted = Vision::frame_time();
    }
    if ((uint32_t)Game::cannon().remaining() < 20000 && !prompted && !st.vo_busy()) {   // 0x4e3c20
        VoiceOver::get().play("PIRT_VO_CUBprompt");
        prompted = true;
    }
}

// 0x4475d0: 2 s fading out with the ghosts, then back at the area's spot fading in (the ghosts cleared once);
// then the end: the EndOfGame runs from the scene (state +0x7a6).
void PirateFlagGame::outro() {
    Lost_Scene &sc = lost_scene();
    uint32_t e = Vision::frame_time() - t0;
    if (e > 4000) {
        sc.skunk.show();                             // 0x4cca70
        sc.dim.setLevel(1.0f);
        phase = 0;
        state().flag_ended = true;                   // +0x7a6
        return;
    }
    if (e > 2000) {
        State &st = state();
        st.setCameraToSpot(3, 1, &sc.rt);            // 0x4ec140
        st.hover_down = false;                       // +0x76
        st.hover_up = false;                         // +0x75
        sc.tokens->visible = true;
        if (loaded) cleanup();
        sc.dim.setLevel((float)((double)(e - 2000) * 0.0005f));
        return;
    }
    sc.dim.setLevel((float)((2000.0f - (double)e) * 0.0005f));
    tickGhosts();
    flag->visible = false;
}

// 0x4477b0: no game cursor; the flag, the ghosts and the pack gone; the hideout's tune; the raccoons back.
void PirateFlagGame::cleanup() {
    Vision::set_cursor(nullptr, Vision::g_cursor_graphic ? Vision::g_cursor_graphic->visible : false);   // 0x4119b0
    flag->unload();                                  // v16
    delete flag;                                     // v3
    flag = nullptr;
    for (Ghost *&g : ghosts) { delete g; g = nullptr; }
    Vision::ras_close(pack);                         // 0x41b050 + 0x419620
    pack = nullptr;
    Lost_Scene &sc = lost_scene();
    sc.flag_on = false;
    sc.playMusic(2);
    sc.raccoon1.show();
    sc.raccoon2.show();
    loaded = false;
}

// 0x4479e0: every 30 ms; the cannon out while playing: lost (result 1).
void PirateFlagGame::update() {
    uint32_t now = Vision::frame_time();
    if (now - last < 0x1e) return;
    last = now;
    if (phase == 3 && !Game::cannon().running()) {
        phase = 4;
        state().last_result = 1;
    }
    switch (phase) {
    case 2: load(); break;
    case 1: intro(); break;
    case 3: play(); break;
    case 4:
        Game::cannon().hide();
        tickGhosts();
        t0 = Vision::frame_time();
        phase = 5;
        break;
    case 5: outro(); break;
    case 6: cleanup(); break;
    default: break;
    }
}

bool PirateFlagGame::tag(Sprite3D *s) {              // 0x447b90
    if (phase != 3) return false;
    for (Ghost *g : ghosts) {
        if (g->state == 3 || g->sprite != s) continue;
        g->state = 3;
        VoiceOver::get().play(g->awww.c_str());
        return true;
    }
    return false;
}

int PirateFlagGame::tagged() {                       // 0x447c00
    if (phase != 3) return 0;
    int n = 0;
    for (Ghost *g : ghosts)
        if (g->state == 3) n++;
    if (n == 1) VoiceOver::get().play("PIRT_VO_CUBhalfwy");
    else if (n == 3) VoiceOver::get().play("PIRT_VO_CUB1more");
    return n;
}

FlagCursor *PirateFlagGame::cursor() {               // 0x447c70
    Sprite3D *s = lost_scene().rt.pick(Vision::g_mouse.x, Vision::g_mouse.y);   // 0x42c710
    flag->waving = false;
    if (!s || phase != 3) return flag;
    for (Ghost *g : ghosts)
        if (g->state != 3 && g->sprite == s) { flag->waving = true; break; }
    return flag;
}

// 0x447cd0: of the free ghosts the nearest (the first of equals); it stands 9 s.
bool PirateFlagGame::nearest(Sprite3D **out) {
    std::vector<Ghost *> free;
    for (Ghost *g : ghosts)
        if (g->state != 3) free.push_back(g);        // 0x4561b0
    if (free.empty()) return false;
    std::vector<float> d;
    for (Ghost *g : free) d.push_back((float)lost_scene().rt.spriteDist(g->sprite));   // 0x42e3d0
    size_t best = 0;
    for (size_t i = 1; i < d.size(); i++)
        if (d[i] < d[best]) best = i;
    *out = free[best]->sprite;
    free[best]->state = 2;
    free[best]->wait = Vision::frame_time() + 9000;
    return true;
}

}  // namespace Game
