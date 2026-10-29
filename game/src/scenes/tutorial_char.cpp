// TutorialCharacter, reconstructed from "Tinker Bell.exe" (see tutorial_char.h).
#include "tutorial_char.h"
#include "dustlift.h"
#include "state.h"
#include "../vision/animation.h"
#include <string.h>

namespace Game {

using Raytrace::Sprite3D;

static bool mouse_down() { return Vision::g_mouse.ldown || Vision::g_mouse.rdown; }   // 0x5a6e30
static bool vo_chaining() {                          // 0x4c93c0: the current line goes on into a "_z" one
    const std::string &c = Vision::VoiceOver::get().current();
    return c.find("_y") != std::string::npos || c.find("_Y") != std::string::npos;
}

TutorialCharacter::TutorialCharacter(const char *nm, Raytrace::RayTrace *r, DustLift **l)   // 0x4cf150
    : RayTraceCharacter(r, nm), Task(nullptr), lift(l) {
    mouse.setClient(this);                           // 0x41f290 (the Task sub-object)
    keys.setClient(this);
}

TutorialCharacter::~TutorialCharacter() {}           // 0x4442e0

void TutorialCharacter::play(const char *anim, int mode) {   // 0x4cf940
    setAnim(anim, nullptr, 3, mode);                 // 0x4cc780
    sprite()->scale = 130.0f;                        // +0x84 = 0x43020000
    sprite()->anim->setFrame(0);
    sprite()->anim->dir = 1;                         // +0x110
}

bool TutorialCharacter::animDone() {                 // 0x4cfed0
    Vision::Animation *a = sprite()->anim;
    if (a->mode == 0) return a->current() == a->count() - 1;
    return a->current() == 0 && a->dir == -1;
}

void TutorialCharacter::init() {                     // 0x4cf4a0
    Task::reset();                                   // 0x421110
    Task::activate();                                // +0x18
    setAnim(root.c_str(), nullptr, 3, 0);
    sprite()->scale = 130.0f;
    sprite()->anim->setFrame(0);
    sprite()->anim->dir = 1;
    sprite()->anim->setFrame(Vision::vision_random(sprite()->anim->count()));   // state +8 (0x425d40)
    mouth_att = sprite()->attach(lip_name.c_str(), mouth_prefix.c_str(), lip_z);   // 0x42f390 + 0x42e4a0
    mouth_att->on = false;                           // +0x17c
    state = 1;
    mouse.activate();                                // +0x18
    keys.activate();
    wave_pending = false;
    root_pending = false;
}

void TutorialCharacter::release() {                  // 0x4cf370
    RayTraceCharacter::release();                    // 0x4cc750
    mouth_att = nullptr;
    Task::deactivate();                              // +0x1c
    mouse.deactivate();
    keys.deactivate();
}

// 0x4cf5d0: every 83 ms the animation's next frame and the state's step. The wave line starting marks a wave to
// follow (on rest).
void TutorialCharacter::run() {
    wait(1);
    for (;;) {
        sprite()->anim->step();                      // 0x403d60
        wait(0x53);
        if (!wave_pending && state != 6 && state != 7 && Vision::VoiceOver::get().isPlaying(wave_line.c_str()))
            wave_pending = true;
        switch (state) {
        case 0:
        root_once:
            if (animDone()) {
                play(root.c_str(), 0);
                state = 1;
            }
            break;
        case 1:
            if (animDone()) {
                int r = Vision::vision_random(100);  // +0x18c
                if (r < 80) play(root.c_str(), 0);
                else if (r < 90) play(idle1.c_str(), 0);
                else play(idle2.c_str(), 0);
            }
            break;
        case 2:
            root_pending = false;
            if (animDone()) {
                play(root.c_str(), 0);
                state = 3;
            }
            break;
        case 3:
            if (animDone()) play(root.c_str(), 0);
            break;
        case 4: {
            bool done = animDone();
            if (!done) {
                sprite()->anim->step();
                done = animDone();
            }
            if (done) {
                const char *t = talk_anim ? talk_anim : roottalk.c_str();
                play(t, 2);
                state = 5;
            }
            break;
        }
        case 5: {
            Sprite3D *s = sprite();
            if (s->anim->current() == 0 && s->anim->dir == -1 && talk_anim) {
                play(roottalk.c_str(), 0);
                talk_anim = nullptr;
            }
            break;
        }
        case 6:
            wave_pending = false;
            play(wave_anim.c_str(), 0);
            state = 7;
            break;
        case 7:
            state = 0;
            break;
        case 8:
            if (stillFlying()) break;
            goto root_once;
        default:
            break;
        }
    }
}

// 0x4cff30: a click on it with dust on the cursor: its line stops, it takes off (the flying idle, back and forth).
void TutorialCharacter::notify2(Vision::Service *s) {
    if (s != &mouse || !mouse_down() || !Game::state().dusting) return;
    if (rt->pick(Vision::g_mouse.x, Vision::g_mouse.y) != sprite() || state == 8) return;
    if (state == 4 || state == 5) Vision::VoiceOver::get().stop();
    setAnim(flyidle.c_str(), nullptr, 3, 2);
    sprite()->scale = 130.0f;
    sprite()->anim->setFrame(0);
    sprite()->anim->dir = 1;
    state = 8;
}

// 0x4cfb40: kept up (Peter), or in the lift with the flying idle showing.
bool TutorialCharacter::stillFlying() {
    if (keep_flying) return true;
    Sprite3D *s = sprite();
    DustLift *l = *lift;
    if (!l->rising(s) && !l->floating(s) && !l->sinking(s)) return false;
    return strcmp(s->anim->name(), flyidle.c_str()) == 0;   // 0x423c60(.., 0)
}

// 0x4cfa10: when not kept up and the lift is done with it (or never had it), at the animation's end: the root.
void TutorialCharacter::settle() {
    if (keep_flying) return;
    Sprite3D *s = sprite();
    DustLift *l = *lift;
    if (l->rising(s) || l->floating(s) || l->sinking(s)) {
        if (strcmp(s->anim->name(), flyidle.c_str()) == 0) return;
    }
    Vision::Animation *a = sprite()->anim;
    if (a->mode == 0) {
        if (a->current() != a->count() - 1) return;
    } else {
        if (a->current() != 0 || a->dir != -1) return;
    }
    setAnim(root.c_str(), nullptr, 3, 0);
    sprite()->scale = 130.0f;
    sprite()->anim->setFrame(0);
    sprite()->anim->dir = 1;
    state = 1;
}

// 0x4cfbd0: may its line start? Not while flying. Talking already: yes. In another animation than the root: once
// that has ended (else it waits in state 4). Then the talk starts. A partner must be ready too.
bool TutorialCharacter::ready() {
    if (state == 8) return false;
    if (state == 5) return (partner && !vo_chaining()) ? partner->ready() : true;
    std::string an = sprite()->anim->name();          // 0x4243f0 + 0x423f50
    if (an != roottalk) {                            // 0x423cf0
        if (an != root) {                            // another animation: let it end first
            Vision::Animation *a = sprite()->anim;
            bool end = (a->mode == 0) ? a->current() == a->count() - 1 : (a->current() == 0 && a->dir == -1);
            if (!end) {
                state = 4;
                if (partner) vo_chaining();          // (the exe asks and ignores the answer)
                return false;
            }
        }
        if (talk_anim) play(talk_anim, 2);
        else play(roottalk.c_str(), 0);
    }
    state = 5;
    if (partner && !vo_chaining()) return partner->ready();
    return true;
}

// 0x4cf3b0: the mouth shows the letter's frame; the partner mouths along.
void TutorialCharacter::mouth(int letter) {
    if (!mouth_att) return;
    mouth_att->on = true;
    static_cast<Vision::Animation *>(mouth_att->g)->setFrame(letter);
    if (partner && !vo_chaining()) partner->mouth(letter);
}

// 0x4cf410: the line ended: the mouth hides; a wave or a return to the root that was asked for follows.
void TutorialCharacter::rest() {
    talk_anim = nullptr;
    if (!mouth_att) return;
    mouth_att->on = false;
    if (partner && !vo_chaining()) partner->rest();
    if (wave_pending) { wave_pending = false; state = 6; return; }
    if (root_pending) { root_pending = false; state = 2; return; }
    state = 0;
}

}  // namespace Game
