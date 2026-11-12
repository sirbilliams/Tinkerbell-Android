// The Indian Village (area 1), reconstructed from "Tinker Bell.exe": IndianVillage_Scene (vtable 0x514e74, ctor
// 0x467fc0, dtor 0x468510, the static at 0x5c8b80, scene "Indian_Village"). Peter stands at the Bird Spy sign (its
// own scene, "birdspy"), the two raccoons at Flying Hoops' (played here), Foxy at Totem Trail's (played here once
// the five totem pieces are found). The pieces: three hidden in the area, one from each of the other two games.
//   load2 v18 0x4660b0, enter v19 0x465060, leave v20 0x465d50, run v10 0x463f90, onMouse v25 0x463290,
//   onKey v26 0x463f00, cursorUpdate v30 0x464c50 (heldCursor 0x464d60, activeCursor 0x464e70), hover v32 0x4689f0;
//   signsOn 0x463270, pickToken 0x463bc0, music 0x466010, groundTokens 0x468530, awards 0x4688c0, hotsOn 0x468b30,
//   hoopsHint 0x468b60, totemHint 0x468c70 (wrongHole 0x469f80, stumpHint 0x46a580, freeTree 0x46a5c0).
//   Totem Trail (in the scene): placeToken 0x462db0, removeToken 0x463120, clearTotem 0x469390, resetHoles 0x469470,
//   makePuzzle 0x4695a0, dressTotem 0x469720, treesStill 0x469950, startTotem 0x4699c0, skipTotem 0x469aa0,
//   nCorrect 0x469b20, checkWin 0x469b70, eyeToTree 0x469cc0, wrongMsg 0x469e00, nPlaced 0x469f60.
//   TotemInterface (vtable 0x514e14, 0xb0 bytes; ctor 0x4627e0, dtor 0x4629d0, load v15 0x462ac0, unload v16
//                   0x462b60, draw v14 0x462bb0, pickIcon 0x462cd0): the pieces won, on a bar.
//   VillageTokenContainer (vtable 0x514e04; any 0x462520, target 0x462550): Tink's help with a piece still hidden.
//   TeepeeAnimations (vtable 0x514f0c, 0xd8 bytes; ctor 0x468e60, dtor 0x4690c0, run 0x469130): now and then a
//                   teepee's wave.
#pragma once
#include "raytrace_scene.h"
#include "gemsign.h"
#include "scene_char.h"
#include "area_events.h"
#include "mappiece.h"
#include "tokens.h"
#include "ambient.h"
#include "acts.h"
#include "../raytrace/mover.h"
#include "../vision/sound.h"
#include "../vision/animation.h"

namespace Game {

class FlyingHoopsGame;
class Cannon;
class PeterCapReward;
class PeterPrompt;

class TotemInterface : public Vision::Graphic {
public:
    TotemInterface();                                // 0x4627e0
    ~TotemInterface() override;                      // 0x4629d0
    void draw() override;                            // v14 0x462bb0
    void load() override;                            // v15 0x462ac0
    void unload() override;                          // v16 0x462b60
    int pickIcon(int x, int y);                      // 0x462cd0: the last won piece's icon under (x, y), or -1
    // +0x98 "TOTM_IF_puzbar_", +0x9c eagle, +0xa0 fox, +0xa4 raccoon, +0xa8 skunk, +0xac bear
    Vision::Animation *bar = nullptr, *icons[5] = {};
};

// (vtable 0x514f0c) the two teepees' people wave now and then (10 to 15 s apart, one at random).
class TeepeeAnimations : public Vision::Task {
public:
    explicit TeepeeAnimations(Raytrace::RayTrace *rt);   // 0x468e60
    void run() override;                             // 0x469130
    std::vector<Vision::Animation *> waves;          // +0xc4
    uint32_t next = 0;                               // +0xd0
    int which = 0;                                   // +0xd4
};

class IndianVillage_Scene : public TinkerbellRaytraceScene {
public:
    explicit IndianVillage_Scene(const char *name);  // 0x467fc0 ("Indian_Village")
    ~IndianVillage_Scene() override;                 // 0x468510
    void load2() override;                           // v18 0x4660b0
    void enter() override;                           // v19 0x465060
    void leave() override;                           // v20 0x465d50
    void run() override;                             // v10 0x463f90
    void onMouse() override;                         // v25 0x463290
    bool onKey() override;                           // v26 0x463f00
    Vision::Graphic *cursorUpdate() override;        // v30 0x464c50
    void hover() override;                           // v32 0x4689f0
    Vision::Graphic *heldCursor(Raytrace::Attachment *a);   // 0x464d60: a piece in hand: its cursor (blinking over a free hole)
    Vision::Graphic *activeCursor(Raytrace::Sprite3D *s, Raytrace::Attachment *a);   // 0x464e70
    void signsOn(bool on);                           // 0x463270
    bool pickToken(Raytrace::Sprite3D *s);           // 0x463bc0
    void music(int n);                               // 0x466010: 0 Flying Hoops, 1 Totem Trail, 2 the area, 3 none
    void groundTokens();                             // 0x468530
    void awards();                                   // 0x4688c0: the signs' stars and caps
    void hotsOn(bool on);                            // 0x468b30
    void hoopsHint();                                // 0x468b60
    void totemHint();                                // 0x468c70
    bool wrongHole(Raytrace::Attachment **out);      // 0x469f80
    bool stumpHint(Raytrace::Sprite3D **out);        // 0x46a580
    bool freeTree(Raytrace::Sprite3D **out);         // 0x46a5c0
    void placeToken(int i);                          // 0x462db0
    bool removeToken(Raytrace::Attachment *a);       // 0x463120
    void clearTotem();                               // 0x469390
    void resetHoles();                               // 0x469470
    void makePuzzle();                               // 0x4695a0
    void dressTotem();                               // 0x469720
    void treesStill();                               // 0x469950
    void startTotem();                               // 0x4699c0
    void skipTotem();                                // 0x469aa0
    int nCorrect();                                  // 0x469b20
    bool checkWin();                                 // 0x469b70
    void eyeToTree();                                // 0x469cc0
    bool wrongMsg();                                 // 0x469e00
    int nPlaced();                                   // 0x469f60
    void interactive();                              // (inlined in run): the prompts, the hot regions, the signs
    void leaveTo(const char *scene);                 // (inlined in onMouse and run)
    bool tinkHelp();                                 // 0x4de970's "indian_village" branch (state +0x181c)

    AmbientSpriteTask ambient;                       // +0x28e8
    SignClick flyh_sign, bird_sign, totm_sign;       // +0x29c0 +0x2b04 +0x2c48
    EndOfGame flyh_end, bird_end;                    // +0x2d8c +0x2ea4
    EndOfPuzzleGame totm_end;                        // +0x2fbc
    TotemMapPiece mappiece;                          // +0x30c0
    CharacterClick flyh_click, bird_click, totm_click;   // +0x31a4 (raccoon) +0x3324 (Peter) +0x34a4 (Foxy)
    Vision::Ras *rac_pack = nullptr;                 // +0x3624
    SceneCharacter rac, rac2;                        // +0x3628 "hoops_raccoon", +0x38b4 "hoops_raccoon2"
    Vision::Ras *fox_pack = nullptr;                 // +0x3b40
    SceneCharacter foxy;                             // +0x3b44 "totem_foxy"
    Vision::Ras *peter_pack = nullptr;               // +0x3dd0
    SceneCharacter peter;                            // +0x3dd4 "birdSpy_peterPan"
    SceneInactivity inactivity;                      // +0x4060
    Inactivity totm_inactivity;                      // +0x41f4
    int correct = 0;                                 // +0x4594: pieces in their right holes (the last count)
    bool stay = false;                               // +0x4598
    Raytrace::CameraMover mover;                     // +0x459c
    int bits = 0;                                    // +0x45b4: Totem Trail's
    GemSign bird, flyh, totm;                        // +0x45b8 (AC) +0x45e8 (AA) +0x4618 (AB)
    HotRegion *bird_hot = nullptr, *flyh_hot = nullptr, *totm_hot = nullptr;   // +0x4648 +0x464c +0x4650
    TotemInterface *iface = nullptr;                 // +0x4654
    FlyingHoopsGame *hoops = nullptr;                // +0x4658 (embedded, 0x690 bytes; its +0xd4 is the scene's
                                                     // +0x472c: the game on, +0xd8 +0x4730: its bits; port:
                                                     // an owned pointer, made with the scene)
    struct Hole { int type, animal; };               // type: 0 (silhouette low), 1 (high), 2 (none)
    Hole puzzle[5] = {};                             // +0x4ce8: each tree's
    bool totem_on = false;                           // +0x4d10
    // +0x4d18.. (embedded, 0x118 each; port: owned pointers): the cursors with a piece in hand
    Vision::Animation *held[5] = {};                 // "TOTM_CU_puz{eagle,fox,raccon,skunk,bear}_"
    Vision::Sound *tunes[3] = {};                    // +0x5290 (embedded, 0x348 each): FLYH_, TOTM_, INDV_MU_music
    // +0x5c68 (embedded, 0x348 each): FLYH_FX_hoopgood, TOTM_FX_puzinsert, puzreturn, grabpuz, stumpknok,
    // woodknock, buzz
    Vision::Sound *fx[7] = {};
    int tune = 3;                                    // +0x7360
    Raytrace::RayTraceCharacter *peter3d = nullptr;  // +0x7364 "indian_village_peter" (vtable 0x514e64)
    Cannon *fuse = nullptr;                          // +0x7368 (the state's +0x7c0)
    Vision::Image puzpieces;                         // +0x7370 "INDV_LY_puzpieces_01"
    Vision::Image enterimg;                          // +0x7418 "INDV_LY_enter_01" (Flying Hoops' sign shows it)
    PeterCapReward *reward = nullptr;                // +0x74c0
    PeterPrompt *prompt = nullptr;                   // +0x74c4
    static std::vector<std::string> win_lines;       // 0x5c8b70 (appended to by every load2)
};

IndianVillage_Scene &village_scene();                // the static at 0x5c8b80

}  // namespace Game
