// The Lost Boys' hideout, reconstructed from "Tinker Bell.exe":
//   Lost_Scene_Base@LostScene    vtable 0x51434c, ctor 0x443b40: the hideout (area 3) with its three game signs and
//                                its three tunes. load2 v18 0x443af0, enter v19 0x443930, leave v20 0x443a40,
//                                music 0x443da0, awards 0x443c70.
//   LostTutorial_Scene@LostScene vtable 0x5143e4, ctor 0x444440 ("lost_tutorial", static at 0x5b1828): Peter
//                                introduces the Lost Boys, the pixie dust, Tink's help and the map.
//                                load2 v18 0x444580, enter v19 0x444860, run v10 0x445650, leave v20 0x4454d0,
//                                mouse v25 0x4445c0, keys v26 0x444770, cursor v30 0x4455e0.
//   the butterfly (0x78 bytes, no vtable, ctor 0x4464b0, update 0x446730) and spotAhead 0x4447d0.
#pragma once
#include "raytrace_scene.h"
#include "gemsign.h"
#include "tutorial_char.h"
#include "../raytrace/mover.h"
#include "../vision/sound.h"
#include "scene_char.h"
#include "area_events.h"
#include "mappiece.h"
#include "tokens.h"
#include "ambient.h"

namespace Game {

class Lost_Scene_Base : public TinkerbellRaytraceScene {
public:
    explicit Lost_Scene_Base(const char *name);      // 0x443b40
    ~Lost_Scene_Base() override;
    void load2() override;                           // v18 0x443af0
    void enter() override;                           // v19 0x443930
    void leave() override;                           // v20 0x443a40
    void playMusic(int n);                           // 0x443da0: 0 pirates, 1 bucket, 2 the hideout, 3 none
    void showAwards();                               // 0x443c70: the caps and stars won on the signs
    GemSign buck, pirt, mudp;                        // +0x28e8 +0x2918 +0x2948
    Vision::Sound *music[3] = {};                    // +0x2978 (embedded, 0x348 each)
    int music_idx = 3;                               // +0x3350
};

// The butterfly that flits in front of the camera (+0x5558).
struct Butterfly {
    explicit Butterfly(Raytrace::RayTrace *rt);      // 0x4464b0
    void update();                                   // 0x446730: one step along the curve (restarting at its end)
    float p0[3], p1[3], p2[3];                       // +0x00 +0x0c +0x18
    float pos[3] = {};                               // +0x24
    Raytrace::Curve3 curve;                          // +0x30
    Raytrace::Sprite3D *sprite = nullptr;            // +0x6c "LOST_AM_butterflyr_"
    float t = 0;                                     // +0x70
    Raytrace::RayTrace *rt;                          // +0x74
};

class LostTutorial_Scene : public Lost_Scene_Base {
public:
    LostTutorial_Scene();                            // 0x444440
    ~LostTutorial_Scene() override;
    void load2() override;                           // v18 0x444580
    void enter() override;                           // v19 0x444860
    void run() override;                             // v10 0x445650
    void leave() override;                           // v20 0x4454d0
    void onMouse() override;                         // v25 0x4445c0
    bool onKey() override;                           // v26 0x444770
    Vision::Graphic *cursorUpdate() override;        // v30 0x4455e0
    void spotAhead(float out[3], float deg, float dist);   // 0x4447d0 (a static helper on the RayTrace)
    void waitTalk();                                 // "while a line plays, wait 30 ms"
    bool lostBoy(Raytrace::Sprite3D *s);             // the six of 0x4445c0's comparisons

    Vision::Ras *rab_pack = nullptr, *fox_pack = nullptr, *peter_pack = nullptr;   // +0x3358 +0x335c +0x3360
    Vision::Ras *cub_pack = nullptr, *rac_pack = nullptr, *skn_pack = nullptr;     // +0x3364 +0x3368 +0x336c
    TutorialCharacter cubby, rabbit, foxy, peter, raccoon1, raccoon2, skunk;   // +0x3370 +0x3578 +0x3780 +0x3988
                                                                             // +0x3b90 +0x3d98 +0x3fa0
    Vision::Sound *fx[6] = {};                       // +0x41a8 (embedded, 0x348 each): bagappear, wrong,
                                                     // tinknoise, TNKgiggle, mapappear, magic
    Butterfly *butterfly = nullptr;                  // +0x5558
    bool butterfly_clicked = false;                  // +0x555c
    bool boy_dusted = false;                         // +0x555d
    bool bag_clicked = false;                        // +0x555e
    bool bell_clicked = false;                       // +0x555f
    bool map_clicked = false;                        // +0x5560
};

class PirateFlagGame;
class BucketGame;
class BucketCursor;
class PeterCapReward;
class PeterPrompt;

// The hideout proper (Lost_Scene@LostScene, vtable 0x514258, ctor 0x43e5b0, the static at 0x5ac4f8): the raccoon
// twins at their bucket tree, Cubby at the pirate flag sign, the skunk at the mud pie sign; the bucket toss and
// pirate flag tag games played here, mud pie in its own scene. Hidden puzzle pieces lie about, found by
// sprinkling dust on their hiding places.
//   load2 v18 0x43eb40, enter v19 0x440520, leave v20 0x440d50, onMouse v25 0x440fa0, onKey v26 0x441990,
//   run v10 0x441a60, cursorUpdate v30 0x442550; flagHint 0x441860, groundTokens 0x4426f0, pickPiece 0x442a70,
//   signsOn 0x442db0, hotOn 0x442dd0, refreshTokens 0x442e00.
class Lost_Scene : public Lost_Scene_Base {
public:
    Lost_Scene();                                    // 0x43e5b0 ("lost")
    ~Lost_Scene() override;
    void load2() override;                           // v18 0x43eb40
    void enter() override;                           // v19 0x440520
    void leave() override;                           // v20 0x440d50
    void run() override;                             // v10 0x441a60
    void onMouse() override;                         // v25 0x440fa0
    bool onKey() override;                           // v26 0x441990
    Vision::Graphic *cursorUpdate() override;        // v30 0x442550
    void flagHint();                                 // 0x441860: Tink shows the nearest free ghost
    void groundTokens();                             // 0x4426f0: the hidden pieces' sprites
    bool pickPiece(Raytrace::Sprite3D *s);           // 0x442a70: a piece on the ground clicked
    void signsOn(bool on);                           // 0x442db0
    void hotOn(bool on);                             // 0x442dd0
    void refreshTokens();                            // 0x442e00
    void interactive();                              // (inlined): inactivity started, hot regions and signs on
    bool tinkHelp();                                 // 0x4de970's "lost" branch (state +0x181c)

    bool stay = false;                               // +0x3358: run's loop goes on
    AmbientSpriteTask ambient;                       // +0x335c
    PirateFlagGame *flag = nullptr;                  // +0x3434
    int flag_bits = 0;                               // +0x3438
    bool flag_on = false;                            // +0x343c (0x5af934)
    BucketGame *bucket = nullptr;                    // +0x3440
    BucketCursor *bcursor = nullptr;                 // +0x3444 (0x5af93c)
    int hits = 0;                                    // +0x3448 (0x5af940): buckets hit this game
    int bucket_bits = 0;                             // +0x344c (0x5af944)
    bool bucket_on = false;                          // +0x3450 (0x5af948)
    BucketMapPiece mappiece;                         // +0x3454
    EndOfGame flag_end, mud_end;                     // +0x3534 (Cubby's) +0x364c (the skunk's)
    EndOfPuzzleGame bucket_end;                      // +0x3764
    SignClick buck_sign, pirt_sign, mudp_sign;       // +0x3868 +0x39ac +0x3af0
    HotRegion *buck_hot = nullptr, *pirt_hot = nullptr, *mudp_hot = nullptr;   // +0x3c34 +0x3c38 +0x3c3c
    TokenInterface *tokens = nullptr;                // +0x3c40 (0x5b0138)
    Vision::Image puzpieces;                         // +0x3c48 "LOST_LY_puzpieces_01"
    CharacterClick buck_click, pirt_click, mudp_click;   // +0x3cf0 +0x3e70 +0x3ff0
    Vision::Ras *peter_pack = nullptr;               // +0x4170
    SceneCharacter peter;                            // +0x4174
    Vision::Ras *rac_pack = nullptr;                 // +0x4400
    SceneCharacter raccoon1, raccoon2;               // +0x4404 +0x4690
    Vision::Ras *cub_pack = nullptr;                 // +0x491c
    SceneCharacter cubby;                            // +0x4920
    Vision::Ras *skn_pack = nullptr;                 // +0x4bac
    SceneCharacter skunk;                            // +0x4bb0
    Vision::Sound grabpuz;                           // +0x4e3c "buck_fx_grabpuz"
    SceneInactivity inactivity;                      // +0x5184
    PeterCapReward *reward = nullptr;                // +0x5318
    PeterPrompt *prompt = nullptr;                   // +0x531c
    static std::vector<std::string> win_lines;       // 0x5ac4e8 (appended to by every load2)
};

Lost_Scene &lost_scene();                            // the static at 0x5ac4f8

}  // namespace Game
