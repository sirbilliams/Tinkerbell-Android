// FadeRectangle, Inactivity and PauseOver, reconstructed from "Tinker Bell.exe" (see acts.h).
#include "acts.h"
#include "voacts.h"

namespace Game {

// ---------------------------------------------------------------- FadeRectangle
FadeRectangle::FadeRectangle() : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {   // 0x4e6100
    rect.w = 640;                                    // +0x104
    rect.h = 480;                                    // +0x108
    rect.visible = true;                             // +0x11c
    rect.z = 0; rect.oz = 0;                         // 0x411e70(0): not in a layer, nothing to re-sort
    rect.setLevel(0.0f);
}

void FadeRectangle::start() {                        // 0x4e61f0
    start_time = frame_time();
    rect.setLevel(0.0f);
    rect.visible = true;
    mode = 0;
}

void FadeRectangle::fadeOut() {                      // 0x4e6220
    start_time = frame_time();
    rect.setLevel(1.0f);
    rect.visible = true;
    mode = 1;
}

// 0x4e6260: level = elapsed * 0.0005 (0x514504), capped at 1 (0x513710); reversed when fading out. At 2 s: done.
void FadeRectangle::draw() {
    if (mode == 2) return;
    uint32_t t = frame_time() - start_time;
    if (t < 2000) {
        float f = (float)t * 0.0005f;
        if (1.0f < f) f = 1.0f;
        if (mode == 1) f = 1.0f - f;
        rect.setLevel(f);
        rect.draw();
        return;
    }
    mode = 2;
    rect.visible = false;
    rect.draw();
}

// ---------------------------------------------------------------- Inactivity
Inactivity::Inactivity() : Act(nullptr) {            // 0x4c4590
    mouse.setClient(this);
    kbd.setClient(this);
}

void Inactivity::add(const char *line, bool flag) {  // 0x4c46e0 (both forms append the line)
    (void)flag;
    lines.push_back(line ? line : "");
}

void Inactivity::start() {                           // 0x4c15f0 (DelayedVO's)
    last = frame_time();
    mouse.activate();
    kbd.activate();
    reset();
    activate();
}

void Inactivity::stop() {                            // 0x4c4690
    if (active()) {
        if (instruction) instruction->stop();        // +0x168 v13
        mouse.deactivate();
        kbd.deactivate();
        deactivate();
    }
}

// 0x4c4860: any event from its mouse or keyboard (the exe doesn't test which kind) restarts the wait and cuts its
// line short.
void Inactivity::notify2(Service *) {
    last = frame_time();
    if (!current.empty() && vo_isPlaying(current.c_str())) VoiceOver::get().stop();
}

void Inactivity::run() {                             // 0x4c48c0
    wait(1);
    for (;;) {
        for (;;) {
            do {
                wait(100);
                if (g_vo_suppressed()) last = frame_time();
            } while (lines.empty() || frame_time() - last <= interval);
            if (!g_vo_suppressed()) break;
            last = frame_time();
        }
        const std::string &l = lines[index];
        if (l == "I") {                              // 0x542484
            if (instruction) { instruction->setName(l.c_str()); instruction->start(); }
        } else {
            vo_play(l.c_str());
        }
        current = l;
        if ((int)lines.size() <= ++index) index = 0;
        last = frame_time();
        if (once) stop();
    }
}

bool Inactivity::isPlaying() {                       // 0x4c4aa0
    if (current.empty()) return false;
    return vo_isPlaying(current.c_str());
}

// ---------------------------------------------------------------- PauseOver
PauseOver::PauseOver() : Act(nullptr) {              // 0x4c58e0
    mouse.setClient(this);
}

void PauseOver::set(Graphic *o, const char *l, uint32_t d) {   // 0x4c5970
    owner = o;
    line = l ? l : "";
    delay = d;
}

void PauseOver::start() {                            // 0x4c59c0
    reset();
    activate();
    mouse.activate();
    over_since = 0;
    played = 0;
}

void PauseOver::stop() {                             // 0x4c59f0
    if (vo_isPlaying(line.c_str())) VoiceOver::get().stop();
    mouse.deactivate();
    if (active()) deactivate();
}

void PauseOver::cancel() {                           // 0x4c5be0
    if (vo_isPlaying(line.c_str())) VoiceOver::get().stop();
    played++;
}

// 0x4c5a60: every 100 ms. Over the owner with the cursor showing: time it; past `delay` play the line, once per
// visit. Off the owner (or no cursor): start again. While the prompts are suppressed the timer only runs on while
// this line is playing.
void PauseOver::run() {
    wait(1);
    for (;;) {
        wait(100);
        bool over = owner && owner->hit(g_mouse.x, g_mouse.y);
        if (over && g_cursor_graphic && g_cursor_graphic->visible) {
            if (over_since == 0) {
                over_since = frame_time();
            } else if (!g_vo_suppressed() || vo_isPlaying(line.c_str())) {
                if (played == 0 && delay < frame_time() - over_since) {
                    vo_play(line.c_str());
                    played++;
                }
            } else if (played == 0) {
                over_since = 0;
            }
        } else {
            over_since = 0;
            played = 0;
        }
    }
}

// ---------------------------------------------------------------- RepeatLine, PromptLine, ComeNear
RepeatLine::RepeatLine(const char *l, uint32_t m) : Act(nullptr), ms(m), line(l ? l : "") {}   // 0x4c4af0

void RepeatLine::start() {                          // 0x4c4b80
    activate();                                      // +0x18
    next = frame_time() + ms;
}

// 0x4c4ba0: every 100 ms; past the time with no voice-over busy, the line, and the time again.
void RepeatLine::run() {
    wait(1);
    for (;;) {
        wait(100);
        if (frame_time() <= next) continue;
        if (g_vo_suppressed()) continue;             // +0x560
        VoiceOver::get().play(line.c_str());         // 0x4c9400
        next = frame_time() + ms;
    }
}

PromptLine::PromptLine(const char *l, uint32_t m) : Act(nullptr), ms(m), line(l ? l : "") {   // 0x4c4c70
    mouse.setClient(this);                           // 0x41f290
}

void PromptLine::start() {                          // 0x4c4d20
    mouse.activate();
    activate();
    next = frame_time() + ms;
}

void PromptLine::stop() {                           // 0x4c4d50
    mouse.deactivate();
    deactivate();
}

void PromptLine::run() {                            // 0x4c4d70
    wait(1);
    for (;;) {
        wait(100);
        if (frame_time() <= next) continue;
        if (g_vo_suppressed()) continue;
        VoiceOver::get().play(line.c_str());
        next = frame_time() + ms;
    }
}

void PromptLine::notify2(Service *) { next = frame_time() + ms; }   // 0x4c4e40

bool PromptLine::isPlaying() { return VoiceOver::get().isPlaying(line.c_str()); }   // 0x4c4e60

ComeNear::ComeNear(Raytrace::RayTrace *r, const char *l1, const char *l2) : Act(nullptr), rt(r) {   // 0x4c1360
    lines.add(l1);                                   // 0x4cc0d0
    lines.add(l2);
}

// 0x4c1410: while the camera is farther than z 4184, the next line every 10 s; then it stops.
void ComeNear::run() {
    wait(1);
    next = frame_time() + 10000;
    while (rt->camz > 4184.0f) {                     // 0x42b540, 0x517070
        wait(0);
        if (frame_time() > next) {
            lines.playNext();                        // 0x4cc0e0
            next = frame_time() + 10000;
        }
    }
    deactivate();                                    // +0x1c
    finish();                                        // 0x420fe0
}

}  // namespace Game
