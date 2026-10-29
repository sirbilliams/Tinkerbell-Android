// London (London_Scene@LondonScene, vtable 0x516a3c, ctor 0x4b6440), reconstructed from "Tinker Bell.exe": the
// tutorial street. After a fade-in four scripts (tasks) run in turn: the arrow keys (LSKeyboardScript), the mouse
// and the ribbon (LSRibbonScript), the "stop when Wendy turns" clock game (LSTickTockScript) and the meeting with
// Peter and Tink (LSHuntScript); then the game goes to "lost_tutorial".
//   enter 0x4b5800 (v19), run 0x4b5300 (v10), leave 0x4b6230 (v20), mouse v25 0x4b50a0, keys v26 0x4b5260,
//   cursor v30 0x4b4f70, per frame 0x4b65f0.
#pragma once
#include "raytrace_scene.h"
#include "london_chars.h"
#include "acts.h"
#include "../raytrace/mover.h"
#include "../vision/animation.h"
#include "../vision/sound.h"

namespace Game {

class London_Scene;

// The sign board with the player's tokens (0x30 bytes at +0x3508; 0x4e6400 ctor, 0x4e6440 init, 0x4e6530 release,
// 0x4e6570 show, 0x4e6590 hide, 0x4e65b0 mouse): the "AA_lond_gs_sign_01" sprite of the street and four attachments
// on it: the tick, the green highlight, the cap and the star.
struct GemSign {
    GemSign(const char *sign, const char *tick, const char *green, const char *cap, const char *star);   // 0x4e6400
    void init(Raytrace::RayTrace *rt);               // 0x4e6440
    void release();                                  // 0x4e6530
    void show();                                     // 0x4e6570: its animation shown, its box back
    void hide();                                     // 0x4e6590
    bool mouse();                                    // 0x4e65b0: the highlight under the mouse; a click plays the click
    const char *names[5];                            // +0x00..+0x10
    Raytrace::Sprite3D *sprite = nullptr;            // +0x14
    Raytrace::Attachment *green = nullptr, *cap = nullptr, *star = nullptr;   // +0x18 +0x1c +0x20
    Raytrace::RayTrace *rt = nullptr;                // +0x24
    bool enabled = false;                            // +0x28
    bool clicked = false;                            // +0x29
    Vision::Sound *click = nullptr;                  // +0x2c "GLOB_FX_GSclick"
};

// WallAnimation (vtable 0x517cec, 0x18 bytes, ctor 0x4f1630): an animation drawn over a wall of the street (John
// and Michael waving in a window).
struct WallAnimation {
    WallAnimation(Raytrace::RayTrace *rt, const char *wall, const char *prefix, int key)   // 0x4f1630
        : rt(rt), wall(wall), prefix(prefix), key(key) {}
    void init();                                     // 0x4f1690: linked to the wall, on, held on frame 0
    void on();                                       // 0x4f1700
    void off();                                      // 0x4f1720
    void play(int delay, int mode);                  // 0x4f1740: from frame 0
    Raytrace::RayTrace *rt;                          // +0x04
    const char *wall, *prefix;                       // +0x08 +0x0c
    Raytrace::WallObject *anim = nullptr;            // +0x10
    int key;                                         // +0x14
};

// RibbonInterface (vtable 0x516b00, 0x618 bytes, ctor 0x4b68f0): the bar at the bottom that holds the ribbon.
class RibbonInterface : public Vision::Graphic {
public:
    RibbonInterface();                               // 0x4b68f0
    ~RibbonInterface() override;                     // 0x4b6a60
    void draw() override;                            // v14 0x4b6b70
    bool enabled = false;                            // +0x98: the ribbon can be taken from the bar
    bool holding = false;                            // +0x99: the ribbon is in the bar
    Vision::Animation bar, idle, rollover, select, cursor;   // +0xa0 +0x1b8 +0x2d0 +0x3e8 +0x500
};

// LondonCursor (vtable 0x516b4c, 0x260 bytes, ctor 0x4b6cf0): an animated cursor when active, a still one else.
class LondonCursor : public Vision::Graphic {
public:
    LondonCursor(const char *active, const char *normal);   // 0x4b6cf0
    ~LondonCursor() override;                        // 0x4b6df0
    void draw() override;                            // v14 0x4afe20
    bool active = false;                             // +0x98
    Vision::Animation anim;                          // +0xa0
    Vision::Image still;                             // +0x1b8
};

// The ribbon hidden in the street (+0x3538): where it is in the game (3 at first; 0 the ribbon is in the bar,
// 1 on the cursor, 2 given to Wendy), whether it can be picked up, and its sprite.
struct RibbonState {
    void place();                                    // 0x4b6bf0: "ribbon" at one of four spots
    void setActive(bool v) { active = v; }           // 0x4b6be0
    int state = 3;                                   // +0x3538
    bool active = false;                             // +0x353c
    Raytrace::Sprite3D *sprite = nullptr;            // +0x3540
    Raytrace::RayTrace *rt = nullptr;                // port: the exe uses the scene's at 0x62d7b0
};

// The scripts: Tasks run one after another by the scene's run.
class LSKeyboardScript : public Vision::Task {       // vtable 0x516ba0, ctor 0x4b8870, run 0x4b8900
public:
    explicit LSKeyboardScript(London_Scene *s) : scene(s) {}
    void run() override;
    London_Scene *scene;
    float start_angle = 0;                           // +0xc0
    int turn = 60;                                   // +0xc4
};

class LSRibbonScript : public Vision::Task {         // vtable 0x516bd4, ctor 0x4b90a0, run 0x4b92e0
public:
    explicit LSRibbonScript(London_Scene *s) : scene(s) {}
    void run() override;
    bool mouse();                                    // 0x4b9130
    void nearPrompts();                              // 0x4b91c0
    London_Scene *scene;
    uint32_t t = 0;                                  // +0xc4
    bool near = false;                               // +0xc8
    bool flag_c9 = false;                            // +0xc9
    bool on_wendy = false;                           // +0xca: the mouse went onto Wendy
    bool clicked_far = false;                        // +0xcb: the ribbon clicked from too far
};

class LSTickTockScript : public Vision::Task {       // vtable 0x516c0c, ctor 0x4b9970, run 0x4b9b50
public:
    explicit LSTickTockScript(London_Scene *s);
    void run() override;
    bool mouse();                                    // 0x4b9ac0
    void step();                                     // 0x4baad0: one frame of the clock game
    void release();                                  // 0x4b9b40 (from the scene's leave)
    London_Scene *scene;
    int secs = 180;                                  // +0xc0: the cannon's time
    bool was_near = false;                           // +0xc4
    Raytrace::CameraMover mover;                     // +0xc8
    int game = 0;                                    // +0xe8: the clock game's state (7 won, 8 caught)
    float t_moved = 0, t_turn = 0, wait_ms = 0;      // +0xf0 +0xf4 +0xf8
    Vision::Sound tick;                              // +0xfc "tick_fx_ticktock"
    int back_min = 3000, back_max = 5000;            // +0x444 +0x448: how long Wendy keeps her back turned
    int first_wait = 2000;                           // +0x44c
    int grace = 1000;                                // +0x450: ms after "stop" before moving counts
    float stop_x = 0, stop_z = 0;                    // +0x454 +0x458: where the camera was when she turned
    bool over_wendy = false, clicked_wendy = false;  // +0x464 +0x465
    bool over_sign = false, clicked_sign = false;    // +0x466 +0x467
};

class LSHuntScript : public Vision::Task {           // vtable 0x516ad0, run 0x4b7320
public:
    explicit LSHuntScript(London_Scene *s) : scene(s) {}
    void run() override;
    bool mouse();                                    // 0x4b72d0
    London_Scene *scene;
    bool clicked_peter = false;                      // +0xd0
    int round = 0;                                   // +0xd4
};

class London_Scene : public TinkerbellRaytraceScene {
public:
    London_Scene();                                  // 0x4b6440
    ~London_Scene() override;
    void enter() override;                           // v19 0x4b5800
    void run() override;                             // v10 0x4b5300
    void leave() override;                           // v20 0x4b6230
    void onMouse() override;                         // v25 0x4b50a0
    bool onKey() override;                           // v26 0x4b5260
    void cursorUpdate() override;                    // v30 0x4b4f70
    void perFrame();                                 // 0x4b65f0: the "all the arrows" picture after some lines
    void scriptFrame();                              // the body of the run's loops
    void waitBusy(Vision::Task *t);                  // the scripts' "while a line plays, wait 30 ms"

    LSKeyboardScript keyboard;                       // +0x28e8
    LSRibbonScript ribbon_script;                    // +0x29c8
    LSTickTockScript ticktock;                       // +0x2a94
    LSHuntScript hunt;                               // +0x2efc
    WallAnimation *john = nullptr, *michael = nullptr;   // +0x2fd4 +0x2fd8
    uint32_t next_wave = 0;                          // +0x2fdc
    LondonCursor *cur_ribbon = nullptr, *cur_normal = nullptr;   // +0x2fe0 +0x2fe4
    bool use_normal = false, use_ribbon = false;     // +0x2fe8 +0x2fe9
    uint32_t arrow_time = 0;                         // +0x31a8
    bool arrow_after[5] = {};                        // +0x31ac..+0x31b0: inter1, inter2, sign2, face1, help1
    AnimationHelper *waterfall = nullptr;            // +0x31b4
    Vision::Sound music;                             // +0x31b8 "LOND_MU_music"
    WendyInLondon *wendy = nullptr;                  // +0x3500
    PeterInLondon *peter = nullptr;                  // +0x3504
    GemSign sign;                                    // +0x3508
    RibbonState ribbon;                              // +0x3538
    RibbonInterface *iface = nullptr;                // +0x3544
    FadeRectangle fade;                              // +0x3548
    Vision::Sound city;                              // +0x369c "lond_ax_city"
    std::vector<Vision::Image *> arrows;             // +0x39e4: left, right, up, down, all, ctrl, enter, shift
    bool clicked = false;                            // +0x39f4
};

}  // namespace Game
