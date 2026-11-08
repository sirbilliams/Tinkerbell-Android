// The Color the Clouds game (scene "colorthecloud", the static at 0x5d0058, vtable 0x514fa0, ctor 0x46ce80, dtor
// 0x46d080), reconstructed from "Tinker Bell.exe" (namespace ColorTheClouds in its RTTI). Seen from above, white
// clouds drift down the screen; the player clicks to drop paint balloons of his colour on them, Peter flies about
// throwing balloons of his own. A splash spreads rings of colour over the cloud; a cloud that leaves the bottom of
// the screen scores (by its size) for whoever painted it last. The cannon's fuse is the clock.
//   notify2 v9 0x46b1b0 (mouse 0x46ae90, keys 0x46b000), run v10 0x46b260, load2 v18 0x46bf80, enter v19 0x46c900,
//   leave v20 0x46ccb0, setupClouds 0x46c0a0, setupBalloons 0x46c470, freeBalloons 0x46c6d0, setupPeter 0x46c750,
//   splash 0x46d310
// Its pieces:
//   CTCColour        (0x4ef750 ctor = set(3), set 0x4ef7b0, name 0x4ef830): one of six paint colours
//   CTCPaintBalloon  vtable 0x514f54 (0xc0 bytes; throw 0x46e560, draw 0x46e6a0)
//   CTCCursor        vtable 0x514ffc (ctor 0x46d3c0, dtor 0x46d560, draw 0x46d650, setReady 0x46d730)
//   ColorableCloud   vtable 0x515048 (ctor 0x46d790, dtor 0x46d930, draw 0x46d9d0, paint 0x46dae0, addSplat 0x46e080)
//   Interface        vtable 0x515098 (ctor 0x46ef70, dtor 0x46f440, draw 0x46f680): the score bar
//   CloudSpawner     (0x1c bytes; spawn 0x46e150, update 0x46e1f0, dtor 0x46e100): starts clouds, moves them, scores
//   PeterAI          (0x2c bytes; setState 0x46e880, target 0x46e8b0, animate 0x46e970, decide 0x46ebf0,
//                    move 0x46ee10, throwing 0x46eeb0)
#pragma once
#include "game2d.h"
#include "peterguide.h"
#include "acts.h"
#include "voacts.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include <vector>

namespace Game {

class Cannon;
class ContinueDialog;

struct CTCColour {                                   // 4 + 4 bytes
    CTCColour() { set(3); }                          // 0x4ef750
    void set(int n);                                 // 0x4ef7b0
    const char *name() const;                        // 0x4ef830
    uint8_t r = 0, g = 0, b = 0;                     // +0 +1 +2
    int id = 0;                                      // +4
};

class CTCPaintBalloon : public Vision::Graphic {     // 0xc0 bytes
public:
    CTCPaintBalloon();
    void draw() override;                            // v14 0x46e6a0
    void throwAt(int x, int y, CTCColour *c);        // 0x46e560
    Vision::Animation *bal = nullptr, *burst = nullptr;   // +0x98 +0x9c
    CTCColour *colour = nullptr;                     // +0xa0
    uint32_t delay = 50;                             // +0xa4
    uint32_t t0 = 0;                                 // +0xa8
    bool bursting = false;                           // +0xb0
    int bal_frame = 0, burst_frame = 0;              // +0xb4 +0xb8
    bool busy = false;                               // +0xbc
};

class CTCCursor : public Vision::Graphic {           // 0x6f8 bytes
public:
    CTCCursor();                                     // 0x46d3c0
    ~CTCCursor() override;                           // 0x46d560
    void draw() override;                            // v14 0x46d650
    void setReady(bool v);                           // 0x46d730
    Vision::Image active, notactive, normal;         // +0x98 +0x140 +0x1e8
    Vision::Animation hand_anim;                     // +0x290 "glob_cu_active_"
    bool hand = false;                               // +0x3a8: over the map icon, or the fuse out
    bool ready = false;                              // +0x3a9
    int chime = 1;                                   // +0x3ac: 0 the "ready" sound due, 1 not ready, 2 played
    Vision::Sound sound;                             // +0x3b0 "COLR_FX_CUactivate01"
};

class ColorableCloud : public Vision::Graphic {      // 0x210 bytes
public:
    explicit ColorableCloud(const char *name);       // 0x46d790
    ~ColorableCloud() override;                      // 0x46d930
    void draw() override;                            // v14 0x46d9d0
    void paint(int cx, int cy, int r0, int r1, uint8_t R, uint8_t G, uint8_t B);   // 0x46dae0
    void addSplat(int x, int y, CTCColour *c, int step);   // 0x46e080
    Vision::Pixelmap *pixelmap();                    // (+0x130: the image's)
    Vision::Image img;                               // +0x98
    std::vector<uint8_t> saved;                      // +0x140 / +0x144: the white pixels
    uint8_t pr = 0, pg = 0, pb = 0;                  // +0x148 +0x14c +0x150 (ints there)
    struct Splat {                                   // +0x154, 0x20 apart
        int x = 0, y = 0, r0 = 0, r1 = 0, step = 0;  // +0 +4 +8 +0xc +0x10
        CTCColour *colour = nullptr;                 // +0x18
        bool active = false;                         // +0x1c
    } splats[5];
    uint32_t t = 0;                                  // +0x1f4
    uint32_t every = 50;                             // +0x1f8
    int maxr = 0;                                    // +0x1fc: the larger of its width and height
    int colour = 6;                                  // +0x200: the last paint's colour (6: none)
    bool active = false;                             // +0x204: on its way down the screen
    int speed = 0;                                   // +0x208
    int kind = 0;                                    // +0x20c: 1 large, 2 medium, 3 small (its points)
};

class CTCInterface : public Vision::Graphic {        // 0x46ef70 (0x170 bytes)
public:
    CTCInterface();
    ~CTCInterface() override;                        // 0x46f440
    void draw() override;                            // v14 0x46f680
    Vision::Image bar;                               // +0x98 "COLR_IF_interface_01"
    Vision::Image *mine = nullptr, *his = nullptr;   // +0x140 +0x144: the two colours' small clouds
    std::vector<Vision::Image *> digits, targets;    // +0x148 +0x158
    int p = 0, s = 0;                                // +0x168 +0x16c
};

class ColorTheCloud_Scene;

struct CloudSpawner {                                // 0x1c bytes (+0x1ca4 in the scene)
    ~CloudSpawner();                                 // 0x46e100
    void spawn();                                    // 0x46e150
    void update();                                   // 0x46e1f0
    ColorTheCloud_Scene *scene;                      // +0
    uint32_t t;                                      // +4
    std::vector<const char *> nice;                  // +0xc
};

struct PeterAI {                                     // 0x2c bytes (+0x1d04 in the scene)
    void setState(int s);                            // 0x46e880
    bool target();                                   // 0x46e8b0
    void animate();                                  // 0x46e970
    void decide();                                   // 0x46ebf0
    void move();                                     // 0x46ee10
    void throwing();                                 // 0x46eeb0
    int tx = 0, ty = 0;                              // +4 +8
    int state = 0;                                   // +0xc: 0 start, 1 deciding, 2 flying to a spot, 3 throwing
    int counter = 0;                                 // +0x10
    TopDownCharacterRenderer *r = nullptr;           // +0x14
    uint32_t t;                                      // +0x18
    int dir = 0, lastdir = 0;                        // +0x1c +0x20: 0 n, 2 e, 4 s, 6 w, 666 hovering, 667 throwing
    Vision::Animation *cur = nullptr;                // +0x24
    bool stop = false;                               // +0x28
};

class ColorTheCloud_Scene : public Game2D_Scene {
public:
    ColorTheCloud_Scene();                           // 0x46ce80
    ~ColorTheCloud_Scene() override;                 // 0x46d080
    void notify2(Vision::Service *s) override;       // v9 0x46b1b0
    void run() override;                             // v10 0x46b260
    void load2() override;                           // v18 0x46bf80
    void enter() override;                           // v19 0x46c900
    void leave() override;                           // v20 0x46ccb0
    void onMouse();                                  // 0x46ae90
    void onKey();                                    // 0x46b000
    void setupClouds();                              // 0x46c0a0
    void setupBalloons();                            // 0x46c470
    void freeBalloons();                             // 0x46c6d0
    void setupPeter();                               // 0x46c750
    void splash(int x, int y, CTCColour *c, int step);   // 0x46d310
    void peterIdleStep();                            // (inlined in the cloud loops)
    void cursorUpdate();                             // (inlined in the run)

    Vision::Mouse mouse;                             // +0x15c
    Vision::Keyboard keyboard;                       // +0x190
    Vision::Ras *pack = nullptr;                     // +0x1c8 "color_the_cloud"
    int nsmall = 0, nmedium = 0;                     // +0x1cc +0x1d0
    Cannon *cannon = nullptr;                        // +0x1d4
    struct SoundSlot : Vision::Sound { SoundSlot() : Sound(nullptr) {} };
    SoundSlot sounds[5];                             // +0x1d8: pladrop, oppdrop, burst, plascore, oppscore
    PeterGuide guide;                                // +0x1240 (its task at +0x12d8)
    bool escaped = false;                            // +0x18c0
    Inactivity inactivity;                           // +0x18c4
    uint32_t last = 0;                               // +0x1c64: the last drop
    Scoreboard *board = nullptr;                     // +0x1c68
    ContinueDialog *dialog = nullptr;                // +0x1c6c
    ScrollGround *ground = nullptr;                  // +0x1c70
    CTCInterface *iface = nullptr;                   // +0x1c74
    CTCCursor *cursor = nullptr;                     // +0x1c78
    ColorableCloud *clouds_[10] = {};                // +0x1c7c
    CloudSpawner *spawner = nullptr;                 // +0x1ca4
    CTCPaintBalloon *balloons[10] = {};              // +0x1ca8
    Vision::Animation *bal_anims[6] = {}, *burst_anims[6] = {};   // +0x1cd0 +0x1ce8
    TopDownCharacterRenderer *peter_r = nullptr;     // +0x1d00
    PeterAI *ai = nullptr;                           // +0x1d04
    bool up = false, down = false, left = false, right = false;   // +0x1d08..+0x1d0b
    CTCColour peter_colour;                          // +0x1d0c
    bool over = false;                               // +0x1d14
    Vision::Sound music;                             // +0x1d18
    float peter_move = 0;                            // +0x2060: of 100, how often Peter moves rather than throws
    float speed_pct = 0;                             // +0x2064: the clouds' speed
    GameEndVo endvo;                                 // +0x2068
    bool playing = false;                            // +0x21d8
    VoList stops;                                    // +0x21dc: "stop it" when Peter is clicked
    Vision::Sound clicky, clickn;                    // +0x2210 +0x2558
};

ColorTheCloud_Scene &colorthecloud_scene();          // the static at 0x5d0058

}  // namespace Game
