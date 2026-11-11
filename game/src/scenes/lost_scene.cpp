// Lost_Scene, the Lost Boys' hideout, reconstructed from "Tinker Bell.exe" (see lost.h).
#include "lost.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "cannon.h"
#include "bucket.h"
#include "pirateflag.h"
#include "peter_reward.h"
#include "dustlift.h"
#include "toolbar.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

std::vector<std::string> Lost_Scene::win_lines;     // 0x5ac4e8

Lost_Scene &lost_scene() {                           // the static at 0x5ac4f8
    static Lost_Scene s;
    return s;
}

void lost_refresh_tokens() { lost_scene().refreshTokens(); }   // 0x442e00 on the static

// 0x43e5b0
Lost_Scene::Lost_Scene()
    : Lost_Scene_Base("lost"),
      ambient(&rt, "lost"),                          // 0x4df580
      mappiece(&rt),                                 // 0x4e8000
      puzpieces("LOST_LY_puzpieces_01", 0, 0, 0, 0, 0, 0, true),   // 0x4179e0
      buck_click("BUCK_VO_RACgreet1_y", "BUCK_VO_RACrecrd1", "BUCK_VO_RACfun1", "BUCK_VO_RACgreet2",
                 "BUCK_VO_RACrecrd2", "BUCK_VO_RACfun2", "BUCKET_TOSS_CHAR", "BUCK_VO_RACmore1_y",
                 "BUCK_VO_RACmore2", "BUCK_VO_RACoverly"),   // 0x4c0990
      pirt_click("PIRT_VO_CUBgreet1", "PIRT_VO_CUBrecrd1", "PIRT_VO_CUBfun1", "PIRT_VO_CUBgreet2",
                 "PIRT_VO_CUBrecrd2", "PIRT_VO_CUBfun2", "PIRATE_FLAG_CHAR", "", "", ""),
      mudp_click("LOST_VO_SKNgreet1", "LOST_VO_SKNrecrd1", "LOST_VO_SKNfun1", "LOST_VO_SKNgreet2",
                 "LOST_VO_SKNrecrd2", "LOST_VO_SKNfun2", "MUD_PIE_CHAR", "", "", ""),
      peter("Peter", &rt, &lift), raccoon1("bucketToss_raccoon", &rt, &lift),
      raccoon2("bucketToss_raccoon2", &rt, &lift), cubby("pirateFlagTag_cubby", &rt, &lift),
      skunk("mudPie_skunk", &rt, &lift),             // 0x4ccad0
      grabpuz("") {                                  // 0x437290 (named by load2)
    inactivity.init(&rt, 30000);                     // 0x4c70e0
    inactivity.add(0, 1000.0f, "GLOB_VO_SKNidle1", "GLOB_VO_SKNidle2", "GLOB_VO_SKNidle3", "");   // 0x4c7100
    inactivity.add(1, 1000.0f, "GLOB_VO_CUBidle1", "GLOB_VO_CUBidle2", "GLOB_VO_CUBidle3", "");
    inactivity.add(2, 1000.0f, "GLOB_VO_RACidle1", "GLOB_VO_RACidle2", "GLOB_VO_RACidle3", "Bucket Toss Game");
}

Lost_Scene::~Lost_Scene() {
    delete flag;                                     // port: the exe's static is never destroyed
    delete bucket;
    delete bcursor;
    delete tokens;
    delete reward;
    delete prompt;
}

// 0x442db0
void Lost_Scene::signsOn(bool on) {
    pirt.enabled = on;                               // +0x2940
    mudp.enabled = on;                               // +0x2970
    buck.enabled = on;                               // +0x2910
}

// 0x442dd0
void Lost_Scene::hotOn(bool on) {
    buck_hot->on = on;                               // +0xc0
    pirt_hot->on = on;
    mudp_hot->on = on;
}

// 0x442e00: the pieces won (state 4) shown on the piece display.
void Lost_Scene::refreshTokens() {
    static const char *const cols[5] = {"blue", "green", "purple", "red", "yellow"};   // 0x53751c..
    for (int i = 0; i < 5; i++)
        if (Game::state().lost[i].state == 4) tokens->show(cols[i]);   // 0x4f1040
}

// v18 0x43eb40: the four characters into the scene's list, the win lines (appended again on every call: the exe's
// one-time guard covers only the vector's construction), each character's animations and lines, the end-of-game
// tasks' lines and the signs' games.
void Lost_Scene::load2() {
    chars.push_back(&raccoon1);                      // 0x430dd0 on +0x2820
    chars.push_back(&raccoon2);
    chars.push_back(&cubby);
    chars.push_back(&skunk);
    Lost_Scene_Base::load2();                        // 0x443af0
    static const char *const wins[9] = {"BUCK_VO_RACother_y", "BUCK_VO_RACuwin2_y", "BUCK_VO_RACuwin3_y",
                                        "PIRT_VO_CUBuwin1", "PIRT_VO_CUBuwin2", "PIRT_VO_CUBuwin3",
                                        "LOST_VO_SKNuwin1", "LOST_VO_SKNuwin2", "LOST_VO_SKNuwin3"};
    for (const char *w : wins) win_lines.push_back(w);
    VoiceOver &vo = VoiceOver::get();

    raccoon1.setIdles("GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_", "GLOB_CA_RACidle3_");
    raccoon1.setTalks("GLOB_CA_RACroottk_", "GLOB_CA_RACtalk1_", "GLOB_CA_RACtalk2_", "GLOB_CA_RACtalk3_");
    raccoon1.setWave("GLOB_CA_RACwin_", &win_lines);
    raccoon1.setFlyIdle("GLOB_CA_RACflyid_");
    raccoon1.eyes = "RAC_eyes";                      // +0x4520
    raccoon1.setLip("bucketToss_raccoon_lip", "RAC_mout", 0);
    static const char *const rac1[29] = {
        "BUCK_VO_RACcall1_y", "BUCK_VO_RACcall2_y", "BUCK_VO_RACcall3", "BUCK_VO_RACcall4", "BUCK_VO_RACcolor_y",
        "BUCK_VO_RACencour", "BUCK_VO_RACfun1", "BUCK_VO_RACgo2", "BUCK_VO_RACgoinfo_y", "BUCK_VO_RACgreet1_y",
        "BUCK_VO_RAChigh", "BUCK_VO_RAClow", "BUCK_VO_RACmore1_y", "BUCK_VO_RACnice1", "BUCK_VO_RACnice2",
        "BUCK_VO_RACnice3", "BUCK_VO_RACnice4", "BUCK_VO_RACnice5", "BUCK_VO_RACnice6", "BUCK_VO_RACnogo_y",
        "BUCK_VO_RACother_y", "BUCK_VO_RACoverly", "BUCK_VO_RACprompt_y", "BUCK_VO_RACrecrd1",
        "BUCK_VO_RACremind_y", "BUCK_VO_RACtoobad_y", "BUCK_VO_RACuwin1_y", "BUCK_VO_RACuwin2_y",
        "BUCK_VO_RACuwin3_y"};
    for (const char *l : rac1) vo.registerLine(l, &raccoon1);   // 0x4c91b0 (+0x4514)

    raccoon1.partner = &raccoon2;                    // +0x4684
    raccoon2.setIdles("GLOB_CA_RACroot_", "GLOB_CA_RACidle1_", "GLOB_CA_RACidle2_", "GLOB_CA_RACidle3_");
    raccoon2.setTalks("GLOB_CA_RACroottk_", "GLOB_CA_RACtalk1_", "GLOB_CA_RACtalk2_", "GLOB_CA_RACtalk3_");
    raccoon2.setWave("GLOB_CA_RACwin_", &win_lines);
    raccoon2.setFlyIdle("GLOB_CA_RACflyid_");
    raccoon2.eyes = "RAC_eyes";                      // +0x47ac
    raccoon2.setLip("bucketToss_raccoon2_lip", "RAC_mout", 0);
    static const char *const rac2[14] = {
        "BUCK_VO_RACcall1_z", "BUCK_VO_RACcall2_z", "BUCK_VO_RACcolor_z", "BUCK_VO_RACgoinfo_z",
        "BUCK_VO_RACgreet1_z", "BUCK_VO_RACmore1_z", "BUCK_VO_RACnogo_z", "BUCK_VO_RACother_z",
        "BUCK_VO_RACprompt_z", "BUCK_VO_RACremind_z", "BUCK_VO_RACtoobad_z", "BUCK_VO_RACuwin1_z",
        "BUCK_VO_RACuwin2_z", "BUCK_VO_RACuwin3_z"};
    for (const char *l : rac2) vo.registerLine(l, &raccoon2);   // (+0x47a0)

    cubby.setIdles("GLOB_CA_CUBroot_", "GLOB_CA_CUBidle1_", "GLOB_CA_CUBidle2_", "GLOB_CA_CUBidle3_");
    cubby.setTalks("GLOB_CA_CUBroottk_", "GLOB_CA_CUBtalk1_", "GLOB_CA_CUBtalk2_", "GLOB_CA_CUBtalk3_");
    cubby.setWave("GLOB_CA_CUBwin_", &win_lines);
    cubby.setFlyIdle("GLOB_CA_CUBflyid_");
    cubby.eyes = "CUB_eyes";                         // +0x4a3c
    cubby.setLip("pirateFlagTag_cubby_lip", "CUB_mout", 0);
    static const char *const cub[18] = {
        "PIRT_VO_CUB1more", "PIRT_VO_CUBcall1", "PIRT_VO_CUBcall2", "PIRT_VO_CUBcall3", "PIRT_VO_CUBchalgm",
        "PIRT_VO_CUBfun1", "PIRT_VO_CUBgo2", "PIRT_VO_CUBgoinfo", "PIRT_VO_CUBgreet1", "PIRT_VO_CUBgreet2",
        "PIRT_VO_CUBhalfwy", "PIRT_VO_CUBprompt", "PIRT_VO_CUBpuzzgm", "PIRT_VO_CUBrecrd1", "PIRT_VO_CUBtoobad",
        "PIRT_VO_CUBuwin1", "PIRT_VO_CUBuwin2", "PIRT_VO_CUBuwin3"};
    for (const char *l : cub) vo.registerLine(l, &cubby);   // (+0x4a30)

    skunk.setIdles("GLOB_CA_SKNroot_", "GLOB_CA_SKNidle1_", "GLOB_CA_SKNidle2_", "GLOB_CA_SKNidle3_");
    skunk.setTalks("GLOB_CA_SKNroottk_", "GLOB_CA_SKNtalk1_", "GLOB_CA_SKNtalk2_", "GLOB_CA_SKNtalk3_");
    skunk.setWave("GLOB_CA_SKNwin_", &win_lines);
    skunk.setFlyIdle("GLOB_CA_SKNflyid_");
    skunk.eyes = "SKN_eyes";                         // +0x4ccc
    skunk.setLip("mudPie_skunk_lip", "SKN_mout", 0);
    static const char *const skn[15] = {
        "LOST_VO_SKNcall1", "LOST_VO_SKNcall2", "LOST_VO_SKNcall3", "LOST_VO_SKNfun1", "LOST_VO_SKNgo2",
        "LOST_VO_SKNgoinfo", "LOST_VO_SKNgreet1", "LOST_VO_SKNhhiya", "LOST_VO_SKNpuzzgm", "LOST_VO_SKNrecrd1",
        "LOST_VO_SKNtoobad", "LOST_VO_SKNuwin1", "LOST_VO_SKNuwin2", "LOST_VO_SKNuwin3", "LOST_VO_SKNviewgm"};
    for (const char *l : skn) vo.registerLine(l, &skunk);   // (+0x4cc0)

    flag_end.setLines("PIRT_VO_CUBtoobad", "PIRT_VO_CUBuwin1", "PIRT_VO_CUBuwin2", "PIRT_VO_CUBuwin3",
                      "PIRT_VO_CUBchalgm", "PIRT_VO_CUBpuzzgm");   // 0x4c1900
    mud_end.setLines("LOST_VO_SKNtoobad", "LOST_VO_SKNuwin1", "LOST_VO_SKNuwin2", "LOST_VO_SKNuwin3",
                     "LOST_VO_SKNviewgm", "LOST_VO_SKNpuzzgm");
    bucket_end.setLines("BUCK_VO_RACuwin1_y", "BUCK_VO_RACother_y", "BUCK_VO_RACtoobad_y", "BUCK_VO_RACuwin2_y",
                        "BUCK_VO_RACuwin3_y");     // 0x4c2a10
    bucket_end.setPiece(&mappiece);                  // 0x4c2ad0
    mudp_sign.set("Mud Pie Game", "LOST_VO_SKNgoinfo", "LOST_VO_SKNgo2", "", nullptr);   // 0x4c7f00
    pirt_sign.set("Pirate Flag Tag Game", "PIRT_VO_CUBgoinfo", "PIRT_VO_CUBgo2", "", nullptr);
    buck_sign.set("Bucket Toss Game", "BUCK_VO_RACgoinfo_y", "BUCK_VO_RACgo2", "BUCK_VO_RACnogo_y", nullptr);
    grabpuz.setName("buck_fx_grabpuz");              // *0x53694c
}

// v19 0x440520: the base's hideout; the twins at the bucket tree, Cubby and the skunk below their signs (each
// registered for the other's idle lines: the exe's quirk), the games' objects, the piece display with the pieces
// won, the pieces still on the ground, the hot regions (the bucket's made before its sign moves to the tree), the
// tree, the map piece, the cursor in the scene's layer, the inactivity spots, and the hideout tune.
void Lost_Scene::enter() {
    reward = nullptr;                                // +0x5318
    prompt = nullptr;                                // +0x531c
    Lost_Scene_Base::enter();                        // 0x443930
    flag_end.setBag(bag);                            // 0x4c19d0
    mud_end.setBag(bag);
    bucket_end.setBag(bag);                          // 0x4c2ac0
    VoiceOver &vo = VoiceOver::get();

    rac_pack = ras_open("Raccoon_Char", "", &data_device());   // 0x419b90
    rac_pack->load();                                // 0x41aa20
    ras_set_current(rac_pack);                       // 0x41a920
    raccoon1.init();                                 // v1
    raccoon2.init();
    raccoon1.setX(3770.0f);                          // 0x4cc8c0
    raccoon2.setX(4070.0f);
    raccoon1.setZ(780.0f);                           // 0x4cc960
    raccoon2.setZ(780.0f);

    cub_pack = ras_open("Cubby_Char", "", &data_device());
    cub_pack->load();
    ras_set_current(cub_pack);
    cubby.init();
    cubby.setX(pirt.sprite->x);                      // +0x292c
    cubby.setZ((float)((double)pirt.sprite->z - 150.0f));   // 0x5142e8
    {
        static const char *const l[3] = {"GLOB_VO_SKNidle1", "GLOB_VO_SKNidle2", "GLOB_VO_SKNidle3"};
        for (const char *s : l) vo.registerLine(s, &cubby);   // (+0x4a30) the exe's swap
    }

    skn_pack = ras_open("Skunk_Char", "", &data_device());
    skn_pack->load();
    ras_set_current(skn_pack);
    skunk.init();
    skunk.setX(mudp.sprite->x);                      // +0x295c
    skunk.setZ((float)((double)mudp.sprite->z - 150.0f));
    {
        static const char *const l[3] = {"GLOB_VO_CUBidle1", "GLOB_VO_CUBidle2", "GLOB_VO_CUBidle3"};
        for (const char *s : l) vo.registerLine(s, &skunk);   // (+0x4cc0)
    }

    flag_on = false;                                 // +0x343c
    flag = new PirateFlagGame(&rt);                  // 0x447050
    bcursor = Gorilla::gnew<BucketCursor>();         // 0x43c9a0
    bcursor->load();                                 // v15
    bucket = new BucketGame(&rt, &over1);            // 0x43b740
    bucket->load();                                  // 0x43b9c0
    bucket_on = false;                               // +0x3450

    tokens = Gorilla::gnew<TokenInterface>("LOST_IF_interface_");   // 0x4f0960
    tokens->add("blue", "LOST_IF_blue_");            // 0x4f0ec0
    tokens->add("green", "LOST_IF_green_");
    tokens->add("purple", "LOST_IF_purple_");
    tokens->add("red", "LOST_IF_red_");
    tokens->add("yellow", "LOST_IF_yellow_");
    tokens->load();                                  // v15
    over1.layer(0)->add(tokens);                     // 0x41ece0, 0x41e930
    refreshTokens();                                 // (inlined: the same loop as 0x442e00)
    groundTokens();                                  // 0x4426f0
    pirt.enabled = false;                            // +0x2940
    mudp.enabled = false;
    buck.enabled = false;

    buck_hot = Gorilla::gnew<HotRegion>(&rt, "Bucket Toss Game", buck.sprite, "BUCK_VO_RACcall1_y",
                                        "BUCK_VO_RACcall2_y", "BUCK_VO_RACcall3", "BUCK_VO_RACcall4");   // 0x4c3b30
    start(buck_hot);                                 // 0x420ef0
    buck.sprite->x = 3870.0f;                        // +0x28fc
    buck.sprite->z = 780.0f;
    mudp_hot = Gorilla::gnew<HotRegion>(&rt, "Mud Pie Game", mudp.sprite, "LOST_VO_SKNcall1", "LOST_VO_SKNcall1",
                                        "LOST_VO_SKNcall2", "LOST_VO_SKNcall3");
    start(mudp_hot);
    pirt_hot = Gorilla::gnew<HotRegion>(&rt, "Pirate Flag Tag Game", pirt.sprite, "PIRT_VO_CUBcall1",
                                        "PIRT_VO_CUBcall1", "PIRT_VO_CUBcall2", "PIRT_VO_CUBcall3");
    start(pirt_hot);
    buck_hot->on = false;                            // +0xc0
    pirt_hot->on = false;
    mudp_hot->on = false;

    over1.layer(0)->add(&Game::cannon());            // state +0x7c0
    bucket->setupTable();                            // 0x43c420
    buck_click.sign = &puzpieces;                    // +0x3e34
    mappiece.load();                                 // v11
    puzpieces.load();                                // v15
    puzpieces.visible = false;                       // +0x3cc4
    over1.layer(0)->add(&puzpieces);
    grabpuz.open();                                  // 0x41f9d0(0)
    stay = true;                                     // +0x3358
    inactivity.setSpot(0, mudp.sprite->x, mudp.sprite->z);   // 0x4c73f0
    inactivity.setSpot(1, pirt.sprite->x, pirt.sprite->z);
    inactivity.setSpot(2, buck.sprite->x, buck.sprite->z);
    State &st = Game::state();
    if (!st.start_in_london && !st.tutorial_skipped) arriveThroughClouds(false);   // +0x1818, +0x7bc, 0x4ab760
    ambient.bag = bag;                               // +0x3420
    ambient.activate();                              // +0x18
    dim.setLevel(1.0f);                              // 0x415f20
    playMusic(2);                                    // 0x443da0
}

// v20 0x440d50
void Lost_Scene::leave() {
    if (reward) { reward->deactivate(); delete reward; reward = nullptr; }   // +0x1c, +0xc
    if (prompt) { prompt->deactivate(); delete prompt; prompt = nullptr; }
    ambient.deactivate();
    raccoon1.release();                              // v2
    raccoon2.release();
    if (rac_pack) ras_close(rac_pack);               // 0x419620
    rac_pack = nullptr;
    cubby.release();
    if (cub_pack) ras_close(cub_pack);
    cub_pack = nullptr;
    skunk.release();
    if (skn_pack) ras_close(skn_pack);
    skn_pack = nullptr;
    buck_hot = mudp_hot = pirt_hot = nullptr;        // (tasks of the scene's, ended with it)
    delete flag;                                     // 0x447090
    flag = nullptr;
    if (bcursor) bcursor->unload();                  // v16
    delete bcursor;                                  // port: the exe's deleting dtor (+0xc)
    bcursor = nullptr;
    if (bucket) bucket->unload();                    // 0x43bc80
    delete bucket;                                   // 0x43b810
    bucket = nullptr;
    mappiece.clear();                                // v12
    flag_end.deactivate();                           // 0x4c1400
    mud_end.deactivate();
    bucket_end.deactivate();
    if (tokens) tokens->unload();
    delete tokens;
    tokens = nullptr;
    buck_sign.stop();                                // 0x4c7fe0
    pirt_sign.stop();
    mudp_sign.stop();
    buck_click.stop();                               // 0x4c12c0
    puzpieces.unload();                              // v16
    peter.release();
    if (peter_pack) ras_close(peter_pack);
    peter_pack = nullptr;
    grabpuz.close();                                 // 0x41fc50
    inactivity.stop();                               // 0x4c7770
    Lost_Scene_Base::leave();                        // 0x443a40
    set_cursor(Game::state().cursor, true);          // 0x4119b0
}

// 0x441860: dust spent with the pirate flag game on: Tink shows the nearest free ghost (or says she cannot help).
void Lost_Scene::flagHint() {
    tink->clearQueue();                              // 0x4ddf30 (+0x50)
    Sprite3D *g = nullptr;
    if (!flag->nearest(&g)) {                        // 0x447cd0
        tink->noHelp();                              // 0x4dc1d0
        return;
    }
    float x = g->x, y = g->y, z = g->z;
    float ax, az;
    tink->ahead(&ax, &az, 100.0f);                   // 0x4da8f0
    tink->pushBasic(1, ax, (float)((double)rt.eyef + 450.0f), az);   // 0x42b480 (+0xac), 0x5142ec
    tink->beckon();                                  // 0x4dbb50
    tink->pushBasic(2, x, (float)((double)g->scale * 0.01f * g->anim->oh + y), z);   // 0x513d4c, anim +0x40
    tink->point(x, y, z);                            // 0x4db7b0
}

// v26 0x441990: Escape during a game here ends it (escaped: +0x79c 5, the game's bit 0x10).
bool Lost_Scene::onKey() {
    if (paused) return false;                        // +0x128
    State &st = Game::state();
    if (stateKeys()) return true;                    // 0x4ec5f0 (+0x12f4)
    if (!g_key.down) return false;
    if (st.key((int)g_key.vk)) return true;          // 0x4baef0
    if ((int)g_key.vk != st.skip_key) return false;  // +0x74
    if (flag_on) {                                   // +0x343c
        st.last_game = 5;                            // +0x79c
        flag_bits |= 0x10;                           // +0x3438
        flag->stop();                                // 0x447240
    }
    if (bucket_on) {                                 // +0x3450
        st.last_game = 5;
        bucket_bits |= 0x10;                         // +0x344c
        bucket->end();                               // 0x43c770
    }
    return true;
}

static const char *const kGround[5] = {"blue_token_on_ground", "green_token_on_ground", "purple_token_on_ground",
                                       "red_token_on_ground", "yellow_token_on_ground"};   // 0x536a08.. (0x43d860)
static const char *const kColours[5] = {"blue", "green", "purple", "red", "yellow"};   // 0x53751c..

// 0x4426f0: the pieces still hidden (state 1) placed on the ground; a hard one shows only if a hiding sprite of
// this scene covers its spot (else it is logged and made easy).
void Lost_Scene::groundTokens() {
    static const char *const prefixes[5] = {"BUCK_GA_PUZblue_", "BUCK_GA_PUZgreen_", "BUCK_GA_PUZpurple_",
                                            "BUCK_GA_PUZredd_", "BUCK_GA_PUZyellow_"};   // 0x537644..
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = Game::state().lost[i];
        if (t.state != 1) continue;
        rt.addSprite(kGround[i], prefixes[i], 1);    // 0x42d9d0
        Sprite3D *s = rt.findSprite(kGround[i]);     // 0x42dc70
        s->load();                                   // 0x42ef20
        s->x = t.x;
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

// 0x442a70: a piece on the ground clicked: an easy one is won (state 4, shown on the display, saved); a hidden
// one: Peter's "it's hidden" line. A hiding place over a hidden piece: "sprinkle dust" line.
bool Lost_Scene::pickPiece(Sprite3D *s) {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (s->name != kGround[i]) continue;         // 0x42f330
        if (!st.lost[i].easy) {                      // 0x4ed260
            VoiceOver::get().play("GLOB_CA_PEThide");   // 0x4c9400
            return false;
        }
        st.lost[i].state = 4;
        if (s->anim) s->anim->visible = false;       // +0x70 +0x7c
        for (int j = 0; j < 5; j++)
            if (st.lost[j].state == 4) tokens->show(kColours[j]);   // 0x4f1040
        st.saveLostTokens();                         // 0x4eec40
        st.saves.flush();                            // 0x4e9840
        return true;
    }
    for (int i = 0; i < 5; i++) {
        TokenInfo &t = st.lost[i];
        if (!t.hidden()) continue;                   // 0x4ed1c0
        if (hiding_sprite("lost", s) && hides_near(t.x, t.z, s)) {   // 0x4e6850, 0x4e69f0
            VoiceOver::get().play("GLOB_CA_PEThddust");
            return false;
        }
    }
    return false;
}

// 0x4de8d0 with the scene's functor (vtable 0x514224: any 0x43d760, target 0x43d790): with a piece still hidden
// (state 1), Tink guides to the first one's sprite.
static bool lost_token_help(Lost_Scene *sc) {
    sc->tink->clearQueue();                          // 0x4ddf30
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {
        if (st.lost[i].state != 1) continue;
        sc->tink->guideTo(sc->rt.findSprite(kGround[i]));   // 0x4dc250
        return true;
    }
    return false;
}

// 0x4de970's "lost" branch (state +0x181c): a game's piece to win (the pirate flag or mud pie game's), the bucket's
// map segment, or the map; else a game not won twice, or the map if the cap is not won yet; else no help.
bool Lost_Scene::tinkHelp() {
    State &st = Game::state();
    for (int i = 0; i < 5; i++) {                    // 0x4dec60
        Sprite3D *s = nullptr;
        if (st.lost[i].state == 3) s = mudp.sprite;  // +0x295c
        else if (st.lost[i].state == 2) s = pirt.sprite;   // +0x292c
        if (s) { tink->guideTo(s); return true; }
    }
    if (!st.saves.mapSegment("Lost")) {              // 0x4decb0: 0x4ea970
        tink->guideTo(buck.sprite);
        return true;
    }
    if (!st.saves.allMapSegments()) {                // 0x4eaa70
        tink->toMap();                               // 0x4db5b0
        return true;
    }
    Sprite3D *s = nullptr;                           // 0x4ded10 (0x4f10e0: "Info" 2)
    if (st.saves.gameInfo("Mud Pie Game") != 2) s = mudp.sprite;
    else if (st.saves.gameInfo("Pirate Flag Tag Game") != 2) s = pirt.sprite;
    else if (st.saves.gameInfo("Bucket Toss Game") != 2) s = buck.sprite;
    if (s) {
        tink->guideTo(s);
        return true;
    }
    if (!st.saves.hasCap()) {                        // 0x4e9af0
        tink->toMap();
        return true;
    }
    tink->noHelp();                                  // 0x4dc1d0
    return false;
}

// v25 0x440fa0
void Lost_Scene::onMouse() {
    if (paused || busy) return;                      // +0x128, +0x28e0
    State &st = Game::state();
    if (bell.click()) {                              // 0x4e7180 (+0x1fe8)
        if (!bag->has(bag->cost2())) return;         // 0x4f08c0, 0x4f02d0
        bag->spend();                                // 0x4f0860
        if (flag_on) { flagHint(); return; }         // 0x441860
        if (bucket_on) { bucket->hint(); return; }   // 0x43c340
        if (lost_token_help(this)) return;           // 0x4de8d0
        tinkHelp();                                  // 0x4de970
        return;
    }
    if (mouse_down() && map.hit(g_mouse.x, g_mouse.y)) {   // +0x19a0 v11
        int r = map.click(g_mouse.x, g_mouse.y, flag_on || bucket_on);   // 0x4efc90
        if (r == 2) change_scene("map");             // 0x41ddf0
        return;
    }
    if (bag->click()) return;                        // 0x4f02f0
    if (pirt.mouse() && pirt.clicked && !SignClick::active_sign) {   // 0x4e65b0, +0x2941, 0x6311ec
        pirt_sign.start();                           // 0x4c7fa0
        flag_bits |= 1;                              // +0x3438
        inactivity.stop();                           // 0x4c7770
        hide_cursor();                               // 0x4119e0
        hotOn(false);
        signsOn(false);
    } else if (buck.mouse() && buck.clicked && !SignClick::active_sign) {
        if (st.allLost(4)) st.lostDone();            // 0x4eef50, 0x4eef80
        float x = buck.sprite->x, z = buck.sprite->z;
        if (st.anyLost(5) || st.allLost(4)) {        // 0x4eef20, 0x4eef50
            st.setTinkHome(x, 600.0f, z);            // 0x4e63b0
            tinkHome();                              // 0x4abb10
        }
        buck_sign.start();
        inactivity.stop();
        hotOn(false);
        signsOn(false);
    } else if (mudp.mouse() && mudp.clicked && !SignClick::active_sign) {
        float sp[4];
        st.cameraSpot(3, 3, sp);                     // 0x4ec180
        st.setSpot4(3, sp);                          // 0x4ec0e0
        stay = false;                                // +0x3358
        mudp_sign.start();
        hide_cursor();
    }
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Raytrace::Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    if (st.dusting) {                                // +0x3c4
        Sprite3D *d = nullptr;
        Raytrace::Attachment *da = nullptr;
        bag->sprinkle(&rt, &d, &da);                 // 0x4f0480
        if (!d) return;
        for (int i = 0; i < 5; i++) {
            TokenInfo &t = st.lost[i];
            if (!t.hidden()) continue;               // 0x4ed1c0
            if (!hiding_sprite("lost", d)) continue; // 0x4e6850
            if (!hides_near(t.x, t.z, d)) continue;  // 0x4e69f0
            t.setEasy();                             // v3
            Sprite3D *g = rt.findSprite(kGround[i]);
            g->anim->setHit(-1, false);              // v6
            g->mode = 1;                             // +0x88
        }
        lift->add(d, 500);                           // 0x4bf0b0
        return;
    }
    if (!mouse_down()) return;
    if (bucket_on) {
        if (bucket->pick(a)) return;                 // 0x43c890
        bucket->toss((float)g_mouse.x, (float)g_mouse.y, bcursor->colour);   // 0x43c960
        bcursor->colour = 6;                         // +0xb4
    }
    if (!s) return;
    if (pickPiece(s)) {                              // 0x442a70
        grabpuz.play();                              // 0x41fd50
        return;
    }
    if (flag_on && flag->tag(s)) {                   // 0x447b90
        if (flag->tagged() != 4) return;             // 0x447c00
        flag->stop();                                // 0x447240
        switch (st.saves.gameInfo("Pirate Flag Tag Game")) {
        case 0:
            flag_bits |= 4;
            pirt.star->on = true;                    // +0x2938
            st.last_result = 2;                      // +0x7a0
            break;
        case 1:
            flag_bits |= 8;
            pirt.cap->on = true;                     // +0x2934
            st.last_result = 3;
            break;
        case 2:
            st.last_result = 4;
            break;
        default: break;
        }
        return;
    }
    if (s == raccoon1.sprite() || s == raccoon2.sprite()) {
        if (!bucket_on) buck_click.start();          // 0x4c12a0
        else buck_click.say(true);                   // 0x4c0b00
        return;
    }
    if (s == cubby.sprite()) { pirt_click.say(flag_on); return; }
    if (s == skunk.sprite()) mudp_click.say(false);
}

// (inlined in run): the hideout interactive again: the inactivity prompts, the hot regions, the signs.
void Lost_Scene::interactive() {
    inactivity.start();                              // 0x4c7730
    hotOn(true);
    signsOn(true);
}

// Peter's cap visit (every game won twice, the cap not given yet), waited for.
static void cap_visit(Lost_Scene *sc) {
    sc->reward = Gorilla::gnew<PeterCapReward>(nullptr, sc, 3, sc->lift);   // 0x4bf280
    sc->reward->activate();                          // +0x18
    while (sc->reward->active()) sc->wait(30);       // +0x14
    delete sc->reward;                               // +0xc
    sc->reward = nullptr;
}

static bool cap_due() {
    Saves &sv = Game::state().saves;
    return sv.hasCap() && !sv.hasPetersCap();        // 0x4e9af0, 0x4ead40
}

// v10 0x441a60: the arrival (Peter's "the end" line after Big Ben, or down through the clouds), the area's welcome,
// the mud pie game's end (and the cap); then every 30 ms: the signs' games started when their talk ends, the
// bucket's end (Peter's map prompt or the cap), the pirate flag game's end, the games' updates. A click on the mud
// pie sign ends the loop: up through the clouds to the mud pie scene.
void Lost_Scene::run() {
    wait(1);                                         // 0x441a96
    State &st = Game::state();
    VoiceOver &vo = VoiceOver::get();
    if (st.start_in_london) {                        // +0x1818
        st.start_in_london = false;
        vo.play("LOST_VO_PETtend");                  // 0x4cca20 (Peter's)
        while (st.vo_busy()) wait(30);               // 0x4bcb50
    } else {
        if (!st.tutorial_skipped)                    // +0x7bc
            while (arriveThroughClouds(false)) wait(0);   // 0x4ab760
        st.tutorial_skipped = false;
        st.firsts->welcome();                        // 0x4c64d0 (+0x788)
        while (st.firsts->welcoming()) wait(0);      // 0x4c6930
        if (st.last_game == 2) mud_end.start();      // +0x79c, 0x4c19e0
        if (mud_end.active()) {
            while (mud_end.active()) wait(0);
            if (cap_due()) cap_visit(this);
        }
    }
    interactive();
    while (stay) {                                   // +0x3358
        wait(30);
        if (buck_sign.active()) {
            while (buck_sign.active()) wait(0);
            set_cursor(st.cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
            if (st.anyLost(5)) {                     // 0x4eef20
                bucket_bits |= 1;                    // +0x344c
                bucket->start();                     // 0x43c5b0
            } else {
                interactive();
            }
        }
        if (bucket_end.active()) {
            if (st.last_result == 2) map.visible = false;   // +0x7a0, +0x1a1c
            while (bucket_end.active()) wait(0);
            if (st.saves.allMapSegments() && !st.saves.hasXSpotSet()) {   // 0x4eaa70, 0x4ead80
                prompt = Gorilla::gnew<PeterPrompt>(nullptr, this, 3, lift);   // 0x4bfea0
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
        if (pirt_sign.active() && !flag_on) {        // +0x343c
            while (pirt_sign.active()) wait(0);
            flag->start();                           // 0x4470c0
        }
        if (st.flag_ended) {                         // +0x7a6
            st.flag_ended = false;
            if (st.last_game != 5) {
                flag_end.start();
                while (flag_end.active()) wait(0);
                if (cap_due()) {
                    show_cursor();                   // 0x4119d0
                    cap_visit(this);
                }
            }
            interactive();
            show_cursor();
        }
        flag->update();                              // 0x4479e0
        bucket->update();                            // 0x43bd70
    }
    leaving = true;                                  // +0x2831
    hide_cursor();                                   // 0x4119e0
    while (mudp_sign.active()) wait(0);
    while (flyThroughClouds(false)) wait(30);        // 0x4ab920
    st.last_game = 2;
    change_scene("mudpie");                          // 0x41ddf0
    wait(0);
    finish();                                        // 0x420fe0
}

// v30 0x442550: the game's cursor (the active one over a table piece; the carried piece), the flag (waving over a
// free ghost), the base's choice; the active one over the signs (no game on) and the pieces on the ground; the dust
// while dusting. Set (shown if one was picked), then the hand over a character.
Vision::Graphic *Lost_Scene::cursorUpdate() {
    Sprite3D *s = rt.pick(g_mouse.x, g_mouse.y);     // 0x42c710
    Raytrace::Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    State &st = Game::state();
    Vision::Graphic *c = st.cursor;                  // +0x5c
    Vision::Graphic *active = st.cursor_active;      // +0x60
    if (bucket_on) {
        if (bucket->hover(a)) c = active;            // 0x43c780
        else if (bcursor->colour != 6 && bcursor->colour != 0) c = bcursor;   // +0xb4
    }
    if (flag_on) c = (Vision::Graphic *)flag->cursor();   // 0x447c70
    if (Vision::Graphic *b = TinkerbellRaytraceScene::cursorUpdate()) c = b;   // 0x4ab6b0
    if (s) {
        if (!flag_on && !bucket_on) {
            if (s->name == "AA_LOST_gs_sign_01") c = active;   // 0x42f330
            if (s->name == "AB_LOST_gs_sign_01") c = active;
            if (s->name == "AC_LOST_gs_sign_01") c = active;
        }
        for (const char *g : kGround)
            if (s->name == g) c = active;
    }
    if (st.dusting) c = &pixie;                      // +0x3c4, +0x17d8
    if (c) set_cursor(c, true);                      // 0x4119b0
    else set_cursor(nullptr, g_cursor_graphic ? g_cursor_graphic->visible : false);
    charCursor();                                    // 0x4abda0
    return c;
}

}  // namespace Game
