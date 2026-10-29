// London, reconstructed from "Tinker Bell.exe" (see london.h).
#include "london.h"
#include "cannon.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "tb.h"

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

// 0x425da0: |a - b| <= r (in 3D).
static bool within(const float a[3], const float b[3], float r) {
    double dx = (double)a[0] - b[0], dy = (double)a[1] - b[1], dz = (double)a[2] - b[2];
    return !((double)r * r < dz * dz + dy * dy + dx * dx);
}

// ---------------------------------------------------------------- GemSign
GemSign::GemSign(const char *s, const char *t, const char *g, const char *c, const char *st)   // 0x4e6400
    : names{s, t, g, c, st} {}

void GemSign::init(Raytrace::RayTrace *r) {          // 0x4e6440
    rt = r;
    sprite = rt->findSprite(names[0]);
    if (!sprite) return;                             // port: the exe would crash without the sign
    sprite->attach(names[1], names[1], 100);         // the tick (kept by the sprite only)
    green = sprite->attach(names[2], names[2], 100);
    cap = sprite->attach(names[3], names[3], 90);
    star = sprite->attach(names[4], names[4], 95);
    if (green) green->on = false;
    if (cap) cap->on = false;
    if (star) star->on = false;
    click = new Sound("GLOB_FX_GSclick");            // 0x420210
    click->type = 7;                                 // +0x340
    click->open();
}

void GemSign::release() {                            // 0x4e6530
    sprite = nullptr;
    green = cap = star = nullptr;
    rt = nullptr;
    if (click) { click->close(); delete click; }     // 0x41fc50
    click = nullptr;
}

void GemSign::show() {                               // 0x4e6570
    if (!sprite) return;
    if (sprite->anim) sprite->anim->visible = true;
    rt->makeBox(sprite, false);                      // 0x429580
}

void GemSign::hide() {                               // 0x4e6590
    if (!sprite) return;
    rt->removeBox(sprite);
    if (sprite->anim) sprite->anim->visible = false;
}

bool GemSign::mouse() {                              // 0x4e65b0
    if (!green) return false;
    green->on = false;
    clicked = false;
    if (!enabled) return green->on;
    if (rt->pick(g_mouse.x, g_mouse.y) != sprite) return green->on;
    green->on = true;
    if (!mouse_down()) return green->on;
    clicked = true;
    if (click) click->play();
    return green->on;
}

// ---------------------------------------------------------------- WallAnimation
void WallAnimation::init() {                         // 0x4f1690
    Raytrace::WallObject *w = rt->findWall(wall);    // 0x42d6a0
    if (!w) return;                                  // port: the exe would crash without the wall
    w->addLink(prefix, prefix, key);                 // 0x42e760
    w->linkOn(prefix, true);                         // 0x42e8c0
    anim = w->findLink(prefix);                      // 0x42e710
    if (!anim) return;
    anim->anim->start_time = frame_time();           // 0x403be0(0, 0)
    anim->anim->mode = 0;
    anim->anim->stop();                              // 0x403c00
    anim->anim->setFrame(0);
}

void WallAnimation::on() {                           // 0x4f1700
    if (Raytrace::WallObject *w = rt->findWall(wall)) w->linkOn(prefix, true);
}
void WallAnimation::off() {                          // 0x4f1720
    if (Raytrace::WallObject *w = rt->findWall(wall)) w->linkOn(prefix, false);
}
void WallAnimation::play(int delay, int mode) {      // 0x4f1740
    if (!anim) return;
    anim->anim->setFrame(0);
    anim->anim->start_time = frame_time() + delay;   // 0x403be0
    anim->anim->mode = mode;
}

// ---------------------------------------------------------------- RibbonInterface / LondonCursor
RibbonInterface::RibbonInterface()                   // 0x4b68f0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), bar("lond_if_intfacbar_"),
      idle("lond_bu_ribbonidl_"), rollover("lond_bu_ribbonrov_"), select("lond_bu_ribbonsel_"),
      cursor("lond_cu_ribbon_") {
    bar.load();
    idle.load();
    rollover.load();
    select.load();
    cursor.load();
    cursor.start_time = frame_time();                // 0x403be0(0, 1)
    cursor.mode = 1;
}
RibbonInterface::~RibbonInterface() {                // 0x4b6a60
    bar.unload();
    idle.unload();
    rollover.unload();
    select.unload();
    cursor.unload();
}
void RibbonInterface::draw() {                       // v14 0x4b6b70
    bar.draw();
    if (!holding) return;
    if (enabled && select.hit(g_mouse.x, g_mouse.y)) {
        select.draw();
        return;
    }
    idle.draw();
}

LondonCursor::LondonCursor(const char *a, const char *n)   // 0x4b6cf0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), anim(a),
      still(n, 0, 0, 0, 0, 0, 0, true) {
    anim.load();
    anim.start_time = frame_time();                  // 0x403be0(0, 1)
    anim.mode = 1;
    still.load();
}
LondonCursor::~LondonCursor() {                      // 0x4b6df0
    anim.unload();
    still.unload();
}
void LondonCursor::draw() {                          // v14 0x4afe20
    if (active) {
        anim.x = x;
        anim.y = y;
        anim.draw();
        return;
    }
    still.x = x;
    still.y = y;
    still.draw();
}

// ---------------------------------------------------------------- RibbonState
// 0x4b6bf0: one of four spots at random; the sprite at 120 percent, standing on the ground.
void RibbonState::place() {
    static const float spots[4][2] = {{2675.0f, 1167.0f}, {4229.0f, 2936.0f}, {4223.0f, 2058.0f}, {4242.0f, 1414.0f}};
    int r = vision_random(4);                        // 0x425d40 (the scene's random +0x281c)
    float x = 0, z = 0;
    if (r >= 0 && r <= 3) { x = spots[r][0]; z = spots[r][1]; }
    sprite = rt->addSprite("ribbon", "lond_ga_ribbon_", 1);   // 0x42d9d0
    Sprite3D *s = rt->findSprite("ribbon");
    if (!s) return;
    s->load();
    s->scale = 120.0f;                               // +0x84
    s->setPos(x, 0.1f, z);
    s->place();
}

// ---------------------------------------------------------------- London_Scene
London_Scene::London_Scene()                         // 0x4b6440
    : TinkerbellRaytraceScene("london"), keyboard(this), ribbon_script(this), ticktock(this), hunt(this),
      music("LOND_MU_music"), sign("AA_lond_gs_sign_01", "lond_GS_tick_", "GLOB_GS_green_", "GLOB_GS_cap_",
                                   "GLOB_GS_star_"),
      city("lond_ax_city") {
    // port: the scene's mouse / keyboard / joystick clients (0x41f290) are set by Raytrace_Scene
}

London_Scene::~London_Scene() {}

void London_Scene::waitBusy(Task *t) {
    while (Game::state().vo_busy()) t->wait(30);     // 0x4bcb50 then 0x420e50(0x1e)
}

// v19 0x4b5800
void London_Scene::enter() {
    busy = false;                                    // +0x28e0
    open(name());                                    // v27 0x4ab0f0
    tink->hide();                                    // 0x4cca40
    // port: +0x39d4 = 2, +0x39dc = 6 (the scene's sound settings, unused here)
    city.open();                                     // +0x369c 0x41f9d0(0)
    wendy = new WendyInLondon(&rt);                  // 0x4bdaf0
    peter = new PeterInLondon(&rt);                  // 0x4bbf90
    ribbon.rt = &rt;
    ticktock.tick.open();                            // 0x4b9b30: the script's sound
    activate();                                      // v6 (+0x18)
    reset();                                         // 0x421110
    iface = new RibbonInterface();                   // 0x4b68f0
    iface->visible = false;
    over1.layer(0)->add(iface);
    waterfall = new AnimationHelper("LOND_LY_waterfall_", 0);
    waterfall->visible = false;
    over1.layer(0)->add(waterfall);
    sign.init(&rt);                                  // 0x4e6440
    rt.addBox("corner_block", 2380.0f, 3000.0f, -5000.0f, 5000.0f, 2440.0f, 2940.0f);   // 0x4292f0
    over1.layer(0)->add(&fade);
    fade.z = -1000;                                  // 0x411e70
    if (fade.layer) fade.layer->sort();
    use_normal = use_ribbon = false;
    hide_cursor();                                   // 0x4119e0
    // port: +0x3698 = &state +0x7c0 (the cannon, Game::cannon())
    // port: +0x1998 / +0x1a1c / +0x2064 visible = 0 (the bag, map icon and bell: not ported yet)
    john = new WallAnimation(&rt, "wall_LOND_WP_ctyscap08_01_41", "LOND_CA_JONwave_", 0x14);   // 0x4f1630
    john->init();
    michael = new WallAnimation(&rt, "wall_LOND_WP_ctyscap08_01_41", "LOND_CA_MICwave_", 0x1e);
    michael->init();
    next_wave = frame_time();
    static const char *const names[8] = {"LOND_LY_arleft_01", "LOND_LY_arright_01", "LOND_LY_arup_01",
                                         "LOND_LY_ardown_01", "LOND_LY_arall_01", "LOND_LY_arctrl_01",
                                         "LOND_LY_enter_01", "LOND_LY_shift_01"};
    arrows.resize(8);
    for (int i = 0; i < 8; i++) arrows[(size_t)i] = new Image(names[i], 0, 0, 0, 0, 0, 0, true);   // 0x4179e0
    for (Image *a : arrows) {
        if (!a) continue;
        a->load();                                   // v15
        a->visible = false;
        over1.layer(0)->add(a);
    }
    cur_normal = new LondonCursor("GLOB_CU_active_", "GLOB_CU_normal_01");   // 0x4b6cf0
    cur_ribbon = new LondonCursor("LOND_CU_ribbon_", "LOND_CU_ribbon_01");
    use_normal = use_ribbon = false;
    // port: with state +0x64 (making a film) the LondonMovieMaker (+0x2ff0) is put up and Wendy, the sign and the
    // waves are hidden; the film maker is a development tool and is not ported
    Game::state().paused3d = false;                  // state +0x64
    arrow_time = 0;                                  // port: the exe leaves +0x31a8 as it was
    for (bool &b : arrow_after) b = false;
    city.play();                                     // 0x41fd50
    float sp[4];
    Game::state().cameraSpot(0, 0, sp);              // 0x4ec140(0, 0, rt)
    rt.setCamera(sp[0], sp[1], sp[2], sp[3]);
    if (const char *c = getenv("TB_RT_CAM")) {       // desktop test aid (not in the game): "x,z,degrees,eye"
        float x = sp[0], z = sp[2], d = sp[3], e = sp[1];
        sscanf(c, "%f,%f,%f,%f", &x, &z, &d, &e);
        rt.setCamera(x, e, z, d);
    }
    music.open();                                    // +0x31b8
    music.play();
}

// v20 0x4b6230
void London_Scene::leave() {
    VoiceOver::get().stop();                         // 0x4c9260
    city.close();                                    // 0x41fc50
    deactivate();                                    // +0x1c
    if (waterfall) { waterfall->detach(); delete waterfall; }
    waterfall = nullptr;
    if (cur_normal) { if (g_cursor_graphic == cur_normal) g_cursor_graphic = nullptr; delete cur_normal; }
    if (cur_ribbon) { if (g_cursor_graphic == cur_ribbon) g_cursor_graphic = nullptr; delete cur_ribbon; }
    cur_normal = cur_ribbon = nullptr;
    delete john;                                     // RayTraceCharacter v1 + delete (the wall keeps the link)
    john = nullptr;
    delete michael;
    michael = nullptr;
    sign.release();                                  // 0x4e6530
    if (iface) { iface->detach(); delete iface; }
    iface = nullptr;
    music.close();
    delete wendy;
    wendy = nullptr;
    delete peter;
    peter = nullptr;
    ticktock.release();                              // 0x4b9b40
    for (Image *a : arrows) {
        if (!a) continue;
        a->unload();                                 // v16
        a->detach();
        delete a;
    }
    arrows.clear();
    fade.detach();
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    reset();                                         // 0x4210d0 (port: the task reset)
    set_cursor(Game::state().cursor, true);          // 0x4119b0(state +0x5c, 1)
}

// The body of each of the run's loops: John's or Michael's wave every 15-20 s, Wendy, the cursor, the arrows.
void London_Scene::scriptFrame() {
    if (frame_time() >= next_wave) {
        next_wave = (uint32_t)vision_random(5000) + frame_time() + 15000;   // state +8: 0x425d40(0x1388)
        WallAnimation *w = vision_random(2) != 0 ? john : michael;
        if (w) w->play(0, 0);                        // 0x4f1740
    }
    wendy->update();                                 // 0x4be780
}

// v10 0x4b5300
void London_Scene::run() {
    wait(1);
    busy = true;                                     // +0x28e0
    fade.start();                                    // 0x4e61f0
    while (fade.mode != 2) wait(30);
    busy = false;
    forceOff();                                      // v24
    paused = false;                                  // +0x128
    Game::state().saves.setTutorialsPassed(true);    // 0x4ea570(1)
    Game::state().saves.flush();                     // 0x4e9840
    // port: the film-making branch (state +0x64, 0x4b53d6..0x4b546f) is not ported (see enter)
    Task *scripts[4] = {&keyboard, &ribbon_script, &ticktock, &hunt};
    for (int i = 0; i < 4; i++) {
        if (getenv("TB_LONDON_SKIP") && i < atoi(getenv("TB_LONDON_SKIP"))) continue;   // test aid: skip scripts
        if (getenv("TB_DEBUG_LONDON")) tb_log("london: script %d", i);
        start(scripts[i]);                           // 0x420ef0
        while (scripts[i]->active()) {               // +0x14
            scriptFrame();
            if (i == 3) peter->update();             // 0x4bc880: only while the hunt runs
            cursorUpdate();                          // v30
            perFrame();                              // 0x4b65f0
            wait(0);
        }
    }
    change_scene("lost_tutorial");                   // 0x41ddf0
    finish();                                        // 0x420fe0
}

// 0x4b65f0: "all the arrows" shows when one of these lines has played out, for two seconds.
void London_Scene::perFrame() {
    static const char *const lines[5] = {"LOND_VO_WENinter2", "LOND_VO_WENinter1", "LOND_VO_WENsign2",
                                         "TICK_VO_WENface1", "LOND_VO_WENhelp1"};
    static const int flag[5] = {1, 0, 2, 3, 4};      // +0x31ad, +0x31ac, +0x31ae, +0x31af, +0x31b0
    VoiceOver &vo = VoiceOver::get();
    for (int i = 0; i < 5; i++) {
        bool &f = arrow_after[flag[i]];
        if (vo.isPlaying(lines[i]) && !f) f = true;
        if (!vo.isPlaying(lines[i]) && f) {
            if (arrows.size() >= 5 && arrows[4]) arrows[4]->visible = true;
            arrow_time = frame_time();
            f = false;
        }
    }
    if (frame_time() - arrow_time > 2000 && arrows.size() >= 5 && arrows[4]) arrows[4]->visible = false;
}

// v30 0x4b4f70: which cursor shows: the active one over Wendy (when she wants a click), Peter (likewise), the
// sign, the ribbon or the ribbon in the bar.
void London_Scene::cursorUpdate() {
    if (!cur_normal || !cur_ribbon) return;
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);
    cur_normal->active = false;
    cur_ribbon->active = false;
    if (s && wendy && s == wendy->sprite && wendy->flag) {
        cur_normal->active = true;
        cur_ribbon->active = true;
    }
    bool act = (s && peter && s == peter->sprite && peter->flag) || (s && s == sign.sprite && sign.enabled) ||
               (s && s == ribbon.sprite && ribbon.active) ||
               (iface && iface->enabled && iface->holding && iface->select.hit(g_mouse.x, g_mouse.y));
    if (act) cur_normal->active = true;
    if (use_normal) { set_cursor(cur_normal, true); return; }
    if (use_ribbon) { set_cursor(cur_ribbon, true); return; }
    hide_cursor();
}

// v25 0x4b50a0: the clicks of the ribbon game, then the running script's own.
void London_Scene::onMouse() {
    if (paused) return;                              // +0x128
    if (busy) return;                                // +0x28e0
    sign.mouse();                                    // 0x4e65b0
    if (mouse_down()) {
        clicked = true;                              // +0x39f4
        Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);
        if (s && s == ribbon.sprite && ribbon.active) {
            iface->holding = true;                   // the ribbon goes into the bar
            iface->visible = true;
            if (ribbon.sprite->anim) ribbon.sprite->anim->visible = false;
            ribbon.state = 0;
        } else if (iface->enabled && iface->holding && iface->select.hit(g_mouse.x, g_mouse.y) && iface->enabled) {
            iface->holding = false;                  // taken onto the cursor
            use_normal = false;
            use_ribbon = true;
            ribbon.state = 1;
        } else if (ribbon.state == 1) {
            if (s && s == wendy->sprite) {           // given to Wendy
                iface->visible = false;
                use_normal = true;
                use_ribbon = false;
                ribbon.state = 2;
            } else {                                 // back into the bar
                iface->holding = true;
                use_normal = true;
                use_ribbon = false;
                ribbon.state = 0;
            }
        }
    }
    if (ticktock.active() && ticktock.mouse()) return;
    if (ribbon_script.active() && ribbon_script.mouse()) return;
    if (hunt.active()) hunt.mouse();
}

// v26 0x4b5260
bool London_Scene::onKey() {
    if (busy) paused = true;                         // +0x28e0 -> +0x128
    if (paused) return false;
    if (Raytrace_Scene::onKey()) return true;
    if (g_key.down && (int)g_key.vk == Game::state().skip_key) {   // 0x5a6dd8, 0x5a6d9c == state +0x74
        Game::state().start_in_london = false;       // state +0x1818
        change_scene("lost");
        return true;
    }
    if (stateKeys()) return true;                    // 0x4ec5f0 (state +0x13a0... on this scene)
    // 0x4baef0 (with a key down) always says no
    return false;
}

// ---------------------------------------------------------------- LSKeyboardScript (0x4b8900)
void LSKeyboardScript::run() {
    London_Scene *s = scene;
    WendyInLondon *w = s->wendy;
    wait(1);
    start_angle = (float)s->rt.angleDeg();          // +0xc0
    struct Step { const char *say, *xp, *idle; int arrow, *force, vk; };
    // left: turn left 60 degrees
    s->forceOff();
    w->say("LOND_VO_WENleft");
    s->waitBusy(this);
    s->arrows[0]->visible = true;
    if (getenv("TB_DEBUG_LONDON")) tb_log("london: keyboard: left");
    w->say("LOND_VO_WENleftxp");
    s->force[2] = 0;                                 // 0x62d758
    s->active_time = frame_time();                   // 0x4a9c90
    while (s->rt.angleDeg() < (double)turn + start_angle) {
        if (g_key.down && g_key.vk != 0x25) {        // VK_LEFT
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENleftid");
            s->active_time = frame_time();
        }
        if (frame_time() - s->active_time >= 10000) {
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENleftid");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->arrows[0]->visible = false;
    // right: back past where it started
    s->forceOff();
    w->say("LOND_VO_WENright");
    s->waitBusy(this);
    s->arrows[1]->visible = true;
    w->say("LOND_VO_WENrighxp");
    s->force[3] = 0;                                 // 0x62d75c
    s->active_time = frame_time();
    while ((double)start_angle < s->rt.angleDeg()) {
        if (g_key.down && g_key.vk != 0x27) {        // VK_RIGHT
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENrghtid");
            s->active_time = frame_time();
        }
        if (frame_time() - s->active_time >= 10000) {
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENrghtid");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->arrows[1]->visible = false;
    // up: forward to within 500 of Wendy
    s->forceOff();
    w->say("LOND_VO_WENup");
    s->waitBusy(this);
    s->arrows[2]->visible = true;
    w->say("LOND_VO_WENupxp");
    s->force[0] = 0;                                 // 0x62d750
    s->active_time = frame_time();
    while (500.0f < s->rt.spriteDist(w->sprite)) {   // 0x4be7c0, 0x515250
        if (g_key.down && g_key.vk != 0x26) {        // VK_UP
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENupid");
            s->active_time = frame_time();
        }
        if (frame_time() - s->active_time >= 10000) {
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENupid");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->arrows[2]->visible = false;
    // down: back to 600 away
    s->forceOff();
    w->say("LOND_VO_WENdown");
    s->waitBusy(this);
    s->arrows[3]->visible = true;
    w->say("LOND_VO_WENdownxp");
    s->force[1] = 0;                                 // 0x62d754
    s->active_time = frame_time();
    while (s->rt.spriteDist(w->sprite) < 600.0f) {   // 0x515a40
        if (g_key.down && g_key.vk != 0x28) {        // VK_DOWN
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENdownid");
            s->active_time = frame_time();
        }
        if (frame_time() - s->active_time >= 10000) {
            if (!Game::state().vo_busy()) w->say("LOND_VO_WENdownid");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->arrows[3]->visible = false;
    finish();
}

// ---------------------------------------------------------------- LSRibbonScript (0x4b92e0)
// 0x4b9130: over Wendy: done with the mouse lesson; a click on the ribbon from too far notes it.
bool LSRibbonScript::mouse() {
    Sprite3D *s = scene->rt.pick(g_mouse.x, g_mouse.y);
    float keep = scene->rt.pick_max;
    if (s && s == scene->wendy->sprite) {
        on_wendy = true;
        return true;
    }
    if (mouse_down()) {
        scene->rt.pick_max = 3.40282347e38f;         // 0x7f7fffff
        Sprite3D *f = scene->rt.pick(g_mouse.x, g_mouse.y);
        scene->rt.pick_max = keep;
        if (f && f == scene->ribbon.sprite) clicked_far = true;
    }
    return false;
}

// 0x4b91c0: "it's around here" when the ribbon comes within reach, "too far" when it goes out of it.
void LSRibbonScript::nearPrompts() {
    London_Scene *s = scene;
    float cam[3], rp[3] = {0, 0, 0};
    s->rt.camPos(cam);
    if (s->ribbon.sprite) { rp[0] = s->ribbon.sprite->x; rp[1] = s->ribbon.sprite->y; rp[2] = s->ribbon.sprite->z; }
    bool n = within(cam, rp, s->rt.pick_max);
    bool said = false;
    if (!near) {
        if (n) { s->wendy->say("LOND_VO_WENaround"); near = true; said = true; }
    } else if (!n) {
        s->wendy->say("LOND_VO_WENfar");
        near = false;
        said = true;
    }
    if (!said && clicked_far && !n) {
        s->wendy->say("LOND_VO_WENcloser");
        clicked_far = false;
    }
    if (!Game::state().vo_busy() && frame_time() - s->active_time > 10000) {
        s->wendy->say("LOND_VO_WENhelp2");
        s->active_time = frame_time();
    }
}

void LSRibbonScript::run() {
    London_Scene *s = scene;
    WendyInLondon *w = s->wendy;
    wait(1);
    s->forceOff();
    w->say("LOND_VO_WENmouse1");
    s->waitBusy(this);
    s->active_time = frame_time();
    s->use_normal = true;
    s->use_ribbon = false;
    w->setFlag(true);
    on_wendy = false;
    do {                                             // the mouse onto Wendy
        if (frame_time() - s->active_time > 10000) {
            w->say("LOND_VO_WENmseidl");
            s->active_time = frame_time();
        }
        wait(30);
    } while (!on_wendy);
    s->waitBusy(this);
    s->forceOff();
    w->say("LOND_VO_WENokcur");
    s->waitBusy(this);
    w->setFlag(false);
    s->ribbon.place();                               // 0x4b6bf0
    w->say("LOND_VO_WENhelp1");
    s->waitBusy(this);
    s->force[2] = s->force[3] = s->force[0] = s->force[1] = 0;
    s->ribbon.setActive(true);
    t = frame_time();                                // +0xc4
    s->active_time = frame_time();
    s->rt.pick_max = 975.0f;                         // 0x516378
    clicked_far = false;
    while (s->ribbon.state != 0) {                   // until it is picked up
        nearPrompts();
        wait(30);
    }
    s->iface->enabled = false;
    s->waitBusy(this);
    s->rt.removeSprite("ribbon");                    // 0x42da60
    s->ribbon.sprite = nullptr;                      // port: the exe keeps the freed pointer (never used again)
    w->say("LOND_VO_WENinter1");
    s->waitBusy(this);
    s->active_time = frame_time();
    while (700.0f < s->rt.spriteDist(w->sprite)) {   // 0x516c00: back to Wendy
        if (frame_time() - s->active_time > 10000) {
            w->say("LOND_VO_WENinter2");
            s->active_time = frame_time();
        }
        wait(30);
    }
    w->say("LOND_VO_WENribbn1");
    s->iface->enabled = true;
    flag_c9 = false;
    while (s->ribbon.state != 2) {
        w->setFlag(false);
        while (s->ribbon.state == 0) {               // take it from the bar
            if (frame_time() - s->active_time > 10000) {
                w->say("LOND_VO_WENribbn2");
                s->active_time = frame_time();
            }
            wait(30);
        }
        wait(30);
        w->say("LOND_VO_WENcursr1");
        w->setFlag(true);
        s->active_time = frame_time();
        while (s->ribbon.state == 1) {               // and onto Wendy
            if (frame_time() - s->active_time > 10000) {
                w->say("LOND_VO_WENcursr2");
                s->active_time = frame_time();
            }
            wait(30);
        }
        if (s->ribbon.state == 0) {
            w->say("LOND_VO_WENbadcur");
            s->waitBusy(this);
        }
    }
    s->use_normal = false;
    s->use_ribbon = false;
    w->say("LOND_VO_WENthank1");
    s->waitBusy(this);
    finish();
}

// ---------------------------------------------------------------- LSTickTockScript (0x4b9b50)
LSTickTockScript::LSTickTockScript(London_Scene *s) : scene(s), tick("tick_fx_ticktock") {}   // 0x4b9970

void LSTickTockScript::release() {                   // 0x4b9b40 (its sound)
    tick.close();
}

// 0x4b9ac0: hovering or clicking Wendy and the sign. Always takes the event.
bool LSTickTockScript::mouse() {
    Sprite3D *s = scene->rt.pick(g_mouse.x, g_mouse.y);
    Sprite3D *ws = scene->wendy->sprite;
    if (mouse_down()) {
        if (s == ws) clicked_wendy = true;
        if (s == scene->sign.sprite) clicked_sign = true;
    } else {
        if (s == ws) over_wendy = true;
        if (s == scene->sign.sprite) over_sign = true;
    }
    return true;
}

// 0x4baad0
void LSTickTockScript::step() {
    WendyInLondon *w = scene->wendy;
    Raytrace::RayTrace &rt = scene->rt;
    if (!cannon().running()) game = 8;
    if (clicked_wendy) game = 7;
    uint32_t now = frame_time();
    float nowf = (float)(int64_t)now;
    if (!tick.isPlaying()) {                         // she is looking: no moving after the grace time
        if (!((double)grace <= (double)nowf - t_moved)) {
            stop_x = rt.camx;
            stop_z = rt.camz;
        }
        if ((double)grace < (double)nowf - t_moved && (rt.camx != stop_x || rt.camz != stop_z)) game = 8;
    }
    double since = (double)(int64_t)now - t_turn;
    switch (game) {
    case 0:
        if (since < wait_ms) return;
        w->toForward();
        game = 1;
        return;
    case 1:
        if (!w->isForward()) return;
        tick.stop();
        t_moved = nowf;
        w->say("TICK_VO_WENstop");
        game = 2;
        return;
    case 2:
        if (Game::state().vo_busy()) return;
        tick.play();
        wait_ms = (float)(int64_t)(vision_random(back_max - back_min) + back_min);
        t_turn = nowf;
        break;
    case 3:
        if (w->isBack()) game = 4;
        return;
    case 4:
        if (since < wait_ms) return;
        w->toForward();
        game = 5;
        return;
    case 5: {
        if (!w->isForward()) return;
        tick.stop();
        t_moved = nowf;
        wait_ms = (float)(int64_t)(vision_random(back_max - back_min) + back_min);
        t_turn = nowf;
        game = 6;
        float wp[3], cam[3];
        w->position(wp);
        rt.camPos(cam);
        if (within(cam, wp, 700.0f)) w->say("LOND_VO_WENidlsn1");
        return;
    }
    case 6:
        if ((double)nowf - t_turn < wait_ms) return;
        tick.play();
        wait_ms = (float)(int64_t)(vision_random(back_max - back_min) + back_min);
        t_turn = nowf;
        break;
    default:
        return;
    }
    w->toBack();                                     // 0x4be630
    game = 3;
}

void LSTickTockScript::run() {
    London_Scene *s = scene;
    WendyInLondon *w = s->wendy;
    wait(1);
    float sp[4];
    Game::state().cameraSpot(0, 0, sp);              // 0x4ec180(0, 0)
    mover.start(&s->rt, sp[0], sp[1], sp[2], sp[3], 1000);
    while (mover.step()) wait(30);
    s->force[2] = s->force[3] = s->force[0] = s->force[1] = 0;
    s->use_normal = s->use_ribbon = false;
    s->sign.enabled = false;                         // 0x630ba0
    w->setFlag(false);
    w->say("LOND_VO_WENsign1");
    s->waitBusy(this);
    s->use_normal = true;
    s->use_ribbon = false;
    w->setFlag(true);
    s->active_time = frame_time();
    clicked_wendy = false;
    do {                                             // a click on Wendy, with prompts when wandering off
        float wp[3], cam[3];
        w->position(wp);
        s->rt.camPos(cam);
        bool n = within(cam, wp, 700.0f);
        bool left = false;
        if (was_near && !n) { was_near = false; left = true; }
        else was_near = n;
        if (left) {
            s->active_time = frame_time();
            w->say("LOND_VO_WENonme");
            s->waitBusy(this);
        }
        w->position(wp);
        s->rt.camPos(cam);
        if (!within(cam, wp, 700.0f)) {
            if (frame_time() - s->active_time > 10000) {
                w->say("LOND_VO_WENonme2");
                s->active_time = frame_time();
            }
        } else if (frame_time() - s->active_time > 10000) {
            w->say("LOND_VO_WENidlsn1");
            s->active_time = frame_time();
        }
        wait(30);
    } while (!clicked_wendy);
    over_sign = false;
    s->use_normal = true;
    s->use_ribbon = false;
    s->sign.enabled = true;
    w->setFlag(false);
    w->say("LOND_VO_WENsign2");
    s->waitBusy(this);
    s->active_time = frame_time();
    while (!over_sign) {                             // the mouse onto the sign
        wait(30);
        if (frame_time() - s->active_time > 10000) {
            w->say("LOND_VO_WENidlsn2");
            s->active_time = frame_time();
        }
    }
    w->say("LOND_VO_WENsign3");
    s->waitBusy(this);
    w->setFlag(true);
    s->active_time = frame_time();
    clicked_wendy = false;
    do {                                             // a click on Wendy again
        wait(30);
        if (frame_time() - s->active_time > 10000) {
            w->say("LOND_VO_WENidlsn3");
            s->active_time = frame_time();
        }
    } while (!clicked_wendy);
    w->setFlag(false);
    s->active_time = frame_time();
    s->sign.enabled = true;
    cannon().setTime(secs);                          // 0x4e3ae0
    cannon().visible = true;
    clicked_sign = false;
    w->say("TICK_VO_WENgreet1");
    s->waitBusy(this);
    s->active_time = frame_time();
    while (!clicked_sign) {                          // a click on the sign
        if (frame_time() - s->active_time > 8000) {
            int r = vision_random(3);
            if (r == 0) w->say("GLOB_VO_WENidle1");
            else if (r == 1) w->say("GLOB_VO_WENidle2");
            else if (r == 2) w->say("GLOB_VO_WENidle3");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->force[2] = s->force[3] = s->force[0] = s->force[1] = 0;
    s->use_normal = true;
    s->use_ribbon = false;
    s->sign.enabled = false;
    w->say("TICK_VO_WENtree1");
    s->waitBusy(this);
    s->active_time = frame_time();
    static const float tree[3] = {3950.0f, 0.1f, 1450.0f};   // 0x4576e000, 0x3dcccccd, 0x44b54000
    for (;;) {                                       // over to the tree
        float cam[3];
        s->rt.camPos(cam);
        if (within(cam, tree, 400.0f)) break;
        if (frame_time() - s->active_time > 10000) {
            w->say("TICK_VO_WENprompt");
            s->active_time = frame_time();
        }
        wait(30);
    }
    s->forceOff();
    if (fabs(s->rt.angleDeg() - 135.0f) > 20.0) {    // 0x5144e8, 0x515258: facing Wendy
        s->force[2] = s->force[3] = 0;
        w->say("TICK_VO_WENface1");
        s->waitBusy(this);
        s->active_time = frame_time();
        while (fabs(s->rt.angleDeg() - 135.0f) > 20.0) {
            if (frame_time() - s->active_time > 5000) {
                w->say("TICK_VO_WENidltrn");
                s->active_time = frame_time();
            }
            wait(30);
        }
    }
    s->forceOff();
    s->waitBusy(this);
    w->showClock();                                  // 0x4be5d0
    while (!w->isForward()) wait(30);
    for (;;) {                                       // the clock game, until won
        w->say("TICK_VO_WENgo1");
        s->waitBusy(this);
        s->force[2] = s->force[3] = s->force[0] = s->force[1] = 0;
        t_moved = (float)(int64_t)frame_time();
        t_turn = (float)(int64_t)frame_time();
        wait_ms = (float)first_wait;
        tick.type = 6;                               // +0x43c: loops
        tick.play();
        cannon().start(secs);                        // 0x4e3a50
        w->setFlag(true);
        w->toBack();
        while (!w->isBack()) wait(30);
        game = 0;
        clicked_wendy = false;
        while (game != 8) {
            step();
            wait(30);
            if (game == 7) break;
        }
        tick.stop();
        w->setFlag(false);
        if (!w->isForward()) {
            w->toForward();
            while (!w->isForward()) wait(30);
        }
        s->waitBusy(this);
        if (game == 8) {                             // caught moving, or out of time: back to the start
            w->say(cannon().running() ? "TICK_VO_WENnostop" : "TICK_VO_WENtoobad");
            cannon().stop();
            s->waitBusy(this);
            s->fade.fadeOut();
            while (s->fade.mode != 2) wait(30);
            s->rt.setCamera(3950.0f, 0.1f, 1450.0f, 135.0f);
            s->fade.start();
            while (s->fade.mode != 2) wait(30);
        }
        if (game == 7) break;
    }
    w->hideClock();
    while (!w->isIdle()) wait(30);
    s->forceOff();
    cannon().hide();                                 // 0x4e3bf0
    w->say("TICK_VO_WENclock");
    s->waitBusy(this);
    if (s->sign.cap) s->sign.cap->on = true;         // the cap on the sign
    w->say("TICK_VO_WENuwin1");
    Game::state().cameraSpot(0, 0, sp);
    mover.start(&s->rt, sp[0], sp[1], sp[2], sp[3], 1000);
    while (mover.step()) wait(30);
    s->waitBusy(this);
    w->say("TICK_VO_WENgscap");
    s->waitBusy(this);
    if (s->sign.green) s->sign.green->on = true;     // 0x630b94
    finish();
}

// ---------------------------------------------------------------- LSHuntScript (0x4b7320)
bool LSHuntScript::mouse() {                         // 0x4b72d0
    Sprite3D *s = scene->rt.pick(g_mouse.x, g_mouse.y);
    if (!mouse_down()) return false;
    if (s && s == scene->peter->sprite) clicked_peter = true;
    return true;
}

// The hunt: Peter crows and lands, Tink arrives, the three talk; the flying lesson (Shift up, Ctrl down, Enter to
// hover) twice; then a click on Peter's hand and up through the clouds with him.
void LSHuntScript::run() {
    London_Scene *s = scene;
    WendyInLondon *w = s->wendy;
    PeterInLondon *p = s->peter;
    Tinkerbell3D *t = s->tink;
    auto busy = [&] { s->waitBusy(this); };
    auto tinkTalk = [&] {                            // her sound, with dust, until it ends
        while (Game::state().vo_busy()) {
            t->moreDust();                           // 0x4db1a0
            wait(30);
        }
    };
    wait(1);
    s->forceOff();
    p->say("LOND_VO_PETcrow");
    busy();
    w->toPoint();                                    // 0x4be690
    while (!w->isPoint()) wait(30);
    w->say("LOND_VO_WENwhat");
    busy();
    p->land();                                       // 0x4bc850
    wait(750);
    w->pointDown();                                  // 0x4be6c0
    while (p->landing()) wait(30);                   // 0x4bcb90
    w->turnToPeter();                                // 0x4be6f0
    p->turnToWendy();                                // 0x4bc7c0
    p->say("LOND_VO_PETgreet");
    t->setX(2763.03f);                               // 0x452cb07b
    t->setY(1500.0f);                                // 0x44bb8000
    t->setZ(2765.46f);                               // 0x452cd75c
    t->wanders = false;                              // +0x13c
    t->show();                                       // 0x4cca70
    t->pushBasic(2, 2763.03f, (float)s->rt.eyeWorld(), 2732.43f);   // 0x4de020: in front of Wendy (0x452ac6e1)
    busy();
    while (!t->idle()) wait(30);
    t->say("lond_fx_tnktinkle");
    tinkTalk();
    w->say("LOND_VO_WENhipete");
    busy();
    w->turnToPlayer();                               // 0x4be720
    p->turnToPlayer();                               // 0x4bc7f0
    while (!p->isIdle()) wait(30);
    p->bow();                                        // 0x4bc760
    while (p->bowing()) wait(30);
    p->say("LOND_VO_PETsayhi");
    busy();
    p->say("LOND_VO_PETtink");
    busy();
    t->pushBasic(2, 2967.78f, (float)s->rt.eyeWorld(), 2589.85f);   // 0x45397c7b, 0x4521dd9a
    while (!t->idle()) wait(30);
    t->say("GLOB_FX_TNKhappy");
    t->wave();                                       // 0x4dbd90
    for (;;) {                                       // until she has waved and her sound ended
        if (t->idle() && !Game::state().vo_busy()) break;
        t->moreDust();
        wait(30);
    }
    p->say("LOND_VO_PETtnkwen");
    busy();
    t->pushBasic(2, 3001.91f, (float)s->rt.eyeWorld(), 2909.0f);    // 0x453b9e8f, 0x4535d000: over to Wendy
    while (!t->idle()) wait(30);
    t->say("glob_fx_tnkangry");
    tinkTalk();
    p->say("LOND_VO_PETangry");
    busy();
    t->pushBasic(2, 2763.03f, (float)s->rt.eyeWorld(), 2732.43f);
    p->turnToWendy();
    w->turnToPeter();
    w->say("LOND_VO_WENwhy");
    busy();
    p->say("LOND_VO_PETgame");
    busy();
    p->turnToPlayer();
    w->turnToPlayer();
    s->clicked = false;                              // +0x39f4
    w->say("LOND_VO_WENyes");
    busy();
    s->active_time = frame_time();
    while (!s->clicked) {                            // any click
        wait(30);
        if (frame_time() - s->active_time > 15000) {
            w->say("LOND_VO_WENgoidle");
            s->active_time = frame_time();
        }
        busy();
    }
    while (!p->isIdle()) wait(30);
    p->say("LOND_VO_PETexplan");
    busy();
    w->turnToPeter();
    p->turnToWendy();
    w->say("LOND_VO_WENtink");
    busy();
    w->turnToPlayer();
    p->turnToPlayer();
    wait(30);
    p->say("LOND_VO_PETtnkhlp");
    busy();
    t->say("GLOB_FX_TNKhappy");
    tinkTalk();
    w->say("LOND_VO_WENletsgo");
    busy();
    w->turnToPeter();
    p->turnToWendy();
    p->say("LOND_VO_PETdust");
    busy();
    w->turnToPlayer();
    p->turnToPlayer();
    t->pushBasic(2, 2947.2f, 600.0f, 2419.04f);      // 0x45383333, 0x44160000, 0x451730a4
    while (!t->idle()) wait(30);
    t->hide();                                       // 0x4cca40
    s->waterfall->visible = true;                    // the dust falls on the player
    s->waterfall->start();                           // 0x4e3190
    t->say("GLOB_FX_DUSTin");
    w->fly();                                        // 0x4be750
    for (;;) {
        if (w->isFlyIdle() && !s->waterfall->playing()) break;
        if (!s->waterfall->playing()) s->waterfall->visible = false;
        wait(30);
    }
    s->waterfall->visible = false;
    for (round = 0; round < 2; round++) {            // the flying lesson
        s->forceOff();
        Game::state().hover_up = false;
        if (round == 0) p->say("LOND_VO_PETletsgo");
        else {
            p->say("LOND_VO_PETupdown");
            busy();
            p->say("LOND_VO_PETgoidle");
        }
        s->allowUpDown();                            // 0x4aa9b0
        Game::state().hover_up = false;
        s->active_time = frame_time();
        while (s->rt.eyef < 200.0f) {                // up (Shift)
            if (!Game::state().vo_busy()) s->arrows[7]->visible = true;
            if (frame_time() - s->active_time > 15000) {
                p->say("lond_vo_petgoidle");
                s->active_time = frame_time();
            }
            wait(30);
        }
        s->arrows[7]->visible = false;
        p->say("LOND_VO_PETfly1");
        s->active_time = frame_time();
        while (1.0f < s->rt.eyef) {                  // down (Ctrl)
            if (!Game::state().vo_busy()) s->arrows[5]->visible = true;
            if (frame_time() - s->active_time > 15000) {
                p->say("LOND_VO_PETflyidl");
                s->active_time = frame_time();
            }
            wait(30);
        }
        s->arrows[5]->visible = false;
        p->say("LOND_VO_PETentrup");
        s->forceOff();
        s->unforce();                                // v34
        s->active_time = frame_time();
        while (!Game::state().hover_up) {            // Enter: hover
            if (!Game::state().vo_busy()) s->arrows[6]->visible = true;
            if (frame_time() - s->active_time > 15000) {
                p->say("LOND_VO_PETentrid");
                s->active_time = frame_time();
            }
            wait(30);
        }
        s->arrows[6]->visible = false;
        p->say("LOND_VO_PETentrdw");
        s->active_time = frame_time();
        while (Game::state().hover_up) {             // Enter again: down
            if (!Game::state().vo_busy()) s->arrows[6]->visible = true;
            if (frame_time() - s->active_time > 15000) {
                p->say("LOND_VO_PETentrdi");
                s->active_time = frame_time();
            }
            wait(30);
        }
        s->arrows[6]->visible = false;
        Game::state().hover_up = false;
    }
    s->forceOff();
    Game::state().hover_up = false;
    Game::state().hover_down = false;
    while (1.0f < s->rt.eyef) wait(30);
    p->say("LOND_VO_PEThand");
    busy();
    s->rt.pick_max = 3.40282347e38f;                 // 0x7f7fffff
    s->active_time = frame_time();
    p->setFlag(true);                                // 0x4bcb60
    s->use_normal = true;
    s->use_ribbon = false;
    p->showHand();                                   // 0x4bc790
    clicked_peter = false;
    do {                                             // a click on Peter's hand
        if (frame_time() - s->active_time > 15000) {
            p->say("LOND_VO_PEThandid");
            s->active_time = frame_time();
        }
        wait(30);
    } while (!clicked_peter);
    s->use_normal = s->use_ribbon = false;
    p->say("LOND_VO_PETgo");
    busy();
    p->fly();                                        // 0x4bc820
    while (s->flyThroughClouds(false)) wait(30);     // 0x4ab920(0)
    p->say("LOND_VO_PETstar");
    busy();
    finish();
}

}  // namespace Game
