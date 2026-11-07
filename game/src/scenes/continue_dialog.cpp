// ContinueDialog, reconstructed from "Tinker Bell.exe" (see continue_dialog.h).
#include "continue_dialog.h"
#include "state.h"
#include "voacts.h"
#include "voiceover.h"

namespace Game {

using namespace Vision;

// 0x4e5330: the three images in the dialog's own screen (all shown), the click sound.
ContinueDialog::ContinueDialog(const std::string &s, const std::string &sg, const std::string &xg)
    : Task(nullptr), screen(0, 0, 0, 0, 0, true, true),   // 0x41eea0
      sign(s.c_str(), 0, 0, 0, 0, 0, 0, true),       // 0x4179e0
      sign_glow(sg.c_str(), 0, 0, 0, 0, 0, 0, true),
      x_glow(xg.c_str(), 0, 0, 0, 0, 0, 0, true),
      click("GLOB_FX_GSclick") {                     // 0x420210
    mouse.setClient(this);                           // 0x41f290
    screen.layer(0)->add(&sign);                     // 0x41ece0, 0x41e930
    screen.layer(0)->add(&sign_glow);
    screen.layer(0)->add(&x_glow);
}

ContinueDialog::~ContinueDialog() {}                 // 0x4e5590

// v6 0x4e5c20: the screen up, both glows off, the game's cursor.
void ContinueDialog::activate() {
    if (active()) return;
    screen.activate();                               // v6
    screen.loadAll();                                // 0x41ec40
    sign_glow.visible = false;                       // +0x294
    x_glow.visible = false;                          // +0x33c
    click.open();                                    // 0x41f9d0(0)
    no = false;
    yes = false;
    set_cursor(Game::state().cursor, true);          // 0x4119b0 (state +0x5c)
    Task::activate();                                // 0x420e70
}

// v7 0x4e5c90
void ContinueDialog::deactivate() {
    if (!active()) return;
    Task::deactivate();                              // 0x420eb0
    set_cursor(nullptr, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0
    click.close();                                   // 0x41fc50
    screen.unloadAll();                              // 0x41ec00
    screen.deactivate();                             // v7
}

// v9 0x4e5ce0: the glow under the mouse; a click picks.
void ContinueDialog::notify2(Service *s) {
    if (s != &mouse) return;
    sign_glow.visible = false;
    x_glow.visible = false;
    if (sign_glow.hit(g_mouse.x, g_mouse.y)) {       // v11
        sign_glow.visible = true;
        if (g_mouse.ldown || g_mouse.rdown) no = true;   // 0x5a6e30
    }
    if (x_glow.hit(g_mouse.x, g_mouse.y)) {
        x_glow.visible = true;
        if (g_mouse.ldown || g_mouse.rdown) yes = true;
    }
}

static uint32_t g_idle_said;                         // 0x631618 (set once: a function static, 0x631614 bit 0)
static bool g_idle_init = false;

// v10 0x4e5d70: Peter asks; once he is done the mouse works; the idle line every 10 s; on a pick, the click, his
// answer, finished when he is done.
void ContinueDialog::run() {
    wait(1);
    VoiceOver &vo = VoiceOver::get();
    vo.play(ask.c_str());                            // 0x4c9400
    while (Game::state().vo_busy()) wait(0);         // +0x560
    mouse.activate();                                // v6
    if (!g_idle_init) {
        g_idle_init = true;
        g_idle_said = frame_time();
    }
    while (!no && !yes) {
        if (idle != "" && frame_time() > g_idle_said + 10000) {   // 0x5464a0, 0x2710
            vo.play(idle.c_str());
            g_idle_said = frame_time();
        }
        wait(0);
    }
    mouse.deactivate();                              // v7
    vo.stop();                                       // 0x4c9260
    click.play();                                    // 0x41fd50
    while (click.isPlaying()) wait(0);               // 0x41fec0
    if (yes && yes_line != "") vo.play(yes_line.c_str());
    else if (no && no_line != "") vo.play(no_line.c_str());
    while (Game::state().vo_busy()) wait(0);
    finish();                                        // 0x420fe0
}

}  // namespace Game
