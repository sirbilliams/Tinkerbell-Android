// The Mud Pie game, reconstructed from "Tinker Bell.exe" (see mudpie.h).
#include "mudpie.h"
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
#include <stdexcept>

namespace Game {

using namespace Vision;


static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0
static int g_flight = 0;                             // 0x535c10: a pie's flight time (ms)
static void dbg(const char *m) { if (getenv("TB_DEBUG_MUDPIE")) tb_log("mudpie: %s (t=%u)", m, frame_time()); }

MudPie_Scene &mudpie_scene() {                       // the static at 0x5a96a0
    static MudPie_Scene s;
    return s;
}

// ---------------------------------------------------------------- MudGround
MudGround::MudGround() : ScrollGround("MUDP_BG_ground1_01", "MUDP_BG_ground2_01") {}

// v18 0x435c70: the throwers move with the ground (one that has gone off the bottom comes back at the next spot);
// the pies in the air go along their lines, gone off the top or bottom.
void MudGround::update() {
    ScrollGround::update();
    MudPie_Scene &sc = mudpie_scene();
    float fdy = (float)dy;
    float fdx = (float)dx;
    for (size_t i = 0; i < sc.throwers.size(); i++) {
        Thrower *t = sc.throwers[i];
        int ny = ftol((double)t->y + fdy);
        if ((double)(t->stand.h / 2) + 240.0 < (double)ny) {   // +0x138, 0x513f1c
            int sx, sy;
            mudpie_scene().nextSpot(&sx, &sy);       // on the static
            t->x = sx;
            t->y = sy;
            t->visible = true;
        } else {
            t->x = ftol((double)t->x + fdx);
            t->y = ny;
        }
    }
    for (size_t i = 0; i < sc.flyers.size(); i++) {
        Flyer *f = sc.flyers[i];
        if (!f->visible) continue;
        double t = (double)(uint64_t)(frame_time() - f->t0);
        f->x = ftol(t * f->vx / (double)g_flight + f->x0);
        int y = ftol(t * f->vy / (double)g_flight + f->y0);
        f->y = y;
        if ((double)y > 240.0 || (double)y < -240.0) f->visible = false;   // 0x513f1c / 0x513f18
    }
}

// ---------------------------------------------------------------- MudCursor
MudCursor::MudCursor() : Graphic(GRAPHIC_ARGS), notactive("MUDP_CU_notactive_"), active("MUDP_CU_active_") {}

MudCursor::~MudCursor() {                            // 0x4397b0
    cur = nullptr;
    hand = nullptr;
}

void MudCursor::draw() {                             // v14 0x439850
    cur->x = x;
    cur->y = y;
    cur->draw();
}

// v15 0x439880: both looping; the game's cursors kept (the hand one started).
void MudCursor::load() {
    notactive.load();
    notactive.play(0, 1);
    active.load();
    active.play(0, 1);
    normal = Game::state().cursor;                   // state +0x5c
    hand = Game::state().cursor_active;              // state +0x60
    static_cast<Animation *>(hand)->play(0, 1);      // 0x403be0
    cur = &notactive;
}

void MudCursor::unload() {                           // v16 0x4398f0
    Graphic::unload();
    notactive.unload();
    active.unload();
    hand = nullptr;
    cur = nullptr;
}

// ---------------------------------------------------------------- MudScore
static const char *const kIcons[4] = {"MUDP_IF_MUDsmall_", "MUDP_IF_DIAsmall_", "MUDP_IF_EMEsmall_",
                                      "MUDP_IF_RUBsmall_"};   // 0x535bf0
static const char *const kThrown[4] = {"MUDP_GA_MUDthrown_", "MUDP_GA_DIAthrown_", "MUDP_GA_EMEthrown_",
                                       "MUDP_GA_RUBthrown_"};   // 0x535c00

// 0x438a80: the bars; the player's ten slots "MUDP_IF_target_01".."10", the skunk's "11".."20" (loaded).
MudScore::MudScore() : Graphic(GRAPHIC_ARGS), bars("MUDP_IF_interface_") {
    bars.load();                                     // v15
    char n[32];
    for (int i = 1; i <= 10; i++) {
        snprintf(n, sizeof n, "MUDP_IF_target_%2.2d", i);   // 0x423ae0
        Image *im = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);
        im->load();
        pslots.push_back(im);
    }
    for (int i = 11; i <= 20; i++) {
        snprintf(n, sizeof n, "MUDP_IF_target_%2.2d", i);
        Image *im = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);
        im->load();
        sslots.push_back(im);
    }
}

MudScore::~MudScore() {                              // 0x438e30
    reset();
    bars.unload();
    for (Image *im : pslots) { im->unload(); delete im; }
    for (Image *im : sslots) { im->unload(); delete im; }
    pslots.clear();
    sslots.clear();
}

void MudScore::reset() {                             // 0x439170
    pscore = 0;
    sscore = 0;
    for (Animation *a : player) { a->unload(); delete a; }
    player.clear();
    for (Animation *a : skunk) { a->unload(); delete a; }
    skunk.clear();
}

void MudScore::draw() {                              // v14 0x4392d0
    bars.draw();
    for (Animation *a : player) a->draw();
    for (Animation *a : skunk) a->draw();
}

// 0x439370: the caught pie's icon in the player's next slot; a gem scores.
void MudScore::addPlayer(int type) {
    // port: the exe's slot lookup throws past the tenth (an uncaught exception); a catch then is ignored instead
    if (player.size() >= pslots.size()) return;
    Animation *a = Gorilla::gnew<Animation>(kIcons[type]);
    a->load();
    Image *slot = pslots[player.size()];
    a->x = slot->ox;                                 // +0x30
    a->y = slot->oy;                                 // +0x34
    a->play(0, 0);
    player.push_back(a);
    if (type != 0) pscore++;
}

void MudScore::addSkunk(int type) {                  // 0x439520
    if (skunk.size() >= sslots.size()) return;       // port: as addPlayer
    Animation *a = Gorilla::gnew<Animation>(kIcons[type]);
    a->load();
    Image *slot = sslots[skunk.size()];
    a->x = slot->ox;
    a->y = slot->oy;
    a->play(0, 0);
    skunk.push_back(a);
    if (type != 0) sscore++;
}

// ---------------------------------------------------------------- Thrower
Thrower::Thrower(const char *s, const char *t, int ddx, int ddy, int rel)   // 0x439930
    : Graphic(GRAPHIC_ARGS), stand(s), thrw(t), dx(ddx), dy(ddy), release(rel) {
    setZ(10);
    stand.load();
    stand.play(0, 1);
    stand.visible = true;                            // +0x14c
    thrw.load();
    thrw.visible = false;                            // +0x264
    visible = false;
}

Thrower::~Thrower() {                                // 0x439a70
    stand.unload();
    thrw.unload();
}

// 0x439b30: the throw from its first frame, the throw sound, a taunt when nobody is talking.
void Thrower::throwAt(Flyer *f) {
    pie = f;
    stand.visible = false;
    thrw.visible = true;
    mudpie_scene().sounds[0].play();                 // 0x5a9800 (the static's)
    thrw.setFrame(0);
    thrw.play(0, 0);
    if (!Game::state().vo_busy()) taunts.playNext(); // +0x560, 0x4cc0e0
}

// v14 0x439b90: standing or throwing; at the release frame the pie flies from the hand. (The throw's last frame
// is count() - 1, so the "back to standing" test never passes: a thrower keeps his last throw frame.)
void Thrower::draw() {
    if (stand.visible) {
        stand.x = x;
        stand.y = y;
        stand.draw();
    } else if (thrw.visible) {
        thrw.x = x;
        thrw.y = y;
        thrw.draw();
        if (pie && thrw.current() >= release) {
            pie->launch((float)(x + dx), (float)(y + dy));   // 0x439ec0
            pie = nullptr;
        }
    } else {
        return;
    }
    if (thrw.visible && thrw.current() == thrw.count()) {
        thrw.visible = false;
        stand.visible = true;
        stand.play(0, 1);
    }
}

// ---------------------------------------------------------------- Flyer
Flyer::Flyer(int t)                                  // 0x439ca0
    : Graphic(GRAPHIC_ARGS), dissolve("MUDP_GA_MUDdisolv_"), thrown(kThrown[t]), type(t) {
    setZ(6);
    dissolve.load();
    dissolve.setHit(-1, true);                       // v6
    thrown.load();
    visible = false;
}

Flyer::~Flyer() {                                    // 0x439dc0
    dissolve.unload();
    thrown.unload();
}

void Flyer::draw() {                                 // v14 0x439e80
    dissolve.x = thrown.x = x;
    dissolve.y = thrown.y = y;
    thrown.draw();
    dissolve.draw();
}

bool Flyer::hitAt(int px, int py) {                  // 0x43a000
    return dissolve.hit(px, py) || thrown.hit(px, py);
}

// 0x439ec0: to a random point up to 320 across (towards the middle) at the far edge, over the flight time; both
// animations at a rate that fits the flight (both by the dissolve's frame count).
void Flyer::launch(float fx, float fy) {
    x0 = fx;
    y0 = fy;
    double r;
    if (fx < 0.0f) r = (double)vision_random(320); // 0x140
    else r = -(double)vision_random(320);
    double ty = y0 < 0.0f ? 240.0 : -240.0;
    vx = (float)(r - x0);
    vy = (float)(ty - y0);
    x = ftol(x0);
    y = ftol(y0);
    dissolve.fps = (int)((uint32_t)(dissolve.count() * 1000) / (uint32_t)g_flight);   // +0x1a8
    thrown.fps = (int)((uint32_t)(dissolve.count() * 1000) / (uint32_t)g_flight);     // +0x2c0 (the dissolve's count)
    dissolve.play(0, 0);
    thrown.play(0, 0);
    visible = true;
    t0 = frame_time();
}

// ---------------------------------------------------------------- Flier
static Animation *flier_anim(const char *fmt, const char *who) {
    char n[0x28];
    snprintf(n, sizeof n, fmt, who);                 // 0x4f396f
    return Gorilla::gnew<Animation>(n);              // 0x4243c0(0x118), 0x404c20
}

Flier::Flier(const char *who) : Graphic(GRAPHIC_ARGS) {   // 0x43a2d0
    idle = flier_anim("MUDP_CA_%sFLYidle_", who);
    fly_no = flier_anim("MUDP_CA_%sFLYno_", who);
    fly_so = flier_anim("MUDP_CA_%sFLYso_", who);
    fly_ea = flier_anim("MUDP_CA_%sFLYea_", who);
    fly_we = flier_anim("MUDP_CA_%sFLYwe_", who);
    str_no = flier_anim("MUDP_CA_%sSTRno_", who);
    str_so = flier_anim("MUDP_CA_%sSTRso_", who);
    str_ea = flier_anim("MUDP_CA_%sSTRea_", who);
    str_we = flier_anim("MUDP_CA_%sSTRwe_", who);
}

Flier::~Flier() {                                    // 0x43a5e0
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we, str_no, str_so, str_ea, str_we}) delete a;
}

// v15 0x43a6e0: at the middle, landed; the flying ones ping-pong.
void Flier::load() {
    x = 0;
    y = 0;
    tx = ty = -999999;                               // 0xfff0bdc1
    phase = 0;
    dir = 0;
    anim_t = frame_time();
    step_t = frame_time();
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we}) {
        a->load();
        a->mode = 2;                                 // +0x104
        a->setHit(1, false);                         // v6
    }
    for (Animation *a : {str_no, str_so, str_ea, str_we}) {
        a->load();
        a->setHit(1, false);
    }
}

void Flier::unload() {                               // v16 0x43a850
    Graphic::unload();
    for (Animation *a : {idle, fly_no, fly_so, fly_ea, fly_we, str_no, str_so, str_ea, str_we}) a->unload();
}

void Flier::reset() {                                // the AI's (inlined): 0x4362f0 / 0x43a050
    tx = ty = -999999;
    phase = 0;
    dir = 0;
    anim_t = frame_time();
    step_t = frame_time();
}

Animation *Flier::strAnim() {                        // 0x43ab70
    switch (dir) {
    case 1: return str_no;
    case 2: return str_so;
    case 3: return str_ea;
    case 4: return str_we;
    }
    return nullptr;
}

Animation *Flier::flyAnim() {                        // 0x43abc0
    switch (dir) {
    case 1: return fly_no;
    case 2: return fly_so;
    case 3: return fly_ea;
    case 4: return fly_we;
    }
    return nullptr;
}

Animation *Flier::current() {
    switch (phase) {
    case 0: return idle;
    case 1: return strAnim();
    case 2: return flyAnim();
    case 3: return strAnim();
    }
    return nullptr;
}

// 0x43a8c0: off towards (x, y), facing the way it is furthest.
void Flier::moveTo(int nx, int ny) {
    step_t = frame_time();
    ty = ny;
    tx = nx;
    int ddx = nx - x, ddy = ny - y;
    phase = 1;
    if (abs(ddx) > abs(ddy)) dir = ddx < 0 ? 4 : 3;
    else dir = ddy < 0 ? 1 : 2;
    Animation *a = strAnim();
    a->setFrame(0);                                  // port: the exe passes a null this when dir is 0 (never)
}

// 0x43a9d0: every 30 ms a tenth of the way (half that while taking off, at least a pixel); there, it lands (the
// take-off backwards); true once landed.
bool Flier::step() {
    uint32_t now = frame_time();
    if (now - step_t < 30) return false;             // 0x1e
    step_t = now;
    if (phase == 3) {
        if (strAnim()->current() != 0) return false;
        phase = 0;
        idle->setFrame(0);
        return true;
    }
    int ex = tx - x, ey = ty - y;
    int mx = ex / 10, my = ey / 10;
    if (phase == 1) {
        mx /= 2;
        my /= 2;
    }
    if (mx == 0 && ex != 0) mx = ex / abs(ex);
    if (my == 0 && ey != 0) my = ey / abs(ey);
    if (mx == 0 && my == 0) {
        phase = 3;
        Animation *a = strAnim();
        a->setFrame(a->count() - 1);
    }
    x += mx;
    y += my;
    return false;
}

// 0x43ac10: the animation by the phase; the take-off ends in flight; landing runs the take-off backwards.
void Flier::animate() {
    if (phase == 0) {
        idle->step();
        return;
    }
    if (phase == 1) {
        Animation *a = strAnim();
        if (a->current() >= a->count() - 1) {
            phase = 2;
            flyAnim()->setFrame(0);
        } else {
            a->step();
        }
        return;
    }
    if (phase == 2) {
        flyAnim()->step();
        return;
    }
    if (phase == 3) {
        Animation *a = strAnim();
        if (a->current() != 0) a->setFrame(a->current() - 1);
    }
}

void Flier::draw() {                                 // v14 0x43adf0
    if (!visible) return;
    Animation *a = current();
    a->x = x;
    a->y = y;
    a->draw();                                       // v14
    uint32_t now = frame_time();
    if (now - anim_t >= 30) {
        animate();
        anim_t = now;
    }
}

// ---------------------------------------------------------------- MudPie_Scene
MudPie_Scene::MudPie_Scene()                         // 0x436fb0
    : Game2D_Scene("mudpie"),                        // 0x535d34
      music(nullptr),                                // 0x420210(0)
      guide("MUDP_VO_PETgreet1", "MUDP_VO_PETgreet2") {   // 0x4ce910
    ai_.mode = 1;
    ai_.chance = 30.0f;                              // 0x41f00000
    last_click = frame_time();                       // +0x1f30
    keyboard.setClient(this);                        // 0x41f290
    mouse.setClient(this);
}

MudPie_Scene::~MudPie_Scene() {}                     // 0x437390

// v18 0x437680: the sounds named, Peter's prompt, the end lines.
void MudPie_Scene::load2() {
    music.setName("MUDP_MU_music");                  // 0x424400
    music.mode = 2;                                  // +0x2290 (+0x338)
    music.type = 6;                                  // +0x2298 (+0x340): loops
    static const char *const names[7] = {"mudp_fx_throw", "mudp_fx_squish", "mudp_fx_gemcatch",
                                         "mudp_fx_success_01", "mudp_fx_dud_01", "mudp_fx_cuclicky",
                                         "mudp_fx_cuclickn"};   // 0x535bd4
    for (int i = 0; i < 7; i++) sounds[i].setName(names[i]);
    inactivity.add("MUDP_VO_PETprompt", false);      // 0x4c46e0
    endvo.setWins("MUDP_VO_PETtbwins_01", "MUDP_VO_PETtbwins_02", "MUDP_VO_PETtbwins_03");
    endvo.setLoses("MUDP_VO_PETtblose_01", "MUDP_VO_PETtblose_02", "MUDP_VO_PETtblose_03");
    endvo.setTie("MUDP_VO_PETtie_01");
    endvo.setWinChallenge("MUDP_VO_PETwinsch_01", "MUDP_VO_PETwinsch_02", "MUDP_VO_PETwinsch_03");
    endvo.setAward("MUDP_VO_PETaward");
    endvo.setLoseChallenge("MUDP_VO_PETlosech_01", "MUDP_VO_PETlosech_02", "MUDP_VO_PETlosech_03");
    endvo.setTie2("MUDP_VO_PETtie_02");
}

// v19 0x437a90: the skunk's badge; input; the "mud_pie" pack; Peter (unless the game is being replayed); the
// cursor; the screen up; the score bars.
void MudPie_Scene::enter() {
    Game2D_Scene::enter();                           // 0x4acfd0
    badge->detach();                                 // 0x412200
    delete badge;                                    // v0(1)
    badge = Gorilla::gnew<PlayerBadge>("GLOB_IF_SNKheadsm_01");   // 0x4ad330
    badge->setZ(-1000);
    screen.layer(0)->add(badge);
    mouse.activate();                                // v6
    keyboard.activate();
    pack = ras_open("mud_pie", nullptr, &data_device());   // 0x419b90
    pack->load();                                    // 0x41aa20
    ras_set_current(pack);                           // 0x41a920
    if (previous != this) {                          // +0xc4
        guide.load(false);                           // 0x4ceb60
        guide.setZ(-950);                            // 0xfffffc4a
        screen.layer(0)->add(&guide);
        guide.Task::activate();                      // +0x2338 v6
    }
    cursor.load();                                   // v15
    cursor.visible = true;                           // +0x1944
    cursor.cur = &cursor.notactive;                  // +0x1960
    set_cursor(&cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
    // port: the cursor hot spot (0x5a6930/34) = 0 (the Image draws from its corner already)
    hide_cursor();                                   // 0x4119e0
    screen.activate();                               // v6
    screen.loadAll();                                // 0x41ec40
    score = Gorilla::gnew<MudScore>();               // 0x438a80
    score->setZ(-1000);
    screen.layer(0)->add(score);
    ai = &ai_;                                       // +0x1e38
    reset();                                         // 0x421110
    activate();                                      // v6
    playing = false;
    input_on = false;
}

// 0x437770: the throwers' spots: "MUDP_TR_target_01".."07" where they are, "08".."12" a screen higher.
void MudPie_Scene::targets() {
    Image im("", 0, 0, 0, 0, 0, 0, true);            // 0x4179e0(0x5464a0)
    char n[32];
    for (int i = 1; i <= 7; i++) {
        snprintf(n, sizeof n, "MUDP_TR_target_%2.2d", i);   // 0x423ae0
        im.setName(n);
        im.load();
        int *p = new int[2];                         // 0x4f34f3(8)
        p[0] = im.x;
        p[1] = im.y;
        spots.push_back(p);
        im.unload();
    }
    for (int i = 8; i <= 12; i++) {
        snprintf(n, sizeof n, "MUDP_TR_target_%2.2d", i);
        im.setName(n);
        im.load();
        int *p = new int[2];
        p[0] = im.x;
        p[1] = im.y - 960;                           // 0x3c0
        spots.push_back(p);
        im.unload();
    }
}

// 0x437cb0: the ground; the spots; the four throwers (each with his taunts) at their first spots; the skunk; the
// cannon; the end scoreboard (hidden); the sounds open and the music on.
void MudPie_Scene::setup() {
    ground.load();                                   // v15
    ground.setZ(12);
    screen.layer(0)->add(&ground);
    targets();                                       // 0x437770
    struct Boy { const char *stand, *thrw, *t1, *t2, *t3; int dx, dy, rel; };
    static const Boy boys[4] = {
        {"MUDP_GA_RABstand_", "MUDP_GA_RABthrw_", "MUDP_VO_RABtaunt1", "MUDP_VO_RABtaunt2", "MUDP_VO_RABtaunt3", 8, 25, 7},
        {"MUDP_GA_RACstand_", "MUDP_GA_RACthrw_", "MUDP_VO_RACtaunt1", "MUDP_VO_RACtaunt2", "MUDP_VO_RACtaunt3", -8, 4, 1},
        {"MUDP_GA_CUBstand_", "MUDP_GA_CUBthrw_", "MUDP_VO_CUBtaunt1", "MUDP_VO_CUBtaunt2", "MUDP_VO_CUBtaunt3", -10, 0, 1},
        {"MUDP_GA_FOXstand_", "MUDP_GA_FOXthrw_", "MUDP_VO_FOXtaunt1", "MUDP_VO_FOXtaunt2", "MUDP_VO_FOXtaunt3", -1, 15, 1},
    };
    for (const Boy &b : boys) {
        Thrower *t = Gorilla::gnew<Thrower>(b.stand, b.thrw, b.dx, b.dy, b.rel);   // 0x4243c0(0x310), 0x439930
        throwers.push_back(t);
        t->taunts.add(b.t1);                         // 0x4cc0d0
        t->taunts.add(b.t2);
        t->taunts.add(b.t3);
    }
    for (size_t i = 0; i < throwers.size(); i++) {
        int sx, sy;
        mudpie_scene().nextSpot(&sx, &sy);           // on the static
        throwers.at(i)->x = sx;
        throwers.at(i)->y = sy;
        throwers.at(i)->visible = true;
        screen.layer(0)->add(throwers.at(i));
    }
    ai->skunk.load();                                // v15
    screen.layer(0)->add(&ai->skunk);
    cannon = &Game::cannon();                        // state +0x7c0
    screen.layer(0)->add(cannon);
    cannon->visible = true;
    board = Gorilla::gnew<Scoreboard>("MUDP_IF_scorescr_01");   // 0x4243c0(0x168), 0x4ada80
    board->setZ(-1000);
    board->visible = false;
    screen.layer(0)->add(board);
    next_throw = frame_time();                       // +0x1f2c
    music.open();                                    // 0x41f9d0(0)
    for (int i = 0; i < 7; i++) sounds[i].open();
    music.play();                                    // 0x41fd50
}

// 0x4362f0: by the skill: the cannon's seconds, the pies' speed, how many are gems; the score cleared; ten new
// pies; the fuse lit; the skunk reset.
void MudPie_Scene::start() {
    int skill;
    if (!Game::state().saves.getSkill(&skill)) skill = 1;   // 0x4e97a0
    switch (skill) {
    case 1: secs = 90; speed = 1.6f; gems = 7; break;
    case 2: secs = 90; speed = 1.8f; gems = 6; break;
    case 3: secs = 60; speed = 2.0f; gems = 5; break;
    }
    g_flight = ftol((double)5000.0f / (double)speed);   // 0x513f20
    spot = 0;                                        // +0x1f54
    score->reset();                                  // 0x439170
    board->visible = false;
    for (size_t i = 0; i < flyers.size(); i++) {
        flyers.at(i)->Graphic::unload();             // v16 (the base's)
        delete flyers.at(i);                         // v0(1)
    }
    flyers.clear();
    int i = 0;
    for (; i < gems; i++) flyers.push_back(Gorilla::gnew<Flyer>(i % 3 + 1));   // 0x4243c0(0x2e8), 0x439ca0
    if (i < 10)
        for (int n = 10 - i; n > 0; n--) flyers.push_back(Gorilla::gnew<Flyer>(0));
    for (size_t k = 0; k < flyers.size(); k++) screen.layer(0)->add(flyers.at(k));
    cannon->start(secs);                             // 0x4e3a50
    escaped = false;
    ai->skunk.reset();
    ai->mode = 1;
}

// 0x436610: how it ended (state +0x7a0): won (by the game's progress 2/3/4), tie 5, lost 1; nothing when skipped.
void MudPie_Scene::result() {
    if (escaped) return;
    State &st = Game::state();
    if (score->pscore > score->sscore) {
        int info = st.saves.gameInfo("Mud Pie Game");   // 0x4e9d70 (*0x543520)
        if (info == 0) st.last_result = 2;
        else if (info == 1) st.last_result = 3;
        else if (info == 2) st.last_result = 4;
    } else if (score->pscore == score->sscore) {
        st.last_result = 5;
    } else {
        st.last_result = 1;
    }
}

// 0x438490: every 2-3 s a thrower on screen throws a pie that is not in the air (when at least five are free).
void MudPie_Scene::spawn() {
    std::vector<Thrower *> ts;
    std::vector<Flyer *> fs;
    if (frame_time() < next_throw) return;
    next_throw += vision_random(1000) + 2000;        // 0x3e8, 0x7d0
    for (size_t i = 0; i < throwers.size(); i++) {
        int ty = throwers.at(i)->y;
        if (ty > 240 || ty < -240) continue;
        ts.push_back(throwers.at(i));
    }
    for (size_t i = 0; i < flyers.size(); i++)
        if (!flyers.at(i)->visible) fs.push_back(flyers.at(i));
    if (ts.empty()) return;
    if (fs.size() < 5) return;
    int a = vision_random((int)ts.size());
    int b = vision_random((int)fs.size());
    ts.at(a)->throwAt(fs.at(b));                     // 0x439b30
}

// 0x4387b0: the nearest pie in the air (within ~316).
Flyer *MudPie_Scene::nearest(int px, int py) {
    Flyer *best = nullptr;
    float bd = 100000.0f;                            // 0x47c35000
    for (size_t i = 0; i < flyers.size(); i++) {
        Flyer *f = flyers.at(i);
        if (!f->visible) continue;
        float fx = (float)(f->x - px);
        double dy = (double)(f->y - py);
        float d = (float)(dy * dy + (double)fx * fx);
        if (d < bd) {
            bd = d;
            best = f;
        }
    }
    return best;
}

// 0x4388d0: the next spot from the list (with the ground's scroll), until one is above the screen or on a thrower.
// (A thrower's own box is empty, so in practice: until one is above the screen.)
void MudPie_Scene::nextSpot(int *px, int *py) {
    for (;;) {
        if ((size_t)spot >= spots.size()) spot = 0;
        *px = spots.at(spot)[0];
        *py = spots.at(spot)[1];
        spot++;
        *px += ground.x;                             // +0x1c00
        *py += ground.row1 - 240;                    // +0x1ca8
        if (*py <= -240) return;
        if (mudpie_scene().overlap(*px, *py)) return;   // on the static
    }
}

bool MudPie_Scene::overlap(int px, int py) {         // 0x4389d0
    bool hit = false;
    for (size_t i = 0; i < throwers.size() && !hit; i++) {
        Thrower *t = throwers.at(i);
        if (t->visible && t->hit(px, py)) hit = true;   // v11 (the base Graphic's)
    }
    return hit;
}

// 0x43a050 (mode 0): the skunk catches any pie it touches (and chooses again), else flies on until it lands.
void MudPie_Scene::aiCatch() {
    Flier &e = ai->skunk;
    int ex = e.x, ey = e.y;
    MudPie_Scene &sc = mudpie_scene();
    for (size_t i = 0; i < sc.flyers.size(); i++) {
        Flyer *f = sc.flyers.at(i);
        if (!f->visible) continue;
        if (!f->hitAt(ex, ey)) continue;
        sc.score->addSkunk(f->type);                 // 0x5ab240 (the static's score)
        f->visible = false;
        e.reset();
        ai->mode = 1;
        return;
    }
    if (e.step()) ai->mode = 1;                      // 0x43a9d0
}

// 0x43a160 (mode 1): by chance (half again when the player leads) after the nearest pie, half way to where it
// lands; otherwise (or when none is up) a random hop within the play area.
void MudPie_Scene::aiChoose() {
    Flier &e = ai->skunk;
    int ex = e.x, ey = e.y;
    float ch = ai->chance;
    MudScore *sc = mudpie_scene().score;
    if (sc->pscore > sc->sscore) ch = (float)(ch * 1.5f);   // 0x513f24
    int nx, ny;
    bool chase = (double)vision_random(100) < (double)ch;   // 0x64
    Flyer *f = chase ? mudpie_scene().nearest(ex, ey) : nullptr;
    if (f) {
        int fy = f->y, fx = f->x;
        ny = (ftol((double)f->y0 + f->vy) - fy) / 2 + fy;
        nx = (ftol((double)f->x0 + f->vx) - fx) / 2 + fx;
    } else {
        nx = vision_random(200) + ex - 100;          // 0xc8
        if (nx < -250) nx += 100;
        if (nx > 250) nx -= 100;
        ny = vision_random(200) + ey - 100;
        if (ny < -100) ny += 100;
        if (ny > 200) ny -= 100;
    }
    e.moveTo(nx, ny);                                // 0x43a8c0
    ai->mode = 0;
}

// v10 0x4366c0
void MudPie_Scene::run() {
    wait(1);
    input_on = false;
    clouds->reset();                                 // 0x4acf00
    dbg("run");
    while (guide.Task::active()) wait(0);            // +0x2338 v5
    dbg("guide done");
    setup();                                         // 0x437cb0
    start();                                         // 0x4362f0
    input_on = true;
    cursor.cur = &cursor.notactive;
    show_cursor();                                   // 0x4119d0
    dbg("setup done");
    while (clouds->fall()) wait(30);                 // 0x4acee0
    dbg("play");
    next_throw = frame_time();
    inactivity.start();                              // v12
    playing = true;
    while (score->player.size() < 10 && score->skunk.size() < 10 && cannon->running() && !escaped) {
        if (ai->mode == 0) aiCatch();
        else if (ai->mode == 1) aiChoose();
        spawn();                                     // 0x438490
        wait(30);
    }
    dbg("play over");
    input_on = false;
    cannon->stop();                                  // 0x4e3bc0
    hide_cursor();
    if (escaped) {
        fader->visible = true;
        fader->start(false, 2000);                   // 0x4a9580
        while (fader->busy()) wait(0);               // 0x4a95c0
        change_scene("lost");
        return;
    }
    clouds->cover();                                 // 0x4acf20
    while (clouds->rise()) wait(30);                 // 0x4acec0
    inactivity.stop();                               // v13
    board->s2 = score->sscore;
    board->s1 = score->pscore;
    board->visible = true;
    dbg("board");
    wait(3000);                                      // 0xbb8
    result();                                        // 0x436610
    endvo.playEnd();                                 // 0x4c3a60
    dbg("endvo");
    while (endvo.playing()) wait(0);                 // 0x4c3ae0
    dbg("endvo done");
    if (score->pscore > score->sscore) {
        change_scene("lost");
    } else {
        dialog = Gorilla::gnew<ContinueDialog>("mudp_if_contgame_01", "mudp_if_signglow_01", "mudp_if_xglow_01");
        dialog->setAsk("mudp_vo_petbegcon_01");      // 0x4e5720
        dialog->setYes("mudp_vo_petbegyes_01");      // 0x4e5860
        dialog->setNo("mudp_vo_petbegno_01");        // 0x4e59a0
        dialog->setIdle("mudp_vo_petbegidl_01");     // 0x4e5ae0
        keyboard.deactivate();                       // v7
        show_cursor();
        call(dialog);                                // 0x420f60
        wait(1);
    }
    finish();                                        // 0x420fe0
}

// v9 0x436240
void MudPie_Scene::notify2(Service *s) {
    if (current_scene() != this) return;             // 0x5a7394
    if (s == &mouse) {
        onMouse();
        return;
    }
    if (s == &keyboard) {
        onKey();
        return;
    }
    if (s == dialog) {
        if (dialog->yes) {                           // +0x6f1: again
            if (board) board->visible = false;
            again();                                 // 0x41dde0
        } else if (dialog->no) {                     // +0x6f0
            change_scene("lost");
        }
        dialog = nullptr;                            // (the finished task is freed with the finished list)
        mouse.activate();                            // v6
        keyboard.activate();
    }
}

// 0x435e90: the cursor by what is under it; a click on the map goes there; a click on a pie in the air catches it
// (half a second apart; the skunk in the way blocks it).
void MudPie_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    cursor.cur = input_on ? (Graphic *)&cursor.notactive : cursor.normal;
    if (!playing) return;
    if (map->hit(g_mouse.x, g_mouse.y)) {            // v11
        cursor.cur = cursor.hand;
        if (mouse_down()) {
            if (map->click(g_mouse.x, g_mouse.y, true) == 2) change_scene("map");   // 0x4efc90
            return;
        }
    }
    uint32_t now = frame_time();
    Flier &e = ai->skunk;
    Animation *a = e.current();
    a->x = e.x;
    a->y = e.y;
    if (a->hit(g_mouse.x, g_mouse.y)) return;        // v11
    if (now - last_click <= 500) {                   // 0x1f4
        if (mouse_down()) sounds[6].play();          // cuclickn
        return;
    }
    if (mouse_down()) sounds[5].play();              // cuclicky
    Flyer *got = nullptr;
    for (size_t i = 0; i < flyers.size(); i++) {
        if (flyers.at(i)->visible && flyers.at(i)->hitAt(g_mouse.x, g_mouse.y)) {
            got = flyers.at(i);
            break;
        }
    }
    if (!got) return;
    if (!mouse_down()) {
        cursor.cur = &cursor.active;
        return;
    }
    if (got->type == 0) {
        sounds[1].play();                            // squish
        sounds[4].play();                            // dud
    } else {
        sounds[2].play();                            // gemcatch
        sounds[3].play();                            // success
    }
    score->addPlayer(got->type);                     // 0x439370
    got->visible = false;
    last_click = now;
}

// 0x436110: before play, the skip key cuts Peter short; in play, up/down set the speed, left/right slide; the skip
// key leaves.
void MudPie_Scene::onKey() {
    State &st = Game::state();
    if (!playing) {
        if ((int)g_key.vk == st.skip_key) guide.stop();   // 0x5a6d9c == state +0x74, 0x4cf060
        return;
    }
    ground.setMult(2);
    if (g_key.down) {                                // 0x5a6dd8
        if ((int)g_key.vk == st.skip_key) {
            st.last_game = 5;                        // +0x79c
            escaped = true;
        } else if (st.key((int)g_key.vk)) {          // 0x4baef0
            return;
        }
    }
    if (key_down(0x26)) ground.setMult(4);           // VK_UP
    if (key_down(0x28)) ground.setMult(1);           // VK_DOWN
    if (key_down(0x27)) ground.setDir(2);            // VK_RIGHT
    if (key_down(0x25)) ground.setDir(1);            // VK_LEFT
    if (!key_down(0x25) && !key_down(0x27)) ground.setDir(0);
    if (key_down(0x25) && key_down(0x27)) ground.setDir(0);
}

// v20 0x4381e0: everything taken down; all tasks finished; the game's cursor back.
void MudPie_Scene::leave() {
    VoiceOver::get().stop();                         // 0x4c9260
    Game2D_Scene::leave();                           // 0x4ad270
    guide.unload();                                  // v16
    deactivate();                                    // v7
    if (board) delete board;
    board = nullptr;
    for (size_t i = 0; i < flyers.size(); i++) delete flyers.at(i);
    flyers.clear();
    ai->skunk.unload();                              // v16
    ai = nullptr;
    for (size_t i = 0; i < throwers.size(); i++) delete throwers.at(i);
    throwers.clear();
    for (size_t i = 0; i < spots.size(); i++) delete[] spots.at(i);
    spots.clear();
    ground.unload();                                 // v16
    if (score) delete score;
    score = nullptr;
    if (cannon) cannon->detach();                    // 0x412200
    screen.deactivate();                             // v7
    screen.unloadAll();                              // 0x41ec00
    screen.clear();                                  // 0x41ebc0
    hide_cursor();
    cursor.visible = false;                          // +0x1944
    cursor.unload();                                 // v16
    ras_release(pack);                               // 0x419620
    pack = nullptr;
    keyboard.deactivate();
    mouse.deactivate();
    music.close();                                   // 0x41fc50
    for (int i = 0; i < 7; i++) sounds[i].close();
    tasks_finish_all();                              // 0x4210d0
    set_cursor(Game::state().cursor, true);          // 0x4119b0
    Game::state().guide_loaded = false;              // +0x7b8
}

}  // namespace Game
