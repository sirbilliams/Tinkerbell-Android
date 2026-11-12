// IndianVillage_Scene, reconstructed from "Tinker Bell.exe" (see indianvillage.h).
#include "indianvillage.h"
#include "flyinghoops.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "voiceover.h"
#include "cannon.h"
#include "peter_reward.h"
#include "dustlift.h"
#include "toolbar.h"
#include "tb.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool dbg_on() { return getenv("TB_DEBUG_VILLAGE") != nullptr; }   // test aid

std::vector<std::string> IndianVillage_Scene::win_lines;   // 0x5c8b70

IndianVillage_Scene &village_scene() {               // the static at 0x5c8b80
    static IndianVillage_Scene s("Indian_Village");
    return s;
}

// eagle 0, fox 1, raccoon 2, skunk 3, bear 4 (the exe's spelling)
static const char *const kGround[5] = {"eagle_token_on_ground", "fox_token_on_ground", "raccon_token_on_ground",
                                       "skunk_token_on_ground", "bear_token_on_ground"};   // 0x53a738..

// "A?_totm_md_totmtree1_01", ? = 'A' + t: the five totem trees.
static std::string tree_name(int t) {
    std::string n = "AA_totm_md_totmtree1_01";    // 0x53a8cc
    n[1] = (char)('A' + t);
    return n;
}
static std::string numbered(const char *fmt, int n) {
    char b[64];
    snprintf(b, sizeof b, fmt, n);                   // 0x4f396f
    return b;
}

// ---------------------------------------------------------------- TotemInterface
TotemInterface::TotemInterface()                     // 0x4627e0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    bar = Gorilla::gnew<Animation>("TOTM_IF_puzbar_");   // 0x4243c0 + 0x404c20
    static const char *const ic[5] = {"TOTM_IF_puzeagle_", "TOTM_IF_puzfox_", "TOTM_IF_puzraccon_",
                                      "TOTM_IF_puzskunk_", "TOTM_IF_puzbear_"};
    for (int i = 0; i < 5; i++) icons[i] = Gorilla::gnew<Animation>(ic[i]);
}

TotemInterface::~TotemInterface() {                  // 0x4629d0
    delete bar;                                      // (each v3(1))
    for (Animation *&a : icons) { delete a; a = nullptr; }
}

void TotemInterface::load() {                        // v15 0x462ac0
    bar->load();
    for (Animation *a : icons) {
        a->load();
        a->setHit(1, false);                         // v6
    }
}

void TotemInterface::unload() {                      // v16 0x462b60
    Graphic::unload();                               // 0x412020
    bar->unload();
    for (Animation *a : icons) a->unload();
}

// v14 0x462bb0: nothing during Flying Hoops; else with Totem Trail on or a piece won or in hand: the bar and each
// piece won (its second frame during Totem Trail).
void TotemInterface::draw() {
    IndianVillage_Scene &sc = village_scene();       // 0x5cd2ac, 0x5cd890 (the static's +0x472c, +0x4d10)
    if (sc.hoops->on) return;
    State &st = Game::state();
    if (!sc.totem_on) {
        int n = 0;
        for (const VillageToken &t : st.village)
            if (t.state == 4 || t.state == 6) n++;
        if (!n) return;
    }
    bar->draw();                                     // v14
    int f = sc.totem_on ? 1 : 0;
    for (int i = 0; i < 5; i++) {
        if (st.village[i].state != 4) continue;
        icons[i]->setFrame(f);                       // 0x403f20
        icons[i]->draw();
    }
}

int TotemInterface::pickIcon(int x, int y) {         // 0x462cd0
    int r = -1;
    for (int i = 0; i < 5; i++)
        if (Game::state().village[i].state == 4 && icons[i]->hit(x, y)) r = i;   // v11
    return r;
}

// ---------------------------------------------------------------- TeepeeAnimations
TeepeeAnimations::TeepeeAnimations(Raytrace::RayTrace *rt) : Task(nullptr) {   // 0x468e60
    Attachment *a = rt->findSprite("AA_indv_am_teepee_01")->attach("tgl wave", "INDV_CA_TGLwav_", 100);   // 0x42f390
    a->period = 0x7fffffff;                          // +0x184
    waves.push_back(static_cast<Animation *>(a->g)); // +0x150
    a = rt->findSprite("AB_indv_am_teepee_01")->attach("tgl wave", "INDV_CA_INCwav_", 100);
    a->period = 0x7fffffff;
    waves.push_back(static_cast<Animation *>(a->g));
    next = (uint32_t)vision_random(5000) + 10000;    // 0x425d40 (state +8)
}

void TeepeeAnimations::run() {                       // 0x469130
    wait(1);
    for (;;) {
        wait(next);
        next = (uint32_t)vision_random(5000) + 10000;
        which = vision_random((int)waves.size());
        waves.at(which)->play(0, 0);                 // 0x403be0(0, 0)
    }
}

// ---------------------------------------------------------------- the scene
// 0x467fc0
IndianVillage_Scene::IndianVillage_Scene(const char *nm)
    : TinkerbellRaytraceScene(nm),
      ambient(&rt, nm),                              // 0x4df580
      mappiece(&rt),                                 // 0x4e8410
      flyh_click("FLYH_VO_RACgreet1_y", "FLYH_VO_RACrecrd1_y", "FLYH_VO_RACfun1_y", "FLYH_VO_RACgreet2",
                 "FLYH_VO_RACrecrd2", "FLYH_VO_RACfun2", "FLYING_HOOPS_CHAR", "", "", ""),   // 0x4c0990
      bird_click("INDV_VO_PETgreet1", "INDV_VO_PETrecrd1", "INDV_VO_PETfun1", "INDV_VO_PETgreet2",
                 "INDV_VO_PETrecrd2", "INDV_VO_PETfun2", "BIRD_SPY_CHAR", "", "", ""),
      totm_click("TOTM_VO_FOXgreet1", "TOTM_VO_FOXrecrd1", "TOTM_VO_FOXfun1", "TOTM_VO_FOXgreet2",
                 "TOTM_VO_FOXrecrd2", "TOTM_VO_FOXfun2", "TOTEM_TRAIL_CHAR", "TOTM_VO_FOXmore1",
                 "TOTM_VO_FOXmore2", "TOTM_VO_FOXoverly"),
      rac("hoops_raccoon", &rt, &lift),              // 0x4ccad0
      rac2("hoops_raccoon2", &rt, &lift),
      foxy("totem_foxy", &rt, &lift),
      peter("birdSpy_peterPan", &rt, &lift),
      bird("AC_indv_gs_sign_01", "INDV_GS_BIRD_01", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_"),   // 0x4e6400
      flyh("AA_indv_gs_sign_01", "INDV_GS_FLYH_01", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_"),
      totm("AB_indv_gs_sign_01", "INDV_GS_TOTM_01", "GLOB_GS_green_", "GLOB_GS_cap_", "GLOB_GS_star_"),
      puzpieces("INDV_LY_puzpieces_01", 0, 0, 0, 0, 0, 0, true),   // 0x4179e0
      enterimg("INDV_LY_enter_01", 0, 0, 0, 0, 0, 0, true) {
    hoops = new FlyingHoopsGame();                   // 0x4611e0 (embedded in the exe)
    static const char *const cu[5] = {"TOTM_CU_puzeagle_", "TOTM_CU_puzfox_", "TOTM_CU_puzraccon_",
                                      "TOTM_CU_puzskunk_", "TOTM_CU_puzbear_"};
    for (int i = 0; i < 5; i++) held[i] = new Animation(cu[i]);   // 0x404c20 (embedded)
    for (Sound *&m : tunes) m = new Sound("");       // 0x437290 x3 (named by load2)
    for (Sound *&f : fx) f = new Sound("");          // 0x437290 x7
    inactivity.init(&rt, 30000);                     // 0x4c70e0
    totm_end.setPiece(&mappiece);                    // 0x4c2ad0
    inactivity.add(0, 1000.0f, "GLOB_VO_PETidle1", "GLOB_VO_PETidle2", "GLOB_VO_PETidle3", "");   // 0x4c7100
    inactivity.add(1, 1000.0f, "GLOB_VO_RACidle1", "GLOB_VO_RACidle2", "GLOB_VO_RACidle3", "");
    inactivity.add(2, 1000.0f, "GLOB_VO_FOXidle1", "GLOB_VO_FOXidle2", "GLOB_VO_FOXidle3", "Totem Trail Game");
}

IndianVillage_Scene::~IndianVillage_Scene() {        // 0x468510 (port: the exe's static is never destroyed)
    delete iface;
    delete reward;
    delete prompt;
    delete peter3d;
    delete hoops;
    for (Animation *&a : held) { delete a; a = nullptr; }
    for (Sound *&m : tunes) { delete m; m = nullptr; }
    for (Sound *&f : fx) { delete f; f = nullptr; }
}

// v18 0x4660b0: the four characters into the scene's list, the tunes and sounds, Totem Trail's first puzzle, the
// signs' games, the end-of-game lines, the win lines (appended again on every call: the exe's one-time guard covers
// only the vector's construction), each character's animations and lines.
void IndianVillage_Scene::load2() {
    chars.push_back(&rac);                           // 0x430dd0 on +0x2820
    chars.push_back(&rac2);
    chars.push_back(&foxy);
    chars.push_back(&peter);
    hoops->bits = 0;                                 // +0x4730
    bits = 0;                                        // +0x45b4
    iface = nullptr;                                 // +0x4654
    static const char *const tn[3] = {"FLYH_MU_music", "TOTM_MU_music", "INDV_MU_music"};   // 0x53a504
    for (int i = 0; i < 3; i++) {
        tunes[i]->setName(tn[i]);                    // 0x424400
        tunes[i]->mode = 2;                          // +0x338
        tunes[i]->type = 6;                          // +0x340
    }
    tune = 3;                                        // +0x7360
    static const char *const fn[7] = {"FLYH_FX_hoopgood", "TOTM_FX_puzinsert", "TOTM_FX_puzreturn", "TOTM_FX_grabpuz",
                                      "TOTM_FX_stumpknok", "TOTM_FX_woodknock", "TOTM_FX_buzz"};   // 0x53a510
    for (int i = 0; i < 7; i++) fx[i]->setName(fn[i]);
    makePuzzle();                                    // 0x4695a0
    bird_sign.set("Bird Spy Game", "INDV_VO_PETgoinfo", "INDV_VO_PETgo2", "", nullptr);   // 0x4c7f00
    flyh_sign.set("Flying Hoops Game", "FLYH_VO_RACgoinfo_y", "FLYH_VO_RACgo2_y", "", &enterimg);
    totm_sign.set("Totem Trail Game", "TOTM_VO_FOXgoinfo", "TOTM_VO_FOXgo2", "TOTM_VO_FOXnogo", nullptr);
    flyh_end.setLines("FLYH_VO_RACtoobad_y", "FLYH_VO_RACuwin1_y", "FLYH_VO_RACuwin2_y", "FLYH_VO_RACuwin3_y",
                      "FLYH_VO_RACchalgm_y", "FLYH_VO_RACpuzzgm_y");   // 0x4c1900
    bird_end.setLines("INDV_VO_PETtoobad", "INDV_VO_PETuwin1", "INDV_VO_PETuwin2", "INDV_VO_PETuwin3",
                      "INDV_VO_PETviewgm", "INDV_VO_PETpuzzgm");
    totm_end.setLines("TOTM_VO_FOXuwin1", "TOTM_VO_FOXother", "TOTM_VO_FOXtoobad", "TOTM_VO_FOXuwin2",
                      "TOTM_VO_FOXuwin3");           // 0x4c2a10
    totm_inactivity.add("TOTM_VO_FOXprompt", false); // 0x4c46e0
    static const char *const wins[9] = {"FLYH_VO_RACuwin1_y", "FLYH_VO_RACuwin2_y", "FLYH_VO_RACuwin3_y",
                                        "TOTM_VO_FOXuwin1", "TOTM_VO_FOXuwin2", "TOTM_VO_FOXuwin3",
                                        "INDV_VO_PETuwin1", "INDV_VO_PETuwin2", "INDV_VO_PETuwin3"};
    for (const char *w : wins) win_lines.push_back(w);
    VoiceOver &vo = VoiceOver::get();

    rac.setIdles("GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_", "GLOB_CA_RACidle3_");   // 0x4cd650
    rac.setTalks("GLOB_CA_RACroottk_", "GLOB_CA_RACtalk1_", "GLOB_CA_RACtalk2_", "GLOB_CA_RACtalk3_");   // 0x4ce2d0
    rac.setWave("GLOB_CA_RACwin_", &win_lines);      // 0x4ce880
    rac.setFlyIdle("GLOB_CA_RACflyid_");             // 0x4ce850
    rac.eyes = "RAC_eyes";                           // +0x3744
    rac.setLip("hoops_raccoon_lip", "RAC_mout", 0);  // 0x4ccd20
    // nice1 and nice2 twice: the exe's
    static const char *const r1[] = {
        "FLYH_VO_RAC1more_y", "FLYH_VO_RACbackno", "FLYH_VO_RACcall1_y", "FLYH_VO_RACcall2", "BUCK_VO_RACcolor_y",
        "FLYH_VO_RACcall3", "FLYH_VO_RACchalgm_y", "FLYH_VO_RACfun1_y", "FLYH_VO_RACgo2_y", "FLYH_VO_RACgoinfo_y",
        "FLYH_VO_RACgreet1_y", "FLYH_VO_RAChalfwy_y", "FLYH_VO_RACnice1", "FLYH_VO_RACnice2", "FLYH_VO_RACnice3",
        "FLYH_VO_RACnice4", "FLYH_VO_RACprompt", "FLYH_VO_RACpuzzgm_y", "FLYH_VO_RACrecrd1_y", "FLYH_VO_RACsmoke",
        "FLYH_VO_RACtoobad_y", "FLYH_VO_RACuwin1_y", "FLYH_VO_RACuwin2_y", "FLYH_VO_RACuwin3_y", "FLYH_VO_RACnice1",
        "FLYH_VO_RACnice2", nullptr};
    for (const char *const *l = r1; *l; l++) vo.registerLine(*l, &rac);   // 0x4c91b0 (+0x3738)

    rac2.setIdles("GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_", "GLOB_CA_RACidle3_");
    rac2.setTalks("GLOB_CA_RACroottk_", "GLOB_CA_RACtalk1_", "GLOB_CA_RACtalk2_", "GLOB_CA_RACtalk3_");
    rac2.setWave("GLOB_CA_RACwin_", &win_lines);
    rac2.setFlyIdle("GLOB_CA_RACflyid_");
    rac2.eyes = "RAC_eyes";                          // +0x39d0
    rac2.setLip("hoops_raccoon2_lip", "RAC_mout", 0);
    static const char *const r2[] = {
        "FLYH_VO_RAC1more_z", "FLYH_VO_RACcall1_z", "BUCK_VO_RACcolor_z", "FLYH_VO_RACchalgm_z", "FLYH_VO_RACfun1_z",
        "FLYH_VO_RACgo2_z", "FLYH_VO_RACgoinfo_z", "FLYH_VO_RACgreet1_z", "FLYH_VO_RAChalfwy_z",
        "FLYH_VO_RACpuzzgm_z", "FLYH_VO_RACrecrd1_z", "FLYH_VO_RACtoobad_z", "FLYH_VO_RACuwin1_z",
        "FLYH_VO_RACuwin2_z", "FLYH_VO_RACuwin3_z", "FLYH_VO_RACnice3", "FLYH_VO_RACnice4", nullptr};
    for (const char *const *l = r2; *l; l++) vo.registerLine(*l, &rac2);   // (+0x39c4)

    // the first talk as the talking root, for Foxy and Peter: the exe's
    foxy.setIdles("GLOB_CA_FOXroot_", "GLOB_CA_FOXidle1_", "GLOB_CA_FOXidle2_", "GLOB_CA_FOXidle3_");
    foxy.setTalks("GLOB_CA_FOXtalk1_", "GLOB_CA_FOXtalk1_", "GLOB_CA_FOXtalk2_", "GLOB_CA_FOXtalk3_");
    foxy.setWave("GLOB_CA_FOXwin_", &win_lines);
    foxy.eyes = "FOX_eyes";                          // +0x3c60
    foxy.setFlyIdle("GLOB_CA_FOXflyid_");
    foxy.setLip("totem_foxy_lip", "FOX_mout", 0);
    static const char *const fox[] = {
        "TOTM_VO_FOXcall1", "TOTM_VO_FOXcall2", "TOTM_VO_FOXcall3", "TOTM_VO_FOXcall4", "TOTM_VO_FOXclkst1",
        "TOTM_VO_FOXclkst2", "TOTM_VO_FOXfun1", "TOTM_VO_FOXgo2", "TOTM_VO_FOXgoinfo", "TOTM_VO_FOXgreet1",
        "TOTM_VO_FOXmore1", "TOTM_VO_FOXnice1", "TOTM_VO_FOXnice2", "TOTM_VO_FOXnice3", "TOTM_VO_FOXnice4",
        "TOTM_VO_FOXnogo", "TOTM_VO_FOXnono", "TOTM_VO_FOXother", "TOTM_VO_FOXoverly", "TOTM_VO_FOXprompt",
        "TOTM_VO_FOXpuz2wr", "TOTM_VO_FOXpuz3wr", "TOTM_VO_FOXpuz4wr", "TOTM_VO_FOXpuz5wr", "TOTM_VO_FOXrecrd1",
        "TOTM_VO_FOXstmpno", "TOTM_VO_FOXtoobad", "TOTM_VO_FOXtoobad2", "TOTM_VO_FOXuwin1", "TOTM_VO_FOXuwin2",
        "TOTM_VO_FOXuwin3", nullptr};
    for (const char *const *l = fox; *l; l++) vo.registerLine(*l, &foxy);   // (+0x3c54)

    peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_", "GLOB_CA_PETidle3_");
    peter.setTalks("GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_");
    peter.setWave("GLOB_CA_PETwin_", &win_lines);
    peter.setFlyIdle("GLOB_CA_PETflyid_");
    peter.eyes = "PET_eyes";                         // +0x3ef0
    peter.setLip("birdSpy_peterPan_lip", "PET_mout", 0);
    // (0x469240..0x4692f0: the list built, "TOTM_VO_PETendgam" put first)
    static const char *const pet[] = {
        "TOTM_VO_PETendgam", "INDV_VO_PETcall1", "INDV_VO_PETcall2", "INDV_VO_PETcall3", "INDV_VO_PETenter1",
        "INDV_VO_PETenter2", "INDV_VO_PETfun1", "INDV_VO_PETgo2", "INDV_VO_PETgoinfo", "INDV_VO_PETgreet1",
        "INDV_VO_PETpuzzgm", "INDV_VO_PETrecrd1", "INDV_VO_PETremind", "INDV_VO_PETtoobad", "INDV_VO_PETuwin1",
        "INDV_VO_PETuwin2", "INDV_VO_PETuwin3", "INDV_VO_PETviewgm", nullptr};
    for (const char *const *l = pet; *l; l++) vo.registerLine(*l, &peter);   // (+0x3ee4)
}

// v19 0x465060: the eye's limit, the area's camera spot (then spot 0 for next time), Flying Hoops' time, the
// end-of-game tasks' bag, the hoops and the smoke, the pieces on the ground, the pieces' bar and the cursors,
// Peter's 3D stand-in, the signs and their hot regions, the sounds, the totem dressed, the signs' stars, the
// characters, the map piece, the pictures, the inactivity spots, the arrival and the tune.
void IndianVillage_Scene::enter() {
    reward = nullptr;                                // +0x74c0
    prompt = nullptr;                                // +0x74c4
    Raytrace::RayTrace::setMaxEye(0x181);            // 0x42b110
    State &st = Game::state();
    float sp[4];
    st.areaSpot(1, sp);                              // 0x4ec0a0
    cam_x = sp[0]; cam_eye = sp[1]; cam_z = sp[2]; cam_deg = sp[3];   // 0x4aa0d0
    st.setAreaSpot(1, 0);                            // 0x4ec120
    hoops->setup();                                  // 0x460da0
    open(name());                                    // v27 (TinkerbellRaytraceScene's 0x4ab0f0)
    flyh_end.setBag(bag);                            // 0x4c19d0
    bird_end.setBag(bag);
    totm_end.setBag(bag);                            // 0x4c2ac0
    hoops->on = false;                               // +0x472c
    hoops->deactivate();                             // v7
    totem_on = false;                                // +0x4d10
    hoops->hoops();                                  // 0x4608b0
    hoops->smoke();                                  // 0x4604c0
    if (const char *t = getenv("TB_VILLAGE_TOKENS")) // test aid (not in the game): the five pieces' states
        for (int i = 0; i < 5 && t[i]; i++) st.village[i].state = t[i] - '0';
    groundTokens();                                  // 0x468530
    iface = Gorilla::gnew<TotemInterface>();         // 0x4243c0, 0xb0
    iface->load();                                   // v15
    iface->setZ(30);                                 // 0x411e70(0x1e)
    over1.layer(0)->add(iface);                      // 0x41ece0, 0x41e930
    for (Animation *h : held) h->load();             // v15
    delete peter3d;                                  // port: the exe makes a new one each time (never freed)
    peter3d = new Raytrace::RayTraceCharacter(&rt, "indian_village_peter");   // 0x4f34f3 (0x50), 0x4cc5c0
    peter3d->init();                                 // v1 (nothing)
    fuse = &Game::cannon();                          // state +0x7c0
    flyh.init(&rt);                                  // 0x4e6440
    flyh.sprite->f90 = 0;                            // +0x90
    totm.init(&rt);
    bird.init(&rt);
    bird.sprite->f90 = 0;
    flyh.enabled = false;                            // +0x4610
    bird.enabled = false;                            // +0x45e0
    totm.enabled = false;                            // +0x4640
    bird_hot = Gorilla::gnew<HotRegion>(&rt, "Bird Spy Game", bird.sprite, "INDV_VO_PETcall1", "INDV_VO_PETcall1",
                                        "INDV_VO_PETcall2", "INDV_VO_PETcall3");   // 0x4c3b30
    start(bird_hot);                                 // 0x420ef0
    totm_hot = Gorilla::gnew<HotRegion>(&rt, "Totem Trail Game", totm.sprite, "TOTM_VO_FOXcall1",
                                        "TOTM_VO_FOXcall2", "TOTM_VO_FOXcall3", "TOTM_VO_FOXcall4");
    start(totm_hot);
    flyh_hot = Gorilla::gnew<HotRegion>(&rt, "Flying Hoops Game", flyh.sprite, "FLYH_VO_RACcall1_y",
                                        "FLYH_VO_RACcall1_y", "FLYH_VO_RACcall2", "FLYH_VO_RACcall3");
    start(flyh_hot);
    bird_hot->on = false;                            // +0xc0
    flyh_hot->on = false;
    totm_hot->on = false;
    activate();                                      // v6
    reset();                                         // 0x421110
    for (Sound *f : fx) f->open();                   // 0x41f9d0(0)
    treesStill();                                    // 0x469950
    dressTotem();                                    // 0x469720
    awards();                                        // 0x4688c0
    stay = true;                                     // +0x4598
    VoiceOver &vo = VoiceOver::get();

    rac_pack = ras_open("Raccoon_Char", "", &data_device());   // 0x419b90
    rac_pack->load();                                // 0x41aa20
    ras_set_current(rac_pack);                       // 0x41a920
    rac.init();                                      // v1
    rac2.init();
    rac.setX(flyh.sprite->x);                        // 0x4cc8c0
    rac2.setX(flyh.sprite->x);
    rac.setZ((float)((double)flyh.sprite->z - 150.0f));   // 0x4cc960, 0x5142e8
    rac2.setZ((float)((double)flyh.sprite->z - 75.0f));   // 0x514e5c

    fox_pack = ras_open("Foxy_Char", "", &data_device());
    fox_pack->load();
    ras_set_current(fox_pack);
    foxy.init();
    foxy.setX(totm.sprite->x);
    foxy.setZ((float)((double)totm.sprite->z - 150.0f));
    {
        static const char *const l[3] = {"GLOB_VO_FOXidle1", "GLOB_VO_FOXidle2", "GLOB_VO_FOXidle3"};
        for (const char *s : l) vo.registerLine(s, &foxy);   // (+0x3c54)
    }

    peter_pack = ras_open("Peter_Char", "", &data_device());
    peter_pack->load();
    ras_set_current(peter_pack);
    peter.init();
    peter.setX(bird.sprite->x);
    peter.setZ((float)((double)bird.sprite->z - 150.0f));
    {
        static const char *const l[] = {
            "GLOB_VO_PETcred", "GLOB_VO_PETdstbag", "GLOB_VO_PETendgam", "GLOB_VO_PETesckey", "GLOB_VO_PETgsback",
            "GLOB_VO_PETgsbird", "GLOB_VO_PETgsbuck", "GLOB_VO_PETgsclou", "GLOB_VO_PETgscoco", "GLOB_VO_PETgscolr",
            "GLOB_VO_PETgsdcbl", "GLOB_VO_PETgsecho", "GLOB_VO_PETgsflyh", "GLOB_VO_PETgsindv", "GLOB_VO_PETgsmudp",
            "GLOB_VO_PETgsperl", "GLOB_VO_PETgspirt", "GLOB_VO_PETgsshbx", "GLOB_VO_PETgsshel", "GLOB_VO_PETidle1",
            "GLOB_VO_PETidle2", "GLOB_VO_PETidle3", "GLOB_VO_PETindv", "GLOB_VO_PETlost", "GLOB_VO_PETmap",
            "GLOB_VO_PETmcov", "GLOB_VO_PETpick", "GLOB_VO_PETquit", "GLOB_VO_PETrecord", "GLOB_VO_PETreplay",
            "GLOB_VO_PETresume", "GLOB_VO_PETsgnin", "GLOB_VO_PETsignin", "GLOB_VO_PETsklr", "GLOB_VO_PETstar",
            "GLOB_VO_PETtogoff", "GLOB_VO_PETtogon", "GLOB_VO_PETtoobig", "GLOB_VO_PETvquit", "GLOB_VO_PETvsgnin",
            nullptr};
        for (const char *const *s = l; *s; s++) vo.registerLine(*s, &peter);   // (+0x3ee4)
    }
    mappiece.load();                                 // v11
    puzpieces.load();                                // v15
    puzpieces.visible = false;                       // +0x73ec
    over1.layer(0)->add(&puzpieces);
    totm_click.sign = &puzpieces;                    // +0x35e8
    enterimg.load();
    enterimg.visible = false;                        // +0x7494
    over1.layer(0)->add(&enterimg);
    inactivity.setSpot(0, bird.sprite->x, bird.sprite->z);   // 0x4c73f0
    inactivity.setSpot(1, flyh.sprite->x, flyh.sprite->z);
    inactivity.setSpot(2, totm.sprite->x, totm.sprite->z);
    if (!st.tutorial_skipped) arriveThroughClouds(false);   // +0x7bc, 0x4ab760
    ambient.bag = bag;                               // +0x29ac
    ambient.activate();                              // +0x18
    music(2);                                        // (inlined)
    if (dbg_on())
        tb_log("village: enter: signs flyh %.0f,%.0f totm %.0f,%.0f bird %.0f,%.0f cam %.0f,%.0f", flyh.sprite->x,
               flyh.sprite->z, totm.sprite->x, totm.sprite->z, bird.sprite->x, bird.sprite->z, rt.camx, rt.camz);
    if (dbg_on())
        for (int t = 0; t < 5; t++) {
            Sprite3D *s = rt.findSprite(tree_name(t).c_str());
            tb_log("village: tree %d at %.0f,%.0f: type %d animal %d", t, s->x, s->z, puzzle[t].type, puzzle[t].animal);
        }
}

// v20 0x465d50 (Flying Hoops is left as it is: its task waits, off)
void IndianVillage_Scene::leave() {
    if (reward) { reward->deactivate(); delete reward; reward = nullptr; }   // +0x1c, +0xc
    if (prompt) { prompt->deactivate(); delete prompt; prompt = nullptr; }
    ambient.deactivate();                            // +0x1c
    State &st = Game::state();
    for (VillageToken &t : st.village)               // set or held pieces back in the bar
        if (t.state == 5 || t.state == 6) t.state = 4;
    if (st.dusting) {                                // +0x3c4
        st.dusting = false;
        bag->refund();                               // 0x4f07b0
    }
    VoiceOver::get().stop();                         // 0x4c9260
    mappiece.clear();                                // v12
    puzpieces.unload();                              // v16
    totm_click.stop();                               // 0x4c12c0
    enterimg.unload();
    bird_hot = totm_hot = flyh_hot = nullptr;        // (tasks of the scene's, ended with it)
    flyh.release();                                  // 0x4e6530
    totm.release();
    bird.release();
    if (iface) iface->unload();                      // v16
    delete iface;                                    // v3(1)
    iface = nullptr;
    for (Animation *h : held) h->unload();           // v16
    deactivate();                                    // v7
    for (Sound *f : fx) f->close();                  // 0x41fc50
    for (Sound *m : tunes) m->close();
    tune = 3;
    flyh_sign.stop();                                // 0x4c7fe0
    bird_sign.stop();
    totm_sign.stop();
    flyh_end.deactivate();                           // 0x4c1400
    bird_end.deactivate();
    totm_end.deactivate();
    rac.release();                                   // v2
    rac2.release();
    if (rac_pack) ras_close(rac_pack);               // 0x419620
    rac_pack = nullptr;
    foxy.release();
    if (fox_pack) ras_close(fox_pack);
    fox_pack = nullptr;
    if (peter_pack) ras_close(peter_pack);           // (before Peter's release: the exe's)
    peter_pack = nullptr;
    peter.release();
    inactivity.stop();                               // 0x4c7770
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    set_cursor(st.cursor, true);                     // 0x4119b0
}

// 0x466010
void IndianVillage_Scene::music(int n) {
    if (tune != 3) tunes[tune]->close();             // 0x41fc50
    tune = n;
    if (n == 3) return;
    tunes[n]->close();
    tunes[n]->open();                                // 0x41f9d0(0)
    tunes[n]->play();                                // 0x41fd50
}

void IndianVillage_Scene::signsOn(bool on) {         // 0x463270
    flyh.enabled = on;                               // +0x4610
    bird.enabled = on;                               // +0x45e0
    totm.enabled = on;                               // +0x4640
}

void IndianVillage_Scene::hotsOn(bool on) {          // 0x468b30
    bird_hot->on = on;                               // +0xc0
    flyh_hot->on = on;
    totm_hot->on = on;
}

// (inlined) to the map: the area's spot 1 is where the camera is now; elsewhere: the area's spot 3. The exe tests
// the Bird Spy scene's name against the one it goes to, so "map" takes the camera and "birdspy" the spot.
void IndianVillage_Scene::leaveTo(const char *scene) {
    float sp[4];
    if (strcmp("birdspy", scene) != 0) {             // 0x631720, 0x423c60
        sp[0] = rt.camx;                             // 0x42b470
        sp[1] = rt.eyef;                             // 0x42b480
        sp[2] = rt.camz;                             // 0x42b540
        sp[3] = (float)rt.angleDeg();                // 0x42b560
    } else {
        Game::state().cameraSpot(1, 3, sp);          // 0x4ec180
    }
    Game::state().setSpot4(1, sp);                   // 0x4ec0e0
    change_scene(scene);                             // 0x41ddf0
}

// 0x468530: the pieces still hidden (state 3) placed on the ground; a hard one shows only if a hiding sprite of
// this scene covers its spot (else it is logged and made easy).
void IndianVillage_Scene::groundTokens() {
    static const char *const prefixes[5] = {"TOTM_GA_puzeagle_", "TOTM_GA_puzfox_", "TOTM_GA_puzraccon_",
                                            "TOTM_GA_puzskunk_", "TOTM_GA_puzbear_"};   // 0x53b5c8..
    for (int i = 0; i < 5; i++) {
        VillageToken &t = Game::state().village[i];
        if (t.state != 3) continue;
        rt.addSprite(kGround[i], prefixes[i], 1);    // 0x42d9d0
        Sprite3D *s = rt.findSprite(kGround[i]);     // 0x42dc70
        s->load();                                   // 0x42ef20
        s->x = t.x;                                  // +0x15a8
        s->y = t.y;
        s->z = t.z;
        if (t.easy) continue;                        // 0x4ed110
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

// 0x4688c0: the signs' star and cap by each game's "Info" (Bird Spy's just won: from the last result).
void IndianVillage_Scene::awards() {
    Saves &sv = Game::state().saves;
    State &st = Game::state();
    switch (sv.gameInfo("Flying Hoops Game")) {      // 0x4e9d70 (*0x543528)
    case 1: flyh.star->on = true; break;             // +0x4608 (+0x17c)
    case 2: flyh.star->on = true; flyh.cap->on = true; break;   // +0x4604
    default: break;
    }
    switch (sv.gameInfo("Totem Trail Game")) {       // *0x54352c
    case 1: totm.star->on = true; break;             // +0x4638
    case 2: totm.star->on = true; totm.cap->on = true; break;   // +0x4634
    default: break;
    }
    switch (sv.gameInfo("Bird Spy Game")) {          // *0x5434fc
    case 0:
        if (st.last_game == 2 && st.last_result == 2) bird.star->on = true;   // +0x79c, +0x7a0, +0x45d8
        break;
    case 1:
        bird.star->on = true;
        if (st.last_game == 2 && st.last_result == 3) bird.cap->on = true;   // +0x45d4
        break;
    case 2: bird.star->on = true; bird.cap->on = true; break;
    default: break;
    }
}

// 0x463bc0: a piece on the ground clicked: an easy one is won (state 4, saved, the knock); a hidden one: Peter's
// "it's hidden" line. A hiding place over a hidden piece: "sprinkle dust" line.
bool IndianVillage_Scene::pickToken(Sprite3D *s) {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (s->name != kGround[i]) continue;         // 0x42f330
        if (!st.village[i].easy) {                   // 0x4ed110
            VoiceOver::get().play("GLOB_CA_PEThide");   // 0x4c9400
            return false;
        }
        st.village[i].state = 4;
        if (s->anim) s->anim->visible = false;       // +0x70 +0x7c
        fx[5]->play();                               // +0x6cd0 "TOTM_FX_woodknock"
        st.saveVillageTokens();                      // 0x4ed6b0
        st.saves.flush();                            // 0x4e9840
        return true;
    }
    for (int i = 0; i < 5; i++) {
        VillageToken &t = st.village[i];
        if (!t.hidden()) continue;                   // 0x4ed120
        if (hiding_sprite("indian_village", s) && hides_near(t.x, t.z, s)) {   // 0x4e6850, 0x4e69f0
            VoiceOver::get().play("GLOB_CA_PEThddust");
            return false;
        }
    }
    return false;
}

// 0x4de8d0 with the scene's functor (VillageTokenContainer, vtable 0x514e04: any 0x462520, target 0x462550): with
// a piece still hidden (state 3), Tink guides to the first one's sprite.
static bool village_token_help(IndianVillage_Scene *sc) {
    sc->tink->clearQueue();                          // 0x4ddf30
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (st.village[i].state != 3) continue;
        sc->tink->guideTo(sc->rt.findSprite(kGround[i]));   // 0x4dc250
        return true;
    }
    return false;
}

// 0x4de970's "indian_village" branch: a game's piece to win (Bird Spy's 1, Flying Hoops' 2), Totem Trail's map
// segment, or the map; else a game not won twice, or the map if the cap is not won yet; else no help.
bool IndianVillage_Scene::tinkHelp() {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {                    // 0x4deeb0
        Sprite3D *s = nullptr;
        if (st.village[i].state == 1) s = bird.sprite;   // +0x45cc
        else if (st.village[i].state == 2) s = flyh.sprite;   // +0x45fc
        if (s) { tink->guideTo(s); return true; }
    }
    if (!st.saves.mapSegment("Village")) {           // 0x4ea870
        tink->guideTo(totm.sprite);                  // +0x462c
        return true;
    }
    if (!st.saves.allMapSegments()) {                // 0x4eaa70
        tink->toMap();                               // 0x4db5b0
        return true;
    }
    Sprite3D *sign = nullptr;                        // 0x4def10 (0x4f10e0: "Info" 2)
    if (st.saves.gameInfo("Bird Spy Game") != 2) sign = bird.sprite;
    else if (st.saves.gameInfo("Flying Hoops Game") != 2) sign = flyh.sprite;
    else if (st.saves.gameInfo("Totem Trail Game") != 2) sign = totm.sprite;
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

// 0x468b60: Tink in front of the camera, high, then at the nearest white hoop (above it), pointing at it.
void IndianVillage_Scene::hoopsHint() {
    tink->clearQueue();                              // 0x4ddf30
    Sprite3D *h = nullptr;
    if (!hoops->nextHoop(&h)) {                      // 0x4619e0
        tink->noHelp();                              // 0x4dc1d0
        return;
    }
    float ax, az;
    tink->ahead(&ax, &az, 300.0f);                   // 0x4da8f0
    tink->pushBasic(1, ax, (float)((double)rt.eyef + 450.0f), az);   // 0x4de020, 0x5142ec
    float x = h->x, y = h->y, z = h->z;
    tink->beckon();                                  // 0x4dbb50
    tink->pushBasic(2, x, (float)((double)y + 400.0f), z);   // 0x514f04
    tink->point(h->x, h->y, h->z);                   // 0x42f700, 0x4db7b0
}

// 0x468c70: a piece in a wrong hole, else the stump (all set), else the nearest tree with a free hole: Tink comes
// high in front of the camera, beckons, flies to 200 in front of it (180 degrees round) and points (down at a hole).
void IndianVillage_Scene::totemHint() {
    tink->clearQueue();
    Attachment *a = nullptr;
    Sprite3D *s = nullptr;
    if (!wrongHole(&a) && !stumpHint(&s) && !freeTree(&s)) {   // 0x469f80, 0x46a580, 0x46a5c0
        tink->noHelp();
        return;
    }
    float ax, az;
    tink->ahead(&ax, &az, 300.0f);
    tink->pushBasic(1, ax, (float)((double)rt.eyef + 450.0f), az);
    if (s) {
        float x = s->x, y = s->y, z = s->z;
        tink->beckon();
        float ox, oz;
        Tinkerbell3D::polar(x, z, &ox, &oz, 180.0f, 200.0f);   // 0x4da950
        tink->pushBasic(2, ox, y, oz);
        tink->point(x, y, z);                        // 0x4db7b0
        return;
    }
    float p[3];
    rt.stickerPos(a->name.c_str(), p);               // 0x42df30
    tink->beckon();
    float ox, oz;
    Tinkerbell3D::polar(p[0], p[2], &ox, &oz, 180.0f, 200.0f);
    tink->pushBasic(2, ox, p[1], oz);
    tink->pointDown();                               // 0x4dba20
}

// 0x469f80: the first piece set in a hole not its own: that hole.
bool IndianVillage_Scene::wrongHole(Attachment **out) {
    State &st = Game::state();
    std::vector<int> v;
    for (int i = 0; i < 5; i++)
        if (st.village[i].state == 5) v.push_back(i);
    if (v.empty()) return false;
    for (int i : v) (void)rt.spriteDist(rt.findSprite(tree_name(st.village[i].slot).c_str()));   // 0x42e3d0 (unused)
    for (int &i : v)
        if (puzzle[st.village[i].slot].animal == i) i = -1;   // in its own hole
    std::vector<int> w;                              // (the -1s removed, the rest copied up to the first -1)
    for (int i : v)
        if (i != -1) w.push_back(i);
    if (w.empty()) return false;
    Attachment *a = rt.findAttached(numbered("hole_%d", st.village[w[0]].slot + 1).c_str());   // 0x42dd00
    if (!a) return false;
    *out = a;
    return true;
}

bool IndianVillage_Scene::stumpHint(Sprite3D **out) {   // 0x46a580: all five set: the stump
    if (nPlaced() != 5) return false;
    *out = rt.findSprite("AA_totm_ga_totstump_01");
    return true;
}

// 0x46a5c0: of the trees with no piece set, the nearest.
bool IndianVillage_Scene::freeTree(Sprite3D **out) {
    State &st = Game::state();
    std::vector<int> used;
    for (const VillageToken &t : st.village)
        if (t.state == 5) used.push_back(t.slot);
    if (used.size() == 5) return false;
    std::sort(used.begin(), used.end());             // 0x46ab50
    std::vector<int> free;
    for (int i = 0; i < 5; i++) {
        auto it = std::lower_bound(used.begin(), used.end(), i);   // 0x46ae00
        if (it == used.end() || i < *it) free.push_back(i);
    }
    std::vector<Sprite3D *> trees;
    for (int t : free) trees.push_back(rt.findSprite(tree_name(t).c_str()));
    std::vector<float> d;
    for (Sprite3D *t : trees) d.push_back((float)rt.spriteDist(t));   // 0x42e3d0
    size_t m = 0;
    for (size_t i = 1; i < d.size(); i++)
        if (d[i] < d[m]) m = i;
    *out = trees.at(m);
    return true;
}

// ---------------------------------------------------------------- Totem Trail
// 0x462db0: the held piece `i` into the hole under the mouse (a piece already there taken in hand); the knock;
// all five set: Foxy's "click the stump"; else a right one: "nice" now and then (beginners: "no, no" for a wrong
// one). No hole under the mouse: the piece back in the bar.
void IndianVillage_Scene::placeToken(int i) {
    State &st = Game::state();
    static const char *const tok[5] = {"TOTM_GA_EAGtoken_", "TOTM_GA_FOXtoken_", "TOTM_GA_RACtoken_",
                                       "TOTM_GA_SKNtoken_", "TOTM_GA_CUBtoken_"};   // the table 0x4630f8
    for (int t = 0; t < 5; t++) {
        int occ = 5;
        bool taken = false;
        for (int j = 0; j < 5; j++)
            if (st.village[j].state == 5 && st.village[j].slot == t) { occ = j; taken = true; break; }
        std::string hole = numbered("hole_%d", t + 1);
        Attachment *ha = rt.findAttached(hole.c_str());   // 0x42dd00
        if (rt.pickAttached(g_mouse.x, g_mouse.y, false) != ha) continue;   // 0x42c950
        st.village[i].state = 5;
        st.village[i].slot = t;
        if (taken) st.village[occ].state = 6;
        float hx = (float)ha->g->x, hy = (float)ha->g->y;   // +0x150 +0x58 +0x5c
        Sprite3D *tree = rt.findSprite(tree_name(t).c_str());
        tree->detach(hole.c_str());                  // 0x42f560
        fx[1]->play();                               // +0x5fb0 "TOTM_FX_puzinsert"
        tree->attach(hole.c_str(), tok[i], 100);     // 0x42f390
        Attachment *na = rt.findAttached(hole.c_str());
        na->frame = 0;                               // +0x180
        na->g->x = (int)hx;                          // ftol
        na->g->y = (int)hy;
        if (nPlaced() == 5) {
            if (st.saves.gameInfo("Totem Trail Game") == 0) VoiceOver::get().play("TOTM_VO_FOXclkst1");
            else VoiceOver::get().play("TOTM_VO_FOXclkst2");
        } else if (nPlaced() == nCorrect()) {        // 0x469f60, 0x469b20
            switch (vision_random(8)) {              // 0x425d40 (state +8)
            case 0: VoiceOver::get().play("TOTM_VO_FOXnice1"); break;
            case 1: VoiceOver::get().play("TOTM_VO_FOXnice2"); break;
            case 2: VoiceOver::get().play("TOTM_VO_FOXnice3"); break;
            case 3: VoiceOver::get().play("TOTM_VO_FOXnice4"); break;
            default: break;
            }
        } else if (st.skill == 1) {                  // +0x68
            VoiceOver::get().play("TOTM_VO_FOXnono");
        }
        correct = nCorrect();                        // +0x4594
        if (dbg_on()) tb_log("village: piece %d in tree %d (%d set, %d right)", i, t, nPlaced(), correct);
        return;
    }
    st.village[i].state = 4;
}

// 0x463120: a set piece clicked: back in the bar, its hole empty again.
bool IndianVillage_Scene::removeToken(Attachment *a) {
    State &st = Game::state();
    for (int t = 0; t < 5; t++) {
        std::string hole = numbered("hole_%d", t + 1);
        Attachment *ha = rt.findAttached(hole.c_str());
        if (ha != a) continue;
        int j = 0;
        for (; j < 5; j++)
            if (st.village[j].state == 5 && st.village[j].slot == t) break;
        if (j == 5) continue;
        st.village[j].state = 4;
        float hx = (float)ha->g->x, hy = (float)ha->g->y;
        Sprite3D *tree = rt.findSprite(tree_name(t).c_str());
        tree->detach(hole.c_str());
        tree->attach(hole.c_str(), "TOTM_GA_hole_", 100);
        fx[2]->play();                               // +0x62f8 "TOTM_FX_puzreturn"
        Attachment *na = rt.findAttached(hole.c_str());
        na->frame = 0;
        na->g->x = (int)hx;
        na->g->y = (int)hy;
        return true;
    }
    return false;
}

// 0x469390: the silhouettes and holes off the trees; set or held pieces back in the bar.
void IndianVillage_Scene::clearTotem() {
    for (int t = 0; t < 5; t++) {
        Sprite3D *tree = rt.findSprite(tree_name(t).c_str());
        tree->detach(numbered("silhouette_%d", t + 1).c_str());
        tree->detach(numbered("hole_%d", t + 1).c_str());
    }
    for (VillageToken &v : Game::state().village)
        if (v.state == 5 || v.state == 6) v.state = 4;
}

// 0x469470: every hole empty again (where it was); set or held pieces back in the bar.
void IndianVillage_Scene::resetHoles() {
    for (int t = 0; t < 5; t++) {
        Sprite3D *tree = rt.findSprite(tree_name(t).c_str());
        std::string hole = numbered("hole_%d", t + 1);
        Attachment *ha = rt.findAttached(hole.c_str());
        float hx = (float)ha->g->x, hy = (float)ha->g->y;
        tree->detach(hole.c_str());
        tree->attach(hole.c_str(), "TOTM_GA_hole_", 100);
        Attachment *na = rt.findAttached(hole.c_str());
        na->frame = 0;
        na->g->x = (int)hx;
        na->g->y = (int)hy;
    }
    for (VillageToken &v : Game::state().village)
        if (v.state == 5 || v.state == 6) v.state = 4;
}

// 0x4695a0: each tree's hole (low, high or with no silhouette) and animal (each once, at random). Before the
// first win (bit 4 clear): at most one tree without a silhouette; after: at most `skill`-many trees with one.
void IndianVillage_Scene::makePuzzle() {
    bool first = !((bits >> 2) & 1);
    int limit = 0;                                   // port: the exe's local is uninitialised for another skill
    int with = 0, without = 0;
    if (first) limit = 5;
    else {
        switch (Game::state().skill) {               // +0x68
        case 1: limit = 4; break;
        case 2: limit = 3; break;
        case 3: limit = 2; break;
        default: break;
        }
    }
    for (int i = 0; i < 5; i++) {
        int k = vision_random(150) / 50;             // 0x425d40 (+0x281c)
        if (first) {
            if (without && k == 2) k = vision_random(100) / 50;
        } else if (with >= limit) {
            k = 2;
        }
        switch (k) {
        case 0: puzzle[i].type = 0; with++; break;
        case 1: puzzle[i].type = 1; with++; break;
        case 2: puzzle[i].type = 2; without++; break;
        default: break;
        }
        int a;
        do {
            a = vision_random(5);                    // (the table 0x46970c: the same value)
            for (int j = 0; j < i; j++)
                if (puzzle[j].animal == a) a = -1;
        } while (a == -1);
        puzzle[i].animal = a;
    }
}

// 0x469720: each tree's silhouette ("beg_" ones for beginners) and empty hole, placed by the hole's kind.
void IndianVillage_Scene::dressTotem() {
    clearTotem();                                    // 0x469390
    static const char *const sill[5] = {"totm_ga_sill01", "totm_ga_sill02", "totm_ga_sill03", "totm_ga_sill04",
                                        "totm_ga_sill05"};   // the table 0x469930
    for (int t = 0; t < 5; t++) {
        std::string pre = sill[puzzle[t].animal];
        pre += Game::state().skill == 1 ? "beg_" : "_";   // 0x53b66c, 0x53a0d4
        Sprite3D *tree = rt.findSprite(tree_name(t).c_str());
        std::string sn = numbered("silhouette_%d", t + 1);
        int sx, sy;
        if (puzzle[t].type == 0) { sx = 0x15; sy = 0x80; }
        else { sx = 0x17; sy = 0x3b; }
        tree->attach(sn.c_str(), pre.c_str(), 100);  // 0x42f390
        Attachment *s = rt.findAttached(sn.c_str());
        s->frame = 0;                                // +0x180
        s->g->x = sx;
        s->g->y = sy;
        std::string hn = numbered("hole_%d", t + 1);
        int hx = sx, hy = sy;                        // (the silhouette's for another kind: the exe's registers)
        switch (puzzle[t].type) {
        case 0: hx = 0xa; hy = 0xa6; break;
        case 1: hx = 0x16; hy = 0x5f; break;
        case 2: hx = 0x32; hy = 0x1e; break;
        default: break;
        }
        tree->attach(hn.c_str(), "TOTM_GA_hole_", 100);
        Attachment *h = rt.findAttached(hn.c_str());
        h->frame = 0;
        h->g->x = hx;
        h->g->y = hy;
    }
}

void IndianVillage_Scene::treesStill() {             // 0x469950
    for (int t = 0; t < 5; t++) rt.findSprite(tree_name(t).c_str())->mode = 0;   // +0x88
}

// 0x4699c0: the signs and hot regions off, the game's tune, the fuse (from the second game on) by skill.
void IndianVillage_Scene::startTotem() {
    signsOn(false);
    hotsOn(false);
    totem_on = true;
    music(1);
    bits |= 1;
    State &st = Game::state();
    int skill = 0;                                   // port: as Flying Hoops' setup (the exe's local)
    if (!st.saves.getSkill(&skill)) skill = 1;       // 0x4e97a0
    int g = st.saves.gameInfo("Totem Trail Game");
    if (g == 1 || g == 2) {
        int secs;
        switch (skill) {
        case 1: secs = 120; break;
        case 2: secs = 90; break;
        case 3: secs = 60; break;
        default: secs = skill; break;
        }
        Game::cannon().visible = true;               // 0x5cfee8 +0x7c
        Game::cannon().start(secs);                  // 0x4e3a50
    }
    st.last_game = 4;                                // +0x79c
    if (dbg_on()) tb_log("village: totem trail on");
    totm_inactivity.start();                         // v12
    correct = 0;                                     // +0x4594
}

// 0x469aa0 (Escape): Totem Trail given up: the holes emptied, the area's tune, no fuse.
void IndianVillage_Scene::skipTotem() {
    if (!totem_on) return;
    signsOn(true);
    hotsOn(true);
    bits |= 0x10;
    resetHoles();                                    // 0x469470
    totem_on = false;
    music(2);
    Game::cannon().stop();                           // 0x4e3bc0
    Game::cannon().hide();                           // 0x4e3bf0
    totm_inactivity.stop();                          // v13
    Game::state().last_game = 5;
}

int IndianVillage_Scene::nCorrect() {                // 0x469b20
    State &st = Game::state();
    int n = 0;
    for (int h = 0; h < 5; h++)
        for (int j = 0; j < 5; j++)
            if (st.village[j].state == 5 && st.village[j].slot == h && puzzle[h].animal == j) n++;
    return n;
}

// 0x469b70: all five right: won (the first time the map segment and the sign's star, then its cap), a new puzzle,
// the end task's flag.
bool IndianVillage_Scene::checkWin() {
    if (nCorrect() < 5) return false;
    if (dbg_on()) tb_log("village: totem trail won");
    totem_on = false;
    music(2);
    Game::cannon().stop();
    Game::cannon().hide();
    signsOn(true);
    hotsOn(true);
    State &st = Game::state();
    switch (st.saves.gameInfo("Totem Trail Game")) {
    case 0:
        bits |= 4;
        totm.star->on = true;                        // +0x4638
        st.saves.setMapSegment("Village");           // 0x4ea8b0
        st.last_result = 2;                          // +0x7a0
        break;
    case 1:
        bits |= 8;
        totm.cap->on = true;                         // +0x4634
        st.last_result = 3;
        break;
    case 2: st.last_result = 4; break;
    default: break;
    }
    makePuzzle();
    dressTotem();
    totm_inactivity.stop();                          // v13
    st.totem_ended = true;                           // +0x7a9
    return true;
}

// 0x469cc0 (the hover while Totem Trail is on and the eye drifts): toward the nearest tree's hole height (high
// for a tree with no silhouette).
void IndianVillage_Scene::eyeToTree() {
    float best = 1e8f;                               // 0x4cbebc20
    int idx = 0;                                     // port: the exe's is uninitialised if none is nearer
    float cx = rt.camx;                              // 0x42b470
    float cy = (float)((double)rt.eyef * 2.05f + 480.0f);   // 0x42b4e0
    float cz = rt.camz;                              // 0x42b540
    for (int t = 0; t < 5; t++) {
        Sprite3D *s = rt.findSprite(tree_name(t).c_str());
        double dx = (double)cx - s->x, dy = (double)cy - s->y, dz = (double)cz - s->z;
        double d = (dz * dz + dy * dy) + dx * dx;
        if (d < best) {
            best = (float)d;
            idx = t;
        }
    }
    float target = puzzle[idx].type == 2 ? 200.0f : 75.0f;
    float v = (float)(((double)target - rt.eyef) * 0.05f);   // 0x514f3c
    if (v > 0.0f) rt.raise(v);                       // 0x42b130
    else rt.lower(-v);                               // 0x42b2d0
}

// 0x469e00: all set, some wrong: Foxy's "two (three, four, five) are wrong".
bool IndianVillage_Scene::wrongMsg() {
    State &st = Game::state();
    int wrong = 0;
    for (int i = 0; i < 5; i++)
        if (puzzle[st.village[i].slot].animal != i) wrong++;
    if (!wrong) return false;
    std::string line;
    switch (wrong) {
    case 2: line = "TOTM_VO_FOXpuz2wr"; break;
    case 3: line = "TOTM_VO_FOXpuz3wr"; break;
    case 4: line = "TOTM_VO_FOXpuz4wr"; break;
    case 5: line = "TOTM_VO_FOXpuz5wr"; break;
    default: break;                                  // (one wrong: no line, the exe's)
    }
    VoiceOver::get().play(line.c_str());             // 0x4c9400
    return true;
}

int IndianVillage_Scene::nPlaced() {                 // 0x469f60
    int n = 0;
    for (const VillageToken &t : Game::state().village)
        if (t.state == 5) n++;
    return n;
}

// ---------------------------------------------------------------- input
// v26 0x463f00: the cheat key ends a game played here.
bool IndianVillage_Scene::onKey() {
    if (busy || paused) return false;                // +0x28e0, +0x128
    State &st = Game::state();
    if (stateKeys()) return true;                    // 0x4ec5f0 (+0x12f4)
    if (!g_key.down) return false;
    if (st.key((int)g_key.vk)) return true;          // 0x4baef0
    if ((int)g_key.vk != st.skip_key) return false;  // +0x74
    skipTotem();                                     // 0x469aa0
    hoops->skip();                                   // 0x461040
    return true;
}

// v32 0x4689f0: the base's, with the eye going to the hoops' or the totem's height while drifting up.
void IndianVillage_Scene::hover() {
    float e = rt.eyef;
    uint32_t now = frame_time();
    uint32_t ms = now - last_hover;
    if (ms < 0x1e) return;
    if (ms > 100) ms = 100;
    last_hover = now;
    State &st = Game::state();
    if (!st.hover_up) {
        if (st.hover_down) return;
        float d = (float)((double)(int64_t)ms * e * 0.00100000005f);
        if (!(e > 1.0f)) return;
        if (e < d) d = (float)((double)e * 0.5f);
        rt.lower(d);
        return;
    }
    if (hoops->on) { hoops->hover(); return; }       // 0x461070
    if (totem_on) { eyeToTree(); return; }           // 0x469cc0
    float v = (float)((200.0f - (double)e) * (int32_t)ms * 0.00100000005f);
    if (v > 0.0f) rt.raise(v);
    else rt.lower(-v);
}

// v25 0x463290
void IndianVillage_Scene::onMouse() {
    if (busy) return;                                // +0x28e0
    State &st = Game::state();
    if (bell.click()) {                              // 0x4e7180 (+0x1fe8)
        if (!hoops->on && !totem_on && bag->has(bag->cost2())) {   // 0x4f08c0, 0x4f02d0
            bag->spend();                            // 0x4f0860
            if (!village_token_help(this)) tinkHelp();   // 0x4de8d0, 0x4de970
        }
        if (hoops->on) hoopsHint();                  // 0x468b60
        else if (totem_on) totemHint();              // 0x468c70
        return;
    }
    if (mouse_down()) {
        if (bag->click()) return;                    // 0x4f02f0
        if (mouse_down() && map.hit(g_mouse.x, g_mouse.y)) {   // +0x19a0 v11
            int r = map.click(g_mouse.x, g_mouse.y, totem_on || hoops->on);   // 0x4efc90
            if (r == 2) leaveTo("map");
            return;
        }
    }
    if (st.dusting) {                                // +0x3c4
        Sprite3D *d = nullptr;
        Attachment *da = nullptr;
        bag->sprinkle(&rt, &d, &da);                 // 0x4f0480
        if (!d) return;
        for (int i = 0; i < 5; i++) {
            VillageToken &t = st.village[i];
            if (!t.hidden()) continue;               // 0x4ed120
            if (!hiding_sprite("indian_village", d)) continue;   // 0x4e6850
            if (!hides_near(t.x, t.z, d)) continue;  // 0x4e69f0
            t.setEasy();                             // v3
            Sprite3D *g = rt.findSprite(kGround[i]);
            g->anim->setHit(-1, false);              // v6
            g->mode = 1;                             // +0x88
        }
        lift->add(d, 500);                           // 0x4bf0b0
        return;
    }
    if (dbg_on() && mouse_down())
        tb_log("village: down %d,%d signs %d%d%d active %d", g_mouse.x, g_mouse.y, flyh.enabled, totm.enabled,
               bird.enabled, SignClick::active_sign);
    if (flyh.mouse() && flyh.clicked && !SignClick::active_sign) {   // 0x4e65b0, +0x4611, 0x6311ec
        inactivity.stop();                           // 0x4c7770
        hotsOn(false);
        flyh_sign.start();                           // 0x4c7fa0
    } else if (totm.mouse() && totm.clicked && !SignClick::active_sign) {
        inactivity.stop();
        hotsOn(false);
        signsOn(false);
        totm_sign.start();
    } else if (bird.mouse() && bird.clicked && !SignClick::active_sign) {
        st.last_game = 2;                            // +0x79c
        bird_sign.start();
        stay = false;                                // +0x4598
    }
    if (!mouse_down()) return;
    if (totem_on) {
        int j = 0;
        for (; j < 5; j++)
            if (st.village[j].state == 6) break;
        if (j < 5) {
            placeToken(j);                           // 0x462db0
            return;
        }
        int i = iface->pickIcon(g_mouse.x, g_mouse.y);   // 0x462cd0
        if (i != -1) {
            fx[3]->play();                           // +0x6640 "TOTM_FX_grabpuz"
            st.village[i].state = 6;
            return;
        }
    } else if (iface->pickIcon(g_mouse.x, g_mouse.y) != -1) {
        fx[6]->play();                               // +0x7018 "TOTM_FX_buzz"
    }
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    if (dbg_on()) tb_log("village: click %d,%d on %s / %s", g_mouse.x, g_mouse.y, s ? s->name.c_str() : "-",
                         a ? a->name.c_str() : "-");
    if (!s) {
        if (a && totem_on) removeToken(a);           // 0x463120
        return;
    }
    if (!totem_on && pickToken(s)) return;           // 0x463bc0
    if (s->name == "AA_totm_ga_totstump_01") {       // 0x42f330
        fx[4]->play();                               // +0x6988 "TOTM_FX_stumpknok"
        if (totem_on) {
            if (nPlaced() == 5) {
                if (!wrongMsg()) checkWin();         // 0x469e00, 0x469b70
            } else {
                VoiceOver::get().play("TOTM_VO_FOXstmpno");
            }
            return;
        }
    }
    if (s->name == "AA_TOTM_GA_stumopen_01") {
        fx[4]->play();
        if (totem_on && checkWin()) return;
    }
    if (s == rac.sprite() || s == rac2.sprite()) flyh_click.say(hoops->on);   // 0x4cc8b0, 0x4c0b00
    else if (s == peter.sprite()) bird_click.say(false);
    else if (s == foxy.sprite()) {
        if (!totem_on) totm_click.start();           // 0x4c12a0
        else totm_click.say(true);
    }
    if (a && totem_on) removeToken(a);
}

// (inlined in run): the area interactive again: the inactivity prompts, the hot regions, the signs.
void IndianVillage_Scene::interactive() {
    if (dbg_on()) tb_log("village: interactive (frame %u)", frame_time());
    inactivity.start();                              // 0x4c7730
    hotsOn(true);
    signsOn(true);
}

static bool cap_due() {
    Saves &sv = Game::state().saves;
    return sv.hasCap() && !sv.hasPetersCap();        // 0x4e9af0, 0x4ead40
}

// Peter's cap visit (every game won twice, the cap not given yet), waited for.
static void cap_visit(IndianVillage_Scene *sc) {
    sc->reward = Gorilla::gnew<PeterCapReward>(&sc->peter, sc, 1, sc->lift);   // 0x4bf280
    sc->reward->activate();                          // +0x18
    while (sc->reward->active()) sc->wait(30);       // +0x14
    delete sc->reward;                               // +0xc
    sc->reward = nullptr;
}

// v10 0x463f90: the teepees, the arrival (down through the clouds), the area's welcome, Bird Spy's end (and the
// cap); then every frame: Flying Hoops started when the raccoon's talk ends (a fade out and in, the camera at the
// start), its end (and the cap), Totem Trail started when Foxy's ends (all pieces won), its end (Peter's map prompt
// or the cap), the fuse running out on it. A click on Peter's sign ends the loop: up through the clouds to Bird Spy.
void IndianVillage_Scene::run() {
    wait(1);
    start(Gorilla::gnew<TeepeeAnimations>(&rt));     // 0x4243c0 (0xd8), 0x468e60, 0x420ef0
    State &st = Game::state();
    if (!st.tutorial_skipped)                        // +0x7bc
        while (arriveThroughClouds(false)) wait(0);  // 0x4ab760
    st.tutorial_skipped = false;
    if (dbg_on()) tb_log("village: arrived (%u)", frame_time());
    st.firsts->welcome();                            // 0x4c64d0 (+0x788)
    while (st.firsts->welcoming()) wait(0);          // 0x4c6930
    if (dbg_on()) tb_log("village: welcomed (%u)", frame_time());
    if (st.last_game == 2) bird_end.start();         // +0x79c, 0x4c19e0
    if (bird_end.active()) {
        while (bird_end.active()) wait(0);
        if (cap_due()) cap_visit(this);
    }
    interactive();
    if (getenv("TB_VILLAGE_TOTEM")) startTotem();    // test aid (not in the game): Totem Trail at once
    while (stay) {                                   // +0x4598
        wait(0);
        if (flyh_sign.active()) {
            while (flyh_sign.active()) wait(0);
            fadeOutBegin();                          // 0x4abab0
            fader->start(false, 2000);               // 0x4a9580 (+0x124)
            while (fader->busy()) wait(0);           // 0x4a95c0
            rt.setCamera(5720.31f, 0.1f, 1344.19f, 144.323f);   // 0x42a570 (0x45b2c27b, 0x44a80614, 0x431052b0)
            fader->start(true, 2000);
            while (fader->busy()) wait(0);
            fadeInEnd();                             // 0x4abae0
            hoops->start();                          // 0x460e70
        }
        if (st.hoops_ended) {                        // +0x7a7
            st.hoops_ended = false;
            if (st.last_game != 5) {
                flyh_end.start();                    // 0x4c19e0
                while (flyh_end.active()) wait(0);
                if (cap_due()) cap_visit(this);
            }
            interactive();
        }
        if (totm_sign.active()) {
            while (totm_sign.active()) wait(0);
            if (st.allVillage(4)) startTotem();      // 0x4ed990, 0x4699c0
            else interactive();
        }
        if (st.totem_ended) {                        // +0x7a9
            st.totem_ended = false;
            totm_end.start();
            if (st.last_result == 2) map.visible = false;   // +0x7a0, +0x1a1c
            while (totm_end.active()) wait(0);
            if (st.saves.allMapSegments() && !st.saves.hasXSpotSet()) {   // 0x4eaa70, 0x4ead80
                prompt = Gorilla::gnew<PeterPrompt>(&peter, this, 1, lift);   // 0x4bfea0
                prompt->activate();
                while (prompt->active()) wait(30);
                delete prompt;
                prompt = nullptr;
            } else if (cap_due()) {
                cap_visit(this);
            }
            map.visible = true;
            inactivity.start();
            signsOn(true);
            hotsOn(true);
        }
        if (totem_on && fuse->visible && !fuse->running()) {   // +0x7368 (+0x7c), 0x4e3b80: the time is up
            totem_on = false;
            fuse->hide();                            // 0x4e3bf0
            for (VillageToken &t : st.village) t.state = 4;
            makePuzzle();                            // 0x4695a0
            dressTotem();                            // 0x469720
            st.last_result = 0;
            st.totem_ended = true;
        }
    }
    leaving = true;                                  // +0x2831
    hide_cursor();                                   // 0x4119e0
    while (bird_sign.active()) wait(0);
    while (flyThroughClouds(false)) wait(30);        // 0x4ab920
    leaveTo("birdspy");                              // (the string at 0x631720)
    wait(0);
    finish();                                        // 0x420fe0
}

// 0x464d60: a piece in hand: its cursor, blinking (frames by the half second) over an empty hole.
Vision::Graphic *IndianVillage_Scene::heldCursor(Attachment *a) {
    State &st = Game::state();
    int j = 0;
    for (; j < 5; j++)
        if (st.village[j].state == 6) break;
    if (j == 5) return nullptr;
    Animation *c = held[j];                          // the table 0x464e5c
    if (a) {
        for (int i = 0; i < 5; i++) {
            if (a->name != numbered("hole_%1.1d", i + 1)) continue;   // 0x42e5b0
            bool taken = false;
            for (const VillageToken &t : st.village)
                if (t.state == 5 && t.slot == i) { taken = true; break; }
            if (taken) continue;
            return c->frame((int)((frame_time() % 500) / 100));   // 0x403ec0
        }
    }
    return c->frame(0);
}

// 0x464e70: the active cursor over a sign that can be clicked, the stump during Totem Trail, a piece on the
// ground, a hole with a piece in it, or a won piece's icon during Totem Trail.
Vision::Graphic *IndianVillage_Scene::activeCursor(Sprite3D *s, Attachment *a) {
    State &st = Game::state();
    bool on = false;
    if (s) {
        if (s->name == "AC_indv_gs_sign_01" && !hoops->on && !totem_on) on = true;   // *0x53a4c0
        if (s->name == "AA_indv_gs_sign_01" && !hoops->on && !totem_on) on = true;   // *0x53a4b8
        if (s->name == "AB_indv_gs_sign_01" && !hoops->on && !totem_on) {            // *0x53a4bc
            int n = 0;
            for (const VillageToken &t : st.village)
                if (t.state == 4 || t.state == 5) n++;
            if (n == 5) on = true;
        }
        if (s->name == "AA_totm_ga_totstump_01" && totem_on) on = true;
        for (const char *g : kGround)
            if (s->name == g) { on = true; break; }
    }
    if (a) {
        for (int i = 0; i < 5; i++) {
            if (a->name != numbered("hole_%1.1d", i + 1)) continue;
            bool taken = false;
            for (const VillageToken &t : st.village)
                if (t.state == 5 && t.slot == i) { taken = true; break; }
            if (taken) { on = true; break; }
        }
    }
    if (totem_on && iface->pickIcon(g_mouse.x, g_mouse.y) != -1) return st.cursor_active;   // +0x60
    if (on) return st.cursor_active;
    return nullptr;
}

// v30 0x464c50: the base's (its choice not used); a piece in hand, else the dust while dusting, else the active
// one; set (shown if one was picked), then the hand over a character.
Vision::Graphic *IndianVillage_Scene::cursorUpdate() {
    TinkerbellRaytraceScene::cursorUpdate();         // 0x4ab6b0
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    State &st = Game::state();
    Vision::Graphic *c = st.cursor;                  // +0x5c
    Vision::Graphic *h = heldCursor(a);              // 0x464d60
    if (h) c = h;
    if (st.dusting) {                                // +0x3c4
        pixie.visible = true;                        // +0x1854
        c = &pixie;                                  // +0x17d8
    } else {
        pixie.visible = false;
    }
    if (!h && !st.dusting) {
        if (Vision::Graphic *r = activeCursor(s, a)) c = r;   // 0x464e70
    }
    if (c) set_cursor(c, true);                      // 0x4119b0
    else set_cursor(nullptr, g_cursor_graphic ? g_cursor_graphic->visible : false);
    // (the hotspot 0x5a6930/34 set from the two locals heldCursor and activeCursor never write: 0; the port has none)
    charCursor();                                    // 0x4abda0
    return c;
}

}  // namespace Game
