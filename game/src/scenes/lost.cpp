// The Lost Boys' hideout and its tutorial, reconstructed from "Tinker Bell.exe" (see lost.h). x87 sums as in the
// 3D code: doubles on the FPU, floats where the exe stores.
#include "lost.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "cannon.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "tb.h"

int tb_input_frame();                                // platform/dinput.cpp (test aid)

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static const float kPi = 3.14159274f;                // 0x5143dc
static const float kDegToRad = 0.00555555569f;       // 0x513d90 (1/180)

// ---------------------------------------------------------------- Lost_Scene_Base
Lost_Scene_Base::Lost_Scene_Base(const char *nm)     // 0x443b40
    : TinkerbellRaytraceScene(nm),
      buck("AB_LOST_gs_sign_01", "LOST_GS_BUCK_", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_"),
      pirt("AA_LOST_gs_sign_01", "LOST_GS_PIRT_", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_"),
      mudp("AC_LOST_gs_sign_01", "LOST_GS_MUDP_", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_") {
    for (Sound *&m : music) m = new Sound("");       // 0x437290 x3 (named by load2)
}

Lost_Scene_Base::~Lost_Scene_Base() {
    for (Sound *&m : music) { delete m; m = nullptr; }
}

// v18 0x443af0: Raytrace_Scene's (0x4aa200, two empty debug texts into over1: port: not made) and the three tunes
// (streamed from their files, looping).
void Lost_Scene_Base::load2() {
    static const char *const names[3] = {"PIRT_MU_music", "BUCK_MU_music", "LOST_MU_music"};   // 0x5376c0
    for (int i = 0; i < 3; i++) {
        music[i]->setName(names[i]);                 // 0x424400
        music[i]->mode = 2;                          // +0x338
        music[i]->type = 6;                          // +0x340
    }
}

// v19 0x443930: the hideout from the area's camera spot (then spot 0 for next time), its walls' animations, the
// signs and what they show.
void Lost_Scene_Base::enter() {
    Raytrace::RayTrace::setMaxEye(0x123);            // 0x42b110
    float sp[4];
    State &st = Game::state();
    st.areaSpot(3, sp);                              // 0x4ec0a0
    cam_x = sp[0]; cam_eye = sp[1]; cam_z = sp[2]; cam_deg = sp[3];   // 0x4aa0d0
    st.setAreaSpot(3, 0);                            // 0x4ec120
    TinkerbellRaytraceScene::open("lost");           // 0x4ab0f0
    Ras *prev = ras_set_current(sprites);            // +0x134
    if (Raytrace::WallObject *w = rt.findWall("wall_LOST_WP_break02_36"))   // port: the exe calls on blindly
        w->addLink("breakb_ani", "LOST_WP_breakbani_", 100);
    if (Raytrace::WallObject *w = rt.findWall("wall_LOST_WP_wall02_01"))
        w->addLink("breakb_ani", "LOST_WP_wallbani_", 100);
    pirt.init(&rt);                                  // 0x4e6440
    buck.init(&rt);
    mudp.init(&rt);
    showAwards();
    ras_set_current(prev);
    activate();                                      // v6
    reset();                                         // 0x421110
    music_idx = 3;
}

// v20 0x443a40
void Lost_Scene_Base::leave() {
    State &st = Game::state();
    if (st.dusting) {
        st.dusting = false;
        bag->refund();                               // 0x4f07b0
    }
    VoiceOver::get().stop();                         // 0x4c9260
    pirt.release();                                  // 0x4e6530
    buck.release();
    mudp.release();
    Game::cannon().detach();                         // 0x412200
    over1.clear();                                   // 0x41ebc0
    if (music_idx != 3) music[music_idx]->close();   // 0x41fc50
    music_idx = 3;
    TinkerbellRaytraceScene::leave();                // 0x4ab470
}

void Lost_Scene_Base::playMusic(int n) {             // 0x443da0
    if (music_idx != 3) music[music_idx]->close();
    music_idx = n;
    if (n == 3) return;
    music[n]->close();
    music[n]->open();
    music[n]->play();
}

// 0x443c70: per game, "Info" in the save: 1 = the star, 2 = the star and the cap. The mud pie sign also shows the
// star after the last mini game (+0x79c) 2 ended as 2, and the cap after it ended as 3 with Info 1.
void Lost_Scene_Base::showAwards() {
    Saves &sv = Game::state().saves;
    switch (sv.gameInfo("Pirate Flag Tag Game")) {   // *0x543530
    case 1: if (pirt.star) pirt.star->on = true; break;
    case 2: if (pirt.star) pirt.star->on = true; if (pirt.cap) pirt.cap->on = true; break;
    default: break;
    }
    switch (sv.gameInfo("Bucket Toss Game")) {       // *0x543534
    case 1: if (buck.star) buck.star->on = true; break;
    case 2: if (buck.star) buck.star->on = true; if (buck.cap) buck.cap->on = true; break;
    default: break;
    }
    State &st = Game::state();
    switch (sv.gameInfo("Mud Pie Game")) {           // *0x543520
    case 0:
        if (st.last_game == 2 && st.last_result == 2 && mudp.star) mudp.star->on = true;
        break;
    case 1:
        if (mudp.star) mudp.star->on = true;
        if (st.last_game == 2 && st.last_result == 3 && mudp.cap) mudp.cap->on = true;
        break;
    case 2:
        if (mudp.star) mudp.star->on = true;
        if (mudp.cap) mudp.cap->on = true;
        break;
    default: break;
    }
}

// ---------------------------------------------------------------- the butterfly
// 0x4447d0 (on the RayTrace): the ground spot `dist` ahead, turned by `deg` from the view.
static void spot_ahead(Raytrace::RayTrace *rt, float out[3], float deg, float dist) {
    float C[3];
    rt->camPos(C);
    double a = (double)kPi * deg * kDegToRad + rt->angf;   // 0x42b550
    out[0] = (float)(cos(a) * dist + C[0]);
    out[1] = 0.1f;
    out[2] = (float)(sin(a) * dist + C[2]);
}

// 0x4464b0: three spots 300 ahead (40 degrees left, straight, 40 right) at height 500; the sprite starts on the
// ground under the first.
Butterfly::Butterfly(Raytrace::RayTrace *r) : rt(r) {
    float a[3], b[3], c[3];
    spot_ahead(rt, a, 40.0f, 300.0f);                // 0x5144c0, 0x5144b8
    float C[3];
    rt->camPos(C);
    b[0] = (float)(cos((double)rt->angf) * 300.0 + C[0]);
    b[2] = (float)(sin((double)rt->angf) * 300.0 + C[2]);
    spot_ahead(rt, c, -40.0f, 300.0f);               // 0x5144b0
    sprite = rt->addSprite("LOST_AM_butterflyr_", "LOST_AM_butterflyr_", -1);   // 0x42d9d0
    sprite->load();                                  // 0x42ef20
    sprite->attach("dust", "glob_am_ambspark_", 0);  // 0x42f390
    sprite->setPos(a[0], 0.1f, a[2]);                // 0x42f740
    p0[0] = a[0]; p0[1] = 500.0f; p0[2] = a[2];
    p1[0] = b[0]; p1[1] = 500.0f; p1[2] = b[2];
    p2[0] = c[0]; p2[1] = 500.0f; p2[2] = c[2];
    curve.set0(p0);
    curve.set1(p1);
    curve.set2(p2);
    t = 0;
}

void Butterfly::update() {                           // 0x446730
    if (!(t < 1.0f)) {
        curve.set0(p0);
        curve.set1(p1);
        curve.set2(p2);
        t = 0;
    }
    curve.at(t, pos);                                // 0x401120
    sprite->x = pos[0];
    sprite->y = pos[1];
    sprite->z = pos[2];
    t = (float)((double)t + 0.005f);                 // 0x5144c4
}

// ---------------------------------------------------------------- LostTutorial_Scene
LostTutorial_Scene::LostTutorial_Scene()             // 0x444440
    : Lost_Scene_Base("lost_tutorial"),
      cubby("tutorial_cubby", &rt, &lift), rabbit("tutorial_rabbit", &rt, &lift), foxy("tutorial_foxy", &rt, &lift),
      peter("tutorial_peter", &rt, &lift), raccoon1("tutorial_raccoon1", &rt, &lift),
      raccoon2("tutorial_raccoon2", &rt, &lift), skunk("tutorial_skunk", &rt, &lift) {
    for (Sound *&s : fx) s = new Sound("");          // 0x437290 x6 (named by load2)
}

LostTutorial_Scene::~LostTutorial_Scene() {
    for (Sound *&s : fx) { delete s; s = nullptr; }
    delete butterfly;                                // port: the exe never frees it
}

void LostTutorial_Scene::load2() {                   // v18 0x444580
    Lost_Scene_Base::load2();
    static const char *const names[6] = {"LOST_FX_bagappear", "LOST_FX_wrong", "LOST_FX_tinknoise",
                                         "LOST_FX_TNKgiggle", "LOST_FX_mapappear", "LOST_FX_magic"};   // 0x5377c0
    for (int i = 0; i < 6; i++) fx[i]->setName(names[i]);
}

void LostTutorial_Scene::spotAhead(float out[3], float deg, float dist) { spot_ahead(&rt, out, deg, dist); }

bool LostTutorial_Scene::lostBoy(Sprite3D *s) {
    return s == cubby.sprite() || s == foxy.sprite() || s == raccoon1.sprite() || s == raccoon2.sprite() ||
           s == skunk.sprite() || s == rabbit.sprite();
}

// A Lost Boy at `deg` from the view, 600 ahead, set up and registered for its lines.
static void place_boy(TutorialCharacter &c, Raytrace::RayTrace *rt, float deg, const char *const a[9]) {
    float C[3];
    rt->camPos(C);
    double ang = (double)kPi * deg * kDegToRad + rt->angf;
    float x = (float)(cos(ang) * 600.0 + C[0]);      // 0x514498
    float z = (float)(sin(ang) * 600.0 + C[2]);
    c.setX(x);
    c.setY(0.1f);
    c.setZ(z);
    c.setIdles(a[0], a[1], a[2]);                    // 0x4cf9a0
    c.setRootTalk(a[3]);                             // 0x4cfea0
    if (a[4]) c.setWave(a[4], a[5]);                 // 0x4d0060
    c.setFlyIdle(a[6]);                              // 0x4d0030
    c.setLip(a[7], a[8], 0);                         // 0x4cf310
    c.init();                                        // v1
}

// v19 0x444860
void LostTutorial_Scene::enter() {
    Lost_Scene_Base::enter();
    cub_pack = ras_open("LOST_CUB", "", &data_device());   // 0x419b90
    cub_pack->load();
    ras_set_current(cub_pack);
    fox_pack = ras_open("LOST_FOX", "", &data_device());
    fox_pack->load();
    ras_set_current(fox_pack);
    rab_pack = ras_open("LOST_RAB", "", &data_device());
    rab_pack->load();
    ras_set_current(rab_pack);
    peter_pack = ras_open("Peter_Char", "", &data_device());
    peter_pack->load();
    ras_set_current(peter_pack);
    rac_pack = ras_open("LOST_RAC", "", &data_device());
    rac_pack->load();
    ras_set_current(rac_pack);
    skn_pack = ras_open("LOST_SKN", "", &data_device());
    skn_pack->load();
    ras_set_current(skn_pack);
    VoiceOver &vo = VoiceOver::get();

    static const char *const cub[9] = {"GLOB_CA_CUBroot_", "GLOB_CA_CUBidle1_", "GLOB_CA_CUBidle2_",
                                       "GLOB_CA_CUBroottk_", "LOST_CA_CUBwave_", "LOST_VO_CUBhhiya",
                                       "GLOB_CA_CUBflyid_", "cubby_lip", "CUB_mout"};
    place_boy(cubby, &rt, 3.0f, cub);                // 0x51329c
    vo.registerLine("LOST_VO_CUBwhat", &cubby);      // 0x4c91b0
    vo.registerLine("LOST_VO_CUBhhiya", &cubby);

    static const char *const fox[9] = {"GLOB_CA_FOXroot_", "GLOB_CA_FOXidle1_", "GLOB_CA_FOXidle2_",
                                       "GLOB_CA_FOXroottk_", "LOST_CA_FOXwave_", "LOST_VO_FOXhhiya",
                                       "GLOB_CA_FOXflyid_", "foxy_lip", "FOX_mout"};
    place_boy(foxy, &rt, -6.2f, fox);                // 0x514494
    vo.registerLine("LOST_VO_FOXhhiya", &foxy);

    static const char *const rab[9] = {"GLOB_CA_RABroot_", "GLOB_CA_RABidle1_", "GLOB_CA_RABidle2_",
                                       "GLOB_CA_RABroottk_", "LOST_CA_RABwave_", "LOST_VO_RABhhiya",
                                       "GLOB_CA_RABflyid_", "echo_rabbit_lip", "RAB_mout"};
    place_boy(rabbit, &rt, -15.0f, rab);             // 0x514490
    vo.registerLine("LOST_VO_RABhhiya", &rabbit);

    static const char *const rac1[9] = {"GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_",
                                        "GLOB_CA_RACroottk_", "LOST_CA_RACwave_", "LOST_VO_RAChhiya",
                                        "GLOB_CA_RACflyid_", "echo_raccoon1_lip", "RAC_mout"};
    place_boy(raccoon1, &rt, 11.6f, rac1);           // 0x51448c
    vo.registerLine("LOST_VO_RAChhiya", &raccoon1);

    raccoon1.partner = &raccoon2;                    // +0x3d8c
    static const char *const rac2[9] = {"GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_",
                                        "GLOB_CA_RACroottk_", "LOST_CA_RACwave_", "LOST_VO_RAChhiya",
                                        "GLOB_CA_RACflyid_", "echo_raccoon2_lip", "RAC_mout"};
    place_boy(raccoon2, &rt, 18.2f, rac2);           // 0x514488

    static const char *const skn[9] = {"GLOB_CA_SKNroot_", "GLOB_CA_SKNidle1_", "GLOB_CA_SKNidle2_",
                                       "GLOB_CA_SKNroottk_", "LOST_CA_SKNwave_", "LOST_VO_SKNhhiya",
                                       "GLOB_CA_SKNflyid_", "echo_skunk_lip", "SKN_mout"};
    place_boy(skunk, &rt, 25.0f, skn);               // 0x514484
    vo.registerLine("LOST_VO_SKNhhiya", &skunk);

    {   // Peter: 500 ahead at -25 degrees, at the camera's height less 393.579, no wave
        float C[3];
        rt.camPos(C);
        double ang = (double)kPi * -25.0f * kDegToRad + rt.angf;   // 0x514480
        float x = (float)(cos(ang) * 500.0 + C[0]);  // 0x514478
        float z = (float)(sin(ang) * 500.0 + C[2]);
        peter.setX(x);
        peter.setY((float)(rt.eyeWorld() - 393.579f));   // 0x42b520, 0x514474
        peter.setZ(z);
        peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_");
        peter.setRootTalk("GLOB_CA_PETroottk_");
        peter.setFlyIdle("GLOB_CA_PETflyid_");
        peter.setLip("peter_lip", "PET_mout", 0);
        peter.init();
    }
    static const char *const lines[17] = {
        "LOST_VO_PETtgreet", "LOST_VO_PETtboys", "LOST_VO_PETtboyjb", "LOST_VO_PETpxdust", "LOST_VO_PETtbag",
        "LOST_VO_PETtdust", "LOST_VO_PETdustid", "LOST_VO_PETdustno", "LOST_VO_PETtbagid", "LOST_VO_PETtfull",
        "LOST_VO_PETtfulid", "LOST_VO_PETtfulex", "LOST_VO_PETtnkhlp", "LOST_VO_PETtnkid", "LOST_VO_PETtickle",
        "LOST_VO_PETtmap", "LOST_VO_PETtmapid"};
    for (const char *l : lines) vo.registerLine(l, &peter);
    vo.registerLine("LOST_VO_PETdustya", &peter);
    peter.keep_flying = true;                        // +0x202
    peter.state = 8;
    peter.play(peter.flyidle.c_str(), 2);            // 0x4cf940
    peter.settle();                                  // 0x4cfa10

    forceOff();                                      // v24
    bell.visible = false;                            // +0x2064
    bag->visible = false;                            // +0x1998 +0x7c
    map.visible = false;                             // +0x1a1c
    {   // Tink 600 ahead at -15 degrees, 700 up, kept from wandering
        float C[3];
        rt.camPos(C);
        double ang = (double)kPi * -15.0f * kDegToRad + rt.angf;
        float x = (float)(cos(ang) * 600.0 + C[0]);
        float z = (float)(sin(ang) * 600.0 + C[2]);
        tink->setX(x);
        tink->setY(700.0f);                          // 0x442f0000
        tink->setZ(z);
        tink->wanders = false;                       // +0x13c
    }
    rt.setCamera(rt.camx, 450.0f, rt.camz, (float)rt.angleDeg());   // 0x42a570 (0x42b470, 0x42b540, 0x42b560)
    for (Sound *s : fx) s->open();                   // 0x41f9d0(0)
    delete butterfly;                                // port: the exe leaks the last one
    butterfly = new Butterfly(&rt);                  // 0x4464b0
    butterfly_clicked = false;                       // +0x555c
    hide_cursor();                                   // 0x4119e0
    arriveThroughClouds(false);                      // 0x4ab760(0)
}

void LostTutorial_Scene::leave() {                   // v20 0x4454d0
    Lost_Scene_Base::leave();
    peter.release();                                 // v2
    cubby.release();
    foxy.release();
    rabbit.release();
    raccoon1.release();
    raccoon2.release();
    skunk.release();
    if (skn_pack) ras_close(skn_pack);               // 0x419620
    skn_pack = nullptr;
    if (rac_pack) ras_close(rac_pack);
    rac_pack = nullptr;
    if (cub_pack) ras_close(cub_pack);
    cub_pack = nullptr;
    if (fox_pack) ras_close(fox_pack);
    fox_pack = nullptr;
    if (rab_pack) ras_close(rab_pack);
    rab_pack = nullptr;
    if (peter_pack) ras_close(peter_pack);
    peter_pack = nullptr;
    for (Sound *s : fx) s->close();                  // 0x41fc50
}

// v25 0x4445c0: with the cursor showing. With dust on it: what the click sprinkled floats up (a Lost Boy marks
// the lesson done). Without: the bell, the bag, the map or the butterfly.
void LostTutorial_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (Game::state().dusting) {
        Sprite3D *s = nullptr;
        Raytrace::Attachment *a = nullptr;
        bag->sprinkle(&rt, &s, &a);                  // 0x4f0480
        if (!s) return;
        lift->add(s, 500);                           // 0x4bf0b0
        if (lostBoy(s)) boy_dusted = true;           // +0x555d
        return;
    }
    if (!mouse_down()) return;
    if (bell.click()) { bell_clicked = true; return; }   // 0x4e7180, +0x555f
    if (bag->click()) { bag_clicked = true; return; }    // 0x4f02f0, +0x555e
    if (map.hit(g_mouse.x, g_mouse.y)) { map_clicked = true; return; }   // +0x5560
    if (rt.hitSprite(rt.findSprite("LOST_AM_butterflyr_"), g_mouse.x, g_mouse.y))   // 0x42c7b0
        butterfly_clicked = true;                    // +0x555c
}

// v26 0x444770: Escape skips the tutorial (to the hideout).
bool LostTutorial_Scene::onKey() {
    if (!g_key.down) return false;
    if (Game::state().key((int)g_key.vk)) return true;   // 0x4baef0
    if ((int)g_key.vk == Game::state().skip_key) {   // +0x74
        Game::state().start_in_london = false;       // +0x1818
        Game::state().tutorial_skipped = true;       // +0x7bc
        change_scene("lost");                        // 0x41ddf0
        return true;
    }
    return false;
}

// v30 0x4455e0: the base's choice (or the game's cursor), the dust cursor while dusting; set, keeping it shown or
// hidden.
Vision::Graphic *LostTutorial_Scene::cursorUpdate() {
    Vision::Graphic *c = Game::state().cursor;       // +0x5c
    if (Vision::Graphic *b = TinkerbellRaytraceScene::cursorUpdate()) c = b;   // 0x4ab6b0
    if (Game::state().dusting) {
        pixie.visible = true;                        // +0x1854
        c = &pixie;
    } else {
        pixie.visible = false;
    }
    set_cursor(c, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
    return c;
}

void LostTutorial_Scene::waitTalk() {                // 0x4bcb50 then 0x420e50(0x1e)
    while (Game::state().vo_busy()) wait(30);
}

// "every 15 s without the step done, the reminder"
static void stage(const char *s) { if (getenv("TB_DEBUG_LOST")) tb_log("lost: %s (frame %d)", s, tb_input_frame()); }   // test aid

#define REMIND(line) do { if (idleTime() > 15000) { vo.play(line); resetIdle(); } } while (0)   // Peter's 0x4cca20

// v10 0x445650
void LostTutorial_Scene::run() {
    wait(1);
    while (arriveThroughClouds(false)) {             // 0x4ab760
        peter.setY((float)(rt.eyeWorld() - 393.579f));
        wait(30);
    }
    peter.keep_flying = false;                       // +0x3b8a
    VoiceOver &vo = VoiceOver::get();
    vo.play("LOST_VO_PETtgreet");                    // 0x4cca20
    waitTalk();
    vo.play("LOST_VO_CUBwhat");
    waitTalk();
    skunk.toRoot();                                  // 0x4d00d0
    raccoon1.toRoot();
    raccoon2.toRoot();
    cubby.toRoot();
    foxy.toRoot();
    rabbit.toRoot();
    peter.talk_anim = "GLOB_CA_PETtalk3_";           // +0x3b8c
    vo.play("LOST_VO_PETtboys");
    waitTalk();
    while (!(skunk.atRoot() && raccoon1.atRoot() && raccoon2.atRoot() && cubby.atRoot() && foxy.atRoot() &&
             rabbit.atRoot()))                       // 0x4d00f0
        wait(30);
    vo.play("LOST_VO_Boyshhiya");                    // 0x4c9400
    skunk.wave();                                    // 0x4d00b0
    raccoon1.wave();
    raccoon2.wave();
    cubby.wave();
    foxy.wave();
    rabbit.wave();
    while (Game::state().vo_busy()) wait(30);
    vo.play("LOST_VO_PETtboyjb");
    waitTalk();
    vo.play("LOST_VO_PETpxdust");
    waitTalk();

    stage("tink to the bag");
    // Tink flies to the bag's corner and points at it; the bag and the bell show
    float p[3];
    spotAhead(p, 17.0f, 250.0f);                     // 0x4447d0
    tink->pushBasic(2, p[0], 300.0f, p[2]);          // 0x4de020
    while (!tink->idle()) wait(30);                  // 0x4de010
    spotAhead(p, 15.0f, 250.0f);
    tink->point(p[0], 200.0f, p[2]);                 // 0x4db7b0
    wait(500);
    bag->visible = true;                             // +0x1998 +0x7c
    bell.visible = true;                             // +0x2064
    fx[0]->play();                                   // "LOST_FX_bagappear"
    {
        float C[3];
        rt.camPos(C);
        double ang = (double)kPi * -15.0f * kDegToRad + rt.angf;
        float x = (float)(cos(ang) * 600.0 + C[0]);
        float z = (float)(sin(ang) * 600.0 + C[2]);
        tink->pushBasic(2, x, 700.0f, z);            // back up out of the way
    }
    vo.play("LOST_VO_PETtbag");
    waitTalk();

    stage("take dust");
    // take dust from the bag
    show_cursor();                                   // 0x4119d0
    boy_dusted = false;
    resetIdle();                                     // 0x4a9c90
    while (!Game::state().dusting) {
        REMIND("LOST_VO_PETtbagid");
        wait(30);
    }
    vo.play("LOST_VO_PETtdust");
    waitTalk();
    stage("sprinkle a boy");
    // sprinkle a Lost Boy
    while (!boy_dusted) {
        resetIdle();
        while (Game::state().dusting) {
            if (boy_dusted) goto take_again;
            REMIND("LOST_VO_PETdustid");
            wait(30);
        }
        if (!boy_dusted) {                           // thrown on something else
            fx[1]->play();                           // "LOST_FX_wrong"
            vo.play("LOST_VO_PETdustno");
            waitTalk();
        }
    take_again:
        resetIdle();
        while (!Game::state().dusting) {
            if (boy_dusted) goto dusted;
            REMIND("LOST_VO_PETtbagid");
            wait(30);
        }
    }
dusted:
    stage("dusted");
    hide_cursor();                                   // 0x4119e0
    {
        State &st = Game::state();
        if (st.dusting) {
            st.throwdust->close();                   // +0x7c
            st.throwdust->open();
            st.throwdust->play();
            st.dusting = false;
        }
    }
    vo.play("LOST_VO_PETdustya");
    waitTalk();

    stage("butterfly");
    // the butterfly: fill the bag
    butterfly->update();                             // 0x446730
    vo.play("LOST_VO_PETtfull");
    wait(2000);
    while (Game::state().vo_busy()) {
        butterfly->update();
        wait(30);
    }
    show_cursor();
    resetIdle();
    while (!butterfly_clicked) {
        if (getenv("TB_LOST_AUTO") && idleTime() > 3000) butterfly_clicked = true;   // test aid: "caught"
        butterfly->update();
        REMIND("LOST_VO_PETtfulid");
        wait(60);
    }
    stage("butterfly caught");
    fx[5]->play();                                   // "LOST_FX_magic"
    bag->fill();                                     // 0x4f0770
    vo.play("LOST_VO_PETtfulex");
    for (;;) {                                       // until it is out of the view (39 degrees off its centre)
        Sprite3D *s = butterfly->sprite;
        float bx = s->x, bz = s->z;                  // 0x42f700
        double a = atan2((double)bz - rt.camz, (double)bx - rt.camx) - rt.angf;   // 0x42cf90
        if (!(a > -3.14159274f)) a += 3.14159274f + 3.14159274f;   // 0x513d28
        else if (!(a < 3.14159274f)) a -= 3.14159274f + 3.14159274f;
        if (!((double)kPi * 0.216666669f > fabs(a))) break;   // 0x5144ac
        butterfly->update();
        wait(30);
    }
    waitTalk();

    stage("bell");
    // Tink's help: the bell
    tink->moreDust();                                // 0x4db1a0
    fx[2]->play();                                   // "LOST_FX_tinknoise"
    vo.play("LOST_VO_PETtnkhlp");
    while (Game::state().vo_busy()) {
        if (fx[2]->isPlaying()) tink->moreDust();    // 0x41fec0
        wait(30);
    }
    bell_clicked = false;                            // +0x555f
    show_cursor();
    resetIdle();
    while (!bell_clicked) {
        REMIND("LOST_VO_PETtnkid");
        wait(30);
    }
    stage("bell rung");
    hide_cursor();
    fx[3]->play();                                   // "LOST_FX_TNKgiggle"
    vo.play("LOST_VO_PETtickle");
    {   // she points right, left and up around herself
        Sprite3D *ts = tink->sprite();
        float tx = ts->x, ty = ts->y, tz = ts->z;    // 0x42f700
        tink->point((float)((double)tx + 100.0f), ty, tz);   // 0x513da0
        tink->point((float)((double)tx - 100.0f), ty, tz);
        tink->point(tx, (float)((double)ty + 100.0f), tz);
    }
    while (Game::state().vo_busy() || !tink->idle()) wait(30);

    stage("map");
    // the map
    fx[4]->play();                                   // "LOST_FX_mapappear"
    map.visible = true;                              // +0x1a1c
    vo.play("LOST_VO_PETtmap");
    waitTalk();
    map_clicked = false;                             // +0x5560
    show_cursor();
    resetIdle();
    while (!map_clicked) {
        REMIND("LOST_VO_PETtmapid");
        wait(30);
    }
    change_scene("map_tutorial");                    // 0x41ddf0
}

#undef REMIND

}  // namespace Game
