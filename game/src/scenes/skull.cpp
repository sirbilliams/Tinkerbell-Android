// Skull_Scene, reconstructed from "Tinker Bell.exe" (see skull.h).
#include "skull.h"
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
static bool dbg_on() { return getenv("TB_DEBUG_SKULL") != nullptr; }   // test aid

std::vector<std::string> Skull_Scene::win_lines;     // 0x5e0178

Skull_Scene &skull_scene() {                         // the static at 0x5dac88
    static Skull_Scene s("skull");                   // 0x483d20
    return s;
}

void skull_refresh_tokens() { skull_scene().refreshTokens(); }   // 0x4857a0 on the static

// 0x483e60 (i): the instruments on the ground, and their names on the bar (0x53d83c..0x53d85c).
static const char *const kGround[5] = {"drum_token_on_ground", "mara_token_on_ground", "horn_token_on_ground",
                                       "flute_token_on_ground", "coco_token_on_ground"};
static const char *const kNames[5] = {"drum", "mara", "horn", "flute", "coco"};

static bool cap_due() {
    Saves &sv = Game::state().saves;
    return sv.hasCap() && !sv.hasPetersCap();        // 0x4e9af0, 0x4ead40
}

// 0x4de8d0 with the scene's functor (SkullTokenContainer, vtable 0x515500: any 0x483d60, target 0x483d90): with
// an instrument still hidden (state 1), Tink guides to the first one whose sprite is there.
static bool skull_token_help(Skull_Scene *sc) {
    sc->tink->clearQueue();                          // 0x4ddf30
    State &st = Game::state();
    bool any = false;                                // 0x483d60
    for (const TokenInfo &t : st.skull)
        if (t.state == 1) any = true;
    if (!any) return false;
    Sprite3D *s = nullptr;                           // 0x483d90
    for (int i = 0; i < 5 && !s; i++)
        if (st.skull[i].state == 1) s = sc->rt.findSprite(kGround[i]);   // 0x42dc70
    sc->tink->guideTo(s);                            // 0x4dc250
    return true;
}

// ---------------------------------------------------------------- the scene
// 0x488260
Skull_Scene::Skull_Scene(const char *nm)
    : TinkerbellRaytraceScene(nm),
      ambient(&rt, nm),                              // 0x4df580
      mappiece(&rt),                                 // 0x4e79d0
      echo_click("ECHO_VO_RABgreet1", "ECHO_VO_RABrecrd1", "ECHO_VO_RABfun1", "ECHO_VO_RABgreet2",
                 "ECHO_VO_RABrecrd2", "ECHO_VO_RABfun2", "ECHO_ECHO_CHAR", "ECHO_VO_RABmore1", "ECHO_VO_RABmore2",
                 "ECHO_VO_RABoverly"),               // 0x4c0990
      firefly_click("SKLR_VO_PETgreet1", "SKLR_VO_PETrecrd1", "SKLR_VO_PETfun1", "SKLR_VO_PETgreet2",
                    "SKLR_VO_PETrecrd2", "SKLR_VO_PETfun2", "FIREFLY_CHAR", "", "", ""),
      shadow_click("SHBX_VO_SKNgreet1", "SHBX_VO_SKNrecrd1", "SHBX_VO_SKNfun1", "SHBX_VO_SKNgreet2",
                   "SHBX_VO_SKNrecrd2", "SHBX_VO_SKNfun2", "SHADOW_BOXING_CHAR", "", "", ""),
      rabbit("echo_rabbit", &rt, &lift),             // 0x4ccad0
      peter("firefly_peterPan", &rt, &lift),
      skunk("shadow_skunk", &rt, &lift),
      puzpieces("SKLR_LY_puzpieces_01", 0, 0, 0, 0, 0, 0, true),   // 0x4179e0
      echo("AB_SKLR_gs_sign_01", "SKLR_GS_ECHO_", "SKLR_GS_green_", "SKLR_GS_cap_", "SKLR_GS_star_"),   // 0x4e6400
      firefly("AC_SKLR_gs_sign_01", "SKLR_GS_FRFLY_", "SKLR_GS_green_", "SKLR_GS_cap_", "SKLR_GS_star_"),
      shbx("AA_SKLR_gs_sign_01", "SKLR_GS_SHBX_", "SKLR_GS_green_", "SKLR_GS_cap_", "SKLR_GS_star_") {
    for (Sound *&m : tunes) m = new Sound("");       // 0x437290 x3 (named by load2)
    for (Sound *&f : fx) f = new Sound("");          // 0x437290 x2
    shbx_end.setLines("SHBX_VO_SKNtoobad", "SHBX_VO_SKNuwin1", "SHBX_VO_SKNuwin2", "SHBX_VO_SKNuwin3",
                      "SHBX_VO_SKNchalgm", "SHBX_VO_SKNpuzzgm");   // 0x4c1900
    pet_end.setLines("SKLR_VO_PETtoobad", "SKLR_VO_PETuwin1", "SKLR_VO_PETuwin2", "SKLR_VO_PETuwin3",
                     "SKLR_VO_PETviewgm", "SKLR_VO_PETpuzzgm");
    echo_end.setLines("ECHO_VO_RABuwin1", "ECHO_VO_RABother", "ECHO_VO_RABtoobad", "ECHO_VO_RABuwin2",
                      "ECHO_VO_RABuwin3");           // 0x4c2a10
    inactivity.init(&rt, 30000);                     // 0x4c70e0
    echo_end.setPiece(&mappiece);                    // 0x4c2ad0
    inactivity.add(0, 1000.0f, "GLOB_VO_PETidle1", "GLOB_VO_PETidle2", "GLOB_VO_PETidle3", "");   // 0x4c7100
    inactivity.add(1, 1000.0f, "GLOB_VO_SKNidle1", "GLOB_VO_SKNidle2", "GLOB_VO_SKNidle3", "");
    inactivity.add(2, 1000.0f, "GLOB_VO_RABidle1", "GLOB_VO_RABidle2", "GLOB_VO_RABidle3", "Echo Echo Game");
}

Skull_Scene::~Skull_Scene() {                        // 0x488780 (port: the exe's static is never destroyed)
    delete reward;
    delete prompt;
    delete echo_game;
    delete shadow;
    delete scursor;
    delete tokens;
    delete inst_snd;
    for (Sound *&m : tunes) { delete m; m = nullptr; }
    for (Sound *&f : fx) { delete f; f = nullptr; }
}

// v18 0x486bd0: Raytrace_Scene's (0x4aa200: port: not made), the tunes and sounds named, the signs' games, Shadow
// Boxing's prompt, the characters into the scene's list, the win lines (appended again on every call: the exe's
// one-time guard covers only the vector's construction), each character's animations and lines.
void Skull_Scene::load2() {
    static const char *const tn[3] = {"SHBX_MU_music", "SKLR_MU_music", "ECHO_MU_music"};   // 0x53d734
    for (int i = 0; i < 3; i++) {
        tunes[i]->setName(tn[i]);                    // 0x424400
        tunes[i]->mode = 2;                          // +0x338
        tunes[i]->type = 6;                          // +0x340
    }
    static const char *const fn[2] = {"sklr_ax_drip", "sklr_fx_bat"};   // 0x53d740
    for (int i = 0; i < 2; i++) fx[i]->setName(fn[i]);
    firefly_sign.set("Firefly Game", "SKLR_VO_PETgoinfo", "SKLR_VO_PETgo2", "", nullptr);   // 0x4c7f00 (*0x54351c)
    shadow_sign.set("Shadow Boxing Game", "SHBX_VO_SKNgoinfo", "SHBX_VO_SKNgo2", "", nullptr);   // (*0x543514)
    echo_sign.set("Echo Echo Game", "ECHO_VO_RABgoinfo", "ECHO_VO_RABgo2", "ECHO_VO_RABnogo", nullptr);   // (*0x543518)
    inactivity2.add("SHBX_VO_SKNprompt", false);     // 0x4c46e0
    chars.push_back(&rabbit);                        // 0x430dd0 on +0x2820
    chars.push_back(&peter);
    chars.push_back(&skunk);
    static const char *const wins[9] = {"ECHO_VO_RABuwin1", "ECHO_VO_RABuwin2", "ECHO_VO_RABuwin3",
                                        "SKLR_VO_PETuwin1", "SKLR_VO_PETuwin2", "SKLR_VO_PETuwin3",
                                        "SHBX_VO_SKNuwin1", "SHBX_VO_SKNuwin2", "SHBX_VO_SKNuwin3"};
    for (const char *w : wins) win_lines.push_back(w);   // 0x443260
    VoiceOver &vo = VoiceOver::get();

    rabbit.setIdles("GLOB_CA_RABroot_", "GLOB_CA_RABidle1_", "GLOB_CA_RABidle2_", "GLOB_CA_RABidle3_");   // 0x4cd650
    rabbit.setTalks("GLOB_CA_RABroottk_", "GLOB_CA_RABtalk1_", "GLOB_CA_RABtalk2_", "GLOB_CA_RABtalk3_");   // 0x4ce2d0
    rabbit.setWave("GLOB_CA_RABwin_", &win_lines);   // 0x4ce880
    rabbit.setFlyIdle("GLOB_CA_RABflyid_");          // 0x4ce850
    rabbit.eyes = "RAB_eyes";                        // +0x3748
    rabbit.setLip("echo_rabbit_lip", "RAB_mout", 0); // 0x4ccd20
    static const char *const rab[] = {
        "ECHO_VO_RABcall1", "ECHO_VO_RABcall2", "ECHO_VO_RABcall3", "ECHO_VO_RABcall4", "ECHO_VO_RABfun1",
        "ECHO_VO_RABgo1", "ECHO_VO_RABgo2", "ECHO_VO_RABgoinfo", "ECHO_VO_RABgreet1", "ECHO_VO_RABmore1",
        "ECHO_VO_RABnice1", "ECHO_VO_RABnice2", "ECHO_VO_RABnice3", "ECHO_VO_RABnice4", "ECHO_VO_RABnogo",
        "ECHO_VO_RABother", "ECHO_VO_RABoverly", "ECHO_VO_RABprompt", "ECHO_VO_RABrecrd1", "ECHO_VO_RABshadow",
        "ECHO_VO_RABtoobad", "ECHO_VO_RABuwin1", "ECHO_VO_RABuwin2", "ECHO_VO_RABuwin3", nullptr};
    for (const char *const *l = rab; *l; l++) vo.registerLine(*l, &rabbit);   // 0x4c91b0 (+0x373c)

    peter.setIdles("GLOB_CA_PETroot_", "GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_", "GLOB_CA_PETidle3_");
    peter.setTalks("GLOB_CA_PETroottk_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_");
    peter.setWave("GLOB_CA_PETwin_", &win_lines);
    peter.setFlyIdle("GLOB_CA_PETflyid_");
    peter.eyes = "PET_eyes";                         // +0x39d8
    peter.setLip("firefly_peterPan_lip", "PET_mout", 0);
    static const char *const pet[] = {
        "SKLR_VO_PETcall1", "SKLR_VO_PETcall2", "SKLR_VO_PETcall3", "SKLR_VO_PETenter1", "SKLR_VO_PETenter2",
        "SKLR_VO_PETfun1", "SKLR_VO_PETgo2", "SKLR_VO_PETgoinfo", "SKLR_VO_PETgreet1", "SKLR_VO_PETpuzzgm",
        "SKLR_VO_PETrecrd1", "SKLR_VO_PETremind", "SKLR_VO_PETtoobad", "SKLR_VO_PETuwin1", "SKLR_VO_PETuwin2",
        "SKLR_VO_PETuwin3", "SKLR_VO_PETviewgm", nullptr};
    for (const char *const *l = pet; *l; l++) vo.registerLine(*l, &peter);   // (+0x39cc)

    skunk.setIdles("GLOB_CA_SKNroot_", "GLOB_CA_SKNidle1_", "GLOB_CA_SKNidle2_", "GLOB_CA_SKNidle3_");
    skunk.setTalks("GLOB_CA_SKNroottk_", "GLOB_CA_SKNtalk1_", "GLOB_CA_SKNtalk2_", "GLOB_CA_SKNtalk3_");
    skunk.setWave("GLOB_CA_SKNwin_", &win_lines);
    skunk.setFlyIdle("GLOB_CA_SKNflyid_");
    skunk.eyes = "SKN_eyes";                         // +0x3c68
    skunk.setLip("shadow_skunk_lip", "SKN_mout", 0);
    static const char *const skn[] = {
        "SHBX_VO_SKN1more", "SHBX_VO_SKNcall1", "SHBX_VO_SKNcall2", "SHBX_VO_SKNcall3", "SHBX_VO_SKNchalgm",
        "SHBX_VO_SKNcroc", "SHBX_VO_SKNfun1", "SHBX_VO_SKNgo2", "SHBX_VO_SKNgoinfo", "SHBX_VO_SKNgone1",
        "SHBX_VO_SKNgone2", "SHBX_VO_SKNgreet1", "SHBX_VO_SKNhalfwy", "SHBX_VO_SKNhelp1", "SHBX_VO_SKNhelp2",
        "SHBX_VO_SKNnice1", "SHBX_VO_SKNnice2", "SHBX_VO_SKNnice3", "SHBX_VO_SKNnice4", "SHBX_VO_SKNpirate",
        "SHBX_VO_SKNprompt", "SHBX_VO_SKNpuzzgm", "SHBX_VO_SKNrecrd1", "SHBX_VO_SKNtoobad", "SHBX_VO_SKNuwin1",
        "SHBX_VO_SKNuwin2", "SHBX_VO_SKNuwin3", nullptr};
    for (const char *const *l = skn; *l; l++) vo.registerLine(*l, &skunk);   // (+0x3c5c)
}

// v19 0x485830: the area's camera spot (then spot 0 for next time), the scene, the end-of-game tasks' bag, Shadow
// Boxing (the animals, the boat, the crocodile, the net), Echo Echo (the instruments), the instruments' bar (those
// won shown), the signs and what they show, the instruments still hidden, the hot regions, the characters, the
// map piece, the pictures, the inactivity spots, the arrival, the ambient sprites, the cave's barrier, the sounds
// and the area's tune.
void Skull_Scene::enter() {
    reward = nullptr;                                // +0x54e0
    prompt = nullptr;                                // +0x54e4
    State &st = Game::state();
    float sp[4];
    st.areaSpot(4, sp);                              // 0x4ec0a0
    cam_x = sp[0]; cam_eye = sp[1]; cam_z = sp[2]; cam_deg = sp[3];   // 0x4aa0d0
    st.setAreaSpot(4, 0);                            // 0x4ec120
    open(name());                                    // v27 (TinkerbellRaytraceScene's 0x4ab0f0)
    shbx_end.setBag(bag);                            // 0x4c19d0
    pet_end.setBag(bag);
    echo_end.setBag(bag);                            // 0x4c2ac0
    shadow_on = false;                               // +0x43b8
    delete shadow;                                   // port: the exe's leave freed the last one
    shadow = new ShadowBoxing(&rt, &over1);          // 0x4f34f3 (0x378), 0x482360
    shadow->setup();                                 // 0x482600
    delete scursor;
    scursor = Gorilla::gnew<ShadowCursor>();         // 0x4243c0 (0xb8), 0x480f10
    scursor->load();                                 // v15
    echo_on = false;                                 // +0x43b9
    delete echo_game;
    echo_game = Gorilla::gnew<EchoEcho>(&rt);        // 0x4243c0 (0x4b0), 0x4896d0
    echo_game->setup();                              // 0x489c00
    delete tokens;
    tokens = Gorilla::gnew<TokenInterface>("ECHO_IF_interface_");   // 0x4243c0 (0x168), 0x4f0960
    tokens->add("mara", "ECHO_IF_mara_");            // 0x4f0ec0
    tokens->add("coco", "ECHO_IF_coco_");
    tokens->add("flute", "ECHO_IF_flte_");
    tokens->add("drum", "ECHO_IF_drum_");
    tokens->add("horn", "ECHO_IF_horn_");
    tokens->load();                                  // v15
    over1.layer(0)->add(tokens);                     // 0x41ece0, 0x41e930
    if (const char *t = getenv("TB_SKULL_TOKENS"))   // test aid (not in the game): the five instruments' states
        for (int i = 0; i < 5 && t[i]; i++) st.skull[i].state = t[i] - '0';
    for (int i = 0; i < 5; i++)
        if (st.skull[i].state == 5) tokens->show(kNames[i]);   // 0x4f1040 (the table 0x486924)
    fuse = &Game::cannon();                          // state +0x7c0
    echo.init(&rt);                                  // 0x4e6440
    firefly.init(&rt);
    shbx.init(&rt);
    echo.enabled = false;                            // +0x5468
    firefly.enabled = false;                         // +0x5498
    shbx.enabled = false;                            // +0x54c8
    Saves &sv = st.saves;
    switch (sv.gameInfo("Echo Echo Game")) {         // 0x4e9d70
    case 1: echo.star->on = true; break;             // +0x5460
    case 2: echo.star->on = true; echo.cap->on = true; break;   // +0x545c
    default: break;
    }
    switch (sv.gameInfo("Shadow Boxing Game")) {
    case 1: shbx.star->on = true; break;             // +0x54c0
    case 2: shbx.star->on = true; shbx.cap->on = true; break;   // +0x54bc
    default: break;
    }
    switch (sv.gameInfo("Firefly Game")) {
    case 0:
        if (st.last_game == 2 && st.last_result == 2) firefly.star->on = true;   // +0x5490
        break;
    case 1:
        firefly.star->on = true;
        if (st.last_game == 2 && st.last_result == 3) firefly.cap->on = true;   // +0x548c
        break;
    case 2:
        firefly.star->on = true;
        firefly.cap->on = true;
        break;
    default: break;
    }
    groundTokens();                                  // 0x488ca0
    activate();                                      // v6
    reset();                                         // 0x421110
    echo_hot = Gorilla::gnew<HotRegion>(&rt, "Echo Echo Game", echo.sprite, "ECHO_VO_RABcall1",
                                        "ECHO_VO_RABcall2", "ECHO_VO_RABcall3", "ECHO_VO_RABcall4");   // 0x4c3b30
    start(echo_hot);                                 // 0x420ef0
    firefly_hot = Gorilla::gnew<HotRegion>(&rt, "Firefly Game", firefly.sprite, "SKLR_VO_PETcall1",
                                           "SKLR_VO_PETcall1", "SKLR_VO_PETcall2", "SKLR_VO_PETcall3");
    start(firefly_hot);
    shadow_hot = Gorilla::gnew<HotRegion>(&rt, "Shadow Boxing Game", shbx.sprite, "SHBX_VO_SKNcall1",
                                          "SHBX_VO_SKNcall1", "SHBX_VO_SKNcall2", "SHBX_VO_SKNcall3");
    start(shadow_hot);
    echo_hot->on = false;                            // +0xc0
    firefly_hot->on = false;
    shadow_hot->on = false;
    VoiceOver &vo = VoiceOver::get();

    rabbit_pack = ras_open("Rabbit_Char", "", &data_device());   // 0x419b90
    rabbit_pack->load();                             // 0x41aa20
    ras_set_current(rabbit_pack);                    // 0x41a920
    rabbit.init();                                   // v1
    rabbit.setX(4425.0f);                            // 0x4cc8c0 (0x458a4800)
    rabbit.setZ(4432.0f);                            // 0x4cc960 (0x458a8000)
    {
        static const char *const l[] = {"MCOV_VO_RABcall1", "MCOV_VO_RABcall2", "MCOV_VO_RABcall3",
                                        "echo_vo_playone", "echo_vo_playtwo", "echo_vo_playthree",
                                        "echo_vo_playfour", "ECHO_VO_RABprompt", "ECHO_VO_RABnice1",
                                        "ECHO_VO_RABnice2", "ECHO_VO_RABnice3", "echo_vo_notright", nullptr};
        for (const char *const *s = l; *s; s++) vo.registerLine(*s, &rabbit);   // 0x4c91b0 (+0x373c)
    }

    peter_pack = ras_open("Peter_Char", "", &data_device());
    peter_pack->load();
    ras_set_current(peter_pack);
    peter.init();
    peter.setX(firefly.sprite->x);                   // +0x5484
    peter.setZ((float)((double)firefly.sprite->z - 150.0f));   // 0x5142e8
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
        for (const char *const *s = l; *s; s++) vo.registerLine(*s, &peter);   // (+0x39cc)
    }

    skunk_pack = ras_open("Skunk_Char", "", &data_device());
    skunk_pack->load();
    ras_set_current(skunk_pack);
    skunk.init();
    skunk.setX(shbx.sprite->x);                      // +0x54b4
    skunk.setZ((float)((double)shbx.sprite->z - 150.0f));
    {
        // "SKNI": the exe's spelling
        static const char *const l[] = {"GLOB_VO_SKNIidle1", "GLOB_VO_SKNIidle2", "GLOB_VO_SKNIidle3",
                                        "SHBX_VO_SKNhalfwy", "SHBX_VO_SKN1more", "SHBX_VO_SKNnice1",
                                        "SHBX_VO_SKNnice2", "SHBX_VO_SKNnice3", "SHBX_VO_SKNnice4",
                                        "SHBX_VO_SKNhelp1", nullptr};
        for (const char *const *s = l; *s; s++) vo.registerLine(*s, &skunk);   // (+0x3c5c)
    }

    mappiece.load();                                 // v11
    puzpieces.load();                                // v15
    puzpieces.visible = false;                       // +0x3e54
    over1.layer(0)->add(&puzpieces);
    echo_click.sign = &puzpieces;                    // +0x32ec
    inactivity.setSpot(0, firefly.sprite->x, firefly.sprite->z);   // 0x4c73f0
    inactivity.setSpot(1, shbx.sprite->x, shbx.sprite->z);
    inactivity.setSpot(2, echo.sprite->x, echo.sprite->z);
    if (!st.tutorial_skipped) arriveThroughClouds(false);   // +0x7bc, 0x4ab760
    ambient.bag = bag;                               // +0x29ac
    ambient.activate();                              // +0x18
    rt.addBarrier("echo_barrier_01", 4300.0f, 4230.0f, 4900.0f, 4230.0f, -100000.0f, 100000.0f);   // 0x42d7d0
    for (Sound *f : fx) f->open();                   // 0x41f9d0(0)
    tune = 1;                                        // +0x43d0 (the inlined music(1), without its first close)
    tunes[1]->close();                               // 0x41fc50
    tunes[1]->open();
    tunes[1]->play();                                // 0x41fd50
    drip_t = 0;                                      // +0x54dc
}

// v20 0x486940
void Skull_Scene::leave() {
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
    echo_click.stop();                               // 0x4c12c0
    for (Sound *f : fx) f->close();                  // 0x41fc50
    echo_hot = firefly_hot = shadow_hot = nullptr;   // (tasks of the scene's, ended with it)
    echo.release();                                  // 0x4e6530
    firefly.release();
    shbx.release();
    over1.clear();                                   // 0x41ebc0
    deactivate();                                    // v7
    scursor->unload();                               // v16
    delete scursor;                                  // v3(1)
    scursor = nullptr;
    shadow->teardown();                              // 0x482830
    delete shadow;                                   // 0x482590
    shadow = nullptr;
    echo_game->teardown();                           // 0x489d80
    delete echo_game;                                // v3(1)
    echo_game = nullptr;
    tokens->unload();                                // v16
    delete tokens;
    tokens = nullptr;
    if (tune != 3) tunes[tune]->close();
    tune = 3;
    mappiece.clear();                                // v12
    firefly_sign.stop();                             // 0x4c7fe0
    shadow_sign.stop();
    echo_sign.stop();
    shbx_end.deactivate();                           // 0x4c1400
    pet_end.deactivate();
    echo_end.deactivate();
    rabbit.release();                                // v2
    if (rabbit_pack) ras_close(rabbit_pack);         // 0x419620
    rabbit_pack = nullptr;
    peter.release();
    if (peter_pack) ras_close(peter_pack);
    peter_pack = nullptr;
    skunk.release();
    if (skunk_pack) ras_close(skunk_pack);
    skunk_pack = nullptr;
    inactivity.stop();                               // 0x4c7770
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    set_cursor(st.cursor, true);                     // 0x4119b0
}

// 0x4894c0
void Skull_Scene::music(int n) {
    if (tune != 3) tunes[tune]->close();             // 0x41fc50
    tune = n;
    if (n == 3) return;
    tunes[n]->close();
    tunes[n]->open();                                // 0x41f9d0(0)
    tunes[n]->play();                                // 0x41fd50
}

void Skull_Scene::hotsOn(bool on) {                  // 0x489560
    echo_hot->on = on;                               // +0xc0
    firefly_hot->on = on;
    shadow_hot->on = on;
}

void Skull_Scene::signsOn(bool on) {                 // 0x485810
    echo.enabled = on;                               // +0x5468
    firefly.enabled = on;                            // +0x5498
    shbx.enabled = on;                               // +0x54c8
}

// 0x4857a0: the instruments won (state 5) shown on the bar.
void Skull_Scene::refreshTokens() {
    for (int i = 0; i < 5; i++)
        if (Game::state().skull[i].state == 5) tokens->show(kNames[i]);   // 0x4f1040
}

// 0x489590: to the map the area's spot 1 is where the camera is now; elsewhere the area's spot 3.
void Skull_Scene::leaveTo(const char *scene) {
    float sp[4];
    if (strcmp("map", scene) == 0) {                 // 0x423c60
        sp[0] = rt.camx;                             // 0x42b470
        sp[1] = rt.eyef;                             // 0x42b480
        sp[2] = rt.camz;                             // 0x42b540
        sp[3] = (float)rt.angleDeg();                // 0x42b560
    } else {
        Game::state().cameraSpot(4, 3, sp);          // 0x4ec180
    }
    Game::state().setSpot4(4, sp);                   // 0x4ec0e0
    change_scene(scene);                             // 0x41ddf0
}

// 0x4893c0: the instrument's sound (a fresh one each time).
void Skull_Scene::instrument(int i) {
    static const char *const n[5] = {"echo_fx_drum", "echo_fx_marac", "echo_fx_horn", "echo_fx_flute",
                                     "echo_fx_coconut"};   // the table 0x4894a0
    const char *name = (i >= 0 && i <= 4) ? n[i] : "";   // port: the exe uses its stale argument slot
    if (inst_snd) {
        if (inst_snd->isPlaying()) inst_snd->stop(); // 0x41fec0, 0x41fe70
        delete inst_snd;                             // v3(1)
    }
    inst_snd = Gorilla::gnew<Sound>(name);           // 0x4243c0 (0x348), 0x420210
    inst_snd->open();
    inst_snd->play();
}

// 0x488ca0: the instruments still hidden (state 1) placed on the ground; a hard one shows only if a hiding sprite
// of this scene covers its spot (else it is logged and made easy).
void Skull_Scene::groundTokens() {
    static const char *const prefixes[5] = {"ECHO_GA_wPUZdrum_", "ECHO_GA_wPUZmara_", "ECHO_GA_wPUZhorn_",
                                            "ECHO_GA_wPUZflte_", "ECHO_GA_wPUZcoco_"};
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = Game::state().skull[i];
        if (t.state != 1) continue;
        rt.addSprite(kGround[i], prefixes[i], 1);    // 0x42d9d0
        Sprite3D *s = rt.findSprite(kGround[i]);     // 0x42dc70
        s->load();                                   // 0x42ef20
        s->x = t.x;
        s->y = t.y;
        s->z = t.z;
        if (t.easy) continue;                        // 0x4ed260
        bool found = false;                          // find_if over the sprites
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

// 0x489030: an instrument on the ground clicked: an easy one is won (state 5, its sound, shown on the bar, saved);
// a hidden one: Peter's "it's hidden" line. A hiding place over a hidden one: "sprinkle dust" line. (Nothing
// found: the signs back on if Shadow Boxing's is.)
bool Skull_Scene::pickToken(Sprite3D *s) {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (s->name != kGround[i]) continue;         // 0x42f330
        if (!st.skull[i].easy) {                     // 0x4ed260
            VoiceOver::get().play("GLOB_CA_PEThide");   // 0x4c9400
            return false;
        }
        st.skull[i].state = 5;
        instrument(i);                               // 0x4893c0
        for (int j = 0; j < 5; j++)
            if (st.skull[j].state == 5) tokens->show(kNames[j]);   // 0x4f1040
        st.saveSkullTokens();                        // 0x4ee4e0
        st.saves.flush();                            // 0x4e9840
        if (s->anim) s->anim->visible = false;       // +0x70 +0x7c
        return true;
    }
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = st.skull[i];
        if (!t.hidden()) continue;                   // 0x4ed1c0
        if (hiding_sprite("skull", s) && hides_near(t.x, t.z, s)) {   // 0x4e6850, 0x4e69f0
            VoiceOver::get().play("GLOB_CA_PEThddust");
            return false;
        }
    }
    if (shbx.enabled) signsOn(true);                 // +0x54c8
    return false;
}

// 0x4de970's "skull" branch: a game's instrument to win (Firefly's 2, Shadow Boxing's 3), Echo Echo's map
// segment, or the map; else a game not won twice, or the map if the cap is not won yet; else no help.
bool Skull_Scene::tinkHelp() {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        Sprite3D *s = nullptr;
        if (st.skull[i].state == 2) s = firefly.sprite;   // +0x5484
        else if (st.skull[i].state == 3) s = shbx.sprite;   // +0x54b4
        if (s) { tink->guideTo(s); return true; }
    }
    if (!st.saves.mapSegment("Skull")) {             // 0x4df0b0: 0x4ea8f0
        tink->guideTo(echo.sprite);                  // +0x5454
        return true;
    }
    if (!st.saves.allMapSegments()) {                // 0x4eaa70
        tink->toMap();                               // 0x4db5b0
        return true;
    }
    Sprite3D *sign = nullptr;                        // 0x4df110 (0x4f10e0: "Info" 2)
    if (st.saves.gameInfo("Firefly Game") != 2) sign = firefly.sprite;
    else if (st.saves.gameInfo("Shadow Boxing Game") != 2) sign = shbx.sprite;
    else if (st.saves.gameInfo("Echo Echo Game") != 2) sign = echo.sprite;
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

// 0x480da0: the skip key during Shadow Boxing: ended unplayed.
void Skull_Scene::shadowSkip() {
    State &st = Game::state();
    bits |= 0x10;
    st.last_game = 5;                                // +0x79c
    shadow_on = false;
    music(1);
    shadow->on = 0;
    shadow->fuse->hide();                            // 0x4e3bf0
    shadow->iface->visible = false;
    st.hover_up = false;                             // +0x75
    st.hover_down = false;                           // +0x76
    st.shadow_ended = true;                          // +0x7a5
    inactivity2.stop();                              // 0x4c4690
}

// 0x480e20: the eighth catch: won (the star, the cap, or played before: by "Info").
void Skull_Scene::shadowEnd() {
    State &st = Game::state();
    if (dbg_on()) tb_log("skull: shadow boxing won (%d caught, %u)", shadow->count(), frame_time());
    shadow_on = false;
    music(1);
    shadow->on = 0;
    shadow->fuse->hide();
    shadow->iface->visible = false;
    st.hover_up = false;
    st.hover_down = false;
    st.shadow_ended = true;
    inactivity2.stop();
    switch (st.saves.gameInfo("Shadow Boxing Game")) {   // *0x543514
    case 0:
        bits |= 4;
        shbx.star->on = true;                        // +0x54c0
        st.last_result = 2;                          // +0x7a0
        break;
    case 1:
        bits |= 8;
        shbx.cap->on = true;                         // +0x54bc
        st.last_result = 3;
        break;
    case 2:
        st.last_result = 4;
        break;
    default: break;
    }
}

// v9 0x4847b0 (not while leaving, nor when another scene is on)
void Skull_Scene::notify2(Service *s) {
    if (leaving) return;                             // +0x2831
    if (current_scene() != this) return;             // 0x5a7394
    if (s == &mouse) onMouse();                      // v25
    if (s == &keys) onKey();                         // v26
}

// v26 0x484720: the skip key ends either game.
bool Skull_Scene::onKey() {
    if (paused) return false;                        // +0x128
    State &st = Game::state();
    if (stateKeys()) return true;                    // 0x4ec5f0 (+0x12f4)
    if (!g_key.down) return false;                   // 0x5a6dd8
    if (st.key((int)g_key.vk)) return true;          // 0x4baef0
    if ((int)g_key.vk != st.skip_key) return false;  // +0x74
    if (shadow_on) shadowSkip();                     // 0x480da0
    if (echo_on) echo_game->skip();                  // 0x489bd0
    return true;
}

// v32 0x4854f0: the base's, but rising with Shadow Boxing on: towards the nearest rock's height.
void Skull_Scene::hover() {
    float e = rt.eyef;                               // 0x42b480
    uint32_t now = frame_time();
    uint32_t ms = now - last_hover;                  // +0x2754
    if (ms < 0x1e) return;
    if (ms > 100) ms = 100;
    last_hover = now;
    State &st = Game::state();
    if (!st.hover_up) {
        if (st.hover_down) return;
        float d = (float)((double)(int64_t)ms * e * 0.00100000005f);   // 0x514344
        if (!(e > 1.0f)) return;
        if (e < d) d = (float)((double)e * 0.5f);
        rt.lower(d);                                 // 0x42b2d0
        return;
    }
    if (shadow_on) {
        shadow->hover();                             // 0x483590
        return;
    }
    float v = (float)((200.0f - (double)e) * (int32_t)ms * 0.00100000005f);   // 0x514340
    if (v > 0.0f) rt.raise(v);                       // 0x42b130
    else rt.lower(-v);
}

// v30 0x485620: Echo Echo's hand, Shadow Boxing's net, the base's choice; the active one over an instrument on the
// ground; the dust while dusting. Set (shown if one was picked), then the hand over a character.
Vision::Graphic *Skull_Scene::cursorUpdate() {
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    rt.pickAttached(g_mouse.x, g_mouse.y, false);    // 0x42c950 (unused)
    State &st = Game::state();
    Vision::Graphic *c = st.cursor;                  // +0x5c
    Vision::Graphic *active = st.cursor_active;      // +0x60
    if (echo_on) c = echo_game->cursor(c);           // 0x489960
    if (shadow_on) c = scursor;                      // +0x43c4
    if (Vision::Graphic *b = TinkerbellRaytraceScene::cursorUpdate()) c = b;   // 0x4ab6b0
    if (s)
        for (const char *g : kGround)
            if (s->name == g) c = active;            // 0x42f330
    if (st.dusting) c = &pixie;                      // +0x3c4, +0x17d8
    if (c) set_cursor(c, true);                      // 0x4119b0
    else set_cursor(nullptr, cur_vis());
    charCursor();                                    // 0x4abda0
    return c;
}

// v25 0x484020: Echo Echo's mouse; the bell (Tink's help: the game's hint, else the instruments or the area's
// help; the click goes on), the bag, the map; the net over an animal; with dust: sprinkled (the crocodile flies
// up); the signs; with Shadow Boxing on, a catch (the eighth wins); else an instrument on the ground or a character.
void Skull_Scene::onMouse() {
    if (paused) return;                              // +0x128
    State &st = Game::state();
    if (echo_on) echo_game->mouse();                 // 0x489ed0
    if (bell.click()) {                              // 0x4e7180 (+0x1fe8)
        if (bag->has(PixieDustBag::cost2())) {       // 0x4f08c0, 0x4f02d0
            bag->spend();                            // 0x4f0860
            if (echo_on) echo_game->hint(tink);      // 0x48a8d0
            else if (shadow_on) shadow->hint();      // 0x4832f0
            else if (!skull_token_help(this)) tinkHelp();   // 0x4de8d0, 0x4de970
        }
    }
    if (mouse_down()) {
        if (bag->click()) return;                    // 0x4f02f0
        if (mouse_down() && map.hit(g_mouse.x, g_mouse.y)) {   // v11
            int r = map.click(g_mouse.x, g_mouse.y, shadow_on || echo_on);   // 0x4efc90
            if (r == 2) leaveTo("map");              // 0x489590
            return;
        }
    }
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    if (shadow_on) scursor->setActive(shadow->wantNet(a));   // 0x4832b0, 0x481280
    if (mouse_down() && st.dusting) {                // +0x3c4
        Sprite3D *d = nullptr;
        Attachment *da = nullptr;
        bag->sprinkle(&rt, &d, &da);                 // 0x4f0480
        if (d && d->name != "gator_base") {          // 0x42f330
            lift->add(d, 500);                       // 0x4bf0b0
            for (int i = 0; i < 5; i++) {
                TokenInfo &t = st.skull[i];
                if (!t.hidden()) continue;           // 0x4ed1c0
                if (!hiding_sprite("skull", d)) continue;   // 0x4e6850
                if (!hides_near(t.x, t.z, d)) continue;     // 0x4e69f0
                t.setEasy();                         // v3
                Sprite3D *g = rt.findSprite(kGround[i]);
                g->anim->setHit(-1, false);          // v6
                g->mode = 1;                         // +0x88
            }
        }
        if (da && da->name == "gator") shadow->gatorDust();   // 0x42e5b0, 0x482910
        return;
    }
    if (echo.mouse() && mouse_down() && !SignClick::active_sign) {   // 0x4e65b0, 0x6311ec
        if (st.allSkull(5)) st.skullDone();          // 0x4ee7f0, 0x4ee820
        signsOn(false);
        inactivity.stop();                           // 0x4c7770
        hotsOn(false);
        if (st.anySkull(6) || st.allSkull(5)) {      // 0x4ee7c0, 0x4ee7f0
            st.setTinkHome(echo.sprite->x, 600.0f, echo.sprite->z);   // 0x4e63b0
            tinkHome();                              // 0x4abb10
        }
        echo_sign.start();                           // 0x4c7fa0
        return;
    }
    if (firefly.mouse() && mouse_down() && !SignClick::active_sign) {
        st.last_game = 2;                            // +0x79c
        firefly_sign.start();
        return;
    }
    if (shbx.mouse()) {
        if (!mouse_down()) return;
        if (!SignClick::active_sign) {
            signsOn(false);
            hotsOn(false);
            inactivity.stop();
            shadow_sign.start();
            return;
        }
    }
    if (!mouse_down()) return;
    if (shadow_on) {
        scursor->miss();                             // 0x481230
        shadow->click(a);                            // 0x482fa0
        if (dbg_on()) tb_log("skull: net at %d,%d on %s: %d caught", g_mouse.x, g_mouse.y, a ? a->name.c_str() : "-",
                             shadow->count());
        if (shadow->count() >= 8) shadowEnd();       // 0x482950, 0x480e20
        return;
    }
    if (!s) return;
    if (pickToken(s)) return;                        // 0x489030
    if (s == rabbit.sprite()) {                      // 0x4cc8b0
        if (!echo_on) echo_click.start();            // 0x4c12a0
        else echo_click.say(true);                   // 0x4c0b00
        return;
    }
    if (s == peter.sprite()) {
        firefly_click.say(false);
        return;
    }
    if (s == skunk.sprite()) shadow_click.say(shadow_on);
}

// (inlined in run): the area interactive again: the inactivity prompts, the hot regions, the signs.
void Skull_Scene::interactive() {
    inactivity.start();                              // 0x4c7730
    hotsOn(true);
    signsOn(true);
}

// (inlined in run) Peter's cap visit (every game won twice, the cap not given yet), waited for.
void Skull_Scene::capVisit() {
    reward = Gorilla::gnew<PeterCapReward>(&peter, this, 4, lift);   // 0x4243c0 (0x104), 0x4bf280
    reward->activate();                              // +0x18
    while (reward->active()) wait(30);               // +0x14
    delete reward;                                   // +0xc
    reward = nullptr;
}

// v10 0x4847f0: the arrival (down through the clouds), the fade, the area's welcome, Firefly's end (and the cap);
// then every 30 ms: Echo Echo started when the rabbit's talk ends (with the five instruments), its end (Peter's map
// prompt or the cap); Shadow Boxing started when the skunk's ends (a fade to the lagoon), its end (a fade back, the
// skunk's end, the cap); Shadow Boxing's step; a drip or a bat now and then. A click on Peter's sign ends the loop:
// up through the clouds to Firefly.
void Skull_Scene::run() {
    wait(1);
    State &st = Game::state();
    if (!st.tutorial_skipped)                        // +0x7bc
        while (arriveThroughClouds(false)) wait(0);  // 0x4ab760
    st.tutorial_skipped = false;
    while (fader->busy()) wait(0);                   // 0x4a95c0 (+0x124)
    st.firsts->welcome();                            // 0x4c64d0 (+0x788)
    while (st.firsts->welcoming()) wait(0);          // 0x4c6930
    if (dbg_on())
        tb_log("skull: welcomed (%u); signs echo %.0f,%.0f firefly %.0f,%.0f shadow %.0f,%.0f cam %.0f,%.0f %.0f",
               frame_time(), echo.sprite->x, echo.sprite->z, firefly.sprite->x, firefly.sprite->z, shbx.sprite->x,
               shbx.sprite->z, rt.camx, rt.camz, rt.angleDeg());
    if (st.last_game == 2) pet_end.start();          // +0x79c, 0x4c19e0
    if (pet_end.active()) {
        while (pet_end.active()) wait(0);
        if (cap_due()) capVisit();
    }
    interactive();
    for (;;) {
        wait(30);
        if (echo_sign.active()) {
            while (echo_sign.active()) wait(0);
            set_cursor(st.cursor, cur_vis());        // 0x4119b0
            if (dbg_on()) tb_log("skull: echo sign done, instruments %d%d%d%d%d", st.skull[0].state,
                                 st.skull[1].state, st.skull[2].state, st.skull[3].state, st.skull[4].state);
            if (st.anySkull(6)) {                    // 0x4ee7c0
                echo_game->start();                  // 0x489980
            } else {
                signsOn(true);
                inactivity.start();
                hotsOn(true);
            }
        }
        if (echo_end.active()) {
            if (dbg_on()) tb_log("skull: echo echo ended: game %d result %d", st.last_game, st.last_result);
            if (st.last_result == 2) map.visible = false;   // +0x7a0, +0x1a1c
            while (echo_end.active()) wait(0);
            if (st.saves.allMapSegments() && !st.saves.hasXSpotSet()) {   // 0x4eaa70, 0x4ead80
                prompt = Gorilla::gnew<PeterPrompt>(&peter, this, 4, lift);   // 0x4243c0 (0x114), 0x4bfea0
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
            signsOn(true);
        }
        if (shadow_sign.active()) {
            while (shadow_sign.active()) wait(0);
            fadeOutBegin();                          // 0x4abab0
            fader->start(false, 2000);               // 0x4a9580
            while (fader->busy()) wait(0);
            rt.setCamera(2502.14f, 0.1f, 2480.51f, 90.7861f);   // 0x42a570 (0x451c623d, 0x451b0829, 0x42b5927c)
            fader->start(true, 2000);
            while (fader->busy()) wait(0);
            fadeInEnd();                             // 0x4abae0
            if (dbg_on()) tb_log("skull: shadow boxing starts (%u)", frame_time());
            shadow->start();                         // 0x482960
        }
        if (st.shadow_ended) {                       // +0x7a5
            st.shadow_ended = false;
            if (dbg_on()) tb_log("skull: shadow boxing ended: game %d result %d", st.last_game, st.last_result);
            fader->start(false, 2000);
            while (fader->busy()) wait(0);
            st.setCameraToSpot(4, 1, &rt);           // 0x4ec140
            fader->start(true, 2000);
            while (fader->busy()) wait(0);
            fadeInEnd();
            if (st.last_game != 5) {
                shbx_end.start();                    // 0x4c19e0
                while (shbx_end.active()) wait(0);
                if (cap_due()) capVisit();
            }
            interactive();
        }
        if (firefly_sign.active()) break;
        shadow->update();                            // 0x482ab0
        uint32_t now = frame_time();                 // 0x4207b0
        if (now > drip_t && now - drip_t > 6000) {
            if (crt_rand() & 1) fx[0]->play();       // 0x4f54da: "sklr_ax_drip"
            else fx[1]->play();                      // "sklr_fx_bat"
            drip_t = (uint32_t)(crt_rand() % 3000) + now;
        }
    }
    leaving = true;                                  // +0x2831
    hide_cursor();                                   // 0x4119e0
    while (firefly_sign.active()) wait(0);
    while (flyThroughClouds(false)) wait(30);        // 0x4ab920
    leaveTo("firefly");                              // 0x489590 (the static string at 0x6316f0)
    wait(0);
    finish();                                        // 0x420fe0
}

}  // namespace Game
