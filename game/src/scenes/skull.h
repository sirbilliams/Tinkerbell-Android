// Skull Rock (area 4), reconstructed from "Tinker Bell.exe": Skull_Scene (vtable 0x515510, ctor 0x488260, dtor
// 0x488780, the static at 0x5dac88, scene "skull"). The rabbit stands at Echo Echo's sign (played here once the five
// instruments are found), Peter at Firefly's (its own scene, "firefly"), the skunk at Shadow Boxing's (played here).
// The instruments: three hidden in the area, one from Firefly, one from Shadow Boxing.
//   load2 v18 0x486bd0, enter v19 0x485830, leave v20 0x486940, run v10 0x4847f0, notify2 v9 0x4847b0,
//   onMouse v25 0x484020, onKey v26 0x484720, cursorUpdate v30 0x485620, hover v32 0x4854f0;
//   shadowSkip 0x480da0, shadowEnd 0x480e20, refreshTokens 0x4857a0, signsOn 0x485810, groundTokens 0x488ca0,
//   pickToken 0x489030, instrument 0x4893c0, music 0x4894c0, hotsOn 0x489560, leaveTo 0x489590.
//   SkullTokenContainer (vtable 0x515500; any 0x483d60, target 0x483d90): Tink's help with an instrument hidden.
#pragma once
#include "raytrace_scene.h"
#include "gemsign.h"
#include "scene_char.h"
#include "area_events.h"
#include "mappiece.h"
#include "tokens.h"
#include "ambient.h"
#include "acts.h"
#include "shadowboxing.h"
#include "echoecho.h"
#include "../vision/sound.h"

namespace Game {

class PeterCapReward;
class PeterPrompt;

class Skull_Scene : public TinkerbellRaytraceScene {
public:
    explicit Skull_Scene(const char *name);          // 0x488260 ("skull")
    ~Skull_Scene() override;                         // 0x488780
    void load2() override;                           // v18 0x486bd0
    void enter() override;                           // v19 0x485830
    void leave() override;                           // v20 0x486940
    void run() override;                             // v10 0x4847f0
    void notify2(Vision::Service *s) override;       // v9 0x4847b0
    void onMouse() override;                         // v25 0x484020
    bool onKey() override;                           // v26 0x484720
    Vision::Graphic *cursorUpdate() override;        // v30 0x485620
    void hover() override;                           // v32 0x4854f0
    void shadowSkip();                               // 0x480da0
    void shadowEnd();                                // 0x480e20: the eighth catch
    void refreshTokens();                            // 0x4857a0
    void signsOn(bool on);                           // 0x485810
    void groundTokens();                             // 0x488ca0
    bool pickToken(Raytrace::Sprite3D *s);           // 0x489030
    void instrument(int i);                          // 0x4893c0: the instrument's sound
    void music(int n);                               // 0x4894c0: 0 Shadow Boxing, 1 the area, 2 Echo Echo, 3 none
    void hotsOn(bool on);                            // 0x489560
    void leaveTo(const char *scene);                 // 0x489590
    void interactive();                              // (inlined in run): the prompts, the hot regions, the signs
    void capVisit();                                 // (inlined in run): Peter's cap, waited for
    bool tinkHelp();                                 // 0x4de970's "skull" branch

    AmbientSpriteTask ambient;                       // +0x28e8
    SignClick echo_sign, firefly_sign, shadow_sign;  // +0x29c0 +0x2b04 +0x2c48
    EndOfGame shbx_end, pet_end;                     // +0x2d8c (the skunk's) +0x2ea4 (Peter's)
    EndOfPuzzleGame echo_end;                        // +0x2fbc
    EchoMapPiece mappiece;                           // +0x30c0
    CharacterClick echo_click, firefly_click, shadow_click;   // +0x31a8 (the rabbit) +0x3328 (Peter) +0x34a8 (the skunk)
    Vision::Ras *rabbit_pack = nullptr;              // +0x3628
    SceneCharacter rabbit;                           // +0x362c "echo_rabbit"
    Vision::Ras *peter_pack = nullptr;               // +0x38b8
    SceneCharacter peter;                            // +0x38bc "firefly_peterPan"
    Vision::Ras *skunk_pack = nullptr;               // +0x3b48
    SceneCharacter skunk;                            // +0x3b4c "shadow_skunk"
    Vision::Image puzpieces;                         // +0x3dd8 "SKLR_LY_puzpieces_01"
    SceneInactivity inactivity;                      // +0x3e80
    Inactivity inactivity2;                          // +0x4014 "SHBX_VO_SKNprompt" (Shadow Boxing's)
    int bits = 0;                                    // +0x43b4: Shadow Boxing's
    bool shadow_on = false;                          // +0x43b8 (0x5df040)
    bool echo_on = false;                            // +0x43b9 (0x5df041)
    EchoEcho *echo_game = nullptr;                   // +0x43bc
    ShadowBoxing *shadow = nullptr;                  // +0x43c0
    ShadowCursor *scursor = nullptr;                 // +0x43c4
    TokenInterface *tokens = nullptr;                // +0x43c8 (0x5df050)
    Vision::Sound *inst_snd = nullptr;               // +0x43cc
    int tune = 3;                                    // +0x43d0
    Vision::Sound *tunes[3] = {};                    // +0x43d4 (embedded, 0x348 each): SHBX_, SKLR_, ECHO_MU_music
    Vision::Sound *fx[2] = {};                       // +0x4dac (embedded): sklr_ax_drip, sklr_fx_bat
    Cannon *fuse = nullptr;                          // +0x543c (the state's +0x7c0)
    GemSign echo, firefly, shbx;                     // +0x5440 (AB) +0x5470 (AC) +0x54a0 (AA)
    HotRegion *echo_hot = nullptr, *firefly_hot = nullptr, *shadow_hot = nullptr;   // +0x54d0 +0x54d4 +0x54d8
    uint32_t drip_t = 0;                             // +0x54dc
    PeterCapReward *reward = nullptr;                // +0x54e0
    PeterPrompt *prompt = nullptr;                   // +0x54e4
    static std::vector<std::string> win_lines;       // 0x5e0178 (appended to by every load2)
};

Skull_Scene &skull_scene();                          // the static at 0x5dac88

}  // namespace Game
