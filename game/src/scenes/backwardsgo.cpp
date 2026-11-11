// Backwards Go, reconstructed from "Tinker Bell.exe" (see backwardsgo.h).
#include "backwardsgo.h"
#include "starry.h"
#include "state.h"
#include "voiceover.h"
#include "../../../platform/dinput.h"
#include "tb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tb_input_frame();                                // platform/dinput.cpp (test aid)

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0
static VoiceOver &vo() { return VoiceOver::get(); }  // the state's +0x3e8

// 0x5d2900 (made by 0x46f9c0, freed at exit): the footsteps of a boy walking.
static Sound &moving() {
    static Sound s("BACK_FX_moving");
    return s;
}

static const float kGridX = 3290.1f;                 // 0x515124
static const float kGridZ = 2050.1f;                 // 0x515120

// ---------------------------------------------------------------- BackwardsGoCharacter
// 0x46fa00: a boy's "aww" (lost) and "rats" (caught out) by his name.
BackwardsGoCharacter::BackwardsGoCharacter(Raytrace::RayTrace *r, const char *nm)
    : RayTraceCharacter(r, nm), aww(nullptr), rats(nullptr) {   // 0x4cc5c0, 0x425d10, 0x420210 x2
    hit = 0;                                         // +0x4c
    struct V { const char *who, *aww, *rats; };
    static const V vs[5] = {{"fox", "BACK_VO_FOXaww", "BACK_VO_FOXrats"}, {"pet", "BACK_VO_PETaww", "BACK_VO_PETrats"},
                            {"skn", "BACK_VO_SKNaww", "BACK_VO_SKNrats"}, {"rab", "BACK_VO_RABaww", "BACK_VO_RABrats"},
                            {"rac", "BACK_VO_RACaww", "BACK_VO_RACrats"}};
    for (const V &v : vs) {
        if (strcmp(v.who, nm) != 0) continue;        // 0x423c60
        aww.setName(v.aww);                          // 0x424400
        rats.setName(v.rats);
        break;
    }
}

BackwardsGoCharacter::~BackwardsGoCharacter() {}     // 0x46fc60

// v1 0x46fce0: its pack "<name>_backwardsgo", on the ground at the grid's start, its sounds opened.
void BackwardsGoCharacter::init() {
    flying = false;                                  // +0x714
    RayTraceCharacter::init();                       // (0x4ef760: nothing)
    char buf[40];
    snprintf(buf, sizeof buf, "%s_backwardsgo", name.c_str());   // 0x4f396f
    pack = ras_open(buf, "", &data_device());        // 0x419b90
    pack->load();                                    // 0x41aa20
    gx = gy = gz = 0;                                // +0x58..+0x60
    st = 0;                                          // +0x718
    caught = false;                                  // +0x71c
    aww.open();                                      // 0x41f9d0(0)
    rats.open();
    if (!moving().isOpen()) moving().open();         // 0x41f9c0
}

void BackwardsGoCharacter::release() {               // v2 0x46fd70
    aww.close();                                     // 0x41fc50
    rats.close();
    moving().close();
    RayTraceCharacter::release();                    // 0x4cc750
    ras_close(pack);                                 // 0x41b050, 0x419620
    pack = nullptr;
}

// 0x46fdc0: "back_ca_<name><move>_" at twice the size, at its height (up or not); the flying moves (not Tink's)
// marked.
void BackwardsGoCharacter::anim(const char *move, bool loop) {
    bool fl;
    if (name == "tnk") fl = false;                   // 0x423c60
    else fl = !strcmp(move, "down") || !strcmp(move, "fly") || !strcmp(move, "up");
    flying = fl;                                     // +0x714
    char buf[64];
    snprintf(buf, sizeof buf, "back_ca_%s%s_", name.c_str(), move);   // 0x4f396f
    Sprite3D *s = setAnim(buf, pack, 1, loop ? 1 : 0);   // 0x4cc780
    s->scale = 200.0f;                               // +0x84
    sprite();                                        // 0x4cc8b0 (unused)
    setY(gy ? fly_y : 0.0f);                         // 0x4cc910 (+0x724)
    refresh();                                       // 0x4cc9e0
}

// 0x46ff20: caught out: up (20 a step) until above 1000.
bool BackwardsGoCharacter::flyAway() {
    if (!(getY() < 1000.0f)) return true;            // 0x513db0
    if (st == 2) return true;
    if (st == 1) {
        if (gy == 0) {
            gy = 1;
            if (getY() < 200.0f && (double)getY() + 20.0f > 200.0f) anim("fly", true);   // 0x514340, 0x5150f8
        }
        setY((float)((double)getY() + 20.0f));
        if ((double)getY() > 1000.0f) {
            st = 2;
            return true;
        }
        return false;
    }
    if (st == 0) {
        if (gy == 0) anim("up", false);
        st = 1;
    }
    return false;
}

// 0x470000: 5 a step along the grid, 10 up or down; the animation for the way (on the ground), or the way down.
void BackwardsGoCharacter::begin() {
    vx = (float)((double)dx * 5.0f);                 // 0x514c84
    vy = (float)((double)dy * 10.0f);                // 0x514f48
    vz = (float)((double)dz * 5.0f);
    if (gy == 0) {
        if (vz < 0) anim("left", true);
        if (vz > 0) anim("right", true);
        if (vy < 0) anim("down", false);
        if (vy > 0) anim("up", false);
        if (vx < 0) anim("back", true);
        if (vx > 0) anim("forwrd", true);
    } else if (vy < 0) {
        anim("down", false);
    }
    dist = 0;                                        // +0x7c
    target = 100.0f;                                 // +0x80
    st = 1;                                          // +0x718
}

// v3 0x470120: choosing (a quarter of the frames): the move asked (`chance` %) or the backwards one (a way up when
// up, or down when down, turned to a step aside), the footsteps on the ground; moving: until 100 along, then its
// square and "fly" / "idle"; true if that was the move asked, else caught out ("rats") and choosing again.
bool BackwardsGoCharacter::step(int ax, int ay, int az) {
    if (st == 2) return true;
    if (st == 1) {
        setX((float)((double)getX() + vx));          // 0x4cc8c0
        setY((float)((double)getY() + vy));
        setZ((float)((double)getZ() + vz));
        double d = (double)fabsf(vz) + fabsf(vy) + fabsf(vx) + dist;
        dist = (float)d;
        if (d < target) return false;
        gx += dx;
        gy += dy;
        gz += dz;
        st = 2;
        anim(gy ? "fly" : "idle", true);
        if (ax == dx && ay == dy && az == dz) return true;
        st = 0;
        caught = true;                               // +0x71c
        rats.play();                                 // 0x41fd50
        return false;
    }
    if (st != 0) return false;
    if ((unsigned)vision_random(1000) < 750) return false;   // 0x425d40 (+0x54)
    if ((unsigned)vision_random(100) < (unsigned)chance) {
        dx = ax;
        dy = ay;
        dz = az;
    } else {
        dx = -ax;
        dy = -ay;
        dz = -az;
        int sgn = (unsigned)vision_random(1000) > 500 ? 1 : -1;
        bool cant_down = dy == -1 && gy == 0;
        if ((dy == 1 && gy == 1) || cant_down) {
            dy = 0;
            if ((unsigned)vision_random(1000) > 500) dz = sgn;
            else dx = sgn;
        }
    }
    if (dy == 0 && !flying && !moving().isPlaying()) moving().play();   // 0x41fec0
    begin();
    return false;
}

// ---------------------------------------------------------------- BackwardsGoTinkerbell
BackwardsGoTinkerbell::BackwardsGoTinkerbell(Raytrace::RayTrace *r, BackwardsGoGame *g)   // (0x470920)
    : BackwardsGoCharacter(r, "tnk"), game(g) {
    fly_y = 200.0f;                                  // +0x724
}

// v3 0x4703e0: she copies the player's move as it starts (or, after the hint, makes the move asked).
bool BackwardsGoTinkerbell::step(int ax, int ay, int az) {
    if (st == 2) {
        show = false;                                // +0x72c
        return true;
    }
    if (st == 1) return BackwardsGoCharacter::step(ax, ay, az);
    if (st != 0) return false;
    if (show) {
        dx = ax;
        dy = ay;
        dz = az;
    } else {
        if (game->move != 1) return false;           // +0x74
        dx = game->mx;                               // +0x80
        dy = game->my;                               // +0x7c
        dz = game->mz;                               // +0x78
    }
    if (dy != 0) {                                   // (begin sets both again)
        target = fly_y;
        vy = (float)((double)vy + vy);
    }
    begin();
    return false;
}

// ---------------------------------------------------------------- BackwardsGoDoppleganger
BackwardsGoDoppleganger::BackwardsGoDoppleganger(Raytrace::RayTraceCharacter *s, const char *nm)   // 0x470480
    : RayTraceCharacter(s->rt, nm), src(s) {}

void BackwardsGoDoppleganger::sync() {               // 0x470510
    Sprite3D *s = src->sprite();                     // 0x4cc8b0
    Sprite3D *m = setAnim(s->prefix.c_str(), nullptr, s->mode, s->anim->mode);   // 0x4cc780 (+0x40, +0x88, +0x104)
    m->anim->setFrame(s->anim->current());           // 0x403f00, 0x403f20
    setX(src->getX());
    setY(src->getY());
    setZ(src->getZ());
    m->scale = s->scale;                             // +0x84
    if (s->anim && s->anim->visible) m->anim->visible = true;
    else if (m->anim) m->anim->visible = false;
}

// ---------------------------------------------------------------- the game
BackwardsGoGame::BackwardsGoGame()                   // 0x4705c0
    : reminder("FLYH_LY_shiftcnt_01", "BACK_VO_PETupdown") {}   // 0x4c6140 (+0xa8)

// 0x470640: the boys and the doppleganger gone (Tink is not: the exe's), the reminder stopped.
BackwardsGoGame::~BackwardsGoGame() {
    for (BackwardsGoCharacter *&c : chars) {
        if (!c) continue;
        c->release();                                // v2
        delete c;                                    // v0(1)
        c = nullptr;
    }
    if (dopple) {
        dopple->release();
        delete dopple;
        dopple = nullptr;
    }
    reminder.stop();                                 // 0x4c6260
}

void BackwardsGoGame::clear() {                      // 0x470730
    state = 0;
    if (tink) {
        tink->release();
        delete tink;
        tink = nullptr;
    }
    for (BackwardsGoCharacter *&c : chars) {
        if (!c) continue;
        c->release();
        delete c;
        c = nullptr;
    }
    if (dopple) {
        dopple->release();
        delete dopple;
        dopple = nullptr;
    }
}

// 0x4707b0: the player at the start, the time to start a move by skill (less once the game has been won).
void BackwardsGoGame::init() {
    px = py = pz = 0;                                // +0x2c..+0x34
    state = 1;
    State &st = Game::state();
    int info = st.saves.gameInfo("Backwards Go Game");   // 0x4e9d70 (*0x543508)
    switch (st.skill) {                              // +0x68
    case 1:
        if (info == 0) timeout = 5000;
        else if (info == 1 || info == 2) timeout = 4000;
        break;
    case 2:
        if (info == 0) timeout = 4000;
        else if (info == 1 || info == 2) timeout = 3000;
        break;
    case 3:
        if (info == 0) timeout = 3000;
        else if (info == 1 || info == 2) timeout = 2000;
        break;
    default: break;
    }
}

// 0x470860: once dark: the pieces' bar hidden, the camera on the grid looking back, Tink, Peter and Foxy away;
// the first game with two boys (3), else five (4).
void BackwardsGoGame::setup() {
    StarrySkies_Scene &sc = starry_scene();
    if (sc.fader->busy()) return;                    // 0x4a95c0
    sc.iface->visible = false;                       // 0x5d6b9c +0x7c
    sc.rt.setCamera(4450.1f, 0.1f, 2050.1f, 180.0f);   // 0x42a570
    sc.tinkOff();                                    // 0x4ab660
    sc.peter.release();                              // 0x4ccd80
    if (sc.peter_pack) ras_close(sc.peter_pack);     // 0x419620
    sc.peter_pack = nullptr;
    sc.foxy.release();
    if (sc.fox_pack) ras_close(sc.fox_pack);
    sc.fox_pack = nullptr;
    int info = Game::state().saves.gameInfo("Backwards Go Game");
    if (info == 0) state = 3;
    else if (info > 0 && info <= 2) state = 4;
}

void BackwardsGoGame::addTink() {                    // 0x470920
    tink = new BackwardsGoTinkerbell(&starry_scene().rt, this);   // 0x4f34f3 (0x730), 0x46fa00
    tink->init();                                    // v1
    tink->anim("idle", true);
    tink->setX(3940.1f);                             // 0x4576419a
    tink->setZ(kGridZ);
}

// 0x4709e0: a boy at (dx, dz) from the grid's corner.
void BackwardsGoGame::addChar(int slot, const char *nm, float ddx, float ddz) {
    chars[slot] = new BackwardsGoCharacter(&starry_scene().rt, nm);   // 0x4f34f3 (0x728)
    chars[slot]->init();
    chars[slot]->anim("idle", true);
    chars[slot]->setX((float)((double)ddx + kGridX));
    chars[slot]->setZ((float)((double)ddz + kGridZ));
}

static BackwardsGoCharacter *boy(const char *nm, float x, float z) {
    BackwardsGoCharacter *c = new BackwardsGoCharacter(&starry_scene().rt, nm);   // 0x4f34f3 (0x728)
    c->init();
    c->anim("idle", true);
    c->setX(x);
    c->setZ(z);
    return c;
}

// 0x470aa0: Cubby moved to the grid (his place kept), Tink, the five boys, the raccoon's twin; the fade back.
void BackwardsGoGame::setupAll() {
    StarrySkies_Scene &sc = starry_scene();
    count = 5;                                       // +0xc
    cub_x = sc.cubby.getX();                         // 0x4cc9b0 (0x5d5eb8)
    cub_z = sc.cubby.getZ();
    sc.cubby.setX(kGridX);
    sc.cubby.setZ(kGridZ);
    addTink();                                       // (inlined)
    chars[0] = boy("fox", 3790.1f, 2300.1f);         // 0x456ce19a, 0x450fc19a
    chars[1] = boy("pet", 3790.1f, 1800.1f);         // 0x44e10334
    chars[2] = boy("skn", 3590.1f, 2200.1f);         // 0x4560619a, 0x4509819a
    chars[3] = boy("rab", 3590.1f, 1900.1f);         // 0x44ed8334
    addChar(4, "rac", 450.0f, 60.0f);
    dopple = new BackwardsGoDoppleganger(chars[4], "rac_doppleganger");   // 0x4f34f3 (0x54)
    dopple->init();                                  // v1
    dopple->sync();
    dopple->setZ((float)((double)chars[4]->getZ() - 120.0f));   // 0x515128
    state = 5;
    sc.fader->start(true, 2000);                     // 0x4a9580
}

// 0x470dd0: once light: input back, the boys' chances by skill (more once the game has been won), the
// reminder of the keys shown.
void BackwardsGoGame::begin() {
    StarrySkies_Scene &sc = starry_scene();
    if (sc.fader->busy()) return;
    show_cursor();                                   // 0x4119d0
    sc.paused = false;                               // 0x5d2d88
    sc.fader->visible = false;                       // +0x7c
    State &st = Game::state();
    int info = st.saves.gameInfo("Backwards Go Game");
    for (int i = 0; i < count; i++) {
        BackwardsGoCharacter *c = chars[i];
        c->st = 0;
        switch (st.skill) {
        case 1:
            if (info == 0) c->chance = 80;
            else if (info == 1 || info == 2) c->chance = 85;
            break;
        case 2:
            if (info == 0) c->chance = 85;
            else if (info == 1 || info == 2) c->chance = 90;
            break;
        case 3:
            if (info == 0) c->chance = 90;
            else if (info == 1 || info == 2) c->chance = 95;
            break;
        default: break;
        }
        c->caught = false;
    }
    f84 = false;                                     // +0x84
    tink->st = 0;
    tink->caught = false;
    first = true;                                    // +0xa4
    state = 12;
    reminder.start();                                // 0x4c6220
}

// 0x470f00
bool BackwardsGoGame::hint() {
    if (state != 8) return false;
    tink->show = true;                               // +0x72c
    t0 += 3000;                                      // +0x70
    return true;
}

// 0x470f30: the game's tune; once Cubby's "start" ends: a call (the first a plain one) that keeps the player on the
// grid (5 x 5, up to one high).
void BackwardsGoGame::command() {
    starry_scene().music(0);                         // 0x478640
    if (vo().isPlaying("BACK_VO_CUBstart")) return;  // 0x4c9690
    struct C { const char *line; int x, y, z; };
    static const C cs[12] = {                        // the table 0x471174
        {"BACK_VO_CUBgleft", 0, 0, -1},  {"BACK_VO_CUBgrght", 0, 0, 1},   {"BACK_VO_CUBgback", 1, 0, 0},
        {"BACK_VO_CUBgforw", -1, 0, 0},  {"BACK_VO_CUBgup", 0, 1, 0},     {"BACK_VO_CUBgdown", 0, -1, 0},
        {"BACK_VO_CUBbgleft", 0, 0, 1},  {"BACK_VO_CUBbgrght", 0, 0, -1}, {"BACK_VO_CUBbgback", -1, 0, 0},
        {"BACK_VO_CUBbgforw", 1, 0, 0},  {"BACK_VO_CUBbgup", 0, -1, 0},   {"BACK_VO_CUBbgdown", 0, 1, 0}};
    for (;;) {
        cx = cy = cz = 0;
        int r = vision_random(first ? 6 : 12);       // 0x425d40
        if (r >= 0 && r < 12) {
            line = cs[r].line;
            cx = cs[r].x;
            cy = cs[r].y;
            cz = cs[r].z;
        }
        bool ok = true;
        if (abs(px + cx) > 2) ok = false;
        if (abs(pz + cz) > 2) ok = false;
        int y = py + cy;
        if (y < 0) ok = false;
        if (y > 1 || !ok) continue;
        break;
    }
    first = false;
    move = 0;                                        // +0x74
    mz = my = mx = 0;                                // +0x78..+0x80
    Game::state().back_stopped = false;              // +0x64c
    vo().play(line.c_str());                         // 0x4c9400
    if (getenv("TB_DEBUG_STARRY")) tb_log("backwardsgo: call %s (%d,%d,%d) frame %d", line.c_str(), cx, cy, cz, tb_input_frame());   // test aid
    state = 7;
}

// 0x4711b0: once the call is said: the arrows (up forward, down back), Shift up, Ctrl down, Enter the height asked;
// then the camera moves the step (more than 100 along).
void BackwardsGoGame::keys() {
    if (vo().isPlaying(line.c_str())) return;
    if (move == 0) {
        bool k = false;
        if (key_down(0x27)) { mz = 1; k = true; }    // VK_RIGHT
        if (key_down(0x25)) { mz = -1; k = true; }   // VK_LEFT
        if (key_down(0x26)) { mx = -1; k = true; }   // VK_UP
        if (key_down(0x28)) { mx = 1; k = true; }    // VK_DOWN
        if (key_down(0x11)) { k = true; my = -1; }   // VK_CONTROL
        if (key_down(0x10)) { k = true; my = 1; }    // VK_SHIFT
        if (getenv("TB_BACK_AUTO")) { mx = cx; my = cy; mz = cz; k = true; }   // test aid: the right move
        if (key_down(0x0d)) my = cy;                 // VK_RETURN
        else if (!k) return;
        dist = 0;                                    // +0x94
        target = 100.0f;                             // +0x98
        move = 1;
        vx = (float)((double)mx * 5.0f);             // +0x88
        vy = (float)((double)my * 10.0f);            // +0x8c
        vz = (float)((double)mz * 5.0f);             // +0x90
        return;
    }
    if (move != 1) return;
    Raytrace::RayTrace &rt = starry_scene().rt;
    float x = rt.camx, e = rt.eyef, z = rt.camz;     // 0x42b470, 0x42b480, 0x42b540
    rt.setCamera((float)((double)x + vx), (float)((double)e + vy), (float)((double)z + vz), 180.0f);   // 0x42a570
    double d = (double)fabsf(vx) + fabsf(vz) + fabsf(vy) + dist;
    dist = (float)d;
    if (d > target) move = 2;
    if (move == 2 && getenv("TB_DEBUG_STARRY")) tb_log("backwardsgo: moved %d,%d,%d", mx, my, mz);   // test aid
}

// 0x471390: the boys step (or fly off), the player's move, Tink. Too slow to start: lost. Everyone done: the
// player's move right: every boy caught out: won (Info 0: the star, 1: the cap), else the next call (the boys a
// little worse at it); wrong: lost.
void BackwardsGoGame::play() {
    State &st = Game::state();
    int done = 0;
    for (int i = 0; i < count; i++) {
        BackwardsGoCharacter *c = chars[i];
        if (c->caught) {
            if (c->flyAway()) done++;                // 0x46ff20
        } else if (c->step(cx, cy, cz)) {            // v3
            done++;
        }
    }
    keys();                                          // 0x4711b0
    tink->step(cx, cy, cz);                          // v3
    uint32_t el = t0 > frame_time() ? 0 : frame_time() - t0;   // 0x4207b0
    auto lost = [&]() {
        state = 9;
        st.last_result = 1;                          // +0x7a0
        for (BackwardsGoCharacter *c : chars)
            if (c && !c->caught) c->aww.play();      // 0x41fd50
    };
    if (move == 0 && el > timeout) {
        lost();
        return;
    }
    if (done < count || move != 2 || tink->st == -1) return;
    if (mx != cx || my != cy || mz != cz) {
        lost();
        return;
    }
    int caught = 0;
    for (int i = 0; i < count; i++)
        if (chars[i]->caught) caught++;
    if (caught >= count) {
        StarrySkies_Scene &sc = starry_scene();
        switch (st.saves.gameInfo("Backwards Go Game")) {
        case 0:
            st.last_result = 2;
            bits |= 4;                               // +0x9c
            sc.back.star->on = true;                 // 0x5d6c24 +0x17c
            break;
        case 1:
            st.last_result = 3;
            bits |= 8;
            sc.back.cap->on = true;                  // 0x5d6c20
            break;
        case 2: st.last_result = 4; break;
        default: break;
        }
        state = 9;
        return;
    }
    px += cx;
    py += cy;
    pz += cz;
    move = 0;
    for (int i = 0; i < count; i++) {
        chars[i]->chance += -3;                      // +0x720
        chars[i]->st = 0;
    }
    tink->st = 0;
    state = 6;
}

// 0x471650: the raccoon's twin kept with him; then the state's step.
void BackwardsGoGame::update() {
    StarrySkies_Scene &sc = starry_scene();
    State &st = Game::state();
    if (dopple) {
        dopple->sync();                              // 0x470510
        dopple->setZ((float)((double)chars[4]->getZ() - 120.0f));   // 0x515128
    }
    static int last_state = -1;                      // test aid
    if (getenv("TB_DEBUG_STARRY") && state != last_state) {
        tb_log("backwardsgo: state %d (t=%u) p %d,%d,%d result %d", state, frame_time(), px, py, pz, st.last_result);
        last_state = state;
    }
    switch (state) {                                 // the table 0x471940
    case 0: init(); break;
    case 1:
        if (sc.back_on) state = 2;                   // 0x5d6ba0
        break;
    case 2: setup(); break;
    case 3:                                          // the first game: Peter and Foxy
        count = 2;
        cub_x = sc.cubby.getX();
        cub_z = sc.cubby.getZ();
        sc.cubby.setX(kGridX);                       // 0x454da19a
        sc.cubby.setZ(kGridZ);                       // 0x4500219a
        addTink();                                   // 0x470920
        addChar(0, "pet", 500.0f, 200.0f);           // 0x4709e0
        addChar(1, "fox", 500.0f, -200.0f);
        state = 5;
        sc.fader->start(true, 2000);
        break;
    case 4: setupAll(); break;
    case 5: begin(); break;
    case 6: command(); break;
    case 7:
        if (!vo().isPlaying(line.c_str())) {
            t0 = frame_time();                       // +0x70
            state = 8;
        }
        break;
    case 8: play(); break;
    case 9:                                          // the end: to black
        st.back_stopped = true;                      // +0x64c
        vo().stop();                                 // 0x4c9260
        sc.fader->start(false, 2000);
        state = 10;
        break;
    case 10:                                         // dark: the area back as it was
        if (sc.fader->busy()) break;
        clear();                                     // 0x470730
        sc.music(3);
        st.setCameraToSpot(2, 1, &sc.rt);            // 0x4ec140
        sc.iface->visible = true;                    // 0x5d6b9c
        sc.fader->start(true, 2000);
        sc.tinkOn();                                 // 0x4ab5c0
        sc.cubby.setX(cub_x);                        // +0x04
        sc.cubby.setZ(cub_z);                        // +0x08
        state = 11;
        sc.reloadPeter();                            // 0x475990
        sc.reloadFoxy();                             // 0x475a10
        break;
    case 11:                                         // light: the area's tune, the end of the game's lines
        if (sc.fader->busy()) break;
        sc.fader->visible = false;
        sc.music(2);
        sc.hotsOn(true);                             // 0x478710
        state = 0;
        st.hover_up = false;                         // +0x75
        sc.forceNone();                              // 0x4abf30
        show_cursor();
        if (st.last_game != 5) sc.back_end.start();  // 0x4c19e0 (0x5d5620)
        sc.back_on = false;
        st.back_ended = true;                        // +0x7a8
        break;
    case 12:
        if (!reminder.active()) {                    // +0x14
            vo().play("BACK_VO_CUBstart");
            state = 6;
        }
        break;
    default: break;
    }
}

void BackwardsGoGame::start() {                      // 0x471980
    bits |= 1;                                       // +0x9c
    state = 0;
}

// 0x4719a0: Escape: quit (5), to black.
void BackwardsGoGame::skip() {
    State &st = Game::state();
    st.last_game = 5;                                // +0x79c
    st.back_stopped = true;
    vo().stop();
    starry_scene().fader->start(false, 2000);        // 0x4a9580
    state = 10;
}

}  // namespace Game
