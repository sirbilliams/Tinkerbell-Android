#include "voacts.h"
#include "state.h"

namespace Vision {

Graphic *g_cursor_graphic = nullptr;               // 0x5a69c4
void set_cursor(Graphic *g, bool visible) {         // 0x4119b0
    g_cursor_graphic = g;
    if (g) g->visible = visible;
}
void show_cursor() { if (g_cursor_graphic) g_cursor_graphic->visible = true; }    // 0x4119d0 (port: null-safe)
void hide_cursor() { if (g_cursor_graphic) g_cursor_graphic->visible = false; }   // 0x4119e0
static bool never() { return false; }
bool (*g_vo_suppressed)() = never;

// ---------------------------------------------------------------- VoList
void VoList::playNext() {                          // 0x4cc0e0
    if (lines.empty()) return;                     // port: the exe indexes blindly
    vo_play(lines[cursor % lines.size()].c_str());
    cursor++;
    if ((int)lines.size() <= cursor) cursor = 0;
}
bool VoList::anyPlaying() const {                  // 0x4cc130
    for (auto &l : lines) if (vo_isPlaying(l.c_str())) return true;
    return false;
}

// ---------------------------------------------------------------- PlayVos
void PlayVos::run() {                              // 0x4c5c80
    wait(1);
    for (;;) {
        wait(0);
        if (next < frame_time() && !g_vo_suppressed()) {
            list->playNext();
            next = frame_time() + period;
        }
    }
}

// ---------------------------------------------------------------- DelayedVO
DelayedVO::DelayedVO(const char *l, uint32_t d) : Act(nullptr), delay(d), line(l ? l : "") {   // 0x4c1530
    mouse.setClient(this);
    kbd.setClient(this);
}
void DelayedVO::start() {                          // 0x4c15f0
    last = frame_time();
    mouse.activate();
    kbd.activate();
    reset();
    activate();
}
void DelayedVO::stop() {                           // 0x4c1630
    if (active()) {
        mouse.deactivate();
        kbd.deactivate();
        deactivate();
    }
}
void DelayedVO::notify2(Service *s) {              // 0x4c1670
    if (s == &mouse && g_mouse.ldown) last = frame_time();
    if (s == &kbd && g_key.down) last = frame_time();
}
void DelayedVO::run() {                            // 0x4c16c0
    wait(1);
    for (;;) {
        wait(0);
        if (delay < frame_time() - last) {
            vo_play(line.c_str());
            while (vo_isPlaying(line.c_str())) wait(100);
            stop();                                // v13; the task is not scheduled again until start()
        }
    }
}

// ---------------------------------------------------------------- PreSceneVo
// 0x4c5d60. port: the exe holds the input with the game state's service at +0x480 (one nothing listens through);
// a service of the port's own does the same.
void PreSceneVo::start(const char *l, const char *s) {
    line = l ? l : "";
    scene = s ? s : "";
    static Task hold(nullptr);
    g_modal_client = &hold;                        // 0x41f370
    activate();
}

void PreSceneVo::run() {                           // 0x4c5dc0
    wait(1);
    if (g_cursor_graphic) g_cursor_graphic->visible = false;     // 0x4119e0
    vo_play(line.c_str());
    while (vo_isPlaying(line.c_str())) wait(0);
    g_modal_client = nullptr;                      // 0x41f380
    change_scene(scene.c_str());                   // 0x41ddf0
    deactivate();
    finish();                                      // 0x420fe0
}

// ---------------------------------------------------------------- VoOneAfterAnother
void VoOneAfterAnother::run() {                    // 0x4cbd90
    wait(1);
    index = 0;
    if (lines.empty()) vision_abort("VoOneAfterAnother: no lines");   // 0x443180: vector index out of range
    for (;;) {
        vo_play(lines[index].c_str());
        do {
            wait(100);
            if (lines.size() <= index) vision_abort("VoOneAfterAnother: bad index");
        } while (vo_isPlaying(lines[index].c_str()));
        index++;
        if (lines.size() <= index) break;
    }
    deactivate();
    if (!lines.empty() && lines.size() <= index) index--;
    if (vo_isPlaying(lines[index].c_str())) VoiceOver::get().stop();
    finish();
}

void VoOneAfterAnother::stop() {                    // 0x4cbcb0
    deactivate();                                    // +0x1c
    if (!lines.empty() && lines.size() <= index) index--;
    if (lines.size() <= index) return;               // port: the exe aborts (0x443180) on an empty list
    if (vo_isPlaying(lines[index].c_str())) VoiceOver::get().stop();   // 0x4c9260
}

// ---------------------------------------------------------------- QuestionRepeater
QuestionRepeater::QuestionRepeater() : Task(nullptr) { mouse.setClient(this); }   // 0x4c5f00
void QuestionRepeater::start(const char *l) {      // 0x4c5fa0
    line = l ? l : "";
    next = frame_time() + 15000;
    if (!active()) {
        mouse.activate();
        activate();
    }
}
void QuestionRepeater::stop() {                    // 0x4c60f0
    mouse.deactivate();
    deactivate();
}
void QuestionRepeater::notify2(Service *s) {       // 0x4c60c0
    if (s == &mouse && g_mouse.ldown) next = frame_time() + 15000;
}
void QuestionRepeater::run() {                     // 0x4c6000
    wait(1);
    for (;;) {
        if (next < frame_time() && !g_vo_suppressed()) {
            vo_play(line.c_str());
            next = frame_time() + 15000;
        }
        wait(0);
    }
}

// ---------------------------------------------------------------- Reminder
Reminder::Reminder(const char *image, const char *l)   // 0x4c6140
    : Task(nullptr), img(image, 0, 0, 0, 0, 0, 0, true), line(l ? l : ""), screen(0, 0, 0, 0, 0, true, true) {
    screen.layer(0)->add(&img);
}
void Reminder::start() {                           // 0x4c6220
    screen.loadAll();
    screen.activate();
    activate();
    g_modal_client = this;                         // 0x41f370 (port: ecx at the call is not visible; the Reminder itself)
}
void Reminder::stop() {                            // 0x4c6260
    screen.unloadAll();
    screen.deactivate();
    deactivate();
}
void Reminder::run() {                             // 0x4c6290
    wait(1);
    vo_play(line.c_str());
    while (vo_isPlaying(line.c_str())) wait(0);
    g_modal_client = nullptr;                      // 0x41f380
    screen.unloadAll();                            // 0x41ec00
    screen.deactivate();
    deactivate();
    finish();
}

// 0x4c3a60: by the result (1..5): the challenge won, the award, the challenge lost (3 and 4), the second tie.
void GameEndVo::playEnd() {
    cur = nullptr;
    switch (Game::state().last_result) {             // +0x7a0, the table 0x4c3ac0
    case 1: cur = &winch; break;
    case 2: cur = &award; break;
    case 3:
    case 4: cur = &losech; break;
    case 5: cur = &tie2; break;
    default: break;
    }
    if (cur) cur->playNext();                        // 0x4cc0e0 (port: the exe calls it on null for another result)
}

}  // namespace Vision
