// Starry Skies (area 2), reconstructed from "Tinker Bell.exe": StarrySkies_Scene (vtable 0x515198, ctor 0x477b30,
// dtor 0x477fa0, the static at 0x5d2c60, scene "starry"). Cubby stands at the Backwards Go sign (played here),
// Peter at Color the Clouds' (its own scene), Foxy at Cloud Burst's, the puzzle game played here once the five
// animal pieces are found. The pieces: three hidden in the area, one from each of the other two games.
//   load2 v18 0x475cd0, enter v19 0x474b80, leave v20 0x475a70, run v10 0x473d60, onMouse v25 0x472f30,
//   onKey v26 0x473b90, cursorUpdate v30 0x4749d0, hover v32 0x4748a0; pickPiece 0x473860, reloadPeter 0x475990,
//   reloadFoxy 0x475a10, groundTokens 0x477fc0, shelf 0x478340, startCloud 0x478500, music 0x478640,
//   hotsOn 0x478710, leaveTo 0x478740, startBack 0x478850, cloudHint 0x478940.
//   StarryInterface2D (vtable 0x515148, 0xb0 bytes; load v15 0x472ce0, unload v16 0x462b60, draw v14 0x472e90,
//                      dtor 0x472c10): the pieces won, on a bar.
//   StarryTokenContainer (vtable 0x515138; any 0x472930, target 0x472a30 / 0x472960): Tink's help with a piece
//                      still hidden.
#pragma once
#include "raytrace_scene.h"
#include "gemsign.h"
#include "scene_char.h"
#include "area_events.h"
#include "mappiece.h"
#include "tokens.h"
#include "ambient.h"
#include "acts.h"
#include "../vision/sound.h"
#include "../vision/animation.h"

namespace Game {

class BackwardsGoGame;
class CloudBurstGame;
class PeterCapReward;
class PeterPrompt;

class StarryInterface2D : public Vision::Graphic {
public:
    StarryInterface2D();                             // (inlined in enter: 0x411b90, 0xb0 bytes)
    ~StarryInterface2D() override;                   // 0x472c10
    void draw() override;                            // v14 0x472e90
    void load() override;                            // v15 0x472ce0
    void unload() override;                          // v16 0x462b60
    // +0x98 "clou_if_puzbar_", +0x9c mouse, +0xa0 cat, +0xa4 dog, +0xa8 tiger, +0xac elephant
    Vision::Animation *bar = nullptr, *mou = nullptr, *cat = nullptr, *dog = nullptr, *tig = nullptr, *ele = nullptr;
};

class StarrySkies_Scene : public TinkerbellRaytraceScene {
public:
    explicit StarrySkies_Scene(const char *name);    // 0x477b30 ("starry")
    ~StarrySkies_Scene() override;                   // 0x477fa0
    void load2() override;                           // v18 0x475cd0
    void enter() override;                           // v19 0x474b80
    void leave() override;                           // v20 0x475a70
    void run() override;                             // v10 0x473d60
    void onMouse() override;                         // v25 0x472f30
    bool onKey() override;                           // v26 0x473b90
    Vision::Graphic *cursorUpdate() override;        // v30 0x4749d0
    void hover() override;                           // v32 0x4748a0
    bool pickPiece(Raytrace::Sprite3D *s);           // 0x473860
    void reloadPeter();                              // 0x475990
    void reloadFoxy();                               // 0x475a10
    void groundTokens();                             // 0x477fc0
    void shelf(bool on);                             // 0x478340: the animals' shelf by Foxy's sign
    void startCloud();                               // 0x478500
    void music(int n);                               // 0x478640: 0 Backwards Go, 1 Cloud Burst, 2 the area, 3 none
    void hotsOn(bool on);                            // 0x478710
    void leaveTo(const char *scene);                 // 0x478740
    void startBack();                                // 0x478850
    void cloudHint();                                // 0x478940: Tink shows the animal asked for the least
    void cloudSkip();                                // (inlined in onKey): Escape during Cloud Burst
    void interactive();                              // (inlined in run): the prompts, the signs, the hot regions
    void signsOn(bool on);                           // (inlined): the three signs' +0x28
    bool tinkHelp();                                 // 0x4de970's "starry" branch (state +0x181c)

    AmbientSpriteTask ambient;                       // +0x28e8
    EndOfGame back_end, star_end;                    // +0x29c0 (Cubby's) +0x2ad8 (Peter's)
    EndOfPuzzleGame clou_end;                        // +0x2bf0
    StarryMapPiece mappiece;                         // +0x2cf4
    CharacterClick back_click, star_click, clou_click;   // +0x2dd4 +0x2f54 +0x30d4
    Vision::Ras *cub_pack = nullptr;                 // +0x3254
    SceneCharacter cubby;                            // +0x3258
    Vision::Ras *peter_pack = nullptr;               // +0x34e4
    SceneCharacter peter;                            // +0x34e8
    Vision::Ras *fox_pack = nullptr;                 // +0x3774
    SceneCharacter foxy;                             // +0x3778
    SceneInactivity inactivity;                      // +0x3a04
    Inactivity cloud_inactivity;                     // +0x3b98
    Vision::Ras *cloud_pack = nullptr;               // +0x3f38 "cloud_burst"
    StarryInterface2D *iface = nullptr;              // +0x3f3c
    bool back_on = false;                            // +0x3f40 (0x5d6ba0)
    bool cloud_on = false;                           // +0x3f41
    GemSign clou, colr, back;                        // +0x3f44 +0x3f74 +0x3fa4
    int bits = 0;                                    // +0x3fd4: Cloud Burst's
    CloudBurstGame *cloud = nullptr;                 // +0x3fd8
    int secs = 0;                                    // +0x3fdc: Cloud Burst's time
    BackwardsGoGame *backgo = nullptr;               // +0x3fe0
    Vision::Sound *tunes[3] = {};                    // +0x3fe4 (embedded, 0x348 each): back_, clou_, star_mu_music
    int tune = 3;                                    // +0x49bc
    // +0x49c0 (embedded, 0x348 each): clou_fx_appearpoof, the five animals' "hunt", star_ax_birdwing (+0x5d70:
    // a bird now and then)
    Vision::Sound *fx[7] = {};
    uint32_t bird_t = 0;                             // +0x60b8
    HotRegion *star_hot = nullptr, *back_hot = nullptr, *clou_hot = nullptr;   // +0x60bc +0x60c0 +0x60c4
    SignClick star_sign, back_sign, clou_sign;       // +0x60c8 +0x620c +0x6350
    bool stay = false;                               // +0x6494
    Vision::Image puzpieces;                         // +0x6498 "STAR_LY_puzpieces_01"
    PeterCapReward *reward = nullptr;                // +0x6540
    PeterPrompt *prompt = nullptr;                   // +0x6544
    static std::vector<std::string> win_lines;       // 0x5d2c50 (appended to by every load2)
};

StarrySkies_Scene &starry_scene();                   // the static at 0x5d2c60

}  // namespace Game
