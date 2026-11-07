// PeterGuide, reconstructed from "Tinker Bell.exe" (see peterguide.h).
#include "peterguide.h"
#include "state.h"

namespace Game {

using namespace Vision;

PeterGuide::PeterGuide(const char *f, const char *a)   // 0x4ce910
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), Task(nullptr),
      enter_a("GLOB_CA_PETenter_"), idle("GLOB_CA_voidle_"), exit_a("GLOB_CA_PETexit_"), mouth_a("PEV_mout"),
      first(f), again(a), eyes("PEV_eyes01", 0, 0, 0, 0, 0, 0, true) {}

PeterGuide::~PeterGuide() {}                         // 0x4cea70

// 0x4ceb60: the pack's animations and eyes (the previous pack made current again), all hidden; z -1005; both
// lines are his.
void PeterGuide::load(bool r) {
    pack = ras_open("PET_Top_Down", nullptr, &data_device());   // 0x419b90
    pack->load();                                    // 0x41aa20
    Ras *prev = ras_set_current(pack);               // 0x41a920
    enter_a.load();                                  // v15
    idle.load();
    exit_a.load();
    mouth_a.load();
    eyes.load();
    if (prev) ras_set_current(prev);
    enter_a.visible = false;
    idle.visible = false;
    exit_a.visible = false;
    mouth_a.visible = false;
    setZ(-1005);                                     // 0xfffffc13
    replay = r;
    VoiceOver &vo = VoiceOver::get();
    vo.registerLine(first, this);                    // 0x4c91b0
    vo.registerLine(again, this);
    Game::state().guide_loaded = true;               // +0x7b8
}

void PeterGuide::unload() {                          // v16 0x4cec80
    if (!pack) return;
    Graphic::unload();                               // 0x412020
    enter_a.unload();
    idle.unload();
    exit_a.unload();
    mouth_a.unload();
    eyes.unload();
    detach();                                        // 0x412200
    if (pack) ras_release(pack);                     // 0x419620
    pack = nullptr;
}

// v14 0x4ced00: the shown animations; while idle the eyes close for a frame every 2-5 s (that frame without the
// mouth), the mouth over it.
void PeterGuide::draw() {
    if (enter_a.visible) enter_a.draw();
    if (exit_a.visible) exit_a.draw();
    if (idle.visible) {
        idle.draw();
        if (blinked) {
            blinked = false;
            int r = vision_random(3000);              // 0x425d40 (state +8)
            blink_at = r + frame_time() + 2000;
            return;
        }
        if (blink_at == 0) {
            int r = vision_random(3000);
            blink_at = r + frame_time();
        }
        if (frame_time() > blink_at) {
            eyes.draw();
            blinked = true;
        }
    }
    if (mouth_a.visible) mouth_a.draw();
}

// v10 0x4cee00
void PeterGuide::run() {
    if (suspended) wait(0);                          // +0x3c (the yield idiom)
    Graphic::visible = true;                         // +0x7c
    idle.visible = false;
    exit_a.visible = false;
    enter_a.visible = true;
    enter_a.play(0, 0);                              // 0x403be0
    enter_a.y = 500;                                 // +0x1bc
    do {
        enter_a.y -= 20;
        wait(30);
    } while (enter_a.y > 0);
    enter_a.visible = false;
    idle.play(0, 0);
    idle.visible = true;
    blink_at = 0;
    blinked = false;
    VoiceOver &vo = VoiceOver::get();
    State &st = Game::state();
    if (replay) vo.play(again);                      // 0x4c9400
    else if (st.firsts->gameFirst()) vo.play(first);   // 0x4c6970
    else vo.play(again);
    while (st.vo_busy()) wait(30);                   // +0x560
    idle.visible = false;
    exit_a.play(0, 0);
    exit_a.visible = true;
    while (exit_a.x < 600) {                         // +0x3e8
        exit_a.step();                               // 0x403d60
        if (exit_a.current() >= 10) exit_a.x += 20;  // 0x403f00
        wait(30);
    }
    if (active()) {
        if (!replay) st.firsts->gameDone();          // 0x4c6af0
        vo.stop();                                   // 0x4c9260
        deactivate();                                // +0x1c
        Graphic::visible = false;
    }
    finish();                                        // 0x420fe0
}

void PeterGuide::stop() {                            // 0x4cf060
    if (!active()) return;
    if (!replay) Game::state().firsts->gameDone();
    VoiceOver::get().stop();
    deactivate();
    Graphic::visible = false;
}

void PeterGuide::mouth(int letter) {                 // 0x4cf0c0
    mouth_a.visible = true;
    mouth_a.setFrame(letter);
}

}  // namespace Game
