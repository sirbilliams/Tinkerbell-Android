// SceneCharacter, reconstructed from "Tinker Bell.exe" (see scene_char.h).
#include "scene_char.h"
#include "dustlift.h"
#include "state.h"
#include "../vision/animation.h"
#include <float.h>
#include <string.h>

namespace Game {

using Raytrace::Sprite3D;

static bool mouse_down() { return Vision::g_mouse.ldown || Vision::g_mouse.rdown; }   // 0x5a6e30
static bool vo_chaining() {                          // 0x4c93c0: the current line goes on into a "_z" one
    const std::string &c = Vision::VoiceOver::get().current();
    return c.find("_y") != std::string::npos || c.find("_Y") != std::string::npos;
}
static bool vo_playing(const std::string &s) { return Vision::VoiceOver::get().isPlaying(s.c_str()); }   // 0x4c9690

SceneCharacter::SceneCharacter(const char *nm, Raytrace::RayTrace *r, DustLift **l)   // 0x4ccad0
    : RayTraceCharacter(r, nm), Task(nullptr), lift(l) {
    mouse.setClient(this);                           // 0x41f290 (the Task sub-object)
    keys.setClient(this);
}

SceneCharacter::~SceneCharacter() {}                 // 0x43e3a0

// 0x4ce2d0: the 19 talking loops the game knows (a stack array of strings in the exe)
void SceneCharacter::setTalks(const char *r, const char *t1, const char *t2, const char *t3) {
    static const char *const loops[] = {
        "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_", "GLOB_CA_PETtalk4_",
        "GLOB_CA_WENtalk1_", "GLOB_CA_WENtalk2_", "GLOB_CA_WENtalk3_",
        "GLOB_CA_RABtalk2_", "GLOB_CA_RABtalk3_",
        "GLOB_CA_SKNtalk1_", "GLOB_CA_SKNtalk2_",
        "GLOB_CA_FOXtalk1_", "GLOB_CA_FOXtalk3_",
        "GLOB_CA_RACtalk1_", "GLOB_CA_RACtalk2_", "GLOB_CA_RACtalk3_",
        "GLOB_CA_CUBtalk1_", "GLOB_CA_CUBtalk2_", "GLOB_CA_CUBtalk3_",
    };
    roottalk = r ? r : "";
    const char *t[3] = {t1, t2, t3};
    for (int i = 0; i < 3; i++) {
        talk[i] = t[i] ? t[i] : "";
        bool found = false;
        for (const char *l : loops)
            if (talk[i] == l) { found = true; break; }   // 0x423cf0
        gesture[i].once = !found;                    // +0x268 / +0x270 / +0x278
    }
}

// 0x4cd550: the animation from its first frame (unless it already shows it in this mode), scale 126.
void SceneCharacter::play(const char *anim, int mode) {
    Sprite3D *s = sprite();
    if (s) {
        if (strcmp(s->anim->name(), anim) == 0 && s->anim->mode == mode) return;   // 0x423c60, +0x104
    }
    setAnim(anim, nullptr, 3, mode);                 // 0x4cc780
    sprite()->scale = 126.0f;                        // +0x84 = 0x42fc0000
    sprite()->anim->setFrame(0);
    sprite()->anim->dir = 1;                         // +0x110
    if (mode == 2) gesture[0].n = gesture[1].n = gesture[2].n = 0;   // +0x26c +0x274 +0x27c
}

bool SceneCharacter::animDone() {                    // 0x4cfed0
    Vision::Animation *a = sprite()->anim;
    if (a->mode == 0) return a->current() == a->count() - 1;
    return a->current() == 0 && a->dir == -1;
}

void SceneCharacter::init() {                        // 0x4cce80
    Task::reset();                                   // 0x421110
    Task::activate();                                // +0x18
    play(root.c_str(), 0);
    sprite()->anim->setFrame(Vision::vision_random(sprite()->anim->count()));   // state +8 (0x425d40)
    mouth_att = sprite()->attach(lip_name.c_str(), mouth_prefix.c_str(), lip_z);   // 0x42f390 + 0x42e4a0
    mouth_att->on = false;                           // +0x17c
    state = 1;
    mouse.activate();                                // +0x18
    keys.activate();
    wait_line = "";                                  // 0x5464a0
    wave_line = "";
    if (eyes != "") {                                // 0x423c60
        // port: the exe makes it without loading it (no 0x42e4a0); the port's attach loads at once
        eyes_att = sprite()->attach(eyes.c_str(), eyes.c_str(), 0);
        eyes_att->on = false;
        blink_at = 0;
    }
}

void SceneCharacter::release() {                     // 0x4ccd80
    RayTraceCharacter::release();                    // 0x4cc750
    mouth_att = nullptr;
    Task::deactivate();                              // +0x1c
    mouse.deactivate();
    keys.deactivate();
    eyes_att = nullptr;
}

// 0x4cd3f0: while it shows its talking root or a gesture, the eyes close for a frame every 1-2 s (the first
// time within 1 s); otherwise they stay open.
void SceneCharacter::blink() {
    if (!eyes_att) return;
    std::string an = sprite()->anim->name();          // 0x4243f0 + 0x423f50
    if (an != roottalk && an != talk[0] && an != talk[1] && an != talk[2]) {   // 0x423cf0
        eyes_att->on = false;
        blink_at = 0;
        return;
    }
    if (eyes_att->on) {
        eyes_att->on = false;
        int r = Vision::vision_random(1000);
        blink_at = r + Vision::frame_time() + 2000;
        return;
    }
    if (blink_at == 0) {
        int r = Vision::vision_random(1000);
        blink_at = r + Vision::frame_time();
    }
    if (Vision::frame_time() > blink_at) eyes_att->on = true;
}

// 0x4cd020: every 83 ms a blink check, the animation's next frame, a win line heard (once settled) and the
// state's step.
void SceneCharacter::run() {
    wait(1);
    for (;;) {
        blink();
        sprite()->anim->step();                      // 0x403d60
        wait(0x53);
        if (state != 6 && state != 7 && state != 8 && wave_lines) {   // port: a null list is skipped (Peter's)
            for (size_t i = 0; i < wave_lines->size(); i++) {
                if (vo_playing((*wave_lines)[i])) {
                    wave_line = (*wave_lines)[i];    // 0x423940 / 0x423b80
                    state = 6;
                    break;
                }
            }
        }
        switch (state) {
        case 0:
            if (animDone()) {
                play(root.c_str(), 0);
                state = 1;
            }
            break;
        case 1: stepIdle(); break;
        case 2: {
            bool done = animDone();
            for (int i = 0; i < 3 && !done; i++) {   // up to three frames ahead
                sprite()->anim->step();
                done = animDone();
            }
            if (done) {
                play(roottalk.c_str(), 0);
                state = 3;
            }
            break;
        }
        case 3: stepTalk(); break;
        case 4:
            if (vo_playing(wait_line)) {
                if (animDone()) play(root.c_str(), 0);
            } else if (animDone() || root == sprite()->anim->name()) {
                play(after_wait.c_str(), 0);
                state = 5;
            }
            break;
        case 5: stepBack(); break;
        case 6: stepWave(); break;
        case 7: stepBack(); break;
        case 8: stepFly(); break;
        default: break;
        }
    }
}

// 0x4cd9b0: at the animation's end, the root (1 in 4) or one of the three idles.
void SceneCharacter::stepIdle() {
    if (!animDone()) return;
    int r = Vision::vision_random(1000);             // +0x1e0
    if (r < 250) play(root.c_str(), 0);
    else if (r < 500) play(idle1.c_str(), 0);
    else if (r < 750) play(idle2.c_str(), 0);
    else play(idle3.c_str(), 0);
}

// 0x4cdac0: in the talking root, at its end one of the gestures (a third each); in a gesture, at its end the
// talking root again, but a talking loop that reaches its far end holds there (stepping between its last two
// frames) while the talk goes on.
void SceneCharacter::stepTalk() {
    std::string an = sprite()->anim->name();
    if (an == roottalk) {
        if (!animDone()) return;
        int r = Vision::vision_random(1000);         // +0x1e0
        int i = r < 334 ? 0 : r < 667 ? 1 : 2;
        play(talk[i].c_str(), gesture[i].once ? 0 : 2);
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (an != talk[i]) continue;
        if (animDone()) {
            play(roottalk.c_str(), 0);
            return;
        }
        if (gesture[i].once) return;
        Vision::Animation *a = sprite()->anim;
        if (a->current() == a->count() - 1 && state == 3) a->setFrame(a->count() - 2);
        return;
    }
}

// 0x4cd930: at the animation's end, the root and the idles.
void SceneCharacter::stepBack() {
    if (!animDone()) return;
    play(root.c_str(), 0);
    state = 1;
}

// 0x4cd6e0: while the win line plays, the root; when it has ended, the cheer.
void SceneCharacter::stepWave() {
    if (vo_playing(wave_line)) {
        if (animDone()) play(root.c_str(), 0);
        return;
    }
    if (animDone() || root == sprite()->anim->name()) {
        play(wave_anim.c_str(), 0);
        state = 7;
    }
}

// 0x4cd830: when not kept up and the lift is done with it (or it shows another animation than the flying
// idle), at the animation's end: the root.
void SceneCharacter::stepFly() {
    if (keep_flying) return;
    Sprite3D *s = sprite();
    DustLift *l = *lift;
    if (l->rising(s) || l->floating(s) || l->sinking(s)) {   // 0x4bf160 / 0x4bf1b0 / 0x4bf200
        if (flyidle == s->anim->name()) return;
    }
    if (!animDone()) return;
    play(root.c_str(), 0);
    state = 1;
}

// 0x4ce770: a click on it with dust on the cursor (picked at any distance): a line it is saying stops, it
// takes off.
void SceneCharacter::notify2(Vision::Service *s) {
    if (s != &mouse || !mouse_down() || !Game::state().dusting) return;
    float keep = rt->pick_max;                       // +0x1170
    rt->pick_max = FLT_MAX;                          // 0x7f7fffff
    Sprite3D *p = rt->pick(Vision::g_mouse.x, Vision::g_mouse.y);   // 0x42c710
    rt->pick_max = keep;
    if (p != sprite() || state == 8) return;
    if (state == 2 || state == 3) Vision::VoiceOver::get().stop();   // 0x4c9260
    play(flyidle.c_str(), 2);
    state = 8;
}

// 0x4ce040: may its line start? Not while flying. Talking already: yes. In the talking root or a gesture: it
// talks. In the root, or another animation that has ended: the talking root starts. Otherwise it waits (state 2)
// for the animation's end. A partner must be ready too.
bool SceneCharacter::ready() {
    if (!mouth_att) return true;
    if (state == 8) return false;
    if (state == 3) return (partner && !vo_chaining()) ? partner->ready() : true;
    std::string an = sprite()->anim->name();          // 0x4243f0 + 0x423f50
    if (an != roottalk && an != talk[0] && an != talk[1] && an != talk[2]) {
        if (an != root && !animDone()) {
            state = 2;
            if (partner) vo_chaining();              // (the exe asks and ignores the answer)
            return false;
        }
        play(roottalk.c_str(), 0);
    }
    state = 3;
    if (partner && !vo_chaining()) return partner->ready();
    return true;
}

// 0x4ccdd0: the mouth shows the letter's frame; the partner mouths along.
void SceneCharacter::mouth(int letter) {
    if (!mouth_att) return;
    mouth_att->on = true;
    static_cast<Vision::Animation *>(mouth_att->g)->setFrame(letter);
    if (partner && !vo_chaining()) partner->mouth(letter);
}

// 0x4cce30: the line ended: the mouth hides, back to the root (state 0); the partner too.
void SceneCharacter::rest() {
    if (!mouth_att) return;
    mouth_att->on = false;
    state = 0;
    if (partner && !vo_chaining()) partner->rest();
}

}  // namespace Game
