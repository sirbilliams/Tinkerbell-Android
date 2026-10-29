// Peter and Wendy in London (PeterInLondon vtable 0x516d78, ctor 0x4bbf90; WendyInLondon vtable 0x516f8c, ctor
// 0x4bdaf0), reconstructed from "Tinker Bell.exe". Each is a MagpieCharacter (it speaks voice lines) standing in
// the 3D street as a sprite, driven by state objects. A state has six slots: v0 enter, v1 leave (false: not yet),
// v2 tick, v3 ready to speak, v4 mouth(letter), v5 the line ended. The character asks for a state (`next`);
// its update switches once the current one lets go. Most states play one AnimationHelper on the sprite
// (sprite +0x70) and, at its end, ask for the next state; the talking ones switch mouth and eye attachments.
#pragma once
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"
#include "../vision/animation.h"
#include "voiceover.h"

namespace Game {

using Raytrace::Curve3;

class LondonChar : public Vision::MagpieCharacter {
public:
    struct State {
        virtual ~State() {}
        virtual void enter(LondonChar *) {}              // v0
        virtual bool leave(LondonChar *) { return true; }   // v1
        virtual void tick(LondonChar *) {}               // v2
        virtual bool ready() { return false; }           // v3
        virtual void mouth(int) {}                       // v4
        virtual void rest(LondonChar *) {}               // v5
    };
    ~LondonChar() override;
    // the MagpieCharacter slots (v1..v3) go to the current state's v3..v5
    bool ready() override;
    void mouth(int letter) override;
    void rest() override;
    // the inlined "ask for a state": idle is not asked for over a talking state
    void request(State *s);
    bool isTalk(const State *s) const;

    Raytrace::RayTrace *rt = nullptr;
    Raytrace::Sprite3D *sprite = nullptr;            // Peter +0x4c, Wendy +0x64
    std::vector<Vision::Ras *> packs;                // Peter +0x40, Wendy +0x58
    State *idle = nullptr;                           // +0x04
    State *talk[4] = {};                             // Peter +0x08.., Wendy +0x0c..
    State *cur = nullptr, *next = nullptr;           // Peter +0x50 +0x54, Wendy +0x68 +0x6c
    std::vector<State *> owned;                      // port: the states, deleted in the dtor
protected:
    void openPacks(const char *a, const char *b);
    State *own(State *s) { owned.push_back(s); return s; }
};

class PeterInLondon : public LondonChar {
public:
    explicit PeterInLondon(Raytrace::RayTrace *rt);  // 0x4bbf90
    ~PeterInLondon() override;                       // 0x4bc560
    void update();                                   // 0x4bc880: every frame (the last London script)
    void say(const char *line);                      // 0x4bc8b0: the talking state for the line, then play it
    void bow() { request(st_bow); }                  // 0x4bc760
    void showHand() { request(st_showhand); }        // 0x4bc790
    void turnToWendy() { request(st_turnwendy); }    // 0x4bc7c0
    void turnToPlayer() { request(st_turnplayer); }  // 0x4bc7f0
    void fly() { request(st_fly); }                  // 0x4bc820
    void land() { request(st_land); }                // 0x4bc850
    void setFlag(bool f) { flag = f; }               // 0x4bcb60 (+0x58)
    bool isIdle() const { return cur == idle; }      // 0x4bcb70
    bool landing() const { return cur == st_land || next == st_land; }   // 0x4bcb90
    bool bowing() const { return cur == st_bow || next == st_bow; }      // 0x4bcbb0
    State *st_bow = nullptr, *st_turnwendy = nullptr, *st_facing = nullptr, *st_turnplayer = nullptr;   // +0x18..+0x24
    State *st_land = nullptr, *st_fly = nullptr, *st_showhand = nullptr, *st_idlehand = nullptr;        // +0x28..+0x34
    bool flag = false;                               // +0x58
};

class WendyInLondon : public LondonChar {
public:
    explicit WendyInLondon(Raytrace::RayTrace *rt);  // 0x4bdaf0
    ~WendyInLondon() override;                       // 0x4be390
    void update();                                   // 0x4be780: every frame
    void say(const char *line);                      // 0x4be880
    void showClock() { request(st_showclock); }      // 0x4be5d0
    void hideClock() { request(st_hideclock); }      // 0x4be600
    void toBack() { request(st_toback); }            // 0x4be630
    void toForward() { request(st_toforward); }      // 0x4be660
    void toPoint() { request(st_topoint); }          // 0x4be690
    void pointDown() { request(st_pointdown); }      // 0x4be6c0
    void turnToPeter() { request(st_turnpeter); }    // 0x4be6f0
    void turnToPlayer() { request(st_turnplayer); }  // 0x4be720
    void fly() { request(st_fly); }                  // 0x4be750
    void position(float out[3]);                     // 0x4be7d0: x, height above the ground, z
    bool isBack() const { return cur == st_back; }   // 0x4bed90
    bool isForward() const { return cur == st_forward; }   // 0x4bedb0
    bool isPoint() const { return cur == st_point; }       // 0x4bedd0
    bool isIdle() const { return cur == idle; }            // 0x4bedf0
    bool isFlyIdle() const { return cur == st_flyidle; }   // 0x4bee10
    void setFlag(bool f) { flag = f; }               // 0x4bed80 (+0x74)
    State *st_rootidle = nullptr;                    // +0x08
    State *st_showclock = nullptr, *st_hideclock = nullptr, *st_toback = nullptr, *st_back = nullptr;   // +0x1c..+0x28
    State *st_toforward = nullptr, *st_forward = nullptr, *st_topoint = nullptr, *st_point = nullptr;   // +0x2c..+0x38
    State *st_pointdown = nullptr, *st_turnpeter = nullptr, *st_facing = nullptr, *st_turnplayer = nullptr;   // +0x3c..+0x48
    State *st_fly = nullptr, *st_flyidle = nullptr;  // +0x4c +0x50
    bool flag = false;                               // +0x74
};

}  // namespace Game
