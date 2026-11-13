// The Shell Game, reconstructed from "Tinker Bell.exe" (see shellgame.h).
#include "shellgame.h"
#include "mermaid.h"
#include "state.h"
#include "tink3d.h"
#include "voacts.h"
#include "voiceover.h"
#include "cannon.h"
#include "shuffle.h"
#include "tb.h"
#include <stdlib.h>
#include <string>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static Animation *anim_of(Attachment *a) { return static_cast<Animation *>(a->g); }
static bool cur_vis() { return g_cursor_graphic ? g_cursor_graphic->visible : false; }

static const char *const kGame = "Shell Game";       // *0x543510
static const int kNone = 9999999;                    // 0x98967f
// 0x53950c: the shells on the log; 0x539520 follows (the cursors), read for a sixth shell (the exe's)
static const char *const kShells[6] = {"SHEL_GA_puzshell1_", "SHEL_GA_puzshell2_", "SHEL_GA_puzshell3_",
                                       "SHEL_GA_puzshell4_", "SHEL_GA_puzshell5_", "SHEL_CU_puzshell1_"};
static const char *const kCursors[5] = {"SHEL_CU_puzshell1_", "SHEL_CU_puzshell2_", "SHEL_CU_puzshell3_",
                                        "SHEL_CU_puzshell4_", "SHEL_CU_puzshell5_"};   // 0x539520
static const char *const kEmpty = "SHELL_GA_target1_";   // *0x5394e4: a hole with no shell

// 0x4564d0: a number below n not in `used`, drawn until one is free. After 10000 draws the free ones are worked
// out, shuffled and one drawn, and that is dropped (the loop goes on drawing): the exe's; the draws are kept.
static int pick(int n, std::vector<int> &used) {
    for (int tries = 0;; tries++) {
        int r = vision_random(n);                    // 0x425d10, 0x425d40
        int c = 0;
        for (size_t i = 0; i < used.size(); i++)
            if (r == used[i]) c++;
        if (c == 0) return r;
        if (tries > 9999) {
            std::vector<int> counts(n, 0);
            for (size_t i = 0; i < used.size(); i++) counts[used.at(i)]++;
            std::vector<int> freev;
            for (int i = 0; i < n; i++)
                if (counts[i] == 0) freev.push_back(i);
            int s = vision_random(6);
            for (int j = 0; j < s; j++)
                if (!freev.empty()) ms_shuffle(freev);   // 0x457be0
            if (!freev.empty()) {
                int sz = (int)freev.size();
                int a = vision_random(sz);
                int b = vision_random(sz + 1);
                (void)freev.at((a + b) % sz);        // 0x457b80
            }
        }
    }
}

// 0x456750. port: the RayTrace is given (the exe writes the static scene's address); the cursors are made with
// their names (the exe names its embedded ones after making them).
ShellGame::ShellGame(Raytrace::RayTrace *r)
    : Task(nullptr), rt(r), grab("shel_fx_grabshell"), drop("SHEL_FX_shelldrop"), nice_vos(&nice, 12000) {   // *0x539554
    for (int i = 0; i < 5; i++) cursors[i] = new Animation(kCursors[i]);   // 0x452d70 + 0x424400
    mouse.setClient(this);                           // 0x41f290
    keys.setClient(this);
    inactivity.add("SHEL_VO_SKNprompt", false);      // 0x4c46e0
    wrong.add("SHEL_FX_notright");                   // 0x4cc0d0
    wrong.add("SHEL_FX_notright");
    wrong.add("SHEL_FX_mixup");
    nice.add("SHEL_VO_SKNnice1");
    nice.add("SHEL_VO_SKNnice2");
    nice.add("SHEL_VO_SKNnice3");
    nice.add("SHEL_VO_SKNnice4");
}

// (inlined) the hole shows that shell; the picture kept where it was.
void ShellGame::putLeft(Attachment *a, int shell) {
    int x = a->g->x, y = a->g->y;                    // +0x58 +0x5c
    a->setPrefix(kShells[shell]);                    // 0x42e4f0
    a->g->x = x;
    a->g->y = y;
}

// port: a hole showing no shell would give the exe's graphic past the five cursors; none here
Graphic *ShellGame::cursorOf(int shell) { return shell < 5 ? cursors[shell] : nullptr; }

int ShellGame::heldShell() {                         // (inlined)
    int k = 0;
    for (; k < 5; k++)
        if (g_cursor_graphic == cursors[k]) break;
    return k;
}

// 0x4569a0 (enter): the button and the ten holes on the log (all off), the cursors; the game won with the shells:
// they lie in the left row.
void ShellGame::setup() {
    log = rt->findSprite("AA_shel_ga_woodlog_01");   // 0x42dc70
    button = log->attach("shel_ga_button_", "shel_ga_button_", -1);   // 0x42f390 (*0x5394b8)
    static const char *const lt[5] = {"LT0", "LT1", "LT2", "LT3", "LT4"};   // 0x5394d0
    static const char *const ltp[5] = {"SHELL_GA_target1L_", "SHELL_GA_target2L_", "SHELL_GA_target3L_",
                                       "SHELL_GA_target4L_", "SHELL_GA_target5L_"};   // 0x5394bc
    static const char *const pt[5] = {"PT0", "PT1", "PT2", "PT3", "PT4"};   // 0x5394f8
    static const char *const ptp[5] = {"SHELL_GA_target1_", "SHELL_GA_target2_", "SHELL_GA_target3_",
                                       "SHELL_GA_target4_", "SHELL_GA_target5_"};   // 0x5394e4
    for (int i = 0; i < 5; i++) {
        cursors[i]->load();                          // v15
        cursors[i]->play(0, 1);                      // 0x403be0
        left[i] = log->attach(lt[i], ltp[i], -1);
        right[i] = log->attach(pt[i], ptp[i], -1);
        right[i]->on = false;                        // +0x17c
        left[i]->on = false;
    }
    if (!Game::state().anyMermaid(5)) return;        // 0x4ef6b0
    for (int i = 0; i < 5; i++) {
        putLeft(left[i], i);
        left[i]->on = true;
        left[i]->period = -1;                        // +0x184
    }
}

// 0x456ad0: the five shells in the left row, the order wanted (each shell once), the fuse by skill after a win
// (none before), the prompt, the sounds, the skunk's nice lines every 12 s.
void ShellGame::start() {
    State &st = Game::state();
    for (int &h : hinted) h = kNone;                 // +0x1228
    for (int i = 0; i < 5; i++) {
        putLeft(left[i], i);
        left[i]->on = true;
        left[i]->period = -1;
    }
    mouse.activate();                                // v6
    keys.activate();
    order.clear();                                   // 0x443690
    for (int i = 0; i < 5; i++) order.push_back(pick(5, order));   // 0x4564d0, 0x430dd0
    if (getenv("TB_DEBUG_MERMAID"))                  // test aid
        tb_log("mermaid: shell order %d %d %d %d %d", order[0], order[1], order[2], order[3], order[4]);
    button_on = false;                               // +0x6e0
    reset();
    activate();
    switch (st.saves.gameInfo(kGame)) {              // 0x4e9d70
    case 0: info = 0; break;
    case 1: case 2: info = 1; break;
    default: break;
    }
    st.last_game = 4;                                // +0x79c
    if (info == 1) {
        uint32_t ms = 0;
        switch (st.skill) {                          // +0x68
        case 1: ms = 90000; break;
        case 2: ms = 60000; break;
        case 3: ms = 30000; break;
        default: break;                              // port: the exe leaves it unset (no such skill)
        }
        Game::cannon().visible = true;               // state +0x83c
        Game::cannon().start((int)(ms / 1000));      // 0x4e3a50
    }
    st.last_game = 4;
    inactivity.start();                              // +0x30
    grab.open();                                     // 0x41f9d0
    drop.open();
    f1224 = 0;
    wrong.cursor = 0;                                // +0x126c
    nice_vos.start();                                // 0x4c5c60
    if (getenv("TB_SHELL_AUTO"))                     // test aid (not in the game): every shell in its hole
        for (int i = 0; i < 5; i++) {
            putLeft(right[i], order[i]);
            right[i]->on = true;
            left[order[i]]->on = false;
        }
}

// 0x456cf0: the shells back in the left row, the right one empty, the walking back; not skipped: the skunk's end
// (else the area interactive again).
void ShellGame::stop() {
    if (!active()) return;
    Mermaid_Scene &sc = mermaid_scene();
    State &st = Game::state();
    sc.tink->wanders = true;                         // 0x5bca64 +0x13c
    if (info == 1) Game::cannon().hide();            // 0x4e3bf0
    for (int i = 0; i < 5; i++) {
        putLeft(left[i], i);
        left[i]->on = true;
        left[i]->period = -1;
    }
    for (Attachment *a : right) a->on = false;
    anim_of(button)->setFrame(0);                    // 0x403f20
    mouse.deactivate();
    keys.deactivate();
    deactivate();
    sc.input->activate();                            // 0x5ba44c (the scene's +0x12c) v6
    inactivity.stop();                               // +0x34
    grab.close();                                    // 0x41fc50
    drop.close();
    if (st.last_game != 5) {
        sc.shell_end.start();                        // 0x4c19e0 (0x5c1c0c)
        nice_vos.deactivate();                       // 0x4c1400
        return;
    }
    sc.inactivity.start();                           // 0x4c7730 (0x5c255c)
    sc.hotsOn(true);                                 // 0x452af0
    nice_vos.deactivate();
}

// v10 0x456e70: the camera to the log (the area's spot 2, in a second), the walking off; then the button's blink
// while all holes are filled; after a win before, the fuse's end ends it.
void ShellGame::run() {
    wait(1);
    State &st = Game::state();
    float sp[4];
    st.cameraSpot(5, 2, sp);                         // 0x4ec180
    mover.start(rt, sp[0], sp[1], sp[2], sp[3], 1000.0);   // 0x4df3d0 (0x408f4000)
    mermaid_scene().input->deactivate();             // v7
    while (mover.step()) wait(0);                    // 0x4df4d0
    for (;;) {
        wait(10);
        if (button_on) anim_of(button)->step();      // 0x403d60
        if (info == 1 && !Game::cannon().running()) break;   // 0x4e3b80
    }
    st.last_result = 0;                              // +0x7a0
    stop();
    finish();
}

// 0x457910: the shell an attachment shows (its animation's name begins with the shell's), 5 for none.
int ShellGame::shellOf(Attachment *a) {
    std::string n = a->g->name();                    // 0x4243f0
    int i = 0;
    for (; i < 5; i++)
        if (n.find(kShells[i]) == 0) break;          // 0x423cf0
    return i;
}

// 0x4579d0: every right hole holds its shell: the sign's star (and the map segment) or cap by the wins before,
// and the result; else after a win before, the result 0. The hints start over.
bool ShellGame::check() {
    State &st = Game::state();
    int i = 0;
    for (; i < 5; i++)
        if (order.at(i) != shellOf(right[i])) break;
    if (i == 5) {
        switch (st.saves.gameInfo(kGame)) {
        case 0:
            mermaid_scene().shell_sign->showAttached("shellGameStar");   // 0x5bcc18, 0x42f690
            st.last_result = 2;
            st.saves.setMapSegment("Mermaid");       // 0x4ea830
            break;
        case 1:
            mermaid_scene().shell_sign->showAttached("shellGameCap");
            st.last_result = 3;
            break;
        case 2: st.last_result = 4; break;
        default: break;
        }
    } else if (info == 1) {
        st.last_result = 0;
    }
    for (int &h : hinted) h = kNone;
    if (getenv("TB_DEBUG_MERMAID")) tb_log("mermaid: shell check: %d right", i);   // test aid
    return i == 5;
}

// 0x457b00: a right hole holding a wrong shell (not "none"), the least hinted, its count up.
bool ShellGame::hint(Attachment **out) {
    int i = 0;
    for (; i < 5; i++)
        if (hinted[i] != kNone) break;
    if (i == 5) return false;
    int m = 0;
    for (int k = 1; k < 5; k++)
        if (hinted[k] < hinted[m]) m = k;
    hinted[m]++;
    *out = right[m];
    return true;
}

// v9 0x456fe0: the skip key ends it unplayed. The mouse (not over a button, no dust): the hole under it steps
// its picture; all right holes filled: the button blinks. A click: a shell taken from a hole, swapped with the one
// in hand, or put into an empty hole (a right hole remembers whether it holds its shell); a shell in hand clicked
// elsewhere goes back to the first empty left hole (the skunk's help line); with all five in, the button checks:
// right, the end; wrong, the skunk says so and the wrong shells go back left.
void ShellGame::notify2(Service *s) {
    State &st = Game::state();
    if (s == &keys) {
        if (g_key.down && (int)g_key.vk == st.skip_key) {   // 0x5a6dd8, 0x5a6d9c, +0x74
            st.last_game = 5;
            stop();
        }
        return;
    }
    if (s != &mouse) return;
    if (mermaid_scene().over_ui) return;             // 0x5bcb50 (+0x2830)
    if (st.dusting) return;                          // +0x3c4
    Attachment *h = rt->hitAttached(log, g_mouse.x, g_mouse.y, false);   // 0x42ca00
    for (int i = 0; i < 5; i++) {
        left[i]->period = -1;
        right[i]->period = -1;
        if (h == left[i] || h == right[i]) {
            anim_of(h)->mode = 1;                    // +0x104
            h->period = 0;
        }
    }
    int n = 0;
    while (n < 5 && right[n]->on) n++;
    if (n == 5) button_on = true;
    else {
        anim_of(button)->setFrame(0);
        button_on = false;
    }
    if (!mouse_down()) return;
    for (int i = 0; i < 5; i++) {
        Attachment *A = left[i];
        if (A->on) {
            if (rt->pickAttached(g_mouse.x, g_mouse.y, false) == A && g_cursor_graphic == st.cursor) {   // 0x42c950
                grab.play();                         // 0x41fd50
                set_cursor(cursorOf(shellOf(A)), cur_vis());   // 0x4119b0
                A->on = false;
                int x = A->g->x, y = A->g->y;
                A->setPrefix(kEmpty);
                A->g->x = x;
                A->g->y = y;
                A->on = false;
                return;
            }
            if (A->on && rt->pickAttached(g_mouse.x, g_mouse.y, false) == A && g_cursor_graphic != st.cursor) {
                grab.play();
                const char *held = kShells[heldShell()];
                set_cursor(cursorOf(shellOf(A)), cur_vis());
                int x = A->g->x, y = A->g->y;
                A->setPrefix(held);
                A->g->x = x;
                A->g->y = y;
                return;
            }
        }
        if (!A->on && rt->pickAttached(g_mouse.x, g_mouse.y, true) == A && g_cursor_graphic != st.cursor) {
            putLeft(A, heldShell());
            A->on = true;
            set_cursor(st.cursor, cur_vis());
            return;
        }
        Attachment *B = right[i];
        if (B->on) {
            if (rt->pickAttached(g_mouse.x, g_mouse.y, false) == B && g_cursor_graphic == st.cursor) {
                grab.play();
                set_cursor(cursorOf(shellOf(B)), cur_vis());
                B->on = false;
                int x = B->g->x, y = B->g->y;
                B->setPrefix(kEmpty);
                B->g->x = x;
                B->g->y = y;
                B->on = false;
                hinted[i] = kNone;
                return;
            }
            if (B->on && rt->pickAttached(g_mouse.x, g_mouse.y, false) == B && g_cursor_graphic != st.cursor) {
                grab.play();
                const char *held = kShells[heldShell()];
                set_cursor(cursorOf(shellOf(B)), cur_vis());
                int x = B->g->x, y = B->g->y;
                B->setPrefix(held);
                B->g->x = x;
                B->g->y = y;
                hinted[i] = order.at(i) == shellOf(B) ? kNone : 0;
                return;
            }
        }
        if (!B->on && rt->pickAttached(g_mouse.x, g_mouse.y, true) == B && g_cursor_graphic != st.cursor) {
            putLeft(B, heldShell());
            B->on = true;
            set_cursor(st.cursor, cur_vis());
            hinted[i] = order.at(i) == shellOf(B) ? kNone : 0;
            return;
        }
    }
    if (g_cursor_graphic != st.cursor) {
        for (int k = 0; k < 5; k++) {
            if (left[k]->on) continue;
            putLeft(left[k], heldShell());
            set_cursor(st.cursor, cur_vis());
            left[k]->on = true;
            drop.play();
            vo_play("SHEL_VO_SKNshhelp");            // 0x4c9400
            return;
        }
    }
    n = 0;
    while (n < 5 && right[n]->on) n++;
    if (n != 5 || rt->pickAttached(g_mouse.x, g_mouse.y, false) != button) return;
    if (check()) {
        stop();
        return;
    }
    wrong.playNext();                                // 0x4cc0e0
    for (int i = 0; i < 5; i++) {
        if (order.at(i) == shellOf(right[i])) continue;
        right[i]->on = false;
        std::string name = right[i]->g->name();
        int x = left[i]->g->x, y = left[i]->g->y;
        left[i]->setPrefix(name.c_str());
        left[i]->g->x = x;
        left[i]->g->y = y;
        left[i]->on = true;
    }
}

}  // namespace Game
