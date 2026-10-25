// The sign-in room (scene "signin"), reconstructed from "Tinker Bell.exe".
//
// Six picture frames on Wendy's wall are the save slots. An empty frame is filled by picking a place
// (London, Skull Rock, Indian Village, Mermaid Cove, Starry Skies, Lost Boys) and a character (Hook, Tinker Bell,
// Peter, Wendy, Tiger Lily, a cub), then a skill level; Big Ben starts the game in London, the map goes straight to
// Neverland (only once the tutorials are passed). The waste basket deletes a frame (it asks first). Wendy talks
// the player through it, with idle prompts and blinking hints.
//   SignInButton  Button@SignInScene  vtable 0x516858  ctor 0x4afe70, dtor 0x4b0070, track 0x4b01b0, draw 0x4b0260,
//                                                      hit 0x4b0320
//   SignInFrame   Frame@SignInScene   vtable 0x5168a4  ctor 0x4b0360, dtor 0x4b0490, setLocation 0x4b0570,
//                                                      setCharacter 0x4b0810, load 0x4b0a50, save 0x4b0c40,
//                                                      erase 0x4b0e80, draw 0x4b07b0
//   SignInCursor  SignInCursor@SignInScene vtable 0x51680c  ctor 0x4afc70, dtor 0x4afd70, draw 0x4afe20
//   SignInWendy   vtables 0x5169a0 / 0x51698c (MagpieCharacter)  ctor 0x4b4310, dtor 0x4b4730, draw 0x4b4990,
//                 idle 0x4b4ac0, ready 0x4b4b30, mouth 0x4b4af0, rest 0x4b4b10
//   SignIn_Scene  SignIn_Scene@SignInScene vtable 0x5168f0  ctor 0x4b1050, load2 0x4b17c0 (v18), enter 0x4b1800
//                 (v19), leave 0x4b2920 (v20), run 0x4b2b70 (v10), notify2 0x4b3e10 (v9), the handlers 0x4b2dd0..
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "voiceover.h"
#include "voacts.h"
#include "acts.h"
#include <string>
#include <vector>

namespace Game {
using namespace Vision;

class SignInButton : public Graphic {
public:
    SignInButton(const char *idl, const char *rov, const char *sel, const char *rollover_vo, const char *click);
    ~SignInButton() override;
    bool hit(int x, int y) override;                 // v11
    void draw() override;                            // v14
    bool track();                                    // 0x4b01b0: the mouse event against the button
    Image idle, rov, sel;                            // +0x98 +0x140 +0x1e8
    PauseOver pause;                                 // +0x290: its line when the mouse rests on it
    Sound *click = nullptr;                          // +0x3a0
    bool clicked = false;                            // +0x3a4: pressed in this mouse event
    bool was_over = false;                           // +0x3a5
    bool enabled = false;                            // +0x3a6
    uint32_t blink = 0;                              // +0x3a8: flashes from this time for 3 s (0 = off)
};

class SignInFrame : public SignInButton {
public:
    SignInFrame(const char *tar, const char *idl, const char *rov, const char *sel, const char *bell_name,
                const char *cap_name);
    ~SignInFrame() override;
    void draw() override;
    void setLocation(int n);                         // 0x4b0570
    void setCharacter(int n);                        // 0x4b0810
    void load(int n);                                // 0x4b0a50: from Save\save<n>
    void save(int n);                                // 0x4b0c40
    void erase(int n);                               // 0x4b0e80
    bool complete() const {                          // the test the scene inlines
        return 0 <= character && character <= 5 && 0 <= location && location <= 5 && 1 <= skill && skill <= 3;
    }
    Image *location_pic = nullptr;                   // +0x3b0
    Image *character_pic = nullptr;                  // +0x3b4
    Image bell, cap, tar;                            // +0x3b8 +0x460 +0x508 (tar: where the pictures go)
    int location = -1;                               // +0x5b0
    int character = -1;                              // +0x5b4
    int skill = 0;                                   // +0x5b8
    bool saved = false;                              // +0x5bc
};

class SignInCursor : public Graphic {
public:
    SignInCursor(const char *active_prefix, const char *normal_name);
    ~SignInCursor() override;
    void draw() override;
    bool active = false;                             // +0x98: over a button
    Animation anim;                                  // +0xa0
    Image normal;                                    // +0x1b8
};

class SignInWendy : public Graphic, public MagpieCharacter {
public:
    SignInWendy();
    ~SignInWendy() override;
    void draw() override;
    bool ready() override;
    void mouth(int letter) override;
    void rest() override;
    void idle();                                     // 0x4b4ac0
    AnimationHelper *root = nullptr;                 // +0x9c
    std::vector<AnimationHelper *> talks;            // +0xa4
    Animation *cur = nullptr;                        // +0xb0
    AnimationHelper *lips = nullptr;                 // +0xb4
    Ras *ras = nullptr;                              // +0xb8 ("sign_wen")
    int state = 0;                                   // +0xbc: 1 idle, 2 talking
    bool picked = false;                             // +0xc0
private:
    void pickTalk();
};

class SignIn_Scene : public Scene {
public:
    SignIn_Scene();                                  // 0x4b1050
    void load2() override;                           // v18 0x4b17c0
    void enter() override;                           // v19 0x4b1800
    void leave() override;                           // v20 0x4b2920
    void run() override;                             // v10 0x4b2b70
    void notify2(Service *s) override;               // v9 0x4b3e10
private:
    void onMouse();                                  // 0x4b3970
    void onFrame(int i);                             // 0x4b2dd0
    void onLocation(int i);                          // 0x4b30c0
    void onCharacter(int i);                         // 0x4b3300
    void onSkill(int i);                             // 0x4b3540
    void onMap();                                    // 0x4b3780
    void onTrash();                                  // 0x4b37d0
    void update();                                   // 0x4b3e70: hints that blink when a prompt ends
    void stopBlinking();                             // 0x4b4160
    void stopPrompts();                              // the twelve +0x34 calls the handlers repeat
    void setEnabled(bool fr, bool lo, bool ch, bool sk);
    void track(SignInButton *b, int group, int i);

    SignInFrame *frames[6] = {};                     // +0xcc (red, blue, purple, orange, yellow, green)
    SignInButton *locs[6] = {};                      // +0xe4
    SignInButton *chars[6] = {};                     // +0xfc
    SignInButton *bigben = nullptr;                  // +0x114
    SignInButton *map = nullptr;                     // +0x118
    SignInButton *skills[3] = {};                    // +0x11c
    SignInButton *trash = nullptr;                   // +0x128
    bool sure = false;                               // +0x12c: the basket asked "are you sure?"
    Sound crumple;                                   // +0x130
    Screen screen;                                   // +0x478
    Mouse mouse;                                     // +0x4f4
    Keyboard keyboard;                               // +0x528
    Ras *ras = nullptr;                              // +0x55c
    Image background;                                // +0x560
    SignInCursor *cursor = nullptr;                  // +0x608
    int slot = -1, loc = -1, chr = -1;               // +0x60c +0x610 +0x614
    int skill_sel = 0;                               // +0x618
    bool tutorials = false;                          // +0x61c
    SignInWendy *wendy = nullptr;                    // +0x620
    std::string next;                                // +0x624: the scene to go to
    FadeRectangle fade;                              // +0x630
    Inactivity frmidl, frame1, bckidl, chridl, difidl, clicksg, cliksg2;   // +0x780 .. +0x1d40
    DelayedVO wall, charpt, levdif, begin, begin2;   // +0x20e0 .. +0x25b0
    Sound music;                                     // +0x26e4
    bool flags[8] = {};                              // +0x2a2c..+0x2a33 (update's edge detectors)
};

}  // namespace Game
