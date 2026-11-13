// WamiBaseChar and WamiChar, reconstructed from "Tinker Bell.exe" (see wamichar.h).
#include "wamichar.h"
#include "state.h"
#include "../vision/animation.h"
#include "../vision/engine.h"
#include "tb.h"
#include <stdlib.h>
#include <string.h>

namespace Game {

using Raytrace::Sprite3D;
using Vision::VoiceOver;

static bool vo_busy() { return Game::state().vo_busy(); }   // *(0x63171c) +0x560

Vision::Sound &WamiBaseChar::clock() {               // 0x4d0150 (destroyed by 0x4d0170)
    static Vision::Sound s("WAMI_FX_CLOCK");          // 0x420210
    return s;
}

WamiBaseChar::WamiBaseChar(const char *nm, Raytrace::RayTrace *r)   // 0x4d0190
    : RayTraceCharacter(r, nm), Task(nullptr) {}

WamiBaseChar::~WamiBaseChar() {}

void WamiBaseChar::setMouth(const char *n, int i) {  // 0x4d0430
    mouths[i] = n ? n : "";                          // 0x423920 for null
}

bool WamiBaseChar::animIs(const char *anim) {
    return strcmp(sprite()->anim->name(), anim) == 0;   // 0x4cc8b0, +0x70, 0x4243f0, 0x423c60
}

// 0x4d0460: the pack (none: its name is never set), the first animation, the mouths up to the first unnamed one
// (made hidden), the task, and the shared clock sound opened (looping).
void WamiBaseChar::start(const char *anim, int mode) {
    if (pack_name != "") {                           // 0x423c60 with 0x5464a0
        pack = Vision::ras_open(pack_name.c_str(), "", &Vision::data_device());   // 0x419b90
        pack->load();                                // 0x41aa20
        Vision::ras_set_current(pack);               // 0x41a920
    }
    rest_att = nullptr;                              // +0x1f8
    eyes_att = nullptr;                              // +0x20c
    playAnim(anim, mode);
    for (int i = 0; i < 4; i++) {
        if (mouths[i] == "") break;
        // the exe tests the name again here (null when empty): never reached
        mouth_att[i] = sprite()->attach(mouths[i].c_str(), mouths[i].c_str(), 0);   // 0x42f390 + 0x42e4a0
        mouth_att[i]->on = false;                    // +0x17c
    }
    Task::reset();                                   // 0x421110
    Task::activate();                                // +0x18
    synced = false;                                  // +0x1f0
    if (!clock().isOpen()) {                         // 0x41f9c0
        clock().open();                              // 0x41f9d0(0)
        clock().type = 6;                            // 0x631578 (+0x340)
    }
}

void WamiBaseChar::release() {                       // 0x4d05a0
    RayTraceCharacter::release();                    // 0x4cc750
    for (auto &m : mouth_att) m = nullptr;
    Task::deactivate();                              // +0x1c
    if (pack_name != "") {
        Vision::ras_close(pack);                     // 0x419620
        pack = nullptr;
    }
    queue.clear();                                   // (swapped with empty ones)
    lines.clear();
    clock().close();                                 // 0x41fc50
}

// 0x4d0990: the eyes shown for one step now and then: 1-2 s apart after a blink, within 1 s at first.
void WamiBaseChar::blink() {
    if (!eyes_att) return;
    std::string an = sprite()->anim->name();          // 0x4243f0 (copied, unused)
    (void)an;
    if (eyes_att->on) {
        eyes_att->on = false;
        int r = Vision::vision_random(1000);         // state +8 (0x425d40)
        blink_at = r + Vision::frame_time() + 2000;
        return;
    }
    if (blink_at == 0) {
        int r = Vision::vision_random(1000);
        blink_at = r + Vision::frame_time();
    }
    if (Vision::frame_time() > blink_at) eyes_att->on = true;
}

// 0x4d0a80: the animation from its first frame (unless it already shows it in this mode), its resting mouth (the
// listening idles) and eyes; state 1.
void WamiBaseChar::playAnim(const char *anim, int mode) {
    Sprite3D *s = sprite();
    if (s) {
        if (strcmp(s->anim->name(), anim) == 0 && s->anim->mode == mode) return;   // +0x104
    }
    if (rest_att) s->detach(rest_att->name.c_str());   // 0x42f560 (+0x154)
    if (eyes_att) s->detach(eyes_att->name.c_str());
    static const char *const idles[7] = {"WAMI_CA_PETlkidle_", "WAMI_CA_PETleidle_", "LOND_CA_PETlwidle_",
                                         "WAMI_CA_HOKidlpan_", "WAMI_CA_SMEwidle_", "WAMI_CA_PETunarm_",
                                         "WAMI_CA_PETunarid_"};
    static const char *const rests[7] = {"PEL_moutrest", "PEL_moutrest", "PER_moutrest", "HOW_moutrest",
                                         "SME_moutrest", "PEA_moutrest", "PEA_moutrest"};
    int found = -1;
    for (int i = 0; i < 7; i++)
        if (strcmp(idles[i], anim) == 0) { found = i; break; }   // 0x423c60
    setAnim(anim, nullptr, 3, mode);                 // 0x4cc780
    Sprite3D *sp = sprite();
    sp->scale = scale;                               // +0x84 = +0x144
    sp->anim->setFrame(0);                           // 0x403f20
    sp->anim->dir = 1;                               // +0x110
    state = 1;                                       // +0x178
    if (found >= 0) rest_att = sp->attach(rests[found], rests[found], -1);   // 0x42f390
    else rest_att = nullptr;
    auto e = eyes_by_anim.find(anim);                // 0x4d4960
    if (e != eyes_by_anim.end()) {
        // port: the exe makes it without loading it; the port's attach loads at once
        eyes_att = sp->attach(e->second.c_str(), e->second.c_str(), -2);
        blink_at = 0;
    } else {
        eyes_att = nullptr;
    }
    holds = 0;                                       // +0x214
}

// 0x4d12a0: once-through (0, 1): the last frame (the first going backwards; Peter's hand at frame 9); back and
// forth (2): back at the first frame going backwards.
bool WamiBaseChar::animDone() {
    Sprite3D *s = sprite();
    Vision::Animation *a = s->anim;
    int m = a->mode;                                 // +0x104
    if (m != 0 && m != 1) return a->current() == 0 && a->dir == -1;   // 0x403f00, +0x110
    if (strcmp(a->name(), "LOND_CA_PEThand_") == 0 && a->current() == 9) return true;
    if (a->dir == -1) return a->current() == 0;
    return a->current() == a->count() - 1;          // 0x403c10
}

void WamiBaseChar::queueAnim(const char *anim, int mode, int dir, bool sync, bool nowait) {   // 0x4d13d0
    Queued q;
    q.anim = anim ? anim : "";
    q.mode = mode;
    q.dir = dir;
    q.synced = sync;
    q.nowait = nowait;
    queue.push_back(q);                              // +0x17c
    state = 2;
}

// 0x4d1510: the next queued animation once the current one ends (or at once if it is "nowait"); Wendy's ready
// and clock gestures hold a frame meanwhile. A root animation ends the queue (state 4: idling).
void WamiBaseChar::animStep() {
    if (queue.empty()) return;                       // +0x1a8
    Queued &front = queue.front();
    if (!front.nowait && !animDone()) {
        Vision::Animation *a = sprite()->anim;
        if (strcmp(a->name(), "WAMI_CA_WENready_") == 0) {
            if (a->current() != 9) return;
            int h = holds++;
            if (h >= max_holds) return;              // +0x218
            a->setFrame(8);
            return;
        }
        if (strcmp(a->name(), "WAMI_CA_WENclock_") == 0) {
            if (a->current() != 10) return;
            int h = holds++;
            if (h >= 15) return;
            a->setFrame(9);
        }
        return;
    }
    Queued q = front;                                // (copied)
    playAnim(q.anim.c_str(), q.mode);
    if (q.dir == -1) {
        sprite()->anim->dir = -1;                    // +0x110
        sprite()->anim->setFrame(sprite()->anim->count() - 1);
    }
    synced = q.synced;                               // +0x1f0
    queue.pop_front();
    if (q.anim != "GLOB_CA_PETroot_" && q.anim != "GLOB_CA_WENroot_") {
        state = 2;
        synced_t0 = Vision::frame_time();            // +0x1f4
    } else {
        state = 4;
    }
}

void WamiBaseChar::sayOn(const char *anim, const char *line) {   // 0x4d1800
    lines.push_back({anim ? anim : "", line ? line : ""});   // +0x1ac
}

// 0x4d1970: the first waiting line, once its animation shows and no voice-over is busy (the clock line plays the
// shared clock sound).
void WamiBaseChar::speak() {
    if (lines.empty()) return;                       // +0x1d8
    Line l = lines.front();                          // (copied)
    if (strcmp(l.anim.c_str(), sprite()->anim->name()) != 0) return;   // 0x423c60
    if (vo_busy()) return;
    if (strcmp(l.line.c_str(), clock().name()) == 0) clock().play();   // 0x4243f0 on 0x631238, 0x41fd50
    else VoiceOver::get().play(l.line.c_str());      // 0x4c9400
    if (getenv("TB_DEBUG_TREASURE")) tb_log("treasure: %s says %s", RayTraceCharacter::name.c_str(), l.line.c_str());   // test aid
    lines.pop_front();
}

bool WamiBaseChar::isQueued(const char *line) {      // 0x4d1da0
    for (const Line &l : lines)
        if (l.line == line) return true;
    return false;
}

// 0x4d1120: every +0x164 ms the waiting line, the frame (by the clock for a synced animation, else the next) and
// the state's step, then a blink.
void WamiBaseChar::run() {
    wait(1);
    for (;;) {
        wait(frame_ms);
        speak();
        if (synced) {
            uint32_t f = (Vision::frame_time() - synced_t0) / frame_ms;   // unsigned
            int n = sprite()->anim->count() - 1;
            if ((int)f > n) f = sprite()->anim->count() - 1;
            sprite()->anim->setFrame((int)f);
            Vision::vision_log("Frame Number = %d", (int)f);   // 0x4208c0
        } else {
            sprite()->anim->step();                  // 0x403d60
        }
        switch (state) {
        case 0: {
            bool done = animDone();
            if (!done) {
                sprite()->anim->step();
                done = animDone();
            }
            if (done) {
                playAnim(idle.c_str(), 1);
                state = 1;
            }
            break;
        }
        case 1: break;
        case 2: animStep(); break;
        default: break;
        }
        blink();
    }
}

// 0x4d1c80: the line's mouth (setInt; else the first) shows the letter; the resting mouth hides.
void WamiBaseChar::mouth(int letter) {
    if (!mouth_att[0]) return;
    mouth_idx = 0;                                   // +0x1ec
    auto it = mouth_by_line.find(VoiceOver::get().current());   // state +0x544
    if (it != mouth_by_line.end()) mouth_idx = it->second;
    Raytrace::Attachment *m = mouth_att[mouth_idx];
    m->on = true;
    static_cast<Vision::Animation *>(m->g)->setFrame(letter);   // 0x403f20 on +0x150
    if (rest_att) rest_att->on = false;
}

void WamiBaseChar::rest() {                          // 0x4d1d50
    if (!mouth_att[0]) return;
    mouth_att[mouth_idx]->on = false;
    if (rest_att) rest_att->on = true;
}

bool WamiBaseChar::ready() { return state == 1 || state == 2; }   // 0x4d1d80

// ---------------------------------------------------------------- WamiChar

WamiChar::WamiChar(const char *nm, Raytrace::RayTrace *r) : WamiBaseChar(nm, r) {}   // 0x4d2170

// 0x4d2280: the task, the pack (none: its name is never set), the root; idling (state 4); Wendy's first idle in 10 s.
void WamiChar::init() {
    Task::activate();                                // +0x18
    if (pack2 != "") {
        pack2_ras = Vision::ras_open(pack2.c_str(), "", &Vision::data_device());
        pack2_ras->load();
        Vision::ras_set_current(pack2_ras);
    }
    start(root.c_str(), 0);                          // 0x4d0460
    state = 4;
    wendy_idle_at = Vision::frame_time() + 10000;    // +0x2a4
}

void WamiChar::release() {                           // 0x4d2320
    Task::deactivate();
    WamiBaseChar::release();                         // 0x4d05a0
    if (pack2 != "") {
        Vision::ras_close(pack2_ras);
        pack2_ras = nullptr;
    }
}

// 0x4d25f0: as SceneCharacter's 0x4ce2d0 with 18 talking loops (no "GLOB_CA_PETtalk4_").
void WamiChar::setTalks(const char *r, const char *t1, const char *t2, const char *t3) {
    static const char *const loops[] = {
        "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_", "GLOB_CA_PETtalk3_",
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
        gesture[i].once = !found;                    // +0x28c / +0x294 / +0x29c
    }
}

void WamiChar::playKeep(const char *anim, int mode) {   // 0x4d2a70
    if (mode == 2) gesture[0].n = gesture[1].n = gesture[2].n = 0;   // +0x290 +0x298 +0x2a0
    int s = state;
    playAnim(anim, mode);
    state = s;
}

// 0x4d2ab0: at the animation's end the root (1 in 4) or an idle; Wendy's first idle only every 10 s.
void WamiChar::idleStep() {
    if (!animDone()) return;
    int r = Vision::vision_random(1000);             // state +8
    const std::string *next;
    if (r < 250) next = &root;
    else if (r < 500) {
        next = &idle1;
        if (RayTraceCharacter::name == "wendy") {    // +0x21
            if (wendy_idle_at > Vision::frame_time()) next = &root;
            else wendy_idle_at = Vision::frame_time() + 10000;
        }
    } else if (r < 750) next = &idle2;
    else next = &idle3;
    int s = state;
    playAnim(next->c_str(), 0);
    state = s;
}

// 0x4d2bf0: as SceneCharacter's stepTalk (0x4cdac0), with state 6.
void WamiChar::talkStep() {
    std::string an = sprite()->anim->name();
    if (an == roottalk) {
        if (!animDone()) return;
        int r = Vision::vision_random(1000);
        int i = r < 334 ? 0 : r < 667 ? 1 : 2;
        if (gesture[i].once) playKeep(talk[i].c_str(), 0);   // (inlined: keeping the state)
        else {
            gesture[0].n = gesture[1].n = gesture[2].n = 0;
            int s = state;
            playAnim(talk[i].c_str(), 2);
            state = s;
        }
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (an != talk[i]) continue;
        if (animDone()) {
            playKeep(roottalk.c_str(), 0);
            return;
        }
        if (gesture[i].once) return;
        Vision::Animation *a = sprite()->anim;
        if (a->current() == a->count() - 1 && state == 6) a->setFrame(a->count() - 2);
        return;
    }
}

// 0x4d2360: every +0x164 ms the waiting line, the next frame, the state's step, a blink.
void WamiChar::run() {
    wait(1);
    for (;;) {
        wait(frame_ms);
        speak();                                     // 0x4d1970
        sprite()->anim->step();                      // 0x403d60
        switch (state) {                             // the table 0x4d2544
        case 0: {
            bool done = animDone();
            if (!done) {
                sprite()->anim->step();
                done = animDone();
            }
            if (done) {
                playAnim(idle.c_str(), 1);
                state = 1;
            }
            break;
        }
        case 1: break;
        case 2: animStep(); break;
        case 3:
            if (animDone()) {
                playAnim(root.c_str(), 0);
                state = 4;
            }
            break;
        case 4: idleStep(); break;
        case 5: {
            bool done = animDone();
            if (!done) {
                sprite()->anim->step();
                done = animDone();
            }
            if (done) {
                playAnim(roottalk.c_str(), 0);
                state = 6;
            }
            break;
        }
        case 6: talkStep(); break;
        default: break;
        }
        blink();
    }
}

// 0x4d2f30: ready in the queue (1, 2) or talking (6); from a talking animation at once; from the root or an idle
// at its end to the talking root (an idle not at its end: state 5, not yet).
bool WamiChar::ready() {
    if (state == 1 || state == 2 || state == 6) return true;
    std::string an = sprite()->anim->name();
    if (an == roottalk || an == talk[0] || an == talk[1] || an == talk[2]) {   // 0x423cf0
        state = 6;
        return true;
    }
    if (an != root && !animDone()) {
        state = 5;
        return false;
    }
    playKeep(roottalk.c_str(), 0);
    state = 6;
    return true;
}

// 0x4d30a0: the mouth hides, the resting one shows; back to the root (state 3) unless in the queue.
void WamiChar::rest() {
    if (!mouth_att[0]) return;
    mouth_att[mouth_idx]->on = false;
    if (rest_att) rest_att->on = true;
    if (state != 1 && state != 2) state = 3;
}

}  // namespace Game
