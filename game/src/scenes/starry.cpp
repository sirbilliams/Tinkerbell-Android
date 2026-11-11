// StarrySkies_Scene, reconstructed from "Tinker Bell.exe" (see starry.h).
#include "starry.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "voiceover.h"
#include "cannon.h"
#include "peter_reward.h"
#include "dustlift.h"
#include "toolbar.h"
#include "backwardsgo.h"
#include "cloudburst.h"
#include "tb.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool dbg_on() { return getenv("TB_DEBUG_STARRY") != nullptr; }   // test aid
static Animation *anim_of(Attachment *a) { return static_cast<Animation *>(a->g); }

std::vector<std::string> StarrySkies_Scene::win_lines;   // 0x5d2c50

StarrySkies_Scene &starry_scene() {                  // the static at 0x5d2c60
    static StarrySkies_Scene s("starry");
    return s;
}

static const char *const kGround[5] = {"cat_coken_on_ground", "dog_coken_on_ground", "elephant_coken_on_ground",
                                       "mouse_coken_on_ground", "tiger_coken_on_ground"};   // 0x53c1c4.. (the exe's
                                                                                            // spelling)
static const char *const kAnimals[5] = {"cat", "dog", "mouse", "elephant", "tiger"};   // 0x53c278.. (this order)

// ---------------------------------------------------------------- StarryInterface2D
StarryInterface2D::StarryInterface2D()               // 0x411b90 (enter's)
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {}

StarryInterface2D::~StarryInterface2D() {            // 0x472c10
    delete bar;                                      // (each v3(1))
    delete mou;
    delete cat;
    delete dog;
    delete tig;
    delete ele;
}

void StarryInterface2D::load() {                     // v15 0x472ce0
    bar = Gorilla::gnew<Animation>("clou_if_puzbar_");   // 0x4243c0 + 0x404c20
    bar->load();                                     // v15
    mou = Gorilla::gnew<Animation>("clou_if_moupuz_");
    mou->load();
    cat = Gorilla::gnew<Animation>("clou_if_catpuz_");
    cat->load();
    dog = Gorilla::gnew<Animation>("clou_if_dogpuz_");
    dog->load();
    tig = Gorilla::gnew<Animation>("clou_if_tigpuz_");
    tig->load();
    ele = Gorilla::gnew<Animation>("clou_if_elepuz_");
    ele->load();
    setZ(10);                                        // 0x411e70(0xa)
}

// v16 0x462b60 (shared with another five-picture class): the elephant's frames stay loaded (the exe's).
void StarryInterface2D::unload() {
    Graphic::unload();                               // 0x412020
    bar->unload();                                   // v16
    mou->unload();
    cat->unload();
    dog->unload();
    tig->unload();
}

// v14 0x472e90: with a piece won (state 4): the bar and each piece won.
void StarryInterface2D::draw() {
    State &st = Game::state();
    if (!st.anyStarry(4)) return;
    bar->draw();                                     // v14
    Animation *const by[5] = {cat, dog, ele, mou, tig};   // the jump table 0x472f1c
    for (int i = 0; i < 5; i++)
        if (st.starry[i].state == 4) by[i]->draw();
}

// ---------------------------------------------------------------- the scene
// 0x477b30
StarrySkies_Scene::StarrySkies_Scene(const char *nm)
    : TinkerbellRaytraceScene(nm),
      ambient(&rt, nm),                              // 0x4df580
      mappiece(&rt),                                 // 0x4e8860
      back_click("BACK_VO_CUBgreet1", "BACK_VO_CUBrecrd1", "BACK_VO_CUBfun1", "BACK_VO_CUBgreet2",
                 "BACK_VO_CUBrecrd2", "BACK_VO_CUBfun2", "BACKWARDS_GO_CHAR", "", "", ""),   // 0x4c0990
      star_click("STAR_VO_PETgreet1", "STAR_VO_PETrecrd1", "STAR_VO_PETfun1", "STAR_VO_PETgreet2",
                 "STAR_VO_PETrecrd2", "STAR_VO_PETfun2", "COLOR_THE_CLOUD_CHAR", "", "", ""),
      clou_click("CLOU_VO_FOXgreet1", "CLOU_VO_FOXrecrd1", "CLOU_VO_FOXfun1", "CLOU_VO_FOXgreet2",
                 "CLOU_VO_FOXrecrd2", "CLOU_VO_FOXfun2", "CLOUD_BURST_CHAR", "CLOU_VO_FOXmore1",
                 "CLOU_VO_FOXmore2", "CLOU_VO_FOXoverly"),
      cubby("pearlToss_cubby", &rt, &lift),          // 0x4ccad0 (the exe's name)
      peter("colorTheCloud_peterPan", &rt, &lift),
      foxy("cloudBurst_foxy", &rt, &lift),
      clou("AA_CLOU_gs_sign_01", "CLOU_GS_CLOU_", "CLOU_GS_green_", "CLOU_GS_cap_", "CLOU_GS_star_"),   // 0x4e6400
      colr("AC_CLOU_gs_sign_01", "CLOU_GS_COLR_", "CLOU_GS_green_", "CLOU_GS_cap_", "CLOU_GS_star_"),
      back("AB_CLOU_gs_sign_01", "CLOU_GS_BACK_", "CLOU_GS_green_", "CLOU_GS_cap_", "CLOU_GS_star_"),
      puzpieces("STAR_LY_puzpieces_01", 0, 0, 0, 0, 0, 0, true) {   // 0x4179e0
    for (Sound *&m : tunes) m = new Sound("");       // 0x437290 x3 (named by load2)
    for (Sound *&f : fx) f = new Sound("");          // 0x437290 x7
    inactivity.init(&rt, 30000);                     // 0x4c70e0
    // the rabbit's idle lines for Peter's spot: the exe's
    inactivity.add(0, 1000.0f, "GLOB_VO_RABidle1", "GLOB_VO_RABidle2", "GLOB_VO_RABidle3", "");   // 0x4c7100
    inactivity.add(1, 1000.0f, "GLOB_VO_CUBidle1", "GLOB_VO_CUBidle2", "GLOB_VO_CUBidle3", "");
    inactivity.add(2, 1000.0f, "GLOB_VO_FOXidle1", "GLOB_VO_FOXidle2", "GLOB_VO_FOXidle3", "Cloud Burst Game");
}

StarrySkies_Scene::~StarrySkies_Scene() {            // 0x477fa0 (port: the exe's static is never destroyed)
    delete backgo;
    delete cloud;
    delete iface;
    delete reward;
    delete prompt;
    for (Sound *&m : tunes) { delete m; m = nullptr; }
    for (Sound *&f : fx) { delete f; f = nullptr; }
}

// v18 0x475cd0: the three characters into the scene's list, Raytrace_Scene's (0x4aa200: port: nothing), the tunes
// and sounds, the signs' games, the end-of-game lines, the win lines (appended again on every call: the exe's
// one-time guard covers only the vector's construction), each character's animations and lines.
void StarrySkies_Scene::load2() {
    chars.push_back(&cubby);                         // 0x430dd0 on +0x2820
    chars.push_back(&peter);
    chars.push_back(&foxy);
    static const char *const tn[3] = {"back_mu_music", "clou_mu_music", "star_mu_music"};   // 0x53c064
    for (int i = 0; i < 3; i++) {
        tunes[i]->setName(tn[i]);                    // 0x424400
        tunes[i]->mode = 2;                          // +0x338
        tunes[i]->type = 6;                          // +0x340
    }
    static const char *const fn[7] = {"clou_fx_appearpoof", "clou_fx_cathunt", "clou_fx_doghunt", "clou_fx_elehunt",
                                      "clou_fx_mouhunt", "clou_fx_tighunt", "star_ax_birdwing"};   // 0x53c070
    for (int i = 0; i < 7; i++) fx[i]->setName(fn[i]);
    star_sign.set("Color The Cloud Game", "STAR_VO_PETgoinfo", "STAR_VO_PETgo2", "", nullptr);   // 0x4c7f00
    back_sign.set("Backwards Go Game", "BACK_VO_CUBgoinfo", "BACK_VO_CUBgo2", "", nullptr);
    clou_sign.set("Cloud Burst Game", "CLOU_VO_FOXgoinfo", "CLOU_VO_FOXgo2", "CLOU_VO_FOXnogo", nullptr);
    back_end.setLines("BACK_VO_CUBtoobad", "BACK_VO_CUBuwin1", "BACK_VO_CUBuwin2", "BACK_VO_CUBuwin3",
                      "BACK_VO_CUBchalgm", "BACK_VO_CUBpuzzgm");   // 0x4c1900
    star_end.setLines("STAR_VO_PETtoobad", "STAR_VO_PETuwin1", "STAR_VO_PETuwin2", "STAR_VO_PETuwin3",
                      "STAR_VO_PETviewgm", "STAR_VO_PETpuzzgm");
    clou_end.setLines("CLOU_VO_FOXuwin1", "CLOU_VO_FOXother", "CLOU_VO_FOXtoobad", "CLOU_VO_FOXuwin2",
                      "CLOU_VO_FOXuwin3");           // 0x4c2a10
    clou_end.setPiece(&mappiece);                    // 0x4c2ad0
    cloud_inactivity.add("CLOU_VO_FOXprompt", false);   // 0x4c46e0
    // Peter's first win line three times: the exe's
    static const char *const wins[9] = {"BACK_VO_CUBuwin1", "BACK_VO_CUBuwin2", "BACK_VO_CUBuwin3",
                                        "STAR_VO_PETuwin1", "STAR_VO_PETuwin1", "STAR_VO_PETuwin1",
                                        "CLOU_VO_FOXother", "CLOU_VO_FOXuwin2", "CLOU_VO_FOXuwin3"};
    for (const char *w : wins) win_lines.push_back(w);
    VoiceOver &vo = VoiceOver::get();

    cubby.setIdles("GLOB_CA_CUBroot_", "GLOB_CA_CUBidle1_", "GLOB_CA_CUBidle2_", "GLOB_CA_CUBidle3_");   // 0x4cd650
    cubby.setTalks("GLOB_CA_CUBroottk_", "GLOB_CA_CUBtalk1_", "GLOB_CA_CUBtalk2_", "GLOB_CA_CUBtalk3_");   // 0x4ce2d0
    cubby.setWave("GLOB_CA_CUBwin_", &win_lines);    // 0x4ce880
    cubby.setFlyIdle("GLOB_CA_CUBflyid_");           // 0x4ce850
    cubby.eyes = "CUB_eyes";                         // +0x3374
    cubby.setLip("backwardsGo_cubby_lip", "CUB_mout", 0);   // 0x4ccd20
    static const char *const cub[28] = {
        "BACK_VO_CUBbgback", "BACK_VO_CUBbgdown", "BACK_VO_CUBbgforw", "BACK_VO_CUBbgleft", "BACK_VO_CUBbgrght",
        "BACK_VO_CUBbgup", "BACK_VO_CUBcall1", "BACK_VO_CUBcall2", "BACK_VO_CUBcall3", "BACK_VO_CUBchalgm",
        "BACK_VO_CUBfun1", "BACK_VO_CUBgback", "BACK_VO_CUBgdown", "BACK_VO_CUBgforw", "BACK_VO_CUBgleft",
        "BACK_VO_CUBgo2", "BACK_VO_CUBgoinfo", "BACK_VO_CUBgreet1", "BACK_VO_CUBgrght", "BACK_VO_CUBgup",
        "BACK_VO_CUBpuzzgm", "BACK_VO_CUBrecrd1", "BACK_VO_CUBstart", "BACK_VO_CUBtoobad", "BACK_VO_CUBuwin1",
        "BACK_VO_CUBuwin2", "BACK_VO_CUBuwin3", nullptr};
    for (const char *const *l = cub; *l; l++) vo.registerLine(*l, &cubby);   // 0x4c91b0 (+0x3368)

    peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_", "GLOB_CA_PETidle3_");
    peter.setTalks("GLOB_CA_PETroottk_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_");
    peter.setWave("GLOB_CA_PETwin_", &win_lines);
    peter.setFlyIdle("GLOB_CA_PETflyid_");
    peter.eyes = "PET_eyes";                         // +0x3604
    peter.setLip("colorTheCloud_peterPan_lip", "PET_mout", 0);
    // the rabbit's third call among Peter's: the exe's
    static const char *const pet[20] = {
        "STAR_VO_PETcall1", "STAR_VO_PETcall2", "MCOV_VO_RABcall3", "STAR_VO_PETcall3", "STAR_VO_PETenter1",
        "STAR_VO_PETenter2", "STAR_VO_PETfun1", "STAR_VO_PETgo2", "STAR_VO_PETgoinfo", "STAR_VO_PETgreet1",
        "STAR_VO_PETpuzzgm", "STAR_VO_PETrecrd1", "STAR_VO_PETremind", "STAR_VO_PETtoobad", "STAR_VO_PETuwin1",
        "STAR_VO_PETuwin2", "STAR_VO_PETuwin3", "STAR_VO_PETviewgm", nullptr};
    for (const char *const *l = pet; *l; l++) vo.registerLine(*l, &peter);   // (+0x35f8)

    foxy.setIdles("GLOB_CA_FOXroot_", "GLOB_CA_FOXidle1_", "GLOB_CA_FOXidle2_", "GLOB_CA_FOXidle3_");
    foxy.setTalks("GLOB_CA_FOXroottk_", "GLOB_CA_FOXtalk1_", "GLOB_CA_FOXtalk2_", "GLOB_CA_FOXtalk3_");
    foxy.setWave("GLOB_CA_FOXwin_", &win_lines);
    foxy.setFlyIdle("GLOB_CA_FOXflyid_");
    foxy.eyes = "FOX_eyes";                          // +0x3894
    foxy.setLip("cloudBurst_foxy_lip", "FOX_mout", 0);
    // (0x4f1130 / 0x469240..0x4692f0: the list built, "CLOU_VO_sameANI" put first)
    static const char *const fox[64] = {
        "CLOU_VO_sameANI", "CLOU_VO_FOX1more", "CLOU_VO_FOX3more", "CLOU_VO_FOXbest", "CLOU_VO_FOXbig",
        "CLOU_VO_FOXbig2", "CLOU_VO_FOXblind", "CLOU_VO_FOXbone", "CLOU_VO_FOXcall1", "CLOU_VO_FOXcall2",
        "CLOU_VO_FOXcall3", "CLOU_VO_FOXcall4", "CLOU_VO_FOXcanine", "CLOU_VO_FOXcat", "CLOU_VO_FOXcheese",
        "CLOU_VO_FOXclaws", "CLOU_VO_FOXcub", "CLOU_VO_FOXfast", "CLOU_VO_FOXflee", "CLOU_VO_FOXfun1",
        "CLOU_VO_FOXfur", "CLOU_VO_FOXgo2", "CLOU_VO_FOXgoinfo", "CLOU_VO_FOXgray", "CLOU_VO_FOXgreet1",
        "CLOU_VO_FOXhole", "CLOU_VO_FOXhunt", "CLOU_VO_FOXking", "CLOU_VO_FOXleash", "CLOU_VO_FOXmane",
        "CLOU_VO_FOXmore1", "CLOU_VO_FOXnana", "CLOU_VO_FOXnice1", "CLOU_VO_FOXnice2", "CLOU_VO_FOXnice3",
        "CLOU_VO_FOXnice4", "CLOU_VO_FOXnine", "CLOU_VO_FOXnogo", "CLOU_VO_FOXother", "CLOU_VO_FOXoverly",
        "CLOU_VO_FOXpeanut", "CLOU_VO_FOXpickup", "CLOU_VO_FOXprompt", "CLOU_VO_FOXpurr", "CLOU_VO_FOXrecrd1",
        "CLOU_VO_FOXsmall", "CLOU_VO_FOXsmall2", "CLOU_VO_FOXtoobad", "CLOU_VO_FOXtusk", "CLOU_VO_FOXuwin1",
        "CLOU_VO_FOXuwin2", "CLOU_VO_FOXuwin3", "CLOU_VO_FOXwag", "CLOU_VO_FOXwolf", "CLOU_VO_FOXzoo", nullptr};
    for (const char *const *l = fox; *l; l++) vo.registerLine(*l, &foxy);   // (+0x3888)
}

// v19 0x474b80: the area's camera spot (then spot 0 for next time), the end-of-game tasks' bag, the Cloud Burst
// pack for the shelf, the pieces' bar, the signs and what they show, the pieces on the ground, the games' objects,
// the sounds, the shelf, the hot regions, the three characters (each registered for the idle lines of one
// another: the exe's), the puzzle picture, the inactivity spots, the map piece, the arrival and the tune.
void StarrySkies_Scene::enter() {
    reward = nullptr;                                // +0x6540
    prompt = nullptr;                                // +0x6544
    State &st = Game::state();
    float sp[4];
    st.areaSpot(2, sp);                              // 0x4ec0a0
    cam_x = sp[0]; cam_eye = sp[1]; cam_z = sp[2]; cam_deg = sp[3];   // 0x4aa0d0
    st.setAreaSpot(2, 0);                            // 0x4ec120
    open(name());                                    // v27 (TinkerbellRaytraceScene's 0x4ab0f0)
    star_end.setBag(bag);                            // 0x4c19d0
    back_end.setBag(bag);
    clou_end.setBag(bag);                            // 0x4c2ac0
    cloud_pack = ras_open("cloud_burst", "", &data_device());   // 0x419b90
    cloud_pack->load();                              // 0x41aa20
    ras_set_current(cloud_pack);                     // 0x41a920
    iface = Gorilla::gnew<StarryInterface2D>();      // 0x4243c0, 0xb0
    iface->load();                                   // v15
    over1.layer(0)->add(iface);                      // 0x41ece0, 0x41e930
    clou.init(&rt);                                  // 0x4e6440
    colr.init(&rt);
    back.init(&rt);
    back.enabled = false;                            // +0x3fcc
    clou.enabled = false;                            // +0x3f6c
    colr.enabled = false;                            // +0x3f9c
    Saves &sv = st.saves;
    switch (sv.gameInfo("Cloud Burst Game")) {       // 0x4e9d70 (*0x543504)
    case 1: clou.star->on = true; break;             // +0x3f64 (+0x17c)
    case 2: clou.star->on = true; clou.cap->on = true; break;   // +0x3f60
    default: break;
    }
    switch (sv.gameInfo("Backwards Go Game")) {      // *0x543508
    case 1: back.star->on = true; break;             // +0x3fc4
    case 2: back.star->on = true; back.cap->on = true; break;   // +0x3fc0
    default: break;
    }
    switch (sv.gameInfo("Color The Cloud Game")) {   // *0x543500
    case 0:
        if (st.last_game == 2 && st.last_result == 2) colr.star->on = true;   // +0x79c, +0x7a0, +0x3f94
        break;
    case 1:
        colr.star->on = true;
        if (st.last_game == 2 && st.last_result == 3) colr.cap->on = true;   // +0x3f90
        break;
    case 2: colr.star->on = true; colr.cap->on = true; break;
    default: break;
    }
    if (const char *t = getenv("TB_STARRY_TOKENS"))  // test aid (not in the game): the five pieces' states
        for (int i = 0; i < 5 && t[i]; i++) st.starry[i].state = t[i] - '0';
    groundTokens();                                  // 0x477fc0
    activate();                                      // v6
    reset();                                         // 0x421110
    back_on = false;                                 // +0x3f40
    backgo = new BackwardsGoGame();                  // 0x4f34f3 (0x298), 0x4705c0
    bits = 0;                                        // +0x3fd4
    cloud_on = false;                                // +0x3f41
    cloud = new CloudBurstGame();                    // 0x4f34f3 (0x1e90), 0x478b10
    tune = 3;                                        // +0x49bc
    for (Sound *f : fx) f->open();                   // 0x41f9d0(0)
    shelf(true);                                     // 0x478340
    bird_t = 0;                                      // +0x60b8
    star_hot = Gorilla::gnew<HotRegion>(&rt, "Color The Cloud Game", colr.sprite, "STAR_VO_PETcall1",
                                        "STAR_VO_PETcall1", "STAR_VO_PETcall2", "STAR_VO_PETcall3");   // 0x4c3b30
    start(star_hot);                                 // 0x420ef0
    clou_hot = Gorilla::gnew<HotRegion>(&rt, "Cloud Burst Game", clou.sprite, "CLOU_VO_FOXcall1",
                                        "CLOU_VO_FOXcall2", "CLOU_VO_FOXcall3", "CLOU_VO_FOXcall4");
    start(clou_hot);
    back_hot = Gorilla::gnew<HotRegion>(&rt, "Backwards Go Game", back.sprite, "BACK_VO_CUBcall1",
                                        "BACK_VO_CUBcall1", "BACK_VO_CUBcall2", "BACK_VO_CUBcall3");
    start(back_hot);
    star_hot->on = false;                            // +0xc0
    back_hot->on = false;
    clou_hot->on = false;
    stay = true;                                     // +0x6494
    VoiceOver &vo = VoiceOver::get();

    cub_pack = ras_open("Cubby_Char", "", &data_device());
    cub_pack->load();
    ras_set_current(cub_pack);
    cubby.init();                                    // v1
    cubby.setX((float)((double)back.sprite->x - 150.0f));   // 0x4cc8c0, 0x5142e8
    cubby.setZ((float)((double)back.sprite->z - 50.0f));    // 0x4cc960, 0x515190
    {
        static const char *const l[3] = {"MCOV_VO_RABcall1", "MCOV_VO_RABcall2", "MCOV_VO_RABcall3"};
        for (const char *s : l) vo.registerLine(s, &cubby);   // (+0x3368) the exe's
    }
    reloadPeter();                                   // (inlined: the same as 0x475990)
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
        for (const char *const *s = l; *s; s++) vo.registerLine(*s, &peter);   // (+0x35f8)
    }
    reloadFoxy();                                    // (inlined: the same as 0x475a10)
    {
        static const char *const l[3] = {"GLOB_VO_FOXidle1", "GLOB_VO_FOXidle2", "GLOB_VO_FOXidle3"};
        for (const char *s : l) vo.registerLine(s, &foxy);   // (+0x3888)
    }
    puzpieces.load();                                // v15
    puzpieces.visible = false;                       // +0x6514
    over1.layer(0)->add(&puzpieces);
    clou_click.sign = &puzpieces;                    // +0x3218
    inactivity.setSpot(0, colr.sprite->x, colr.sprite->z);   // 0x4c73f0
    inactivity.setSpot(1, back.sprite->x, back.sprite->z);
    inactivity.setSpot(2, clou.sprite->x, clou.sprite->z);
    mappiece.load();                                 // v11
    if (cloud_pack) ras_close(cloud_pack);           // 0x41b050, 0x419620
    cloud_pack = nullptr;
    if (!st.tutorial_skipped) arriveThroughClouds(false);   // +0x7bc, 0x4ab760
    ambient.bag = bag;                               // +0x29ac
    ambient.activate();                              // +0x18
    Raytrace::RayTrace::setMaxEye(0x173);            // 0x42b110
    music(2);                                        // 0x478640
    if (dbg_on())
        tb_log("starry: enter: signs clou %.0f,%.0f colr %.0f,%.0f back %.0f,%.0f cam %.0f,%.0f", clou.sprite->x,
               clou.sprite->z, colr.sprite->x, colr.sprite->z, back.sprite->x, back.sprite->z, rt.camx, rt.camz);
}

// 0x475990: Peter at Color the Clouds' sign (150 nearer).
void StarrySkies_Scene::reloadPeter() {
    peter_pack = ras_open("Peter_Char", "", &data_device());
    peter_pack->load();
    ras_set_current(peter_pack);
    peter.init();                                    // v1
    peter.setX(colr.sprite->x);                      // +0x3f88
    peter.setZ((float)((double)colr.sprite->z - 150.0f));
}

// 0x475a10: Foxy by the shelf.
void StarrySkies_Scene::reloadFoxy() {
    fox_pack = ras_open("Foxy_Char", "", &data_device());
    fox_pack->load();
    ras_set_current(fox_pack);
    foxy.init();
    foxy.setX(6275.1f);                              // 0x45c418cd
    foxy.setZ(2587.6f);                              // 0x4521b99a
}

// v20 0x475a70
void StarrySkies_Scene::leave() {
    if (reward) { reward->deactivate(); delete reward; reward = nullptr; }   // +0x1c, +0xc
    if (prompt) { prompt->deactivate(); delete prompt; prompt = nullptr; }
    ambient.deactivate();                            // +0x1c
    State &st = Game::state();
    if (st.dusting) {                                // +0x3c4
        st.dusting = false;
        bag->refund();                               // 0x4f07b0
    }
    VoiceOver::get().stop();                         // 0x4c9260
    puzpieces.unload();                              // v16
    clou_click.stop();                               // 0x4c12c0
    star_hot = back_hot = clou_hot = nullptr;        // (tasks of the scene's, ended with it)
    colr.release();                                  // 0x4e6530
    back.release();
    clou.release();
    for (Sound *f : fx) f->close();                  // 0x41fc50
    music(3);
    delete backgo;                                   // 0x470640 + free
    backgo = nullptr;
    delete cloud;                                    // 0x478d80 + free
    cloud = nullptr;
    deactivate();                                    // v7
    if (iface) iface->unload();                      // v16
    delete iface;                                    // v3(1)
    iface = nullptr;
    star_sign.stop();                                // 0x4c7fe0
    back_sign.stop();
    clou_sign.stop();
    back_end.deactivate();                           // 0x4c1400
    star_end.deactivate();
    clou_end.deactivate();
    mappiece.clear();                                // v12
    cubby.release();                                 // v2
    if (cub_pack) ras_close(cub_pack);               // 0x419620
    cub_pack = nullptr;
    peter.release();
    if (peter_pack) ras_close(peter_pack);
    peter_pack = nullptr;
    foxy.release();
    if (fox_pack) ras_close(fox_pack);
    fox_pack = nullptr;
    inactivity.stop();                               // 0x4c7770
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    set_cursor(st.cursor, true);                     // 0x4119b0
}

// 0x477fc0: the pieces still hidden (state 3) placed on the ground; a hard one shows only if a hiding sprite of
// this scene covers its spot (else it is logged and made easy).
void StarrySkies_Scene::groundTokens() {
    static const char *const prefixes[5] = {"CLOU_GA_catpuz_", "CLOU_GA_dogpuz_", "CLOU_GA_elepuz_",
                                            "CLOU_GA_moupuz_", "CLOU_GA_tigpuz_"};   // 0x53cb50..
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = Game::state().starry[i];
        if (t.state != 3) continue;
        rt.addSprite(kGround[i], prefixes[i], 1);    // 0x42d9d0
        Sprite3D *s = rt.findSprite(kGround[i]);     // 0x42dc70
        s->load();                                   // 0x42ef20
        s->x = t.x;                                  // +0x1630
        s->y = t.y;
        s->z = t.z;
        if (t.easy) continue;                        // 0x4ed260
        bool found = false;                          // 0x443590: find_if over the sprites
        for (Sprite3D *h : rt.sprites)
            if (hiding_sprite(name(), h) && hides_near(t.x, t.z, h)) { found = true; break; }
        if (found) {
            s->mode = 0;                             // +0x88
            continue;
        }
        vision_log("Hard hiding place at x=%f z=%f in scene %s is invalid.", (double)s->x, (double)s->z, name());
        t.setEasy();                                 // v3
    }
}

// 0x478340: on: the shelf ("interface3D", made once) by Foxy's sign (which moves aside) with the five animals on it,
// shown once the pieces are all won; off: the shelf gone.
void StarrySkies_Scene::shelf(bool on) {
    if (!on) {
        rt.removeSprite("interface3D");              // 0x42da60
        return;
    }
    if (!rt.findSprite("interface3D")) {
        rt.addSprite("interface3D", "clou_am_shelf_", 1);
        Sprite3D *s = rt.findSprite("interface3D");
        s->load();
        s->x = 6400.1f;                              // 0x45c800cd
        rt.findSprite("AA_CLOU_gs_sign_01")->x = 6300.1f;   // 0x45c4e0cd
        s->z = 2687.6f;                              // 0x4527f99a
        s->y = (float)((double)s->anim->frame(0)->h * 0.5f);   // 0x403ec0, +0x40, 0x513298
        rt.findSprite("AA_CLOU_gs_sign_01")->z = 2837.6f;   // 0x4531599a
        s->scale = 200.0f;                           // +0x84
        s->attach("mouse", "clou_ga_moushelf_", 100);   // 0x42f390
        s->attach("cat", "clou_ga_catshelf_", 100);
        s->attach("dog", "clou_ga_dogshelf_", 100);
        s->attach("elephant", "clou_ga_eleshelf_", 100);
        s->attach("tiger", "clou_ga_tigshelf_", 100);
    }
    State &st = Game::state();
    static const char *const order[5] = {"mouse", "cat", "dog", "elephant", "tiger"};
    for (const char *n : order) rt.findAttached(n)->on = st.anyStarry(5);   // 0x42dd00, 0x4ee070, +0x17c
}

// 0x478640
void StarrySkies_Scene::music(int n) {
    if (tune != 3) {
        if (n == tune && tunes[tune]->isPlaying()) return;   // 0x41fec0
        tunes[tune]->close();                        // 0x41fc50
    }
    tune = n;
    if (n == 3) return;
    tunes[n]->close();
    tunes[n]->open();                                // 0x41f9d0(0)
    tunes[n]->play();                                // 0x41fd50
}

void StarrySkies_Scene::hotsOn(bool on) {            // 0x478710
    star_hot->on = on;                               // +0xc0
    back_hot->on = on;
    clou_hot->on = on;
}

void StarrySkies_Scene::signsOn(bool on) {
    back.enabled = on;                               // +0x3fcc
    clou.enabled = on;                               // +0x3f6c
    colr.enabled = on;                               // +0x3f9c
}

// 0x478740: to the map: the area's spot 2 is where the camera is now; elsewhere: the area's spot 3.
void StarrySkies_Scene::leaveTo(const char *scene) {
    float sp[4];
    if (!strcmp(scene, "map")) {                     // 0x423c60
        sp[0] = rt.camx;                             // 0x42b470
        sp[1] = rt.eyef;                             // 0x42b480
        sp[2] = rt.camz;                             // 0x42b540
        sp[3] = (float)rt.angleDeg();                // 0x42b560
    } else {
        Game::state().cameraSpot(2, 3, sp);          // 0x4ec180
    }
    Game::state().setSpot4(2, sp);                   // 0x4ec0e0
    change_scene(scene);                             // 0x41ddf0
}

// 0x478850: the hot regions off, input paused, the camera's place kept (spot 2), no moving; a fade to black, then
// the game (its update sees back_on).
void StarrySkies_Scene::startBack() {
    hotsOn(false);                                   // (inlined)
    paused = true;                                   // 0x5d2d88
    backgo->start();                                 // 0x471980
    float sp[4] = {rt.camx, rt.eyef, rt.camz, (float)rt.angleDeg()};   // 0x42b470, 0x42b480, 0x42b540, 0x42b560
    Game::state().setSpot4(2, sp);                   // 0x4ec0e0
    forceOff();                                      // 0x4abf50 (on the static)
    fader->visible = true;                           // +0x7c
    fader->start(false, 2000);                       // 0x4a9580
    back_on = true;                                  // 0x5d6ba0
    Game::state().last_game = 3;                     // +0x79c
}

// 0x478500: the area's tune for Cloud Burst, the hot regions off, the pieces given (5), the shelf, no moving, the
// fuse by skill (a game won before), the prompts, the first round.
void StarrySkies_Scene::startCloud() {
    music(1);
    hotsOn(false);                                   // (inlined, on the static's fields)
    cloud_on = true;                                 // +0x3f41
    bits |= 1;                                       // +0x3fd4
    State &st = Game::state();
    for (TokenInfo &t : st.starry) t.state = 5;      // (0x4ee0d0 inlined, without its save)
    shelf(true);
    forceOff();                                      // v24
    int g = st.saves.gameInfo("Cloud Burst Game");   // *0x543504
    if (g == 1) bits |= 4;
    else if (g == 2) bits |= 8;
    switch (st.skill) {                              // +0x68
    case 1: secs = 150; break;
    case 2: case 3: secs = 120; break;
    default: break;
    }
    if (g != 0) {
        Game::cannon().visible = true;               // state +0x83c
        Game::cannon().start(secs);                  // 0x4e3a50
    }
    st.last_game = 4;                                // +0x79c
    cloud_inactivity.start();                        // v12
    cloud->state = 0;                                // +0x1e8c
    cloud->start();                                  // (+0x1d14).0x471be0
}

// 0x478940: Tink flies to the shelf's animal least shown so far (never an answer: its count is 9999999), at the
// side of it seen from the camera, and points down at it.
void StarrySkies_Scene::cloudHint() {
    tink->clearQueue();                              // 0x4ddf30
    int *m = cloud->asked;                           // +0x1ce8: the first smallest
    for (int *p = cloud->asked + 1; p != cloud->asked + 5; p++)
        if (*p < *m) m = p;
    int i = (int)(m - cloud->asked);
    (*m)++;
    static const char *const names[5] = {"cat", "dog", "elephant", "mouse", "tiger"};   // the table 0x478ae8
    Attachment *at = rt.findAttached(names[i]);      // 0x42dd00
    float p[3];
    rt.stickerPos(at->name.c_str(), p);              // 0x42df30
    int w = at->g->ow;                               // +0x150 +0x3c
    float P[3] = {p[0], 0.1f, p[2]};                 // 0x3dcccccd
    float C[3];
    rt.camPos(C);                                    // 0x42b490
    double dx = (double)C[0] - P[0], dy = (double)C[1] - P[1], dz = (double)C[2] - P[2];
    float dist = (float)(sqrt(dx * dx + dy * dy + dz * dz) - 40.0f);   // 0x425d50, 0x5144c0
    double ang = atan2((double)P[2] - C[2], (double)P[0] - C[0]);   // 0x425d80
    ang += atan2((double)w * 0.5f + 10.0f, (double)dist);   // 0x513298, 0x514f48 (fpatan)
    float nx = (float)(cos(ang) * dist + C[0]);
    float nz = (float)(sin(ang) * dist + C[2]);
    tink->pushBasic(2, nx, (float)((double)p[1] - 75.0f), nz);   // 0x4de020, 0x514e5c
    tink->pointDown();                               // 0x4dba20
    tinkHome();                                      // 0x4abb10
}

// (inlined in onKey) Escape during Cloud Burst.
void StarrySkies_Scene::cloudSkip() {
    cloud_on = false;                                // +0x3f41
    bits |= 0x10;                                    // +0x3fd4
    show_cursor();                                   // 0x4119d0
    music(2);
    hotsOn(true);
    forceNone();                                     // v23
    cloud->hideAll();                                // (0x47a320 inlined)
    Game::cannon().hide();                           // 0x4e3bf0
    tink->wanders = true;                            // +0x13c
    inactivity.start();                              // 0x4c7730
    hotsOn(true);
    cloud_inactivity.stop();                         // v13
    cloud->repeater.stop();                          // +0x1d88, 0x4c60f0
    signsOn(true);
}

// v26 0x473b90: Escape ends a game played here.
bool StarrySkies_Scene::onKey() {
    if (paused) return false;                        // +0x128
    State &st = Game::state();
    if (stateKeys()) return true;                    // 0x4ec5f0 (+0x12f4)
    if (!g_key.down) return false;
    if (st.key((int)g_key.vk)) return true;          // 0x4baef0
    if ((int)g_key.vk != st.skip_key) return false;  // +0x74
    if (cloud_on) cloudSkip();
    if (back_on && !fader->busy()) backgo->skip();   // 0x4a95c0, 0x4719a0
    return true;
}

// v32 0x4748a0: the base's (0x443820, copied) when Backwards Go is not on.
void StarrySkies_Scene::hover() {
    if (back_on) return;                             // +0x3f40
    TinkerbellRaytraceScene::hover();
}

// 0x473860: a piece on the ground clicked: an easy one is won (state 4, saved); a hidden one: Peter's "it's hidden"
// line. A hiding place over a hidden piece: "sprinkle dust" line.
bool StarrySkies_Scene::pickPiece(Sprite3D *s) {
    State &st = Game::state();
    static const int order[5] = {0, 1, 2, 4, 3};     // cat, dog, elephant, tiger, mouse: the exe's compares
    for (int k = 0; k < 5; k++) {
        int i = order[k];
        if (s->name != kGround[i]) continue;         // 0x42f330
        if (!st.starry[i].easy) {                    // 0x4ed260
            VoiceOver::get().play("GLOB_CA_PEThide");   // 0x4c9400
            return false;
        }
        st.starry[i].state = 4;
        st.saveStarryTokens();                       // 0x4edd90
        st.saves.flush();                            // 0x4e9840
        s->anim->visible = false;                    // +0x70 +0x7c
        return true;
    }
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = st.starry[i];
        if (!t.hiddenStarry()) continue;             // 0x4ed160
        if (hiding_sprite("starry", s) && hides_near(t.x, t.z, s)) {   // 0x4e6850, 0x4e69f0
            VoiceOver::get().play("GLOB_CA_PEThddust");
            return false;
        }
    }
    return false;
}

// 0x4de8d0 with the scene's functor (StarryTokenContainer, vtable 0x515138: any 0x472930, target 0x472960): with
// a piece still hidden (state 3), Tink guides to the first one's sprite.
static bool starry_token_help(StarrySkies_Scene *sc) {
    sc->tink->clearQueue();                          // 0x4ddf30
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (st.starry[i].state != 3) continue;
        sc->tink->guideTo(sc->rt.findSprite(kGround[i]));   // 0x4dc250
        return true;
    }
    return false;
}

// 0x4de970's "starry" branch: a game's piece to win (Color the Clouds' 2, Backwards Go's 1), Cloud Burst's map
// segment, or the map; else a game not won twice, or the map if the cap is not won yet; else no help.
bool StarrySkies_Scene::tinkHelp() {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {                    // 0x4defb0
        Sprite3D *s = nullptr;
        if (st.starry[i].state == 2) s = rt.findSprite("AC_CLOU_gs_sign_01");
        else if (st.starry[i].state == 1) s = rt.findSprite("AB_CLOU_gs_sign_01");
        if (s) { tink->guideTo(s); return true; }
    }
    if (!st.saves.mapSegment("Starry")) {            // 0x4ea9f0
        tink->guideTo(rt.findSprite("AA_CLOU_gs_sign_01"));
        return true;
    }
    if (!st.saves.allMapSegments()) {                // 0x4eaa70
        tink->toMap();                               // 0x4db5b0
        return true;
    }
    const char *sign = nullptr;                      // 0x4df010 (0x4f10e0: "Info" 2)
    if (st.saves.gameInfo("Color The Cloud Game") != 2) sign = "AC_CLOU_gs_sign_01";
    else if (st.saves.gameInfo("Backwards Go Game") != 2) sign = "AB_CLOU_gs_sign_01";
    else if (st.saves.gameInfo("Cloud Burst Game") != 2) sign = "AA_CLOU_gs_sign_01";
    if (sign) {
        if (Sprite3D *s = rt.findSprite(sign)) {
            tink->guideTo(s);
            return true;
        }
    }
    if (!st.saves.hasCap()) {                        // 0x4e9af0
        tink->toMap();
        return true;
    }
    tink->noHelp();                                  // 0x4dc1d0
    return false;
}

// v25 0x472f30
void StarrySkies_Scene::onMouse() {
    if (paused) return;                              // +0x128
    State &st = Game::state();
    if (bell.click()) {                              // 0x4e7180 (+0x1fe8)
        if (!bag->has(bag->cost2())) return;         // 0x4f08c0, 0x4f02d0
        bag->spend();                                // 0x4f0860
        if (cloud_on) { cloudHint(); return; }       // 0x478940
        if (back_on) { backgo->hint(); return; }     // 0x470f00
        if (starry_token_help(this)) return;         // 0x4de8d0
        tinkHelp();                                  // 0x4de970
        return;
    }
    if (mouse_down()) {
        if (bag->click()) return;                    // 0x4f02f0
        if (mouse_down() && map.hit(g_mouse.x, g_mouse.y)) {   // +0x19a0 v11
            int r = map.click(g_mouse.x, g_mouse.y, cloud_on || back_on);   // 0x4efc90
            if (r == 2) leaveTo("map");              // 0x478740
            return;
        }
    }
    if (st.dusting) {                                // +0x3c4
        Sprite3D *d = nullptr;
        Attachment *da = nullptr;
        bag->sprinkle(&rt, &d, &da);                 // 0x4f0480
        if (!d) return;
        for (int i = 0; i < 5; i++) {
            TokenInfo &t = st.starry[i];
            if (!t.hiddenStarry()) continue;         // 0x4ed160
            if (!hiding_sprite("starry", d)) continue;   // 0x4e6850
            if (!hides_near(t.x, t.z, d)) continue;  // 0x4e69f0
            t.setEasy();                             // v3
            Sprite3D *g = rt.findSprite(kGround[i]);
            g->anim->setHit(-1, false);              // v6
            g->mode = 1;                             // +0x88
        }
        lift->add(d, 500);                           // 0x4bf0b0
        return;
    }
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    if (dbg_on() && mouse_down()) tb_log("starry: click %d,%d on %s", g_mouse.x, g_mouse.y, s ? s->name.c_str() : "-");
    if (cloud_on)                                    // the shelf's animals back to their first frame
        for (const char *n : kAnimals) anim_of(rt.findAttached(n))->setFrame(0);   // 0x42dd00, 0x403f20
    if (a && cloud_on)                               // and the one under the mouse lit
        for (const char *n : kAnimals)
            if (a->name == n) anim_of(a)->setFrame(1);   // 0x423840
    if (clou.mouse() && mouse_down() && !SignClick::active_sign) {   // 0x4e65b0, 0x6311ec
        if (cloud_on || back_on) return;
        if (st.allStarry(4)) st.starryDone();        // 0x4ee0a0, 0x4ee0d0
        signsOn(false);
        hotsOn(false);
        inactivity.stop();                           // 0x4c7770
        clou_sign.start();                           // 0x4c7fa0
        if (st.anyStarry(5) || st.allStarry(4)) {    // 0x4ee070, 0x4ee0a0
            // port: the exe reads the picked sprite blindly (the sign under the mouse: one is there)
            float x = s ? s->x : clou.sprite->x, z = s ? s->z : clou.sprite->z;
            st.setTinkHome((float)((double)x + 200.0f), 700.0f, (float)((double)z + 100.0f));   // 0x4e63b0
            tinkHome();                              // 0x4abb10
        }
        return;
    }
    if (back.mouse() && mouse_down() && !SignClick::active_sign) {
        if (cloud_on || back_on) return;
        signsOn(false);
        hotsOn(false);
        inactivity.stop();
        back_sign.start();
        return;
    }
    if (colr.mouse()) {
        if (!mouse_down()) return;
        if (!SignClick::active_sign) {
            if (cloud_on || back_on) return;
            star_sign.start();
            stay = false;                            // +0x6494
            return;
        }
    }
    if (!mouse_down()) return;
    if (s) {
        if (pickPiece(s)) return;                    // 0x473860 (no sound here)
        if (s == cubby.sprite()) { back_click.say(back_on); return; }   // 0x4cc8b0, 0x4c0b00
        if (s == peter.sprite()) { star_click.say(false); return; }
        if (s == foxy.sprite()) {
            if (!cloud_on) clou_click.start();       // 0x4c12a0
            else clou_click.say(true);
            return;
        }
    }
    if (a && cloud_on) {                             // an animal on the shelf: Cloud Burst's answer
        if (a->name == "cat") cloud->choose(0);      // 0x47bc10
        else if (a->name == "dog") cloud->choose(1);
        else if (a->name == "mouse") cloud->choose(3);
        else if (a->name == "elephant") cloud->choose(2);
        else if (a->name == "tiger") cloud->choose(4);
    }
}

// (inlined in run): the area interactive again: the inactivity prompts, the signs, the hot regions.
void StarrySkies_Scene::interactive() {
    inactivity.start();                              // 0x4c7730
    signsOn(true);
    hotsOn(true);
}

static bool cap_due() {
    Saves &sv = Game::state().saves;
    return sv.hasCap() && !sv.hasPetersCap();        // 0x4e9af0, 0x4ead40
}

// Peter's cap visit (every game won twice, the cap not given yet), waited for.
static void cap_visit(StarrySkies_Scene *sc) {
    sc->reward = Gorilla::gnew<PeterCapReward>(&sc->peter, sc, 2, sc->lift);   // 0x4bf280
    sc->reward->activate();                          // +0x18
    while (sc->reward->active()) sc->wait(30);       // +0x14
    delete sc->reward;                               // +0xc
    sc->reward = nullptr;
}

// v10 0x473d60: the arrival (down through the clouds), the area's welcome, Color the Clouds' end (and the cap);
// then every 30 ms: Cloud Burst started when Foxy's talk ends (all pieces in), its end (Peter's map prompt or the
// cap), Backwards Go started when Cubby's ends, its end, the games' updates (or the hover), a bird now and then.
// A click on Peter's sign ends the loop: up through the clouds to Color the Clouds.
void StarrySkies_Scene::run() {
    wait(1);
    State &st = Game::state();
    if (!st.tutorial_skipped)                        // +0x7bc
        while (arriveThroughClouds(false)) wait(0);  // 0x4ab760
    st.tutorial_skipped = false;
    while (fader->busy()) wait(0);                   // 0x4a95c0 (+0x124)
    st.firsts->welcome();                            // 0x4c64d0 (+0x788)
    while (st.firsts->welcoming()) wait(0);          // 0x4c6930
    if (st.last_game == 2) star_end.start();         // +0x79c, 0x4c19e0
    if (star_end.active()) {
        while (star_end.active()) wait(0);
        if (cap_due()) cap_visit(this);
    }
    interactive();
    while (stay) {                                   // +0x6494
        wait(30);
        if (clou_sign.active()) {
            while (clou_sign.active()) wait(0);
            set_cursor(st.cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
            if (st.anyStarry(5)) startCloud();       // 0x4ee070, 0x478500
            else {
                signsOn(true);
                hotsOn(true);
                inactivity.start();
            }
        }
        if (clou_end.active()) {
            if (st.last_result == 2) map.visible = false;   // +0x7a0, +0x1a1c
            while (clou_end.active()) wait(0);
            if (st.saves.allMapSegments() && !st.saves.hasXSpotSet()) {   // 0x4eaa70, 0x4ead80
                prompt = Gorilla::gnew<PeterPrompt>(&peter, this, 2, lift);   // 0x4bfea0
                prompt->activate();
                while (prompt->active()) wait(30);
                delete prompt;
                prompt = nullptr;
            } else if (cap_due()) {
                cap_visit(this);
            }
            map.visible = true;
            interactive();
        }
        if (back_sign.active()) {
            while (back_sign.active()) wait(0);
            startBack();                             // 0x478850
        }
        if (st.back_ended) {                         // +0x7a8
            st.back_ended = false;
            if (st.last_game != 5) {
                back_end.start();                    // 0x4c19e0
                music(2);
                while (back_end.active()) wait(0);
                if (cap_due()) cap_visit(this);
            }
            interactive();
        }
        if (back_on) backgo->update();               // 0x471650
        if (cloud_on) cloud->update();               // 0x47b7c0
        if (!back_on && !cloud_on) hover();          // v32
        uint32_t now = frame_time();                 // 0x4207b0
        if (now > bird_t && now - bird_t > 6000) {
            fx[6]->play();                           // +0x5d70, 0x41fd50
            bird_t = (uint32_t)(crt_rand() % 3000) + now;   // 0x4f54da
        }
    }
    leaving = true;                                  // +0x2831
    hide_cursor();                                   // 0x4119e0
    while (star_sign.active()) wait(0);
    while (flyThroughClouds(false)) wait(30);        // 0x4ab920
    st.last_game = 2;
    leaveTo("colorthecloud");                        // 0x478740 (the string at 0x6316d0)
    wait(0);
    finish();                                        // 0x420fe0
}

// v30 0x4749d0: the base's choice; the active one over the shelf's animals (Cloud Burst on), the signs and the
// pieces on the ground; the dust while dusting. Set (shown if one was picked), then the hand over a character.
Vision::Graphic *StarrySkies_Scene::cursorUpdate() {
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    State &st = Game::state();
    Vision::Graphic *c = st.cursor;                  // +0x5c
    if (Vision::Graphic *b = TinkerbellRaytraceScene::cursorUpdate()) c = b;   // 0x4ab6b0
    Vision::Graphic *active = st.cursor_active;      // +0x60
    if (a && cloud_on)
        for (const char *n : kAnimals)
            if (a->name == n) c = active;            // 0x423840
    if (s) {
        if (s == clou.sprite) c = active;            // +0x3f58
        if (s == back.sprite) c = active;            // +0x3fb8
        if (s == colr.sprite) c = active;            // +0x3f88
        static const int order[5] = {0, 1, 2, 4, 3};
        for (int i : order)
            if (s->name == kGround[i]) c = active;   // 0x42f330
    }
    if (st.dusting) c = &pixie;                      // +0x3c4, +0x17d8
    if (c) set_cursor(c, true);                      // 0x4119b0
    else set_cursor(nullptr, g_cursor_graphic ? g_cursor_graphic->visible : false);
    charCursor();                                    // 0x4abda0
    return c;
}

}  // namespace Game
