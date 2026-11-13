// Mermaid_Scene, reconstructed from "Tinker Bell.exe" (see mermaid.h).
#include "mermaid.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "voiceover.h"
#include "cannon.h"
#include "peter_reward.h"
#include "dustlift.h"
#include "toolbar.h"
#include "tb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool cur_vis() { return g_cursor_graphic ? g_cursor_graphic->visible : false; }   // (each set_cursor's)
static bool dbg_on() { return getenv("TB_DEBUG_MERMAID") != nullptr; }   // test aid

std::vector<std::string> Mermaid_Scene::win_lines;  // 0x5c3168

Mermaid_Scene &mermaid_scene() {                     // the static at 0x5ba320
    static Mermaid_Scene s("Mermaid");
    return s;
}

static const char *const kGround[5] = {"Shell 0", "Shell 1", "Shell 2", "Shell 3", "Shell 4"};   // 0x538740

// ---------------------------------------------------------------- MermaidTokensInterface
// 0x44d6c0. port: the icons are made with their names (the exe names its embedded ones after making them).
MermaidTokensInterface::MermaidTokensInterface()
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),   // 0x411b90
      bar("SHEL_IF_puzbar_01", 0, 0, 0, 0, 0, 0, true), buzz("SHEL_FX_buzz") {   // 0x4179e0, 0x420210
    static const char *const n[5] = {"SHEL_IF_puzshell01_", "SHEL_IF_puzshell02_", "SHEL_IF_puzshell03_",
                                     "SHEL_IF_puzshell04_", "SHEL_IF_puzshell05_"};
    for (int i = 0; i < 5; i++) icons[i] = new Animation(n[i]);   // 0x452d70 + 0x424400
}

MermaidTokensInterface::~MermaidTokensInterface() {  // 0x44d830
    for (Animation *&a : icons) { delete a; a = nullptr; }
}

void MermaidTokensInterface::load() {                // v15 0x44d8d0
    bar.load();
    for (Animation *a : icons) a->load();
    buzz.open();                                     // 0x41f9d0(0)
}

void MermaidTokensInterface::unload() {              // v16 0x44d910
    Graphic::unload();                               // 0x412020
    bar.unload();
    for (Animation *a : icons) a->unload();
    buzz.close();                                    // 0x41fc50
}

// v14 0x44d960: with a shell won and Pearl Toss off: the bar and each shell won.
void MermaidTokensInterface::draw() {
    State &st = Game::state();
    if (!st.anyMermaid(4)) return;                   // 0x4ef680
    if (mermaid_scene().pearl.active()) return;      // 0x5bcc20 (+0x2900)
    bar.draw();                                      // v14
    for (int i = 0; i < 5; i++)
        if (st.mcov[i].state == 4) icons[i]->draw();
}

// 0x4de8d0 with the scene's functor (MermaidTokenContainer, vtable 0x5147d4: any 0x44d4f0, target 0x44d520): with
// a shell still hidden (state 1), Tink guides to the first one whose sprite is there.
static bool mermaid_token_help(Mermaid_Scene *sc) {
    sc->tink->clearQueue();                          // 0x4ddf30
    State &st = Game::state();
    bool any = false;                                // 0x44d4f0
    for (const TokenInfo &t : st.mcov)
        if (t.state == 1) any = true;
    if (!any) return false;
    Sprite3D *s = nullptr;                           // 0x44d520
    for (int i = 0; i < 5 && !s; i++)
        if (st.mcov[i].state == 1) s = sc->rt.findSprite(kGround[i]);   // 0x42dc70
    sc->tink->guideTo(s);                            // 0x4dc250
    return true;
}

// 0x4abf90: the help's dust paid, if the bag has it.
static bool pay_help(Mermaid_Scene *sc) {
    if (!sc->bag->has(PixieDustBag::cost2())) return false;   // 0x4f08c0, 0x4f02d0
    sc->bag->spend();                                // 0x4f0860
    return true;
}

static bool cap_due() {
    Saves &sv = Game::state().saves;
    return sv.hasCap() && !sv.hasPetersCap();        // 0x4e9af0, 0x4ead40
}

// ---------------------------------------------------------------- the scene
// 0x44d9c0
Mermaid_Scene::Mermaid_Scene(const char *nm)
    : TinkerbellRaytraceScene(nm),
      pearl(&rt),                                    // 0x4546b0
      shell(&rt),                                    // 0x456750
      ambient(&rt, nm),                              // 0x4df580
      pearl_click("PERL_VO_CUBgreet1", "PERL_VO_CUBrecrd1", "PERL_VO_CUBfun1", "PERL_VO_CUBgreet2",
                  "PERL_VO_CUBrecrd2", "PERL_VO_CUBfun2", "PEARL_TOSS_CHAR", "", "", ""),   // 0x4c0990
      coco_click("MCOV_VO_RABgreet1", "MCOV_VO_RABrecrd1", "MCOV_VO_RABfun1", "MCOV_VO_RABgreet2",
                 "MCOV_VO_RABrecrd2", "MCOV_VO_RABfun2", "COCONUTS_AWAY_CHAR", "", "", ""),
      shell_click("SHEL_VO_SKNgreet1", "SHEL_VO_SKNrecrd1", "SHEL_VO_SKNfun1", "SHEL_VO_SKNgreet2",
                  "SHEL_VO_SKNrecrd2", "SHEL_VO_SKNfun2", "SHELL_CHAR", "SHEL_VO_SKNmore1", "SHEL_VO_SKNmore2",
                  "SHEL_VO_SKNoverly"),
      rabbit("coconutsAway_rabbit", &rt, &lift),     // 0x4ccad0
      skunk("shell_skunk", &rt, &lift),
      cubby("pearlToss_cubby", &rt, &lift),
      mappiece(&rt),                                 // 0x4e7500
      puzpieces("MCOV_LY_puzpieces_01", 0, 0, 0, 0, 0, 0, true) {   // 0x4179e0
    for (Sound *&m : tunes) m = new Sound("");       // 0x437290 x3 (named by load2)
    for (Sound *&f : fx) f = new Sound("");
    pearl_end.setLines("PERL_VO_CUBtoobad", "PERL_VO_CUBuwin1", "PERL_VO_CUBuwin2", "PERL_VO_CUBuwin3",
                       "PERL_VO_CUBchalgm", "PERL_VO_CUBpuzzgm");   // 0x4c1900
    coco_end.setLines("MCOV_VO_RABtoobad", "MCOV_VO_RABuwin1", "MCOV_VO_RABuwin2", "MCOV_VO_RABuwin3",
                      "MCOV_VO_RABviewgm", "MCOV_VO_RABpuzzgm");
    shell_end.setLines("SHEL_VO_SKNuwin1", "SHEL_VO_SKNother", "SHEL_VO_SKNtoobad", "SHEL_VO_SKNuwin2",
                       "SHEL_VO_SKNuwin3");          // 0x4c2a10
    shell_end.setPiece(&mappiece);                   // 0x4c2ad0
    inactivity.init(&rt, 30000);                     // 0x4c70e0
    inactivity.add(0, 1000.0f, "GLOB_VO_RABidle1", "GLOB_VO_RABidle2", "GLOB_VO_RABidle3", "");   // 0x4c7100
    inactivity.add(1, 1000.0f, "GLOB_VO_CUBidle1", "GLOB_VO_CUBidle2", "GLOB_VO_CUBidle3", "");
    inactivity.add(2, 1000.0f, "GLOB_VO_SKNidle1", "GLOB_VO_SKNidle2", "GLOB_VO_SKNidle3", "Shell Game");
}

Mermaid_Scene::~Mermaid_Scene() {                    // 0x44e480 (port: the exe's static is never destroyed)
    delete reward;
    delete prompt;
    for (Sound *&m : tunes) { delete m; m = nullptr; }
    for (Sound *&f : fx) { delete f; f = nullptr; }
}

// v18 0x44e9b0: the tunes and sounds named, the signs' games, the characters into the scene's list, the win lines
// (appended again on every call: the exe's one-time guard covers only the vector's construction), each
// character's animations and lines.
void Mermaid_Scene::load2() {
    static const char *const tn[3] = {"PERL_MU_music", "SHEL_MU_music", "MCOV_MU_music"};   // 0x5385ac
    for (int i = 0; i < 3; i++) {
        tunes[i]->setName(tn[i]);                    // 0x424400
        tunes[i]->mode = 2;                          // +0x338
        tunes[i]->type = 6;                          // +0x340
    }
    static const char *const fn[3] = {"glob_fx_gsclick", "shel_fx_grabpuz", "mcov_ax_gull"};   // 0x538754
    for (int i = 0; i < 3; i++) fx[i]->setName(fn[i]);
    coco_sign_click.set("Coconuts Away Game", "MCOV_VO_RABgoinfo", "MCOV_VO_RABgo2", "", nullptr);   // 0x4c7f00
    pearl_sign_click.set("Pearl Toss Game", "PERL_VO_CUBgoinfo", "PERL_VO_CUBgo2", "", nullptr);
    shell_sign_click.set("Shell Game", "SHEL_VO_SKNgoinfo", "SHEL_VO_SKNgo2", "SHEL_VO_SKNnogo", nullptr);
    chars.push_back(&rabbit);                        // 0x430dd0 on +0x2820
    chars.push_back(&skunk);
    chars.push_back(&cubby);
    static const char *const wins[9] = {"PERL_VO_CUBuwin1", "PERL_VO_CUBuwin2", "PERL_VO_CUBuwin3",
                                        "MCOV_VO_RABuwin1", "MCOV_VO_RABuwin2", "MCOV_VO_RABuwin3",
                                        "SHEL_VO_SKNuwin1", "SHEL_VO_SKNuwin2", "SHEL_VO_SKNuwin3"};
    for (const char *w : wins) win_lines.push_back(w);   // 0x443260
    VoiceOver &vo = VoiceOver::get();

    rabbit.setIdles("GLOB_CA_RABroot_", "GLOB_CA_RABidle1_", "GLOB_CA_RABidle2_", "GLOB_CA_RABidle3_");   // 0x4cd650
    rabbit.setTalks("GLOB_CA_RABroottk_", "GLOB_CA_RABtalk1_", "GLOB_CA_RABtalk2_", "GLOB_CA_RABtalk3_");   // 0x4ce2d0
    rabbit.setWave("GLOB_CA_RABwin_", &win_lines);   // 0x4ce880
    rabbit.setFlyIdle("GLOB_CA_RABflyid_");          // 0x4ce850
    rabbit.eyes = "RAB_eyes";                        // +0x84f0
    rabbit.setLip("coconutsAway_rabbit_lip", "RAB_mout", 0);   // 0x4ccd20
    static const char *const rab[] = {
        "MCOV_VO_RABcall1", "MCOV_VO_RABcall2", "MCOV_VO_RABcall3", "MCOV_VO_RABfun1", "MCOV_VO_RABgo2",
        "MCOV_VO_RABgoinfo", "MCOV_VO_RABgreet1", "MCOV_VO_RABpuzzgm", "MCOV_VO_RABrecrd1", "MCOV_VO_RABtoobad",
        "MCOV_VO_RABuwin1", "MCOV_VO_RABuwin2", "MCOV_VO_RABuwin3", "MCOV_VO_RABviewgm", nullptr};
    for (const char *const *l = rab; *l; l++) vo.registerLine(*l, &rabbit);   // 0x4c91b0 (+0x84e4)

    skunk.setIdles("GLOB_CA_SKNroot_", "GLOB_CA_SKNidle1_", "GLOB_CA_SKNidle2_", "GLOB_CA_SKNidle3_");
    skunk.setTalks("GLOB_CA_SKNroottk_", "GLOB_CA_SKNtalk1_", "GLOB_CA_SKNtalk2_", "GLOB_CA_SKNtalk3_");
    skunk.setWave("GLOB_CA_SKNwin_", &win_lines);
    skunk.setFlyIdle("GLOB_CA_SKNflyid_");
    skunk.eyes = "SKN_eyes";                         // +0x8780
    skunk.setLip("shell_skunk_lip", "SKN_mout", 0);
    static const char *const skn[] = {
        "SHEL_VO_SKNcall1", "SHEL_VO_SKNcall2", "SHEL_VO_SKNcall3", "SHEL_VO_SKNcall4", "SHEL_VO_SKNfun1",
        "SHEL_VO_SKNgo2", "SHEL_VO_SKNgoinfo", "SHEL_VO_SKNgreet1", "SHEL_VO_SKNmixup", "SHEL_VO_SKNmore1",
        "SHEL_VO_SKNnice1", "SHEL_VO_SKNnice2", "SHEL_VO_SKNnice3", "SHEL_VO_SKNnice4", "SHEL_VO_SKNnogo",
        "SHEL_VO_SKNnright", "SHEL_VO_SKNother", "SHEL_VO_SKNoverly", "SHEL_VO_SKNprompt", "SHEL_VO_SKNrecrd1",
        "SHEL_VO_SKNshhelp", "SHEL_VO_SKNtoobad", "SHEL_VO_SKNuwin1", "SHEL_VO_SKNuwin2", "SHEL_VO_SKNuwin3",
        nullptr};
    for (const char *const *l = skn; *l; l++) vo.registerLine(*l, &skunk);   // (+0x8774)

    cubby.setIdles("GLOB_CA_CUBroot_", "GLOB_CA_CUBidle1_", "GLOB_CA_CUBidle2_", "GLOB_CA_CUBidle3_");
    cubby.setTalks("GLOB_CA_CUBroottk_", "GLOB_CA_CUBtalk1_", "GLOB_CA_CUBtalk2_", "GLOB_CA_CUBtalk3_");
    cubby.setWave("GLOB_CA_CUBwin_", &win_lines);
    cubby.setFlyIdle("GLOB_CA_CUBflyid_");
    cubby.eyes = "CUB_eyes";                         // +0x8a10
    cubby.setLip("pearlToss_cubby_lip", "CUB_mout", 0);
    static const char *const cub[] = {
        "PERL_VO_CUB1more", "PERL_VO_CUBcall1", "PERL_VO_CUBcall2", "PERL_VO_CUBcall3", "PERL_VO_CUBchalgm",
        "PERL_VO_CUBfun1", "PERL_VO_CUBgo2", "PERL_VO_CUBgoinfo", "PERL_VO_CUBgreet1", "PERL_VO_CUBhalfwy",
        "PERL_VO_CUBhelp1", "PERL_VO_CUBhelp2", "PERL_VO_CUBnice1", "PERL_VO_CUBnice2", "PERL_VO_CUBnice3",
        "PERL_VO_CUBnice4", "PERL_VO_CUBprompt", "PERL_VO_CUBpuzzgm", "PERL_VO_CUBrecrd1", "PERL_VO_CUBtoobad",
        "PERL_VO_CUBuwin1", "PERL_VO_CUBuwin2", "PERL_VO_CUBuwin3", nullptr};
    for (const char *const *l = cub; *l; l++) vo.registerLine(*l, &cubby);   // (+0x8a04)
}

// v19 0x44feb0: the area's camera spot (then spot 0 for next time), the scene, the end-of-game tasks' bag, the
// eye's limit, the signs, the mermaids, the Shell Game's log, the shells on the ground, the shells' bar, the beach
// barriers, the sounds, the signs' hot regions, the characters, the map piece, the pictures, the inactivity
// spots, the arrival, the gulls and the tune.
void Mermaid_Scene::enter() {
    reward = nullptr;                                // +0x8d10
    prompt = nullptr;                                // +0x8d14
    State &st = Game::state();
    float sp[4];
    st.areaSpot(5, sp);                              // 0x4ec0a0
    cam_x = sp[0]; cam_eye = sp[1]; cam_z = sp[2]; cam_deg = sp[3];   // 0x4aa0d0
    st.setAreaSpot(5, 0);                            // 0x4ec120
    open(name());                                    // v27 (TinkerbellRaytraceScene's 0x4ab0f0)
    pearl_end.setBag(bag);                           // 0x4c19d0
    coco_end.setBag(bag);
    shell_end.setBag(bag);                           // 0x4c2ac0
    Raytrace::RayTrace::setMaxEye(0xe6);             // 0x42b110
    signs();                                         // 0x4522b0
    mermaids();                                      // 0x452160
    activate();                                      // v6
    shell.setup();                                   // 0x4569a0
    tune = 3;                                        // +0x6bf8
    if (const char *t = getenv("TB_MERMAID_TOKENS")) // test aid (not in the game): the five shells' states
        for (int i = 0; i < 5 && t[i]; i++) st.mcov[i].state = t[i] - '0';
    groundTokens();                                  // 0x4526a0
    over1.layer(0)->add(&iface);                     // 0x41ece0, 0x41e930
    iface.load();                                    // v15
    rt.addBarrier("beach_barrier_01", 6344.0f, 2090.0f, 3710.0f, 4744.0f, -100000.0f, 100000.0f);   // 0x42d7d0
    rt.addBarrier("beach_barrier_02", 2550.0f, 4744.0f, 3710.0f, 4744.0f, -100000.0f, 100000.0f);
    rt.addBarrier("shell_barrier_01", 2610.0f, 2184.0f, 3104.0f, 1779.0f, -100000.0f, 100000.0f);
    for (Sound *f : fx) f->open();                   // 0x41f9d0(0)
    gull_t = 0;                                      // +0x75d4
    coco_hot = Gorilla::gnew<HotRegion>(&rt, "Coconuts Away Game", rt.findSprite("AC_mcov_gs_sign_01"),
                                        "MCOV_VO_RABcall1", "MCOV_VO_RABcall1", "MCOV_VO_RABcall2",
                                        "MCOV_VO_RABcall3");   // 0x4243c0 (0xf4), 0x4c3b30
    start(coco_hot);                                 // 0x420ef0
    pearl_hot = Gorilla::gnew<HotRegion>(&rt, "Pearl Toss Game", rt.findSprite("AB_mcov_gs_sign_01"),
                                         "PERL_VO_CUBcall1", "PERL_VO_CUBcall1", "PERL_VO_CUBcall2",
                                         "PERL_VO_CUBcall3");
    start(pearl_hot);
    shell_hot = Gorilla::gnew<HotRegion>(&rt, "Shell Game", rt.findSprite("AA_mcov_gs_sign_01"),
                                         "SHEL_VO_SKNcall1", "SHEL_VO_SKNcall2", "SHEL_VO_SKNcall3",
                                         "SHEL_VO_SKNcall4");
    start(shell_hot);
    coco_hot->on = false;                            // +0xc0
    pearl_hot->on = false;
    shell_hot->on = false;
    VoiceOver &vo = VoiceOver::get();

    rabbit_pack = ras_open("Rabbit_Char", "", &data_device());   // 0x419b90
    rabbit_pack->load();                             // 0x41aa20
    ras_set_current(rabbit_pack);                    // 0x41a920
    rabbit.init();                                   // v1
    rabbit.setX(coco_sign->x);                       // 0x4cc8c0
    rabbit.setZ((float)((double)coco_sign->z - 150.0f));   // 0x4cc960, 0x5142e8
    for (const char *l : {"MCOV_VO_RABcall1", "MCOV_VO_RABcall2", "MCOV_VO_RABcall3"})
        vo.registerLine(l, &rabbit);                 // 0x4c91b0 (+0x84e4)

    skunk_pack = ras_open("Skunk_Char", "", &data_device());
    skunk_pack->load();
    ras_set_current(skunk_pack);
    skunk.init();
    skunk.setX((float)((double)shell_sign->x + 180.0f));   // 0x513da4
    skunk.setZ((float)((double)shell_sign->z + 40.0f));    // 0x5144c0
    // "SKNI": the exe's spelling
    for (const char *l : {"GLOB_VO_SKNIidle1", "GLOB_VO_SKNIidle2", "GLOB_VO_SKNIidle3"})
        vo.registerLine(l, &skunk);                  // (+0x8774)

    cubby_pack = ras_open("Cubby_Char", "", &data_device());
    cubby_pack->load();
    ras_set_current(cubby_pack);
    cubby.init();
    cubby.setX((float)((double)pearl_sign->x + 110.0f));   // 0x5148c0
    cubby.setZ(pearl_sign->z);
    for (const char *l : {"GLOB_VO_CUBidle1", "GLOB_VO_CUBidle2", "GLOB_VO_CUBidle3"})
        vo.registerLine(l, &cubby);                  // (+0x8a04)

    mappiece.load();                                 // v11
    puzpieces.load();                                // v15
    puzpieces.visible = false;                       // +0x8ce4
    over1.layer(0)->add(&puzpieces);
    shell_click.sign = &puzpieces;                   // +0x8200
    inactivity.setSpot(0, coco_sign->x, coco_sign->z);     // 0x4c73f0
    inactivity.setSpot(1, pearl_sign->x, pearl_sign->z);
    inactivity.setSpot(2, shell_sign->x, shell_sign->z);
    if (!st.tutorial_skipped) arriveThroughClouds(false);  // +0x7bc, 0x4ab760
    ambient.bag = bag;                               // +0x769c
    ambient.activate();                              // +0x18
    music(2);                                        // (inlined)
}

// v20 0x450740 (the cubby released twice: the exe's)
void Mermaid_Scene::leave() {
    if (reward) { reward->deactivate(); delete reward; reward = nullptr; }   // +0x1c, +0xc
    if (prompt) { prompt->deactivate(); delete prompt; prompt = nullptr; }
    ambient.deactivate();                            // +0x1c
    State &st = Game::state();
    if (st.dusting) {                                // +0x3c4
        st.dusting = false;
        bag->refund();                               // 0x4f07b0
    }
    VoiceOver::get().stop();                         // 0x4c9260
    Raytrace::RayTrace::resetMaxEye();               // 0x42b120
    iface.unload();                                  // v16
    puzpieces.unload();
    shell_click.stop();                              // 0x4c12c0
    coco_hot = pearl_hot = shell_hot = nullptr;      // (tasks of the scene's, ended with it)
    pearl.stop();                                    // 0x455610
    shell.stop();                                    // 0x456cf0
    deactivate();                                    // v7
    music(3);                                        // (inlined)
    for (Sound *f : fx) f->close();                  // 0x41fc50
    pearl_end.deactivate();                          // 0x4c1400
    coco_end.deactivate();
    shell_end.deactivate();
    pearl_sign_click.stop();                         // 0x4c7fe0
    coco_sign_click.stop();
    shell_sign_click.stop();
    inactivity.stop();                               // 0x4c7770
    cubby.release();                                 // v2
    mappiece.clear();                                // v12
    rabbit.release();
    if (rabbit_pack) ras_close(rabbit_pack);         // 0x419620
    rabbit_pack = nullptr;
    skunk.release();
    if (skunk_pack) ras_close(skunk_pack);
    skunk_pack = nullptr;
    cubby.release();
    if (cubby_pack) ras_close(cubby_pack);
    cubby_pack = nullptr;
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    set_cursor(st.cursor, cur_vis());                // 0x4119b0
}

// 0x452600
void Mermaid_Scene::music(int n) {
    if (tune != 3) tunes[tune]->close();             // 0x41fc50
    tune = n;
    if (n == 3) return;
    tunes[n]->close();
    tunes[n]->open();                                // 0x41f9d0(0)
    tunes[n]->play();                                // 0x41fd50
}

void Mermaid_Scene::hotsOn(bool on) {                // 0x452af0
    coco_hot->on = on;                               // +0xc0
    pearl_hot->on = on;
    shell_hot->on = on;
}

// 0x452b20: to the map the area's spot 1 is where the camera is now; elsewhere the area's spot 3.
void Mermaid_Scene::leaveTo(const char *scene) {
    float sp[4];
    if (strcmp("map", scene) == 0) {                 // 0x423c60
        sp[0] = rt.camx;                             // 0x42b470
        sp[1] = rt.eyef;                             // 0x42b480
        sp[2] = rt.camz;                             // 0x42b540
        sp[3] = (float)rt.angleDeg();                // 0x42b560
    } else {
        Game::state().cameraSpot(5, 3, sp);          // 0x4ec180
    }
    Game::state().setSpot4(5, sp);                   // 0x4ec0e0
    change_scene(scene);                             // 0x41ddf0
}

// 0x452160: each mermaid's rock and her idle (x, then the height, then z: the exe's); the second one five frames on.
void Mermaid_Scene::mermaids() {
    Sprite3D *r = rt.addSprite("rock_a", "PERL_GA_MERrock_", 1);   // 0x42d9d0
    r->load();                                       // 0x42ef20
    r->x = 2989.0f;                                  // 0x453ad000
    r->place();                                      // 0x42f7f0
    r->z = 5255.0f;                                  // 0x45a43800
    r->mode = 0;                                     // +0x88
    idle_a = rt.addSprite("PERL_GA_MERidlea_", "PERL_GA_MERidlea_", 1);
    idle_a->load();
    idle_a->x = 2989.0f;
    idle_a->place();
    idle_a->z = 5250.0f;                             // 0x45a41000
    idle_a->mode = 1;
    r = rt.addSprite("rock_b", "PERL_GA_MERrock_", 1);
    r->load();
    r->x = 2689.0f;                                  // 0x45281000
    r->place();
    r->z = 5355.0f;                                  // 0x45a75800
    r->mode = 0;
    idle_b = rt.addSprite("PERL_GA_MERidleb_", "PERL_GA_MERidlea_", 1);
    idle_b->load();
    idle_b->x = 2689.0f;
    idle_b->place();
    idle_b->z = 5350.0f;                             // 0x45a73000
    idle_b->anim->setFrame(5);                       // 0x403f20
    idle_b->mode = 1;
}

// 0x4522b0: each sign's picture, highlight (hidden), star and cap by its game's "Info" (Coconuts Away's just won:
// from the last result).
void Mermaid_Scene::signs() {
    State &st = Game::state();
    Saves &sv = st.saves;
    pearl_sign = rt.findSprite("AB_mcov_gs_sign_01");   // 0x42dc70
    pearl_sign->attach("MCOV_GS_PEARL_", "MCOV_GS_PEARL_", -1);   // 0x42f390
    pearl_sign->attach("pearlGameSignHighlight", "MCOV_GS_green_", -1);
    pearl_sign->hideAttached("pearlGameSignHighlight");   // 0x42f620
    pearl_sign->attach("pearlGameStar", "MCOV_GS_star_", -1);
    pearl_sign->attach("pearlGameCap", "MCOV_GS_cap_", -1);
    switch (sv.gameInfo("Pearl Toss Game")) {        // 0x4e9d70
    case 0:
        pearl_sign->hideAttached("pearlGameStar");
        pearl_sign->hideAttached("pearlGameCap");
        break;
    case 1:
        pearl_sign->showAttached("pearlGameStar");   // 0x42f690
        pearl_sign->hideAttached("pearlGameCap");
        break;
    case 2:
        pearl_sign->showAttached("pearlGameStar");
        pearl_sign->showAttached("pearlGameCap");
        break;
    default: break;
    }
    shell_sign = rt.findSprite("AA_mcov_gs_sign_01");
    shell_sign->attach("MCOV_GS_SHEL_", "MCOV_GS_SHEL_", -1);
    shell_sign->attach("shellGameSignHighlight", "MCOV_GS_green_", -1);
    shell_sign->hideAttached("shellGameSignHighlight");
    shell_sign->attach("shellGameStar", "MCOV_GS_star_", -1);
    shell_sign->attach("shellGameCap", "MCOV_GS_cap_", -1);
    switch (sv.gameInfo("Shell Game")) {
    case 0:
        shell_sign->hideAttached("shellGameStar");
        shell_sign->hideAttached("shellGameCap");
        break;
    case 1:
        shell_sign->showAttached("shellGameStar");
        shell_sign->hideAttached("shellGameCap");
        break;
    case 2:
        shell_sign->showAttached("shellGameStar");
        shell_sign->showAttached("shellGameCap");
        break;
    default: break;
    }
    coco_sign = rt.findSprite("AC_mcov_gs_sign_01");
    coco_sign->attach("MCOV_GS_COCO_", "MCOV_GS_COCO_", -1);
    coco_sign->attach("coconutGameSignHighlight", "MCOV_GS_green_", -1);
    coco_sign->hideAttached("coconutGameSignHighlight");
    coco_sign->attach("coconutsGameStar", "MCOV_GS_star_", -1);
    coco_sign->attach("coconutsGameCap", "MCOV_GS_cap_", -1);
    switch (sv.gameInfo("Coconuts Away Game")) {     // *0x543524
    case 0:
        coco_sign->hideAttached("coconutsGameCap");
        if (st.last_game == 2 && st.last_result == 2) coco_sign->showAttached("coconutsGameStar");   // +0x79c +0x7a0
        else coco_sign->hideAttached("coconutsGameStar");
        break;
    case 1:
        coco_sign->showAttached("coconutsGameStar");
        coco_sign->hideAttached("coconutsGameCap");
        if (st.last_game == 2 && st.last_result == 3) coco_sign->showAttached("coconutsGameCap");
        else coco_sign->hideAttached("coconutsGameCap");
        break;
    case 2:
        coco_sign->showAttached("coconutsGameStar");
        coco_sign->showAttached("coconutsGameCap");
        break;
    default: break;
    }
}

// 0x4526a0: the shells still hidden (state 1) placed on the ground; a hard one shows only if a hiding sprite of
// this scene covers its spot (else it is logged and made easy).
void Mermaid_Scene::groundTokens() {
    static const char *const prefixes[5] = {"SHEL_GA_puzshell1_", "SHEL_GA_puzshell2_", "SHEL_GA_puzshell3_",
                                            "SHEL_GA_puzshell4_", "SHEL_GA_puzshell5_"};   // 0x53872c
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = Game::state().mcov[i];
        if (t.state != 1) continue;
        Sprite3D *s = rt.addSprite(kGround[i], prefixes[i], 1);   // 0x42d9d0
        s->load();                                   // 0x42ef20
        s->x = t.x;                                  // +0x17a8
        s->y = t.y;
        s->z = t.z;
        if (t.easy) continue;                        // 0x4ed260
        bool found = false;                          // 0x443590: find_if over the sprites
        for (Sprite3D *h : rt.sprites)
            if (hiding_sprite(name(), h) && hides_near(s->x, s->z, h)) { found = true; break; }
        if (found) {
            s->mode = 0;                             // +0x88
            continue;
        }
        vision_log("Hard hiding place at x=%f z=%f in scene %s is invalid.", (double)s->x, (double)s->z, name());
        t.setEasy();                                 // v3
    }
}

// 0x4528d0: a shell on the ground clicked: an easy one is won (state 4, saved, the grab); a hidden one: Peter's
// "it's hidden" line. A hiding place over a hidden shell: "sprinkle dust" line.
void Mermaid_Scene::pickToken(Sprite3D *s) {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (s->name != kGround[i]) continue;         // 0x42f330
        if (!st.mcov[i].easy) {                      // 0x4ed260
            vo_play("GLOB_CA_PEThide");              // 0x4c9400
            return;
        }
        st.mcov[i].state = 4;
        st.saveMermaidTokens();                      // 0x4ef3a0
        st.saves.flush();                            // 0x4e9840
        if (s->anim) s->anim->visible = false;       // +0x70 +0x7c
        fx[1]->play();                               // +0x6f44 "shel_fx_grabpuz"
        return;
    }
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = st.mcov[i];
        if (!t.hidden()) continue;                   // 0x4ed1c0
        if (hiding_sprite("mermaid", s) && hides_near(t.x, t.z, s)) {   // 0x4e6850, 0x4e69f0
            vo_play("GLOB_CA_PEThddust");
            return;
        }
    }
}

// 0x452c30: Tink to the side of a misplaced shell's hole (the least hinted), 40 short of it, and points down at
// it; none: no help.
void Mermaid_Scene::shellHint() {
    tink->clearQueue();                              // 0x4ddf30
    Attachment *at = nullptr;
    if (!shell.hint(&at)) {                          // 0x457b00
        tink->noHelp();                              // 0x4dc1d0
        tinkHome();                                  // 0x4abb10
        return;
    }
    float p[3];
    rt.stickerPos(at->name.c_str(), p);              // 0x42df30
    float P[3] = {p[0], 0.1f, p[2]};                 // 0x3dcccccd
    float C[3];
    rt.camPos(C);                                    // 0x42b490
    double dx = (double)C[0] - P[0], dy = (double)C[1] - P[1], dz = (double)C[2] - P[2];
    float dist = (float)(sqrt(dx * dx + dy * dy + dz * dz) - 40.0f);   // 0x425d50, 0x5144c0
    double ang = atan2((double)P[2] - C[2], (double)P[0] - C[0]);   // 0x425d80
    ang += atan2(30.0, (double)dist);                // 0x5148c8 (fpatan)
    float nx = (float)(cos(ang) * dist + C[0]);
    float nz = (float)(sin(ang) * dist + C[2]);
    tink->pushBasic(2, nx, (float)((double)p[1] - 100.0f), nz);   // 0x4de020, 0x513da0
    tink->pointDown();                               // 0x4dba20
    tinkHome();
}

// 0x4de970's "Mermaid" branch: a game's shell to win (Coconuts Away's 2, Pearl Toss's 3), the Shell Game's map
// segment, or the map; else a game not won twice, or the map if the cap is not won yet; else no help.
bool Mermaid_Scene::tinkHelp() {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {                    // 0x4dedb0
        Sprite3D *s = nullptr;
        if (st.mcov[i].state == 2) s = coco_sign;    // +0x28fc
        else if (st.mcov[i].state == 3) s = pearl_sign;   // +0x28f0
        if (s) { tink->guideTo(s); return true; }
    }
    if (!st.saves.mapSegment("Mermaid")) {           // 0x4ea7f0
        tink->guideTo(shell_sign);                   // +0x28f8
        return true;
    }
    if (!st.saves.allMapSegments()) {                // 0x4eaa70
        tink->toMap();                               // 0x4db5b0
        return true;
    }
    Sprite3D *sign = nullptr;                        // 0x4dee10 (0x4f10e0: "Info" 2)
    if (st.saves.gameInfo("Coconuts Away Game") != 2) sign = coco_sign;
    else if (st.saves.gameInfo("Pearl Toss Game") != 2) sign = pearl_sign;
    else if (st.saves.gameInfo("Shell Game") != 2) sign = shell_sign;
    if (sign) {
        tink->guideTo(sign);
        return true;
    }
    if (!st.saves.hasCap()) {                        // 0x4e9af0
        tink->toMap();
        return true;
    }
    tink->noHelp();                                  // 0x4dc1d0
    return false;
}

// (inlined in run) Peter's cap visit (every game won twice, the cap not given yet), waited for. No Peter stands
// here: the reward brings its own.
void Mermaid_Scene::capVisit() {
    reward = Gorilla::gnew<PeterCapReward>(nullptr, this, 5, lift);   // 0x4243c0 (0x104), 0x4bf280
    reward->activate();                              // +0x18
    while (reward->active()) wait(30);               // +0x14
    delete reward;                                   // +0xc
    reward = nullptr;
}

// v9 0x452120 (not while paused)
void Mermaid_Scene::notify2(Service *s) {
    if (paused) return;                              // +0x128
    if (s == &keys) onKey();                         // v26
    if (s == &mouse) onMouse();                      // v25
}

// v26 0x4516c0: Escape ends either game.
bool Mermaid_Scene::onKey() {
    State &st = Game::state();
    if (g_key.down) {                                // 0x5a6dd8
        if (g_key.vk == 0x1b) {                      // 0x5a6d9c
            pearl.stop();                            // 0x455610
            shell.stop();                            // 0x456cf0
        }
        if (st.key((int)g_key.vk)) return true;      // 0x4baef0
    }
    return stateKeys();                              // 0x4ec5f0 (+0x12f4)
}

// v25 0x451720: the cursor for the buttons; a click on the bag (the dust on the cursor), the map (left at once
// unless a game is on), the bell (Tink's help: the Shell Game's or Pearl Toss's hint, else the shells or the
// area's help); with dust: sprinkled. With no game on, the signs light under the mouse and start on a click.
// Else a click: a shell on the ground, or a character.
void Mermaid_Scene::onMouse() {
    if (leaving) return;                             // +0x2831
    State &st = Game::state();
    if (dbg_on() && mouse_down())
        tb_log("mermaid: click %d,%d (dist %.0f %.0f %.0f) pearl %d shell %d coco %d ui %d", g_mouse.x, g_mouse.y,
               pearl_sign->dist, shell_sign->dist, coco_sign->dist,
               rt.hitSprite(pearl_sign, g_mouse.x, g_mouse.y), rt.hitSprite(shell_sign, g_mouse.x, g_mouse.y),
               rt.hitSprite(coco_sign, g_mouse.x, g_mouse.y), over_ui);
    Vision::Graphic *c = uiCursor();                 // 0x4abb70
    Vision::Graphic *cur = g_cursor_graphic;
    if (c && (cur == st.cursor || cur == st.cursor_active || st.dusting)) set_cursor(c, cur_vis());   // 0x4119b0
    else if (cur == st.cursor_active) set_cursor(st.cursor, cur_vis());
    cur = g_cursor_graphic;
    if ((cur == st.cursor || cur == st.cursor_active) && mouse_down() && bag->click()) {   // 0x4f02f0
        pixie.visible = true;                        // +0x1854
        set_cursor(&pixie, true);
        return;
    }
    if (mouse_down()) {
        int r = map.click(g_mouse.x, g_mouse.y, true);   // 0x4efc90
        if (r) {
            if ((pearl.active() || shell.active()) && r != 2) return;
            leaveTo("map");
            return;
        }
    }
    if (bell.click()) {                              // 0x4e7180 (+0x1fe8)
        if (shell.active()) {
            if (pay_help(this)) shellHint();         // 0x4abf90, 0x452c30
            return;
        }
        if (pearl.active()) {
            if (!pay_help(this)) return;
            tink->clearQueue();                      // 0x4ddf30
            Attachment *a = nullptr;
            if (!pearl.hint(&a)) {                   // 0x455950
                tink->noHelp();                      // 0x4dc1d0
                return;
            }
            float p[3];
            rt.stickerPos(a->name.c_str(), p);       // 0x42df30
            tink->pushBasic(2, p[0], (float)((double)p[1] - 200.0f), p[2]);   // 0x4de020, 0x514340
            tink->point(p[0], p[1], p[2]);           // 0x4db7b0
            tinkHome();                              // 0x4abb10
            return;
        }
        if (!bag->has(PixieDustBag::cost2())) {      // 0x4f08c0, 0x4f02d0
            tink->noHelp();
            return;
        }
        bag->spend();                                // 0x4f0860
        if (mermaid_token_help(this)) return;        // 0x4de8d0
        tinkHelp();                                  // 0x4de970
        return;
    }
    if (st.dusting) {                                // +0x3c4
        Sprite3D *d = nullptr;
        Attachment *da = nullptr;
        bag->sprinkle(&rt, &d, &da);                 // 0x4f0480
        if (!d) return;
        for (int i = 0; i < 5; i++) {
            TokenInfo &t = st.mcov[i];
            if (!t.hidden()) continue;               // 0x4ed1c0
            if (!hiding_sprite("mermaid", d)) continue;   // 0x4e6850
            if (!hides_near(t.x, t.z, d)) continue;  // 0x4e69f0
            t.setEasy();                             // v3
            Sprite3D *g = rt.findSprite(kGround[i]);
            g->anim->setHit(-1, false);              // v6
            g->mode = 1;                             // +0x88
        }
        lift->add(d, 500);                           // 0x4bf0b0
        return;
    }
    pixie.visible = false;                           // +0x1854
    if (!g_cursor_graphic || !g_cursor_graphic->visible) set_cursor(st.cursor, true);
    if (!pearl.active() && !shell.active() && !over_ui) {   // +0x2830
        set_cursor(st.cursor, cur_vis());
        pearl_sign->hideAttached("pearlGameSignHighlight");   // 0x42f620
        shell_sign->hideAttached("shellGameSignHighlight");
        coco_sign->hideAttached("coconutGameSignHighlight");
        rt.pick_max = 975.0f;                        // +0x12b0 = 0x516378
        if (rt.hitSprite(pearl_sign, g_mouse.x, g_mouse.y) && !SignClick::active_sign) {   // 0x42c7b0, 0x6311ec
            set_cursor(st.cursor_active, cur_vis());
            pearl_sign->showAttached("pearlGameSignHighlight");   // 0x42f690
            if (mouse_down()) {
                fx[0]->play();                       // +0x6bfc "glob_fx_gsclick"
                set_cursor(st.cursor, cur_vis());
                pearl_sign->hideAttached("pearlGameSignHighlight");
                hotsOn(false);
                inactivity.stop();                   // 0x4c7770
                st.setTinkHome(pearl_sign->x, 400.0f, pearl_sign->z);   // 0x4e63b0
                tinkHome();
                pearl_sign_click.start();            // 0x4c7fa0
                return;
            }
        }
        if (rt.hitSprite(shell_sign, g_mouse.x, g_mouse.y) && !SignClick::active_sign && !over_ui) {
            set_cursor(st.cursor_active, cur_vis());
            shell_sign->showAttached("shellGameSignHighlight");
            if (mouse_down()) {
                fx[0]->play();
                if (st.allMermaid(4)) st.mermaidDone();   // 0x4ef6e0, 0x4ef710
                set_cursor(st.cursor, cur_vis());
                shell_sign->hideAttached("shellGameSignHighlight");
                hotsOn(false);
                inactivity.stop();
                if (st.anyMermaid(5) || st.allMermaid(4)) {   // 0x4ef6b0, 0x4ef6e0
                    st.setTinkHome(shell_sign->x, 700.0f, shell_sign->z);
                    tinkHome();
                }
                shell_sign_click.start();
                return;
            }
        }
        if (rt.hitSprite(coco_sign, g_mouse.x, g_mouse.y) && !SignClick::active_sign && !over_ui) {
            set_cursor(st.cursor_active, cur_vis());
            coco_sign->showAttached("coconutGameSignHighlight");
            if (!mouse_down()) return;
            fx[0]->play();
            st.last_game = 2;                        // +0x79c
            set_cursor(st.cursor, cur_vis());
            coco_sign->hideAttached("coconutGameSignHighlight");
            coco_sign_click.start();
            return;
        }
    }
    if (!mouse_down()) return;
    rt.pick_max = 1000.0f;                           // +0x12b0
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    if (!s) return;
    pickToken(s);                                    // 0x4528d0
    if (s == cubby.sprite()) pearl_click.say(pearl.active());   // 0x4cc8b0, 0x4c0b00
    else if (s == rabbit.sprite()) coco_click.say(false);
    else if (s == skunk.sprite()) {
        if (!shell.active()) shell_click.start();    // 0x4c12a0
        else shell_click.say(true);
    }
}

// v10 0x450980: the arrival (down through the clouds), the area's welcome, Coconuts Away's end (and the cap);
// then every frame: the tune for the game on, a gull now and then; Pearl Toss started when Cubby's talk ends (a
// fade to the trees), its end (a fade back, Cubby's end, the cap); the Shell Game started when the skunk's ends
// (with the shells), its end (Peter's map prompt or the cap). A click on the rabbit's sign ends the loop: up
// through the clouds to Coconuts Away.
void Mermaid_Scene::run() {
    wait(1);
    State &st = Game::state();
    if (!st.tutorial_skipped)                        // +0x7bc
        while (arriveThroughClouds(false)) wait(0);  // 0x4ab760
    st.tutorial_skipped = false;
    st.firsts->welcome();                            // 0x4c64d0 (+0x788)
    while (st.firsts->welcoming()) wait(0);          // 0x4c6930
    if (dbg_on())
        tb_log("mermaid: welcomed (%u); signs pearl %.0f,%.0f shell %.0f,%.0f coco %.0f,%.0f cam %.0f,%.0f %.0f",
               frame_time(), pearl_sign->x, pearl_sign->z, shell_sign->x, shell_sign->z, coco_sign->x, coco_sign->z,
               rt.camx, rt.camz, rt.angleDeg());
    if (st.last_game == 2) coco_end.start();         // +0x79c, 0x4c19e0
    if (coco_end.active()) {
        while (coco_end.active()) wait(0);
        if (cap_due()) capVisit();
    }
    inactivity.start();                              // 0x4c7730
    hotsOn(true);
    for (;;) {
        wait(0);
        if (pearl.active() && tune != 0) music(0);   // (inlined)
        else if (shell.active() && tune != 1) music(1);
        else if (!pearl.active() && !shell.active() && tune != 2) music(2);
        uint32_t now = frame_time();                 // 0x4207b0
        if (now > gull_t && now - gull_t > 6000) {
            fx[2]->play();                           // +0x728c "mcov_ax_gull"
            gull_t = (uint32_t)(crt_rand() % 3000) + now;   // 0x4f54da
        }
        if (pearl_sign_click.active()) {
            while (pearl_sign_click.active()) wait(0);
            fadeOutBegin();                          // 0x4abab0
            fader->start(false, 2000);               // 0x4a9580 (+0x124)
            while (fader->busy()) wait(0);           // 0x4a95c0
            rt.setCamera(3447.74f, 0.1f, 3225.49f, 104.797f);   // 0x42a570 (0x45577bd7, 0x454997d7, 0x42d19810)
            fader->start(true, 2000);
            while (fader->busy()) wait(0);
            fadeInEnd();                             // 0x4abae0
            if (dbg_on()) tb_log("mermaid: pearl toss starts (%u)", frame_time());
            pearl.start();                           // 0x454830
        }
        if (st.pearl_ended) {                        // +0x7a4
            st.pearl_ended = false;
            if (dbg_on()) tb_log("mermaid: pearl toss ended: game %d result %d", st.last_game, st.last_result);
            fader->start(false, 2000);
            while (fader->busy()) wait(0);
            st.setCameraToSpot(5, 1, &rt);           // 0x4ec140
            fader->start(true, 2000);
            while (fader->busy()) wait(0);
            fadeInEnd();
            if (st.last_game != 5) {
                pearl_end.start();                   // 0x4c19e0
                while (pearl_end.active()) wait(0);
                if (cap_due()) capVisit();
            }
            inactivity.start();
            hotsOn(true);
        }
        if (shell_sign_click.active()) {
            while (shell_sign_click.active()) wait(0);
            set_cursor(st.cursor, cur_vis());        // 0x4119b0
            if (dbg_on()) tb_log("mermaid: shell sign done, shells %d%d%d%d%d", st.mcov[0].state, st.mcov[1].state,
                                 st.mcov[2].state, st.mcov[3].state, st.mcov[4].state);
            if (st.anyMermaid(5)) shell.start();     // 0x4ef6b0, 0x456ad0
            else {
                inactivity.start();
                hotsOn(true);
            }
        }
        if (shell_end.active()) {
            if (dbg_on()) tb_log("mermaid: shell game ended: game %d result %d", st.last_game, st.last_result);
            if (st.last_result == 2) map.visible = false;   // +0x7a0, +0x1a1c
            while (shell_end.active()) wait(0);
            if (st.saves.allMapSegments() && !st.saves.hasXSpotSet()) {   // 0x4eaa70, 0x4ead80
                prompt = Gorilla::gnew<PeterPrompt>(nullptr, this, 5, lift);   // 0x4243c0 (0x114), 0x4bfea0
                prompt->activate();
                while (prompt->active()) wait(30);
                delete prompt;
                prompt = nullptr;
            } else if (cap_due()) {
                capVisit();
            }
            map.visible = true;
            inactivity.start();
            hotsOn(true);
        }
        if (coco_sign_click.active()) break;
    }
    hide_cursor();                                   // 0x4119e0
    leaving = true;                                  // +0x2831
    while (coco_sign_click.active()) wait(0);
    while (flyThroughClouds(false)) wait(30);        // 0x4ab920
    st.last_game = 2;
    leaveTo("coconut");                              // 0x452b20
    wait(0);
    finish();                                        // 0x420fe0
}

}  // namespace Game
