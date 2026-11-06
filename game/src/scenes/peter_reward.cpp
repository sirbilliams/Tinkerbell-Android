// PeterCapReward and PeterPrompt, reconstructed from "Tinker Bell.exe" (see peter_reward.h).
#include "peter_reward.h"
#include "scene_char.h"
#include "raytrace_scene.h"
#include "state.h"
#include "voacts.h"
#include <math.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

// Both visits bring their own Peter when the scene has none (the same setup in both).
static SceneCharacter *own_peter(Raytrace::RayTrace *rt, DustLift **lift, Ras **pack) {
    static std::vector<std::string> no_wins;         // 0x631088 / 0x6310a8 (two empty statics in the exe)
    *pack = ras_open("Peter_Char", "", &data_device());   // 0x419b90
    (*pack)->load();                                 // 0x41aa20
    ras_set_current(*pack);                          // 0x41a920
    SceneCharacter *p = Gorilla::gnew<SceneCharacter>("peterPan", rt, lift);   // 0x4ccad0
    p->setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_", "GLOB_CA_PETidle3_");
    p->setTalks("GLOB_CA_PETroottk_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_");
    p->setWave("GLOB_CA_PETwin_", &no_wins);
    p->setFlyIdle("GLOB_CA_PETflyid_");
    p->setLip("firefly_peterPan_lip", "PET_mout", 0);
    p->init();                                       // v1
    return p;
}

static void wait_fade(Task *t, TinkerbellRaytraceScene *sc) {
    while (sc->fader->busy()) t->wait(30);           // 0x4a95c0
}

static void wait_talk(Task *t) {
    while (Game::state().vo_busy()) t->wait(30);     // 0x4bcb50
}

// ---------------------------------------------------------------- PeterCapReward
PeterCapReward::PeterCapReward(SceneCharacter *p, TinkerbellRaytraceScene *sc, int a, DustLift *l)   // 0x4bf280
    : Task(nullptr), peter(p), lift(l), scene(sc), rt(&sc->rt), area(a) {
    cap = rt->addSprite("reward cap", "GLOB_GS_cap_", 0);   // 0x42d9d0
    cap->load();                                     // 0x42ef20
    curve = new Raytrace::Curve3();                  // 0x401000
}

PeterCapReward::~PeterCapReward() {                  // 0x4bf3b0
    if (own) {
        own->release();                              // v2
        delete own;                                  // v0
    }
    own = nullptr;
    peter = nullptr;
    cap = rt->findSprite("reward cap");
    if (cap) rt->removeSprite("reward cap");         // 0x42da60
    cap = nullptr;
    if (pack) ras_close(pack);                       // 0x419620
    pack = nullptr;
    delete curve;
}

// 0x4bf480: the camera kept and moved to the area's spot 0; Peter 500 ahead on the ground (his place kept); the
// cap's curve from above him at the eye's height, through the half-way point, to 250 ahead of the camera.
void PeterCapReward::setup() {
    rt->camPos(cam);                                 // 0x42b490 (+0xdc)
    cam_deg = (float)rt->angleDeg();                 // 0x42b560 (+0xe8)
    Game::state().setCameraToSpot(area, 0, rt);      // 0x4ec140
    if (!peter) {
        own = own_peter(rt, &lift, &pack);           // +0xc0, +0xc8
        peter = own;
    }
    VoiceOver &vo = VoiceOver::get();
    vo.registerLine("GLOB_VO_PETrecord", peter);     // 0x4c91b0 (the magpie at +0x110)
    vo.registerLine("GLOB_VO_PETsignin", peter);
    float C[3];
    rt->camPos(C);
    float a = rt->angf;                              // 0x42b550
    peter_pos[0] = peter->getX();                    // 0x4cc9b0 (+0xec)
    peter_pos[1] = peter->getY();                    // 0x4cc9c0
    peter_pos[2] = peter->getZ();                    // 0x4cc9d0
    float nz = (float)(sin(a) * 500.0 + C[2]);       // 0x514478
    float nx = (float)(cos(a) * 500.0 + C[0]);
    peter->setX(nx);
    peter->setY(0.1f);
    peter->setZ(nz);
    float p1[3] = {nx, 0.1f, nz};
    float eye = (float)rt->eyeWorld();               // 0x42b520
    float z2 = (float)(sin((double)rt->angf) * 250.0 + rt->camz);   // 0x5144a0, 0x42b540
    float eye2 = (float)rt->eyeWorld();
    float x2 = (float)(cos((double)rt->angf) * 250.0 + rt->camx);   // 0x42b470
    float p2[3] = {x2, eye2, z2};
    float d[3], h[3], m[3];
    for (int i = 0; i < 3; i++) d[i] = p2[i] - p1[i];                 // 0x425eb0
    for (int i = 0; i < 3; i++) h[i] = d[i] / 2.0f;                   // 0x425f10
    for (int i = 0; i < 3; i++) m[i] = p1[i] + h[i];                  // 0x425e50
    float p0[3] = {nx, eye, nz};
    curve->set0(p0);                                 // 0x401040
    curve->set1(m);                                  // 0x401060
    curve->set2(p2);                                 // 0x401080
}

// 0x4bf990: the cap along the curve, a second in all; false once past it.
bool PeterCapReward::capStep() {
    uint32_t dt = frame_time() - t0;                 // +0xfc
    float t = (float)((double)(uint64_t)dt * 0.001f);   // 0x513294 (fild qword)
    if (!(t < 1.0f)) return false;                   // 0x513710
    float p[3];
    curve->at(t, p);                                 // 0x401120
    cap->setPos(p[0], p[1], p[2]);                   // 0x42f740
    return true;
}

// v10 0x4bfa60: the cap saved; the scene paused in the dark: the camera and Peter set, light again, Peter's
// "record" line, the cap flies at the player, 3 s, it goes (removed under another name: the exe's quirk, so it
// is only unloaded), Peter's "sign in" line, dark, everything put back, light, the scene goes on.
void PeterCapReward::run() {
    wait(1);
    Game::state().saves.setPetersCap(true);          // 0x4eacf0
    scene->paused = true;                            // +0x128
    scene->forceOff();                               // v24
    scene->fader->start(false, 1000);                // 0x4a9580
    wait_fade(this, scene);
    setup();                                         // 0x4bf480
    scene->fader->start(true, 1000);
    wait_fade(this, scene);
    VoiceOver &vo = VoiceOver::get();
    vo.play("GLOB_VO_PETrecord");                    // 0x4cca20
    wait_talk(this);
    t0 = frame_time();                               // +0xfc
    while (capStep()) wait(30);
    wait(3000);
    rt->removeSprite("GLOB_GA_cap_");                // 0x42da60 (not the cap's name)
    cap->unload();                                   // 0x42f2b0
    cap = nullptr;
    vo.play("GLOB_VO_PETsignin");
    wait_talk(this);
    scene->fader->start(false, 1000);
    wait_fade(this, scene);
    rt->setCamera(cam[0], cam[1], cam[2], cam_deg);  // 0x42a760
    peter->setX(peter_pos[0]);
    peter->setY(peter_pos[1]);
    peter->setZ(peter_pos[2]);
    scene->fader->start(true, 1000);
    wait_fade(this, scene);
    scene->paused = false;
    scene->forceNone();                              // v23
    deactivate();                                    // +0x1c
}

// ---------------------------------------------------------------- PeterPrompt
PeterPrompt::PeterPrompt(SceneCharacter *p, TinkerbellRaytraceScene *sc, int a, DustLift *l)   // 0x4bfea0
    : Task(nullptr), peter(p), scene(sc), rt(&sc->rt), area(a), lift(l) {
    mouse.setClient(this);                           // 0x41f290
    g_modal_client = this;                           // 0x41f370
}

PeterPrompt::~PeterPrompt() {                        // 0x4bff80
    g_modal_client = nullptr;                        // 0x41f380
    mouse.deactivate();                              // +0x1c
    if (own) {
        own->release();
        delete own;
    }
    own = nullptr;
    peter = nullptr;
    if (pack) ras_close(pack);
    pack = nullptr;
}

// v9 0x4c0040: the map icon clicked (and opened): the line stops, to the treasure map.
void PeterPrompt::notify2(Service *) {
    if (!(g_mouse.ldown || g_mouse.rdown)) return;   // 0x5a6e30
    if (!scene->map.hit(g_mouse.x, g_mouse.y)) return;   // +0x19a0 v11
    if (scene->map.click(g_mouse.x, g_mouse.y, false) != 2) return;   // 0x4efc90
    VoiceOver::get().stop();                         // 0x4c9260
    change_scene("map_treasure");                    // 0x41ddf0
}

// 0x4c00c0: the camera to the area's spot 0; Peter 500 ahead (his height kept).
void PeterPrompt::setup() {
    Game::state().setCameraToSpot(area, 0, rt);
    if (!peter) {
        own = own_peter(rt, &lift, &pack);           // +0xf4, +0xfc
        peter = own;
    }
    VoiceOver &vo = VoiceOver::get();
    vo.registerLine("WAMI_VO_PETfive", peter);
    vo.registerLine("WAMI_VO_PETfiveid", peter);
    float C[3];
    rt->camPos(C);
    float a = rt->angf;
    Sprite3D *s = peter->sprite();
    float y = s->y;                                  // 0x42f700
    float x = (float)(cos(a) * 500.0 + C[0]);
    float z = (float)(sin(a) * 500.0 + C[2]);
    s->setPos(x, y, z);                              // 0x42f740
}

// v10 0x4c03c0: in the dark, set up; the map icon shown; light; Peter's line, again every 15 s until the map is
// opened (notify2 leaves the scene).
void PeterPrompt::run() {
    wait(1);
    mouse.activate();                                // +0x18
    scene->paused = true;
    scene->forceOff();
    scene->fader->start(false, 1000);
    wait_fade(this, scene);
    setup();                                         // 0x4c00c0
    scene->map.visible = true;                       // +0x1a1c
    scene->fader->start(true, 1000);
    wait_fade(this, scene);
    VoiceOver &vo = VoiceOver::get();
    vo.play("WAMI_VO_PETfive");
    said = frame_time();                             // +0x10c
    for (;;) {
        if (frame_time() - said > 15000) {           // 0x3a98
            said = frame_time();
            vo.play("WAMI_VO_PETfiveid");
        }
        wait(30);
    }
}

}  // namespace Game
