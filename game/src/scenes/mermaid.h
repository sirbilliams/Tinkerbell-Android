// Mermaid Cove (area 5), reconstructed from "Tinker Bell.exe": Mermaid_Scene (vtable 0x514830, ctor 0x44d9c0,
// dtor 0x44e480, the static at 0x5ba320, scene "Mermaid"). Cubby stands at the Pearl Toss sign (played here), the
// rabbit at Coconuts Away's (its own scene, "coconut"), the skunk at the Shell Game's (played here once the five
// shells are found). The shells: one hidden in the area, two from Coconuts Away, two from Pearl Toss.
//   load2 v18 0x44e9b0, enter v19 0x44feb0, leave v20 0x450740, run v10 0x450980, notify2 v9 0x452120,
//   onMouse v25 0x451720, onKey v26 0x4516c0; mermaids 0x452160, signs 0x4522b0, music 0x452600,
//   groundTokens 0x4526a0, pickToken 0x4528d0, hotsOn 0x452af0, leaveTo 0x452b20, shellHint 0x452c30.
//   MermaidTokensInterface (vtable 0x5147e4, 0x6b8+0x348 bytes; ctor 0x44d6c0, dtor 0x44d830, draw v14 0x44d960,
//                           load v15 0x44d8d0, unload v16 0x44d910): the shells won, on a bar.
//   MermaidTokenContainer (vtable 0x5147d4; any 0x44d4f0, target 0x44d520): Tink's help with a shell still hidden.
#pragma once
#include "raytrace_scene.h"
#include "scene_char.h"
#include "area_events.h"
#include "mappiece.h"
#include "tokens.h"
#include "ambient.h"
#include "acts.h"
#include "pearltoss.h"
#include "shellgame.h"
#include "../vision/sound.h"
#include "../vision/animation.h"

namespace Game {

class PeterCapReward;
class PeterPrompt;

class MermaidTokensInterface : public Vision::Graphic {
public:
    MermaidTokensInterface();                        // 0x44d6c0
    ~MermaidTokensInterface() override;              // 0x44d830
    void draw() override;                            // v14 0x44d960
    void load() override;                            // v15 0x44d8d0
    void unload() override;                          // v16 0x44d910
    Vision::Image bar;                               // +0x98 "SHEL_IF_puzbar_01"
    Vision::Animation *icons[5] = {};                // +0x140 (0x118 each) "SHEL_IF_puzshell0<n>_"
    Vision::Sound buzz;                              // +0x6b8 "SHEL_FX_buzz"
};

class Mermaid_Scene : public TinkerbellRaytraceScene {
public:
    explicit Mermaid_Scene(const char *name);        // 0x44d9c0 ("Mermaid")
    ~Mermaid_Scene() override;                       // 0x44e480
    void load2() override;                           // v18 0x44e9b0
    void enter() override;                           // v19 0x44feb0
    void leave() override;                           // v20 0x450740
    void run() override;                             // v10 0x450980
    void notify2(Vision::Service *s) override;       // v9 0x452120
    void onMouse() override;                         // v25 0x451720
    bool onKey() override;                           // v26 0x4516c0
    void mermaids();                                 // 0x452160: the two mermaids on their rocks
    void signs();                                    // 0x4522b0: the signs' sprites, highlights, stars and caps
    void music(int n);                               // 0x452600: 0 Pearl Toss, 1 the Shell Game, 2 the area, 3 none
    void groundTokens();                             // 0x4526a0
    void pickToken(Raytrace::Sprite3D *s);           // 0x4528d0
    void hotsOn(bool on);                            // 0x452af0
    void leaveTo(const char *scene);                 // 0x452b20
    void shellHint();                                // 0x452c30
    bool tinkHelp();                                 // 0x4de970's "Mermaid" branch
    void capVisit();                                 // (inlined in run): Peter's cap (no Peter here), waited for

    Raytrace::Sprite3D *idle_a = nullptr, *idle_b = nullptr;   // +0x28e8 +0x28ec: the mermaids' idles
    Raytrace::Sprite3D *pearl_sign = nullptr;        // +0x28f0 "AB_mcov_gs_sign_01"
    Raytrace::Sprite3D *shell_sign = nullptr;        // +0x28f8 "AA_mcov_gs_sign_01"
    Raytrace::Sprite3D *coco_sign = nullptr;         // +0x28fc "AC_mcov_gs_sign_01"
    PearlTossMonitor pearl;                          // +0x2900
    ShellGame shell;                                 // +0x44a8
    MermaidTokensInterface iface;                    // +0x5820
    Vision::Sound *tunes[3] = {};                    // +0x6220 (embedded, 0x348 each): PERL_, SHEL_, MCOV_MU_music
    int tune = 3;                                    // +0x6bf8
    Vision::Sound *fx[3] = {};                       // +0x6bfc: glob_fx_gsclick, shel_fx_grabpuz, mcov_ax_gull
    uint32_t gull_t = 0;                             // +0x75d4
    AmbientSpriteTask ambient;                       // +0x75d8
    HotRegion *coco_hot = nullptr, *pearl_hot = nullptr, *shell_hot = nullptr;   // +0x76b0 +0x76b4 +0x76b8
    EndOfGame pearl_end, coco_end;                   // +0x76bc +0x77d4
    EndOfPuzzleGame shell_end;                       // +0x78ec
    SignClick pearl_sign_click, coco_sign_click, shell_sign_click;   // +0x79f0 +0x7b34 +0x7c78
    CharacterClick pearl_click, coco_click, shell_click;   // +0x7dbc (Cubby) +0x7f3c (the rabbit) +0x80bc (the skunk)
    SceneInactivity inactivity;                      // +0x823c
    Vision::Ras *rabbit_pack = nullptr;              // +0x83d0
    SceneCharacter rabbit;                           // +0x83d4 "coconutsAway_rabbit"
    Vision::Ras *skunk_pack = nullptr;               // +0x8660
    SceneCharacter skunk;                            // +0x8664 "shell_skunk"
    Vision::Ras *cubby_pack = nullptr;               // +0x88f0
    SceneCharacter cubby;                            // +0x88f4 "pearlToss_cubby"
    ShellMapPiece mappiece;                          // +0x8b80
    Vision::Image puzpieces;                         // +0x8c68 "MCOV_LY_puzpieces_01"
    PeterCapReward *reward = nullptr;                // +0x8d10
    PeterPrompt *prompt = nullptr;                   // +0x8d14
    static std::vector<std::string> win_lines;       // 0x5c3168 (appended to by every load2)
};

Mermaid_Scene &mermaid_scene();                      // the static at 0x5ba320

}  // namespace Game
