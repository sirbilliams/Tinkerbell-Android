// The treasure scene's films, reconstructed from "Tinker Bell.exe" (see wamimovie.h). Every wait in the scripts is
// wait(0) unless written otherwise.
#include "wamimovie.h"
#include "treasure.h"
#include "state.h"
#include "tink3d.h"
#include "tb.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool cur_vis() { return g_cursor_graphic ? g_cursor_graphic->visible : false; }
static bool vo_playing(const char *l) { return VoiceOver::get().isPlaying(l); }   // 0x4c9690
static void vo_play(const char *l) { VoiceOver::get().play(l); }                  // 0x4c9400
static void mark(const char *what) {                 // test aid (TB_DEBUG_TREASURE)
    if (getenv("TB_DEBUG_TREASURE")) tb_log("treasure: %s (%u)", what, frame_time());
}

// ---------------------------------------------------------------- WamiMovie

WamiMovie::WamiMovie(Raytrace::RayTrace *r)          // 0x4933b0
    : Task(nullptr), rt(r), peter("peter", r), wendy("wendy", r), near(r, "WAMI_VO_PETcall1", "WAMI_VO_WENcall1"),
      hook("hook", r), smee("smee", r) {
    peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETroot_", "GLOB_CA_PETroot_", "GLOB_CA_PETroot_");   // 0x4d2560
    peter.setTalks("GLOB_CA_PETroottk_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_");   // 0x4d25f0
    peter.setMouth("PET_mout", 0);                   // 0x4d0430
    peter.setMouth("PEL_mout", 1);
    peter.setMouth("PER_mout", 2);
    peter.setEyes("GLOB_CA_PETroot_", "PET_eyes");   // 0x4d08a0
    peter.setEyes("GLOB_CA_PETroottk_", "PET_eyes");
    peter.setEyes("GLOB_CA_PETtalk1_", "PET_eyes");
    peter.setEyes("GLOB_CA_PETtalk2_", "PET_eyes");
    peter.setEyes("GLOB_CA_PETtalk3_", "PET_eyes");
    peter.setEyes("GLOB_CA_PETtalk3_", "PET_eyes");  // (twice in the exe)
    peter.setEyes("LOND_CA_PETlwidle_", "PER_eyes");
    peter.setEyes("WAMI_CA_PETleidle_", "PEL_eyes");
    peter.setEyes("WAMI_CA_PETlkidle_", "PEL_eyes");
    wendy.setMouth("WEN_mout", 0);
    wendy.setMouth("WEL_mout", 1);
    wendy.setTalks("GLOB_CA_WENroottk_", "GLOB_CA_WENtalk1_", "GLOB_CA_WENtalk2_", "GLOB_CA_WENtalk3_");
    wendy.setEyes("GLOB_CA_WENroot_", "WEN_eyes");
    wendy.setEyes("GLOB_CA_WENroottk_", "WEN_eyes");
    wendy.setEyes("GLOB_CA_WENtalk1_", "WEN_eyes");
    wendy.setEyes("GLOB_CA_WENtalk2_", "WEN_eyes");
    wendy.setEyes("GLOB_CA_WENtalk3_", "WEN_eyes");
    wendy.setEyes("LOND_CA_WENpetidl_", "WEL_eyes");
    hook.setMouth("HOK_mout", 0);
    hook.setMouth("HOW_mout", 1);
    hook.setMouth("HOS_mout", 2);
    hook.setEyes("WAMI_CA_HOKidlpan_", "HOW_eyes");
    hook.scale = 180.0f;                             // +0x960 (0x43340000)
    hook.setEyes("WAMI_CA_HOKscarid_", "HOS_eyes");
    smee.setMouth("SME_mout", 0);
    smee.setEyes("WAMI_CA_SMEwidle_", "SME_eyes");
    mouse.setClient(this);                           // 0x41f290
}

// 0x493710: the characters' lines (and which mouth each takes), the unused prompts.
void WamiMovie::load() {
    VoiceOver &vo = VoiceOver::get();
    struct L { const char *line; int mouth; };       // mouth -1: no setInt
    static const L pet[] = {
        {"WAMI_VO_PETcall1", -1}, {"wami_vo_petnicejb", -1}, {"WAMI_VO_PETgame", -1}, {"WAMI_VO_PETcheer", -1},
        {"WAMI_VO_PETfight", 1}, {"WAMI_VO_PETcallid", -1}, {"WAMI_VO_PETcheer", -1}, {"WAMI_VO_PETcall2", -1},
        {"WAMI_VO_PETgreet1", -1}, {"WAMI_VO_PETgo1", -1}, {"WAMI_VO_PETangry", 1}, {"WAMI_VO_PETlokout", 1},
        {"WAMI_VO_PETwhat", -1}, {"WAMI_VO_PETlaugh", -1}, {"WAMI_VO_PETthank", -1}, {"WAMI_VO_PETgift", -1},
        {"WAMI_VO_PETbell", -1}, {"WAMI_VO_PETfly", -1}, {"WAMI_VO_PEThandid", -1}, {"WAMI_VO_PETgo", -1},
    };
    static const L wen[] = {
        {"WAMI_VO_WENcall1", -1}, {"wami_vo_wennicejb", -1}, {"WAMI_VO_WENfight", 1}, {"WAMI_VO_WENplayer", 1},
        {"WAMI_VO_WENcheer", -1}, {"WAMI_VO_WENclock", -1}, {"WAMI_VO_WENlaugh", -1}, {"WAMI_VO_WENgo", -1},
        {"WAMI_VO_WENthank", -1},
    };
    static const L hok[] = {
        {"WAMI_VO_HOKfight", 1}, {"WAMI_VO_HOKlaugh", 1}, {"WAMI_VO_HOKhard", 1}, {"WAMI_VO_HOKagree", -1},
        {"WAMI_VO_HOKjoke", -1}, {"WAMI_VO_HOKthreat", 1}, {"WAMI_VO_HOKfetch", 1}, {"WAMI_VO_HOKscared", 2},
        {"WAMI_VO_HOKrun", 2},
    };
    static const L sme[] = {
        {"WAMI_VO_SMEhard", -1}, {"WAMI_VO_SMEagree", -1}, {"WAMI_VO_SMEscared", -1}, {"WAMI_VO_SMErun", -1},
    };
    for (const L &l : pet) {
        vo.registerLine(l.line, &peter);             // 0x4c91b0
        if (l.mouth >= 0) peter.setInt(l.line, l.mouth);   // 0x4d1bd0
    }
    for (const L &l : wen) {
        vo.registerLine(l.line, &wendy);
        if (l.mouth >= 0) wendy.setInt(l.line, l.mouth);
    }
    for (const L &l : hok) {
        vo.registerLine(l.line, &hook);
        if (l.mouth >= 0) hook.setInt(l.line, l.mouth);
    }
    for (const L &l : sme) vo.registerLine(l.line, &smee);
    inact1.add("WAMI_VO_PETcallid", false);          // 0x4c46e0
    inact1.setInterval(15000);                       // 0x4c4850
    inact2.add("GLOB_VO_PETidle1", false);
    inact2.add("GLOB_VO_PETidle1", false);
    inact2.add("GLOB_VO_PETidle1", false);
    inact2.setInterval(15000);
}

// 0x493e40: Peter and Wendy by the door, the call to come near, Hook and Smee placed off to the left, hidden.
void WamiMovie::enter() {
    reset();                                         // 0x421110
    activate();
    peter.init();                                    // v1
    peter.setX(2730.0f);                             // 0x452aa000
    peter.setY(0.0f);
    peter.setZ(2950.0f);                             // 0x45386000
    wendy.setIdles("GLOB_CA_WENroot_", "GLOB_CA_WENidle3_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_");
    wendy.init();
    wendy.setX(2875.0f);                             // 0x4533b000
    wendy.setY(0.0f);
    wendy.setZ(2975.0f);                             // 0x4539f000
    near.start();                                    // 0x4c19e0
    running = true;                                  // +0x818
    hook.start("WAMI_CA_HOKenter_", 1);              // 0x4d0460
    hook.setX(2000.0f);                              // 0x44fa0000
    hook.setZ(2950.0f);
    hook.hide();                                     // 0x4cca40
    smee.start("WAMI_CA_SMEwalk_", 1);
    smee.setX(1850.0f);                              // 0x44e74000
    smee.setZ(3000.0f);                              // 0x453b8000
    smee.hide();
    agreed = false;                                  // +0xcd8
}

void WamiMovie::skip() {                             // 0x493f50
    if (!running) return;
    deactivate();
    peter.release();                                 // v2
    wendy.release();
    hook.release();
    smee.release();
    near.stop();                                     // 0x4c1400
    running = false;
    mouse.deactivate();                              // +0x141c
    inact1.stop();                                   // +0x34
    inact2.stop();
}

void WamiMovie::playWait(const char *line) {
    mark(line);
    vo_play(line);
    while (vo_playing(line)) wait(0);
}

// 0x493ff0
void WamiMovie::run() {
    wait(1);
    while (near.active()) wait(0);                   // +0x614
    hide_cursor();                                   // 0x4119e0
    treasure_scene().input->deactivate();            // *0x5f6e7c (the scene's +0x12c)
    tween.start(rt, 2568.14f, 0.1f, 3379.91f, 286.089f, 2000.0);   // 0x4520823d 0x3dcccccd 0x45533e8f 0x438f0b64
    while (tween.step()) wait(0);
    while (Game::state().vo_busy()) wait(0);         // +0x560
    playWait("WAMI_VO_PETnicejb");
    playWait("WAMI_VO_WENnicejb");
    playWait("WAMI_VO_PETgame");
    vo_play("WAMI_VO_HOKinterp");
    wait(1500);
    peter.queueAnim("WAMI_CA_PETready_", 0, 1, false, false);   // 0x4d13d0
    peter.queueAnim("WAMI_CA_PETlkidle_", 1, 1, false, false);
    peter.sayOn("WAMI_CA_PETlkidle_", "WAMI_VO_PETfight");   // 0x4d1800
    wendy.max_holds = 25;                            // +0x584 (wendy +0x218)
    wendy.queueAnim("WAMI_CA_WENready_", 2, 1, false, false);
    wendy.queueAnim("LOND_CA_WENpetidl_", 1, 1, false, false);
    wait(1500);
    hook.frame_ms = 100;                             // +0x980
    smee.frame_ms = 100;                             // +0xbdc
    hookTween.init(hook.getX(), 2360.0, 4200.0);     // 0x4df310
    smeeTween.init(smee.getX(), 2250.0, 5200.0);
    hook.show();                                     // 0x4cca70
    smee.show();
    hook.queueAnim("WAMI_CA_HOKenter_", 0, 1, true, true);
    hook.queueAnim("WAMI_CA_HOKenter_", 0, 1, true, false);
    hook.queueAnim("WAMI_CA_HOKstridl_", 0, 1, true, false);
    hook.queueAnim("WAMI_CA_HOKidlpan_", 1, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEwalk_", 0, 1, true, true);
    for (int i = 0; i < 4; i++) smee.queueAnim("WAMI_CA_SMEwalk_", 0, 1, true, false);
    smee.queueAnim("WAMI_CA_SMEstop_", 0, 1, true, false);
    smee.queueAnim("WAMI_CA_SMEwidle_", 0, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEplay_", 0, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEidle_", 0, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEplay_", 0, -1, false, false);
    smee.queueAnim("WAMI_CA_SMEwidle_", 1, 1, false, false);
    tween.start(rt, 2534.77f, 0.1f, 3659.11f, 272.979f, 1500.0);
    while (tween.step() || hookTween.running || smeeTween.running) {
        hook.setX((float)hookTween.value());         // 0x4df350
        smee.setX((float)smeeTween.value());
        wait(0);
    }
    hook.frame_ms = 83;                              // 0x53
    smee.frame_ms = 83;
    wendy.setIdles("GLOB_CA_WENroot_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_");
    mark("waiting for PETfight");
    while (peter.isQueued("WAMI_VO_PETfight") || vo_playing("WAMI_VO_PETfight")) wait(0);   // 0x4d1da0
    playWait("WAMI_VO_HOKfight");
    playWait("WAMI_VO_WENfight");
    playWait("WAMI_VO_HOKlaugh");
    playWait("WAMI_VO_HOKhard");
    playWait("WAMI_VO_SMEhard");
    playWait("WAMI_VO_WENplayer");
    hook.queueAnim("WAMI_CA_HOKdoorb_", 0, 1, false, false);
    hook.queueAnim("WAMI_CA_HOKlplay_", 0, 1, false, false);
    hook.queueAnim("WAMI_CA_HOKidle_", 1, 1, false, false);
    hook.sayOn("WAMI_CA_HOKdoorb_", "WAMI_FX_whisper");
    hook.sayOn("WAMI_CA_HOKidle_", "WAMI_VO_HOKagree");
    smee.queueAnim("WAMI_CA_SMEwhisp_", 0, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEwidle_", 1, 1, false, false);
    // the exe's quirk: Hook's queue and the voice-over asked about an animation's name (never true)
    while (hook.isQueued("WAMI_CA_SMEwhisp_") || vo_playing("WAMI_CA_SMEwhisp_")) wait(0);
    mark("waiting for HOKagree");
    while (hook.isQueued("WAMI_VO_HOKagree") || vo_playing("WAMI_VO_HOKagree")) {
        if (hook.animIs("WAMI_CA_HOKidle_") && !agreed) {
            peter.queueAnim("WAMI_CA_PETready_", 0, -1, false, false);
            peter.queueAnim("GLOB_CA_PETroot_", 1, 1, false, false);
            peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_", "GLOB_CA_PETroot_");
            wendy.queueAnim("LOND_CA_WENlokply_", 0, 1, false, false);
            wendy.queueAnim("GLOB_CA_WENroot_", 1, 1, false, false);
            wendy.setIdles("GLOB_CA_WENroot_", "GLOB_CA_WENidle3_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_");
            agreed = true;
        }
        wait(0);
    }
    playWait("WAMI_VO_SMEagree");
    smee.queueAnim("WAMI_CA_SMEplay_", 0, 1, false, false);
    smee.queueAnim("WAMI_CA_SMEidle_", 1, 1, false, false);
    playWait("WAMI_VO_WENcheer");
    mark("waiting for Peter's ready");
    while (peter.animIs("WAMI_CA_PETready_")) wait(0);
    playWait("WAMI_VO_PETcheer");
    playWait("WAMI_VO_HOKjoke");
    tween.start(rt, 2562.07f, 50.1f, 3173.9f, -90.0f, 1500.0);   // 0x4520211f 0x42486666 0x45465e66 0xc2b40000
    while (tween.step()) wait(0);
    mark("film done");
    show_cursor();                                   // 0x4119d0
    if (running) {
        deactivate();
        peter.release();
        wendy.release();
        hook.release();
        smee.release();
        near.stop();
        running = false;
        mouse.deactivate();
        inact1.stop();
        inact2.stop();
    }
    finish();                                        // 0x420fe0
}

// ---------------------------------------------------------------- Movie2Ending

Movie2Ending::Movie2Ending(Raytrace::RayTrace *r)
    : Task(nullptr), rt(r), repeat("WAMI_VO_PEThandid", 15000) {   // 0x4c4af0
    mouse.setClient(this);                           // 0x41f290
}

// 0x496e50: waiting for the click on Peter (his line every 15 s); a cloud put 100 in front of the camera.
void Movie2Ending::start() {
    reset();
    activate();
    mouse.activate();                                // +0x19c
    repeat.start();                                  // v12
    state = 0;                                       // +0x1d0
    Sprite3D *cloud = treasure_scene().cloud1;       // *0x5f9498 (the scene's +0x2748)
    cloud->anim->visible = true;                     // +0x70 +0x7c
    float a = (float)rt->angleDeg();                 // 0x42b560
    const float pi = 3.14159274f, k = 0.00555555569f;   // 0x51597c, 0x513d90
    float x = (float)(rt->camx + cos((double)pi * a * k) * 100.0);   // 0x42b470, 0x5134c0
    cloud->y = 650.0f;                               // +0x7c (0x44228000)
    cloud->x = x;                                    // +0x78
    cloud->scale = 200.0f;                           // +0x84 (0x43480000)
    cloud->z = (float)(rt->camz + sin((double)pi * a * k) * 100.0);   // 0x42b540
    star_said = false;                               // +0x320
}

// 0x496f20
void Movie2Ending::run() {
    wait(1);
    for (;;) {
        switch (state) {                             // the table 0x49717c (1..6)
        case 1:
            if (!vo_playing("WAMI_VO_PETgo")) {
                state = 2;
                cam.start(rt, rt->camx, (float)((double)rt->eyef + 500.0f), rt->camz, (float)rt->angleDeg(), 5000.0);
                peterY.init(peter->getY(), (double)peter->getY() + 950.0, 5000.0);   // 0x515a48
                peter->playKeep("LOND_CA_PEThndfly_", 1);   // 0x4d2a70
            }
            break;
        case 2:
            if (peter->getY() > 600.0f && !Game::state().vo_busy()) {   // 0x515a40
                state = 5;
                star_said = true;                    // (so "PETstar" never plays: the exe's)
            }
            cam.step();                              // 0x4df4d0
            peter->setY((float)peterY.value());      // 0x4df270
            break;
        case 3:                                      // (never entered)
            if (peterY.running) {
                peter->setY((float)peterY.value());
            } else if (!vo_playing("WAMI_VO_PETohno")) {
                peter->queueAnim("LOND_CA_PEThand_", 0, 1, false, false);
                peter->queueAnim("LOND_CA_PEThandidl_", 1, 1, false, false);
                state = 4;
            }
            break;
        case 4: handIdle(); break;
        case 5: rising(); break;
        case 6:
            if (treasure_scene().fader->busy()) cam.step();   // *0x5f6e74, 0x4a95c0
            else change_scene("signin");             // 0x41ddf0
            break;
        default: break;
        }
        wait(0);
    }
}

void Movie2Ending::handIdle() {                      // 0x4972a0
    if (peter->animIs("LOND_CA_PEThandidl_")) {
        state = 0;
        repeat.start();
    }
}

// 0x497340: "PETstar" (if not yet) while Peter and the camera rise; then the fade out.
void Movie2Ending::rising() {
    if (!star_said && !Game::state().vo_busy()) {
        vo_play("WAMI_VO_PETstar");
        star_said = true;
    }
    if (cam.step() || peterY.running || vo_playing("WAMI_VO_PETstar")) {
        peter->setY((float)peterY.value());
        return;
    }
    state = 6;
    treasure_scene().fadeOutBegin();                 // 0x4abab0
    treasure_scene().fader->start(false, 2000);      // 0x4a9580
    cam.step();
}

// 0x4971a0: the plain cursor; over Peter (waiting) the active one; a click on him: "go" (state 1).
void Movie2Ending::notify2(Service *s) {
    State &st = Game::state();
    set_cursor(st.cursor, cur_vis());                // 0x4119b0 (+0x5c)
    if (s == &mouse && state == 0 && rt->hitSprite(peter->sprite(), g_mouse.x, g_mouse.y))   // 0x42c7b0
        set_cursor(st.cursor_active, cur_vis());     // +0x60
    if (s == &mouse && mouse_down() && state == 0 && rt->hitSprite(peter->sprite(), g_mouse.x, g_mouse.y)) {
        repeat.stop();                               // v13
        vo_play("WAMI_VO_PETgo");
        state = 1;
    }
}

// ---------------------------------------------------------------- SmoothRunner, BellBehindPixieDust

void SmoothRunner::run() {                           // 0x497400
    wait(1);
    while (tw->running) {                            // +0x3c
        switch (axis) {
        case 0: ch->setX((float)tw->value()); break;
        case 1: ch->setY((float)tw->value()); break;
        case 2: ch->setZ((float)tw->value()); break;
        default: break;
        }
        wait(0);
    }
    deactivate();
    finish();
}

BellBehindPixieDust::BellBehindPixieDust()           // 0x4978c0
    : Task(nullptr), screen(0, 0, 0, 0, 0, true, true), bell("WAMI_AM_bell_01", 0, 0, 0, 0, 0, 0, true),
      puff("GLOB_BU_BAGpffsel_") {
    screen.layer(0)->add(&bell);                     // 0x41ece0, 0x41e930
    screen.layer(0)->add(&puff);
}

void BellBehindPixieDust::run() {                    // 0x497a20: the puff to its end, then gone
    wait(1);
    while (puff.step()) wait(100);                   // 0x403d60
    screen.unloadAll();                              // 0x41ec00
    screen.deactivate();
    deactivate();
    finish();
}

// ---------------------------------------------------------------- WamiMovie2

WamiMovie2::WamiMovie2(Raytrace::RayTrace *r)        // 0x495140
    : Task(nullptr), rt(r), tnk(new Sound("WAMI_FX_TNKtalk")), ending(r) {}   // 0x420210

WamiMovie2::~WamiMovie2() { delete tnk; }

// 0x495260: Peter, Wendy, Hook and Smee back by the open door.
void WamiMovie2::start() {
    reset();
    activate();
    peter->setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETroot_", "GLOB_CA_PETroot_", "GLOB_CA_PETroot_");
    peter->init();                                   // v1
    peter->setX(2730.0f);
    peter->setZ(2950.0f);
    wendy->setIdles("GLOB_CA_WENroot_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_", "GLOB_CA_WENroot_");
    wendy->init();
    wendy->setX(2875.0f);                            // 0x4533b000
    wendy->setZ(2950.0f);
    hook->start("WAMI_CA_HOKidle_", 1);              // 0x4d0460
    hook->setX(2360.0f);                             // 0x45138000
    hook->setZ(2950.0f);
    smee->start("WAMI_CA_SMEidle_", 1);
    smee->setX(2190.0f);                             // 0x4508e000
    smee->setZ(3050.0f);                             // 0x453ea000
    running = true;                                  // +0xd4
    once = false;                                    // +0xd5
    if (!tnk->isOpen()) tnk->open();                 // 0x41f9c0, 0x41f9d0
}

void WamiMovie2::skip() {                            // 0x4953a0
    if (!active()) return;                           // v5
    deactivate();
    peter->release();
    wendy->release();
    hook->release();
    smee->release();
    running = false;
    tnk->close();                                    // 0x41fc50
    ending.deactivate();                             // +0x1c
    ending.mouse.deactivate();
    ending.repeat.stop();                            // +0x34
    runner.deactivate();
    bell.screen.unloadAll();                         // 0x41ec00
    bell.screen.deactivate();
    bell.deactivate();
}

void WamiMovie2::playWait(const char *line) {
    mark(line);
    vo_play(line);
    while (vo_playing(line)) wait(0);
}

void WamiMovie2::waitQueued(WamiBaseChar *c, const char *line) {
    while (c->isQueued(line) || vo_playing(line)) wait(0);
}

void WamiMovie2::waitUntil(WamiBaseChar *c, const char *anim) {
    while (!c->animIs(anim)) wait(0);
}

// 0x495470
void WamiMovie2::run() {
    wait(1);
    tween.start(rt, 2532.03f, 0.1f, 3680.15f, 268.919f, 2000.0);   // 0x451e407b 0x3dcccccd 0x45660266 0x438675a2
    while (tween.step()) wait(0);
    smee->queueAnim("WAMI_CA_SMEidle_", 0, 1, false, false);
    smee->queueAnim("WAMI_CA_SMEidle_", 0, 1, false, false);
    smee->queueAnim("WAMI_CA_SMEidle_", 0, 1, false, false);
    smee->queueAnim("WAMI_CA_SMEplay_", 0, -1, false, false);
    smee->queueAnim("WAMI_CA_SMEwidle_", 1, 1, false, false);
    hook->queueAnim("WAMI_CA_HOKthreat_", 0, 1, false, false);
    hook->queueAnim("WAMI_CA_HOKidlpan_", 1, 1, false, false);
    hook->sayOn("WAMI_CA_HOKidlpan_", "WAMI_VO_HOKthreat");
    peter->queueAnim("WAMI_CA_PETready_", 0, 1, false, false);
    peter->queueAnim("WAMI_CA_PETlkidle_", 1, 1, false, false);
    once = false;
    // Hook threatens; once he is at it, Wendy gets ready
    while (hook->isQueued("WAMI_VO_HOKthreat") || vo_playing("WAMI_VO_HOKthreat")) {
        if (hook->animIs("WAMI_CA_HOKidlpan_") && !once) {
            wendy->max_holds = 50;                   // +0x218
            wendy->queueAnim("WAMI_CA_WENready_", 2, 1, false, false);
            wendy->queueAnim("GLOB_CA_WENroot_", 1, 1, false, false);
            once = true;
        }
        wait(0);
    }
    waitUntil(peter, "WAMI_CA_PETlkidle_");
    playWait("WAMI_VO_PETangry");
    wendy->queueAnim("WAMI_CA_WENpocket_", 0, 1, false, false);
    wendy->queueAnim("WAMI_CA_WENhidebh_", 1, 1, false, false);
    wendy->sayOn("WAMI_CA_WENhidebh_", "WAMI_FX_CLOCK");
    playWait("WAMI_VO_HOKfetch");
    once = false;
    // Wendy's pocket clock ticks: Hook and Smee take fright
    while (wendy->isQueued("WAMI_FX_CLOCK") || WamiBaseChar::clock().isPlaying()) {   // 0x41fec0
        wendy->state = 2;                            // +0x178
        if (wendy->animIs("WAMI_CA_WENhidebh_") && !once) {
            hook->queueAnim("WAMI_CA_HOKscared_", 0, 1, false, false);
            hook->queueAnim("WAMI_CA_HOKscarid_", 1, 1, false, false);
            smee->queueAnim("WAMI_CA_SMEscared_", 0, 1, false, false);
            smee->queueAnim("WAMI_CA_SMEscarid_", 0, 1, false, false);
            smee->queueAnim("WAMI_CA_SMEscared_", 0, -1, false, false);
            smee->queueAnim("WAMI_CA_SMEwidle_", 1, 1, false, false);
            once = true;
        }
        if (WamiBaseChar::clock().isPlaying()) break;
        wait(0);
    }
    if (WamiBaseChar::clock().isPlaying()) {
        for (;;) {
            if (hook->animIs("WAMI_CA_HOKscared_")) {
                hook->sayOn("WAMI_CA_HOKscarid_", "WAMI_VO_HOKscared");
                break;
            }
            wait(0);
            if (!WamiBaseChar::clock().isPlaying()) break;
        }
    }
    waitQueued(hook, "WAMI_VO_HOKscared");
    smee->sayOn("WAMI_CA_SMEwidle_", "WAMI_VO_SMEscared");
    waitQueued(smee, "WAMI_VO_SMEscared");
    playWait("WAMI_VO_PETlokout");
    playWait("WAMI_VO_HOKrun");
    // Hook runs off (back and to the side), Smee after him
    hook->queueAnim("WAMI_CA_HOKstrlev_", 0, 1, false, false);
    hook->queueAnim("WAMI_CA_HOKleave_", 1, 1, false, false);
    hookX.init(hook->getX(), 1600.0, 5000.0);        // 0x4df310
    hookZ.init(hook->getZ(), (double)hook->getZ() - 200.0f, 5000.0);   // 0x514340
    once = false;
    vo_play("WAMI_VO_SMErun");
    while (vo_playing("WAMI_VO_SMErun") && hookX.running) {
        while (!hook->animIs("WAMI_CA_HOKleave_")) wait(0);
        if (hook->animIs("WAMI_CA_HOKleave_")) {
            hook->setX((float)hookX.value());        // 0x4df350
            hook->setZ((float)hookZ.value());
            if (!once) {
                peter->queueAnim("WAMI_CA_PETready_", 0, -1, false, false);
                peter->queueAnim("GLOB_CA_PETroot_", 1, 1, false, false);
                once = true;
            }
        }
        wait(0);
    }
    smee->queueAnim("WAMI_CA_SMEturn_", 0, 1, false, false);
    smee->queueAnim("WAMI_CA_SMErun_", 1, 1, false, false);
    smeeX.init(smee->getX(), 1600.0, 3000.0);
    once = false;
    while (hookX.running || smeeX.running) {         // +0x1fc +0x23c
        if (hook->animIs("WAMI_CA_HOKleave_")) {
            hook->setX((float)hookX.value());
            hook->setZ((float)hookZ.value());
            if (!once) once = true;
        }
        if (smee->animIs("WAMI_CA_SMErun_")) smee->setX((float)smeeX.value());
        wait(0);
    }
    WamiBaseChar::clock().stop();                    // 0x41fe70
    waitUntil(peter, "GLOB_CA_PETroot_");
    vo_play("WAMI_VO_PETwhat");
    tween.start(rt, 2568.14f, 0.1f, 3379.91f, 286.089f, 2000.0);
    while (tween.step()) wait(0);
    while (vo_playing("WAMI_VO_PETwhat")) wait(0);
    peter->queueAnim("LOND_CA_PETlokwen_", 0, 1, false, false);
    peter->queueAnim("LOND_CA_PETlwidle_", 1, 1, false, false);
    playWait("WAMI_VO_WENclock");
    wendy->queueAnim("WAMI_CA_WENclock_", 0, 1, false, false);
    wendy->queueAnim("WAMI_CA_WENpocket_", 0, -1, false, false);
    wendy->queueAnim("GLOB_CA_WENroot_", 1, 1, false, false);
    waitUntil(wendy, "GLOB_CA_WENroot_");
    peter->queueAnim("LOND_CA_PETlokwen_", 0, -1, false, false);
    peter->queueAnim("GLOB_CA_PETroot_", 1, 1, false, false);
    waitUntil(peter, "GLOB_CA_PETroot_");
    Tinkerbell3D *tk = treasure_scene().tink;        // *0x5f9494
    tk->push(new TinkCmdAnimation("GLOB_CA_TNKidle6_", "LOND_SF_TNKtinkle"));   // 0x4dc530, 0x4de160
    treasure_scene().tinkHome();                     // 0x4abb10
    playWait("WAMI_VO_PETlaugh");
    playWait("WAMI_VO_WENlaugh");
    playWait("WAMI_VO_PETthank");
    playWait("WAMI_VO_WENthank");
    playWait("WAMI_VO_PETgift");
    // Tink comes to Peter, waves, and the bell shows behind a puff of dust
    tk->pushBasic(2, (float)((double)peter->getX() - 40.0f), 450.0f, peter->getZ());   // 0x4de020, 0x5144c0
    tk->hiya();                                      // 0x4dbe60
    tnk->play();                                     // 0x41fd50
    bell.screen.loadAll();                           // 0x41ec40
    bell.puff.dir = 1;                               // +0x2f8
    bell.puff.mode = 0;                              // +0x2ec
    bell.screen.activate();
    bell.reset();
    bell.activate();
    playWait("WAMI_VO_PETbell");
    float ax, az;
    tk->ahead(&ax, &az, 200.0f);                     // 0x4da8f0
    tk->pushBasic(2, ax, 300.0f, az);
    tk->blowKiss();                                  // 0x4dbf90
    treasure_scene().tinkHome();
    playWait("WAMI_VO_WENgo");
    // Wendy flies up; Peter holds out his hand
    runner.ch = wendy;                               // +0x970
    runner.tw = &wendyY;                             // +0x974
    runner.axis = 1;                                 // +0x978
    wendyY.init(wendy->getY(), 2000.0, 6000.0);      // 0x4df200
    wendy->playKeep("GLOB_CA_WENflyid_", 1);         // 0x4d2a70
    runner.activate();                               // +0x18
    peter->queueAnim("LOND_CA_PEThand_", 0, 1, false, false);
    peter->queueAnim("LOND_CA_PEThandidl_", 1, 1, false, false);
    tween.start(rt, 2727.95f, 0.1f, 3349.79f, 271.339f, 1500.0);   // 0x452a7f33 0x3dcccccd 0x45515ca4 0x4387ab64
    vo_play("WAMI_VO_PETfly");
    while (vo_playing("WAMI_VO_PETfly") || tween.step()) wait(0);
    waitUntil(peter, "LOND_CA_PEThandidl_");
    show_cursor();
    ending.start();                                  // 0x496e50
    while (ending.active()) wait(0);
    for (;;) wait(0);                                // (until skipped: the ending changes the scene)
}

// ---------------------------------------------------------------- WamiMovie3

WamiMovie3::WamiMovie3(Raytrace::RayTrace *r) : Task(nullptr), rt(r) {   // 0x4974f0
    uwin.add("WAMI_VO_ALLuwin");                     // 0x4cba80
}

void WamiMovie3::skip() {                            // 0x497600
    running = false;
    deactivate();
    uwin.stop();                                     // 0x4cbcb0
}

// 0x497620: "you win" while the camera looks left and right at the open door.
void WamiMovie3::run() {
    wait(1);
    uwin.start();                                    // 0x4c19e0
    cam.start(rt, 2567.0f, 0.1f, 2930.0f, 270.0f, 1500.0);   // 0x45207000 0x45372000 0x43870000
    while (cam.step()) wait(0);
    cam.start(rt, 2567.0f, 0.1f, 2930.0f, 268.0f, 1500.0);   // 0x43860000
    while (cam.step()) wait(0);
    cam.start(rt, 2567.0f, 0.1f, 2930.0f, 272.0f, 3000.0);   // 0x43880000
    while (cam.step()) wait(0);
    cam.start(rt, 2567.0f, 0.1f, 2930.0f, 270.0f, 1500.0);
    while (cam.step()) wait(0);
    while (uwin.active()) wait(0);
    running = false;
    deactivate();
    uwin.stop();
    finish();
}

}  // namespace Game
