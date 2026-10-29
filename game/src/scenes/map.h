// The map of Neverland, reconstructed from "Tinker Bell.exe":
//   Button@MapScene                vtable 0x51605c, ctor 0x4a6470, dtor 0x4a66a0, hit v11 0x4a69f0, draw v14 0x4a67c0,
//                                  update 0x4a6860, showRov 0x4a6970, showIdle 0x4a69b0, setPauseLine 0x4a6a60
//   TokenCollectionDisplay@MapScene vtable 0x516008, ctor 0x4a6a90 (5 pieces "<prefix>puzp<n>_01" + a sign),
//                                  load v15 0x4a6c40, unload v16 0x4a6c80, draw v14 0x4a6cc0, count v18, signed v19;
//                                  per area: IndianVillage 0x515f84, StarrySkies 0x515f30, LostBoys 0x515edc,
//                                  SkullRock 0x515e88, MermaidCove 0x515e34
//   Credits                        vtables 0x515cf4 (Graphic) / 0x515cc4 (Task at +0x98), ctor 0x4a44c0,
//                                  start 0x4a4880, draw v14 0x4a48f0, load v15 0x4a4770, unload v16 0x4a47f0,
//                                  notify2 0x4a4a40
//   Map_Scene_Base@MapScene        vtable 0x515dcc, ctor 0x4a5b90, dtor 0x4a5f10, notify2 v9 0x4a52f0,
//                                  load2 v18 0x4a5330, enter v19 0x4a5930, leave v20 0x4a5a90, setMode v24 0x4a6390,
//                                  makeButtons 0x4a5420, deleteButtons 0x4a5850, showButtons 0x4a6280
//   MapTutorialCursor@MapScene     vtable 0x5161a0 (made by MapTutorial's enter), move 0x4a9180, draw v14 0x4a9170
//   MapTutorial_Scene@MapScene     vtable 0x5161ec, ctor 0x4a9340 ("map_tutorial"), enter v19 0x4a7ce0,
//                                  keys v23 0x4a81a0, leave v20 0x4a81e0, run v10 0x4a82f0, highlight 0x4a8f10
#pragma once
#include "acts.h"
#include "voacts.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include <vector>

namespace Game {

class MapButton : public Graphic {                   // Button@MapScene
public:
    explicit MapButton(const char *base);            // 0x4a6470: "<base>idl_" / "<base>rov_" (the X spot: <base>)
    ~MapButton() override;                           // 0x4a66a0
    bool hit(int x, int y) override;                 // v11 0x4a69f0
    void draw() override;                            // v14 0x4a67c0: the "no" picture, if it has one, else the animation
    bool update();                                   // 0x4a6860: lit under the mouse (the hand cursor); true on a click
    void showRov();                                  // 0x4a6970
    void showIdle();                                 // 0x4a69b0
    void setPauseLine(const char *line);             // 0x4a6a60: a line when the mouse rests on it
    Animation idle, rov;                             // +0x98 +0x1b0
    Animation *shown = nullptr;                      // +0x2c8
    PauseOver pause;                                 // +0x2cc
    bool enabled = true;                             // +0x3dc
    Image nono;                                      // +0x3e0: shown instead when named (a button not yet usable)
private:
    void show(Animation *a);
};

class TokenCollectionDisplay : public Graphic {      // TokenCollectionDisplay@MapScene
public:
    TokenCollectionDisplay(const char *prefix, const char *sign);   // 0x4a6a90
    ~TokenCollectionDisplay() override;              // 0x4a6bb0
    void load() override;                            // v15 0x4a6c40
    void unload() override;                          // v16 0x4a6c80
    void draw() override;                            // v14 0x4a6cc0: as many pieces as won, the sign when it is had
    virtual int count() { return 0; }                // v18
    virtual bool hasSign() { return false; }         // v19
    Image *piece[5] = {};                            // +0x98 (embedded, 0xa8 each)
    Image sign;                                      // +0x3e0
};
// The per-area counts read the game's progress records (state +0x15a0.. per area; 0 for a new player) and the
// save's "<Area> Treasure Map" / "Map Segment".
struct IndianVillageTCD : TokenCollectionDisplay {   // 0x515f84
    IndianVillageTCD() : TokenCollectionDisplay("glob_if_indv", "GLOB_LY_INDVmapsg_01") {}
    int count() override;                            // 0x4a6dd0
    bool hasSign() override;                         // 0x4a6df0
};
struct StarrySkiesTCD : TokenCollectionDisplay {     // 0x515f30
    StarrySkiesTCD() : TokenCollectionDisplay("glob_if_star", "GLOB_LY_STARmapsg_01") {}
    int count() override;                            // 0x4a6eb0
    bool hasSign() override;                         // 0x4a6ee0
};
struct LostBoysTCD : TokenCollectionDisplay {        // 0x515edc
    LostBoysTCD() : TokenCollectionDisplay("glob_if_lost", "GLOB_LY_LOSTmapsg_01") {}
    int count() override;                            // 0x4a6fa0
    bool hasSign() override;                         // 0x4a6fd0
};
struct SkullRockTCD : TokenCollectionDisplay {       // 0x515e88
    SkullRockTCD() : TokenCollectionDisplay("glob_if_skrl", "GLOB_LY_SKRLmapsg_01") {}
    int count() override;                            // 0x4a7090
    bool hasSign() override;                         // 0x4a70c0
};
struct MermaidCoveTCD : TokenCollectionDisplay {     // 0x515e34
    MermaidCoveTCD() : TokenCollectionDisplay("glob_if_mcov", "GLOB_LY_MCOVmapsg_01") {}
    int count() override;                            // 0x4a7180
    bool hasSign() override;                         // 0x4a71b0
};

class Credits;
class CreditsTask : public Task {                    // the Task sub-object (+0x98, vtable 0x515cc4)
public:
    explicit CreditsTask(Credits *c) : Task(nullptr), credits(c) {}
    void notify2(Service *s) override;               // 0x4a4a40: a click ends them
    Credits *credits;
};

class Credits : public Graphic {
public:
    Credits();                                       // 0x4a44c0
    ~Credits() override;                             // 0x4a46a0
    void draw() override;                            // v14 0x4a48f0: they roll up 1/23 px a ms over three backdrops
    void load() override;                            // v15 0x4a4770
    void unload() override;                          // v16 0x4a47f0
    void start();                                    // 0x4a4880: the input to them alone until they end
    void end();                                      // (the shared "task off, mouse off, modal off, hidden")
    CreditsTask task;                                // +0x98
    Image back1, back2, back3;                       // +0x158 +0x200 +0x2a8
    Image *lines[6] = {};                            // +0x350 (embedded, 0xa8 each): "GLOB_CR_credits_0<n>"
    uint32_t last = 0;                               // +0x740
    Ras *pack = nullptr;                             // +0x744 "Credits"
    Mouse mouse;                                     // +0x748
    int speed = 23;                                  // +0x77c: ms per pixel
};

class Map_Scene_Base : public Scene {
public:
    explicit Map_Scene_Base(const char *name);       // 0x4a5b90
    ~Map_Scene_Base() override;                      // 0x4a5f10
    void notify2(Service *s) override;               // v9 0x4a52f0: the mouse -> v22, the keys -> v23
    void load2() override;                           // v18 0x4a5330
    void enter() override;                           // v19 0x4a5930
    void leave() override;                           // v20 0x4a5a90
    virtual void onMouse() = 0;                      // v22
    virtual void onKey() = 0;                        // v23
    virtual void setMode(int m);                     // v24 0x4a6390: 0 the map and its buttons, 1 the treasure map
    void makeButtons();                              // 0x4a5420
    void deleteButtons();                            // 0x4a5850
    void showButtons(bool b);                        // 0x4a6280

    Ras *pack = nullptr;                             // +0xd0 "map"
    int mode = 0;                                    // +0xd4
    Sound music;                                     // +0xd8 "nvmp_mu_music"
    Mouse mouse;                                     // +0x420
    Keyboard keys;                                   // +0x454
    Screen screen;                                   // +0x488: the map
    MapButton *indv = nullptr, *mcov = nullptr, *skrl = nullptr, *star = nullptr, *lost = nullptr;   // +0x504..+0x514
    MapButton *crd = nullptr, *pik = nullptr, *quit = nullptr, *sign = nullptr, *tmap = nullptr;    // +0x518..+0x528
    MapButton *rtrn = nullptr, *xspot = nullptr;    // +0x52c +0x530
    Credits credits;                                 // +0x538
    Screen credit_screen;                            // +0xcb8
    PreSceneVo previo;                               // +0xd34
    Image map1, map2;                                // +0xe10 +0xeb8 "GLOB_BG_nevermap_01/02"
    Image tmap_indv, tmap_lost, tmap_mcov, tmap_skrl, tmap_star;   // +0xf60..+0x1200: the treasure map's segments
    IndianVillageTCD tcd_indv;                       // +0x12a8
    StarrySkiesTCD tcd_star;                         // +0x1730
    LostBoysTCD tcd_lost;                            // +0x1bb8
    SkullRockTCD tcd_skrl;                           // +0x2040
    MermaidCoveTCD tcd_mcov;                         // +0x24c8
};

class MapTutorialCursor : public Graphic {           // vtable 0x5161a0
public:
    MapTutorialCursor();                             // (the enter's inlined ctor)
    ~MapTutorialCursor() override;                   // 0x4a9110
    void draw() override;                            // v14 0x4a9170: the cursor it shows
    bool move();                                     // 0x4a9180: a fifth of the way to the target; false once there
    Graphic *normal, *active;                        // +0x98 +0x9c: the game's cursors
    Graphic *cur;                                    // +0xa0
    int tx = 0, ty = 0;                              // +0xa4 +0xa8: the target
};

class MapTutorial_Scene : public Map_Scene_Base {
public:
    MapTutorial_Scene();                             // 0x4a9340 ("map_tutorial")
    ~MapTutorial_Scene() override;                   // 0x4a93a0
    void enter() override;                           // v19 0x4a7ce0
    void leave() override;                           // v20 0x4a81e0
    void run() override;                             // v10 0x4a82f0
    void onMouse() override {}                       // v22 (0x402e30: nothing)
    void onKey() override;                           // v23 0x4a81a0: the skip key goes on to "lost"
    void highlight();                                // 0x4a8f10: the buttons under the cursor lit
    void moveTo(int x, int y);                       // the "target, then move while highlighting" loops
    void moveTo(MapButton *b) { moveTo(b->x, b->y); }
    void playWait(int i);                            // "play line i, wait while it plays"
    std::vector<Sound *> lines;                      // +0x2954..+0x295c
    MapTutorialCursor *cursor = nullptr;             // +0x2960
};

}  // namespace Game
