// Treasure_Scene, reconstructed from "Tinker Bell.exe" (see treasure.h).
#include "treasure.h"
#include "state.h"
#include "tink3d.h"
#include "shuffle.h"
#include "toolbar.h"
#include "tb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30
static bool cur_vis() { return g_cursor_graphic ? g_cursor_graphic->visible : false; }   // (each set_cursor's)

// The globals 0x53e4c0..0x53e4dc.
static const char *const kTileImage = "WAMI_GA_tilep_";   // *0x53e4c0
static const char *const kSlotImage = "WAMI_BG_tile";     // *0x53e4c4
static const char *const kDoor = "AA_WAMI_BG_dooropen_01";   // *0x53e4c8
static const char *const kClue = "WAMI_VO_PETclue";       // *0x53e4cc
static const uint32_t kClueEvery = 10000;            // 0x53e4d8
static const uint32_t kFirstClue = 1000;             // 0x53e4dc

Treasure_Scene &treasure_scene() {                   // the static at 0x5f6d50
    static Treasure_Scene s("treasure");             // 0x48f630
    return s;
}

static std::string answer_name(int k, int j) {       // 0x48f090
    char b[32];
    snprintf(b, sizeof b, "answer%i%2.2d", k, j);
    return b;
}
static std::string tile_name(int i) {                // 0x48f430: numbered from 1
    char b[32];
    snprintf(b, sizeof b, "%s%2.2d", kTileImage, i + 1);
    return b;
}
static std::string grid_name(int i) {                // 0x48f530
    char b[32];
    snprintf(b, sizeof b, "grid%2.2d", i);
    return b;
}
// 0x48f1a0: "grid<n>": n
static bool is_grid(const char *n, int *out) {
    if (strncmp(n, "grid", 4) != 0) return false;
    if (out) *out = atoi(n + 4);                     // 0x4f5951
    return true;
}
// 0x48f2c0: "answer<k><jj>": the slot (one digit) and the tile
static bool is_answer(const char *n, int *k, int *j) {
    if (strncmp(n, "answer", 6) != 0) return false;
    if (k) {
        char d[2] = {n[6], 0};
        *k = atoi(d);
    }
    if (j) *j = atoi(n + 7);
    return true;
}

Treasure_Scene::Treasure_Scene(const char *nm)      // 0x48f660
    : TinkerbellRaytraceScene(nm), tile_cursor("", 0, 0, 0, 0, 0, 0, true),   // 0x4179e0
      movie(&rt), movie2(&rt), movie3(&rt), prompt("WAMI_VO_PETprompt", 15000) {   // 0x4c4c70
    tada = new Sound("WAMI_FX_tada");                // 0x420210
    opmusic = new Sound("WAMI_MU_opmusic");
    music = new Sound("WAMI_MU_music");
    movie.chars(&movie2.peter, &movie2.wendy, &movie2.hook, &movie2.smee);   // 0x495100
    movie2.link();                                   // 0x495460
}

Treasure_Scene::~Treasure_Scene() {                  // 0x490630 (port: the exe's static is never destroyed)
    delete tada;
    delete opmusic;
    delete music;
}

// v18 0x490930: the film's lines, the wrong-answer lines (appended again on every call, as the exe), the music
// streamed and looping.
void Treasure_Scene::load2() {
    movie.load();                                    // 0x493710
    wrong.add("WAMI_VO_HOKtoobad");                  // 0x4cba80
    wrong.add("WAMI_VO_PETagain");
    music->mode = 2;                                 // +0x5b24
    music->type = 6;                                 // +0x5b2c
}

// 0x490e40: five of the sixteen tiles (a fresh order shuffled five times) and, by the skill, how many clues are
// the hard ones (2, 3, 4), in a shuffled order.
void Treasure_Scene::makePuzzle() {
    std::vector<int> a(16);
    for (int i = 0; i < 16; i++) a[i] = i;
    vision_random(16);                               // (unused)
    for (int n = 0; n < 5; n++) ms_shuffle(a);       // 0x4f54da
    for (int i = 0; i < 5; i++) solution[i] = a[i];
    for (bool &h : hard) h = false;
    State &st = Game::state();
    int skill = 0;                                   // port: the exe's local is not set when the key is missing
    st.saves.getSkill(&skill);                       // 0x4e97a0
    int n = skill == 1 ? 2 : skill == 2 ? 3 : skill == 3 ? 4 : skill;
    for (int i = 0; i < n && i < 5; i++) hard[i] = true;
    vision_random(16);                               // (unused)
    std::vector<char> h(hard, hard + 5);
    for (int r = 0; r < 5; r++) ms_shuffle(h);
    for (int i = 0; i < 5; i++) hard[i] = h[i] != 0;
    if (const char *f = getenv("TB_TREASURE_PUZZLE"))   // test aid: "a,b,c,d,e" the tiles wanted
        sscanf(f, "%d,%d,%d,%d,%d", &solution[0], &solution[1], &solution[2], &solution[3], &solution[4]);
    if (getenv("TB_DEBUG_TREASURE"))                 // test aid
        tb_log("treasure: puzzle %d%s %d%s %d%s %d%s %d%s", solution[0], hard[0] ? "H" : "", solution[1],
               hard[1] ? "H" : "", solution[2], hard[2] ? "H" : "", solution[3], hard[3] ? "H" : "", solution[4],
               hard[4] ? "H" : "");
}

// v19 0x490980
void Treasure_Scene::enter() {
    static const char *const pk[6] = {"peter_char", "WAMI_PET", "Wendy_char", "WAMI_WEN", "WAMI_HOK", "WAMI_SME"};
    for (int i = 0; i < 6; i++) {
        packs[i] = ras_open(pk[i], "", &data_device());   // 0x419b90
        packs[i]->load();                            // 0x41aa20
        ras_set_current(packs[i]);                   // 0x41a920
    }
    open(name());                                    // v27 (0x4ab0f0)
    reset();                                         // 0x421110
    activate();
    Sprite3D *door = rt.findSprite(kDoor);           // 0x42dc70
    for (int i = 0; i < 16; i++) door->attach(grid_name(i).c_str(), tile_name(i).c_str(), 100);   // 0x42f390
    for (int k = 0; k < 5; k++) {
        char c[2] = {(char)('a' + k), 0};
        for (int j = 0; j < 16; j++) {
            char p[64];
            snprintf(p, sizeof p, "%s%s_%2.2d", kSlotImage, c, j + 1);
            door->attach(answer_name(k, j).c_str(), p, 100);
        }
    }
    makePuzzle();
    for (int &s : slots) s = -1;                     // +0x2908..
    state = 0;
    clue_t = frame_time() + kFirstClue - kClueEvery;
    for (int i = 0; i < 16; i++) rt.findSprite(kDoor)->showAttached(grid_name(i).c_str());   // 0x42f690
    for (int k = 0; k < 5; k++) answerShow(k, 0, false);
    last_clue = 9999999;                             // +0x4e68
    VoiceOver::get().play("WAMI_VO_PETgame");        // 0x4c9400
    door->mode = 3;                                  // +0x88
    mouse.activate();                                // +0x1328
    movie.enter();                                   // 0x493e40
    tnk_pack = ras_open("WAMI_TNK", "", &data_device());
    tnk_pack->load();
    ras_set_current(tnk_pack);
    playing = false;                                 // +0x4f7c
    bag->visible = false;                            // +0x1998 +0x7c
    tada->open();                                    // 0x41f9d0
    opmusic->open();
    rt.findSprite("AA_tres_am_tree2_01")->z -= 50.0f;   // +0x80, 0x515190
    rt.findSprite("AA_tres_am_bush1_01")->z -= 50.0f;
    force[4] = force[5] = 1;                         // 0x4aa9c0: no rising or sinking
    music->open();
    music->play();                                   // 0x41fd50
    rt.setCamera(2654.52f, 0.1f, 4403.27f, 260.496f);   // 0x42a570 (0x4525e852 0x3dcccccd 0x45899a29 0x43823f7d)
}

// v20 0x490fe0
void Treasure_Scene::leave() {
    VoiceOver::get().stop();                         // 0x4c9260
    deactivate();                                    // v7
    State &st = Game::state();
    set_cursor(st.cursor, cur_vis());                // 0x4119b0
    tile_cursor.detach();                            // 0x412200
    tile_cursor.unload();                            // v16
    movie.skip();                                    // 0x493f50
    movie2.skip();                                   // 0x4953a0
    movie3.skip();                                   // 0x497600
    ras_close(tnk_pack);                             // 0x419620
    tnk_pack = nullptr;
    prompt.stop();                                   // +0x34
    TinkerbellRaytraceScene::leave();                // 0x4ab470
    set_cursor(st.cursor, true);
    wrong.stop();                                    // 0x4cbcb0
    tada->close();                                   // 0x41fc50
    opmusic->close();
    for (Ras *&p : packs) {
        ras_close(p);
        p = nullptr;
    }
    music->close();
}

// v10 0x491130: the film, then the puzzle until it is solved; the door opens (WamiMovie3), the ending (WamiMovie2).
void Treasure_Scene::run() {
    wait(1);
    State &st = Game::state();
    rt.setCamera(2654.52f, 0.1f, 4403.27f, 260.496f);
    st.setTinkHome(2795.0f, 600.0f, 2950.0f);        // +0x7ac (0x452eb000 0x44160000 0x45386000)
    tinkHome();                                      // 0x4abb10
    while (VoiceOver::get().isPlaying("WAMI_VO_PETgame")) wait(0);   // 0x4c9690
    while (movie.running) wait(0);                   // +0x31e8
    input->deactivate();                             // +0x12c
    st.setTinkHome(2700.0f, 500.0f, 2900.0f);        // 0x4528c000 0x43fa0000 0x45354000
    tinkHome();
    rt.setCamera(2562.07f, 50.1f, 3173.9f, -90.0f);  // 0x4520211f 0x42486666 0x45465e66 0xc2b40000
    VoiceOver::get().play("WAMI_VO_PETpreclue");
    while (VoiceOver::get().isPlaying("WAMI_VO_PETpreclue")) wait(0);
    playing = true;
    prompt.start();                                  // +0x30
    show_cursor();                                   // 0x4119d0
    for (;;) {
        switch (state) {
        case 0: step(); break;
        case 3:                                      // a wrong set: once its lines end, a new puzzle
            if (frame_time() - clue_t > 2000 && !wrong.active()) {   // v5
                held(false);                         // 0x491db0
                makePuzzle();
                for (int &s : slots) s = -1;
                state = 0;
                clue_t = frame_time() + kFirstClue - kClueEvery;
                for (int i = 0; i < 16; i++) gridShow(i, true);
                for (int k = 0; k < 5; k++) answerShow(k, 0, false);
                last_clue = 9999999;
            }
            break;
        default: break;
        }
        if (tada->isPlaying()) {                     // 0x41fec0: solved
            playing = false;
            prompt.stop();                           // +0x34
            st.saves.setTreasure(true);              // 0x4eadc0
            st.returning = true;                     // +0x1819
            hide_cursor();                           // 0x4119e0
            music->stop();                           // 0x41fe70
            movie3.start();                          // 0x4975e0
            while (movie3.running) wait(0);          // +0x4ba4
            tada->stop();
            st.setTinkHome(2795.0f, 600.0f, 2950.0f);
            tinkHome();
            music->play();
            movie2.start();                          // 0x495260
            while (movie2.running) wait(0);          // +0x3ef4
        }
        wait(0);
    }
}

// v9 0x491a20: the mouse (with the cursor shown) to v25, the keyboard to v26.
void Treasure_Scene::notify2(Service *s) {
    if (s == &mouse && g_cursor_graphic && g_cursor_graphic->visible) onMouse();
    if (s == &keys) onKey();
}

// v26 0x491580: the skip key skips the playing film (the ending: to the sign-in room).
bool Treasure_Scene::onKey() {
    State &st = Game::state();
    if (!g_key.down || (int)g_key.vk != st.skip_key) return false;   // 0x5a6dd8, 0x5a6d9c, +0x74
    if (getenv("TB_DEBUG_TREASURE"))                 // test aid
        tb_log("treasure: skip key (%u) film %d open %d ending %d", frame_time(), (int)movie.active(),
               (int)movie3.running, (int)movie2.running);
    if (movie.active()) {                            // v5
        movie.skip();
        return true;
    }
    if (movie3.running) {                            // +0x4ba4
        movie3.skip();
        return true;
    }
    if (movie2.running) {                            // +0x3ef4
        movie2.skip();
        change_scene("signin");                      // 0x41ddf0
        return true;
    }
    return false;
}

// v25 0x491610: a click; else the plain cursor back (when the ending is not waiting) and the corner buttons' cursor
// unless a tile is held.
void Treasure_Scene::onMouse() {
    if (mouse_down()) {
        click();
        return;
    }
    State &st = Game::state();
    if (!movie.mouseOn() && !movie2.ending.active() && g_cursor_graphic == st.cursor_active)   // 0x4950f0, +0x43a8
        set_cursor(st.cursor, cur_vis());
    Graphic *c = cursorUpdate();                     // v30
    if (c && state != 1) set_cursor(c, cur_vis());
}

// 0x4916b0: the bell (Tink shows the clue's tile while playing; else her "no help"), the map, the puzzle.
void Treasure_Scene::click() {
    if (bell.click() && playing) {                   // 0x4e7180
        bellHelp();
        return;
    }
    if (bell.click() && !playing) {
        tink->clearQueue();                          // 0x4ddf30
        tink->noHelp();                              // 0x4dc1d0
        tinkHome();
        return;
    }
    int r = map.click(g_mouse.x, g_mouse.y, true);   // 0x4efc90
    if (r) {
        if (!playing || r == 2) change_scene("map");
        return;
    }
    if (!playing) return;
    if (state == 0) pick();
    else if (state == 1) place();
}

void Treasure_Scene::setTileCursor(int tile) {
    char b[40];
    snprintf(b, sizeof b, "%s%02d%s", "wami_cu_tile", tile, "_01");   // 0x53e6e0
    tile_cursor.setName(b);                          // 0x424400
    tile_cursor.load();                              // v15
    set_cursor(&tile_cursor, cur_vis());             // 0x4119b0
}

// 0x491790: a tile of the grid, or one put in a slot (that slot is the next to fill), onto the cursor.
void Treasure_Scene::pick() {
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, false);   // 0x42c950
    if (!a) return;
    if (is_grid(a->name.c_str(), &held_tile)) {
        setTileCursor(held_tile + 1);
        state = 1;
        gridShow(held_tile, false);
        return;
    }
    if (is_answer(a->name.c_str(), &next, nullptr)) {
        answerShow(next, 0, false);
        held_tile = slots[next];
        setTileCursor(held_tile + 1);
        state = 1;
        slots[next] = -1;
    }
}

// 0x4918f0: the held tile back to the grid, or into the next slot (any other slot: nothing).
void Treasure_Scene::place() {
    Attachment *a = rt.pickAttached(g_mouse.x, g_mouse.y, true);
    if (!a) return;
    State &st = Game::state();
    if (is_grid(a->name.c_str(), nullptr)) {
        gridShow(held_tile, true);
    } else {
        int k = 0;
        if (!is_answer(a->name.c_str(), &k, nullptr)) return;
        if (k != next) return;
        answerShow(k, held_tile, true);
        slots[k] = held_tile;
    }
    set_cursor(st.cursor, cur_vis());
    tile_cursor.detach();                            // 0x412200
    tile_cursor.unload();                            // v16
    state = 0;
    clue_t = frame_time() + kFirstClue - kClueEvery;
}

// 0x491a60: all five slots filled: right (the door opens: state 2) or wrong (state 3, the lines); else a clue
// every 10 s.
void Treasure_Scene::step() {
    next = -1;
    for (int k = 0; k < 5; k++)
        if (slots[k] == -1) { next = k; break; }
    if (next != -1) {
        if (frame_time() - clue_t > kClueEvery) {
            clue_t = frame_time();
            clue();
        }
        return;
    }
    state = 2;
    for (int k = 0; k < 5; k++)
        if (slots[k] != solution[k]) state = 3;
    if (state == 2) {
        rt.findSprite(kDoor)->anim->play(0, 0);      // 0x403be0
        for (int k = 0; k < 5; k++) answerShow(k, 0, false);
        for (int i = 0; i < 16; i++) gridShow(i, false);
        tada->play();
        opmusic->play();
        map.visible = false;                         // +0x1a1c
        return;
    }
    wrong.start();                                   // 0x4c19e0
    clue_t = frame_time();
}

void Treasure_Scene::gridShow(int i, bool on) {      // 0x491b80
    Sprite3D *d = rt.findSprite(kDoor);
    if (on) d->showAttached(grid_name(i).c_str());   // 0x42f690
    else d->hideAttached(grid_name(i).c_str());      // 0x42f620
}

void Treasure_Scene::answerShow(int k, int j, bool on) {   // 0x491c50
    if (on) {
        rt.findSprite(kDoor)->showAttached(answer_name(k, j).c_str());
        return;
    }
    for (int t = 0; t < 16; t++) rt.findSprite(kDoor)->hideAttached(answer_name(k, t).c_str());
}

void Treasure_Scene::held(bool on) {                 // 0x491db0
    if (on) {
        setTileCursor(held_tile + 1);
        return;
    }
    set_cursor(Game::state().cursor, cur_vis());
    tile_cursor.detach();
    tile_cursor.unload();
}

// 0x491e70: the next slot's clue ("H" for a hard one) unless the prompt is playing; a new slot marks its row.
void Treasure_Scene::clue() {
    char line[32];
    snprintf(line, sizeof line, "%s%i", kClue, solution[next] + 1);   // 0x53e704
    if (hard[next]) strcat(line, "H");               // 0x53e700
    if (!prompt.isPlaying()) VoiceOver::get().play(line);   // 0x4c4e60
    if (last_clue != next) {
        last_clue = next;
        for (int &r : rows) r = 0;
        rows[solution[next] / 4] = 9999999;
    }
}

// 0x491f60: Tink flies to the clue's tile in the grid (4 by 4) and points.
void Treasure_Scene::bellHelp() {
    int s = solution[next];
    float row = (float)(s / 4);
    float col = (float)(s % 4);
    tink->clearQueue();                              // 0x4ddf30
    tink->pushBasic(2, (float)(2620.0f - (double)col * 22.0f), (float)(500.0f - (double)row * 50.0f), 2900.0f);   // 0x515974 0x515978 0x515250 0x515190
    tink->push(new TinkCmdAnimation("GLOB_CA_TNKptea_", nullptr));   // 0x4dc530
    tinkHome();
}

}  // namespace Game
