// The map of Neverland, reconstructed from "Tinker Bell.exe" (see map.h).
#include "map.h"
#include "state.h"
#include "../vision/engine.h"
#include <stdio.h>
#include <string.h>

namespace Game {

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

// Animation::setFrame(0) + 0x403be0(0, 1) (looping from now)
static void restart(Animation *a) {
    a->setFrame(0);
    a->start_time = frame_time();
    a->mode = 1;
}

// ---------------------------------------------------------------- MapButton
MapButton::MapButton(const char *base)               // 0x4a6470
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), idle(nullptr), rov(nullptr),
      nono("", 0, 0, 0, 0, 0, 0, true) {             // 0x5464a0: an empty name
    if (strcmp("GLOB_LY_Xspot_", base) == 0) {       // the X spot: one animation, looping
        idle.prefix = base; idle.setName(base);
        rov.prefix = base; rov.setName(base);
        idle.mode = 1;                               // +0x19c
    } else {
        char b[260];
        snprintf(b, sizeof b, "%s%s_", base, "idl");   // "%s%s_"
        idle.prefix = b; idle.setName(b);
        snprintf(b, sizeof b, "%s%s_", base, "rov");
        rov.prefix = b; rov.setName(b);
    }
    idle.load();                                     // v15
    idle.setHit(1, true);
    rov.load();
    idle.setHit(1, true);                            // (the exe sets the idle's twice, not the roll-over's)
    rov.mode = 1;                                    // +0x2b4
    z = 100;                                         // 0x411e70(100)
    if (layer) layer->sort();
    show(&idle);
    x = idle.x;                                      // +0xf0 +0xf4
    y = idle.y;
}

MapButton::~MapButton() {                            // 0x4a66a0
    pause.stop();                                    // +0x34
    idle.unload();
    rov.unload();
    nono.unload();
}

void MapButton::show(Animation *a) {                 // 0x4a6970 / 0x4a69b0
    if (shown == a) return;
    restart(a);
    shown = a;
}
void MapButton::showRov() { show(&rov); }
void MapButton::showIdle() { show(&idle); }

bool MapButton::hit(int px, int py) {                // v11 0x4a69f0
    if (!enabled || !visible) return false;
    return idle.hit(px, py) || rov.hit(px, py);
}

void MapButton::draw() {                             // v14 0x4a67c0
    if (!visible) return;
    if (strcmp(nono.name(), "") != 0) { nono.draw(); return; }   // 0x423c60(0x5464a0, 0)
    shown->draw();
}

// 0x4a6860: under the mouse: lit (when enabled, with the hand cursor) and a click says so.
bool MapButton::update() {
    if (idle.hit(g_mouse.x, g_mouse.y) || rov.hit(g_mouse.x, g_mouse.y)) {
        if (enabled) {
            show(&rov);
            set_cursor(Game::state().cursor_active, g_cursor_graphic ? g_cursor_graphic->visible : false);   // +0x60
            return mouse_down();
        }
        show(&idle);
        return mouse_down();
    }
    show(&idle);
    return false;
}

void MapButton::setPauseLine(const char *line) {     // 0x4a6a60
    pause.set(this, line, 1000);
    pause.start();
}

// ---------------------------------------------------------------- TokenCollectionDisplay
TokenCollectionDisplay::TokenCollectionDisplay(const char *prefix, const char *sg)   // 0x4a6a90
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), sign(sg, 0, 0, 0, 0, 0, 0, true) {
    for (int i = 0; i < 5; i++) {
        char b[260];
        snprintf(b, sizeof b, "%spuzp%1.1d_01", prefix, i + 1);
        piece[i] = new Image(b, 0, 0, 0, 0, 0, 0, true);   // 0x453740 + 0x424400
    }
}

TokenCollectionDisplay::~TokenCollectionDisplay() {  // 0x4a6bb0
    for (Image *&p : piece) { delete p; p = nullptr; }
}

void TokenCollectionDisplay::load() {                // v15 0x4a6c40
    for (Image *p : piece) p->load();
    sign.load();
}

void TokenCollectionDisplay::unload() {              // v16 0x4a6c80
    Graphic::unload();
    for (Image *p : piece) p->unload();
    sign.unload();
}

void TokenCollectionDisplay::draw() {                // v14 0x4a6cc0
    int n = count();
    for (int i = 0; i < n; i++) piece[i]->draw();
    if (hasSign()) sign.draw();
}

static int count_of(const int g[5], int a, int b) {
    int n = 0;
    for (int i = 0; i < 5; i++) {
        if (g[i] == a) n++;
        if (g[i] == b) n++;
    }
    return n;
}
int IndianVillageTCD::count() {                      // 0x4a6dd0: the games ended as 4
    int n = 0;
    for (int v : Game::state().games_indv) if (v == 4) n++;
    return n;
}
bool IndianVillageTCD::hasSign() { return Game::state().saves.mapSegment("Village"); }   // 0x4ea870
int StarrySkiesTCD::count() { return count_of(Game::state().games_star, 4, 5); }        // 0x4a6eb0
bool StarrySkiesTCD::hasSign() { return Game::state().saves.mapSegment("Starry"); }     // 0x4ea9f0
int LostBoysTCD::count() { return count_of(Game::state().games_lost, 4, 5); }           // 0x4a6fa0
bool LostBoysTCD::hasSign() { return Game::state().saves.mapSegment("Lost"); }          // 0x4ea970 (inlined)
int SkullRockTCD::count() { return count_of(Game::state().games_skrl, 5, 6); }          // 0x4a7090
bool SkullRockTCD::hasSign() { return Game::state().saves.mapSegment("Skull"); }        // 0x4ea8f0
int MermaidCoveTCD::count() { return count_of(Game::state().games_mcov, 4, 5); }        // 0x4a7180
bool MermaidCoveTCD::hasSign() { return Game::state().saves.mapSegment("Mermaid"); }    // 0x4ea7f0

// ---------------------------------------------------------------- Credits
Credits::Credits()                                   // 0x4a44c0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), task(this),
      back1("GLOB_BK_credback1_01", 0, 0, 0, 0, 0, 0, true), back2("GLOB_BK_credback2_01", 0, 0, 0, 0, 0, 0, true),
      back3("GLOB_BK_credback3_01", 0, 0, 0, 0, 0, 0, true) {
    for (int i = 0; i < 6; i++) {
        char b[32];
        snprintf(b, sizeof b, "GLOB_CR_credits_%02d", i + 1);
        lines[i] = new Image(b, 0, 0, 0, 0, 0, 0, true);   // 0x453740 + 0x424400
    }
    mouse.setClient(&task);                          // 0x41f290
}

Credits::~Credits() {                                // 0x4a46a0
    for (Image *&l : lines) { delete l; l = nullptr; }
}

void Credits::load() {                               // v15 0x4a4770
    pack = ras_open("Credits", "", &data_device());  // 0x419b90
    pack->load();
    ras_set_current(pack);
    back1.load();
    back2.load();
    back3.load();
    for (Image *l : lines) l->load();
}

void Credits::end() {
    task.deactivate();                               // +0x1c
    mouse.deactivate();
    g_modal_client = nullptr;                        // 0x41f380
    visible = false;
}

void Credits::unload() {                             // v16 0x4a47f0
    Graphic::unload();
    if (task.active()) end();                        // +0x14
    back1.unload();                                  // (only the first backdrop: the others go with the pack)
    for (Image *l : lines) l->unload();
    if (pack) ras_close(pack);                       // 0x419620
    pack = nullptr;
}

void Credits::start() {                              // 0x4a4880
    speed = 23;                                      // +0x77c
    last = frame_time();
    for (int i = 0; i < 6; i++) lines[i]->y = 480 * (i + 1);
    task.activate();                                 // +0x18
    mouse.activate();
    g_modal_client = &task;                          // 0x41f370
    back2.y = 0;                                     // +0x25c
    back3.y = 0;                                     // +0x304
}

void Credits::draw() {                               // v14 0x4a48f0
    back1.draw();
    back2.draw();
    back3.draw();
    int td = (int)(frame_time() - last);
    vision_log("td = %d", td);                       // 0x53f718
    last = frame_time();
    for (Image *l : lines) l->y -= td / speed;
    double s = (double)(480 - lines[0]->y);
    if (!(s <= 1600.0)) s = 1600.0;                  // 0x515d50
    back2.y = (int)(s * 0.01875);                    // 0x515d48 (0x4f3948: truncation)
    back3.y = (int)(s * 0.09375);                    // 0x515d40
    for (Image *l : lines)
        if (-480 < l->y && l->y < 480) l->draw();
    if (lines[5]->y < -480 && task.active()) end();
}

void CreditsTask::notify2(Service *s) {              // 0x4a4a40
    if (s == &credits->mouse && mouse_down() && active()) credits->end();
}

// ---------------------------------------------------------------- Map_Scene_Base
Map_Scene_Base::Map_Scene_Base(const char *nm)       // 0x4a5b90
    : Scene(nm), music("nvmp_mu_music"), screen(0, 0, 0, 0, 0, true, true), credit_screen(0, 0, 0, 0, 0, true, true),
      previo("", ""),
      map1("GLOB_BG_nevermap_01", 0, 0, 0, 0, 0, 0, true), map2("GLOB_BG_nevermap_02", 0, 0, 0, 0, 0, 0, true),
      tmap_indv("GLOB_LY_TMAPindv_01", 0, 0, 0, 0, 0, 0, true), tmap_lost("GLOB_LY_TMAPlost_01", 0, 0, 0, 0, 0, 0, true),
      tmap_mcov("GLOB_LY_TMAPmcov_01", 0, 0, 0, 0, 0, 0, true), tmap_skrl("GLOB_LY_TMAPskrl_01", 0, 0, 0, 0, 0, 0, true),
      tmap_star("GLOB_LY_TMAPstar_01", 0, 0, 0, 0, 0, 0, true) {
    keys.setClient(this);                            // 0x41f290
    mouse.setClient(this);
}

Map_Scene_Base::~Map_Scene_Base() { deleteButtons(); }   // 0x4a5f10 (port: the buttons if a leave never came)

void Map_Scene_Base::notify2(Service *s) {           // v9 0x4a52f0
    if (current_scene() != this) return;
    if (s == &mouse) onMouse();                      // v22
    if (s == &keys) onKey();                         // v23
}

void Map_Scene_Base::load2() {                       // v18 0x4a5330
    Graphic *g[7] = {&map1, &map2, &tmap_indv, &tmap_lost, &tmap_mcov, &tmap_skrl, &tmap_star};
    for (Graphic *x : g) screen.layer(0)->add(x);
    music.mode = 2;                                  // +0x410: streamed from its file
    music.type = 6;                                  // +0x418: looping
    credit_screen.depth = -1000;                     // +0xd24
    credit_screen.layer(0)->add(&credits);
}

void Map_Scene_Base::enter() {                       // v19 0x4a5930
    pack = ras_open("map", "", &data_device());      // 0x419b90 (+0xd0)
    pack->load();
    ras_set_current(pack);
    screen.activate();                               // +0x18
    screen.loadAll();                                // 0x41ec40
    map1.z = 1000;                                   // 0x411e70(1000)
    if (map1.layer) map1.layer->sort();
    map2.z = 1000;
    if (map2.layer) map2.layer->sort();
    makeButtons();
    TokenCollectionDisplay *t[5] = {&tcd_indv, &tcd_star, &tcd_lost, &tcd_skrl, &tcd_mcov};
    for (TokenCollectionDisplay *d : t) {
        d->load();                                   // v15
        screen.layer(0)->add(d);
    }
    setMode(0);                                      // v24
    activate();                                      // v6
    music.open();                                    // 0x41f9d0(0)
    music.play();
    credit_screen.loadAll();
    credit_screen.activate();
    credits.visible = false;                         // +0x5b4
}

void Map_Scene_Base::leave() {                       // v20 0x4a5a90
    VoiceOver::get().stop();                         // 0x4c9260
    deactivate();                                    // +0x1c
    mouse.deactivate();
    keys.deactivate();
    screen.deactivate();
    screen.unloadAll();                              // 0x41ec00
    deleteButtons();                                 // 0x4a5850
    TokenCollectionDisplay *t[5] = {&tcd_indv, &tcd_star, &tcd_lost, &tcd_skrl, &tcd_mcov};
    for (TokenCollectionDisplay *d : t) d->unload();
    if (pack) ras_close(pack);                       // 0x419620
    pack = nullptr;
    music.close();                                   // 0x41fc50
    credit_screen.unloadAll();
    credit_screen.deactivate();
    previo.deactivate();                             // 0x4c1400
    set_cursor(Game::state().cursor, true);                // 0x4119b0(+0x5c, 1)
}

// 0x4a6390: 0 the map (its buttons and tokens), 1 the treasure map (the segments found).
void Map_Scene_Base::setMode(int m) {
    mode = m;                                        // +0xd4
    if (m == 0) {
        showButtons(true);
        map2.visible = false;                        // +0xf34
        tmap_indv.visible = tmap_lost.visible = tmap_mcov.visible = tmap_skrl.visible = tmap_star.visible = false;
    } else if (m == 1) {
        showButtons(false);
        map2.visible = true;
        Saves &sv = Game::state().saves;
        tmap_indv.visible = sv.mapSegment("Village");   // 0x4ea870
        tmap_lost.visible = sv.mapSegment("Lost");      // 0x4ea970
        tmap_mcov.visible = sv.mapSegment("Mermaid");   // 0x4ea7f0
        tmap_skrl.visible = sv.mapSegment("Skull");     // 0x4ea8f0
        tmap_star.visible = sv.mapSegment("Starry");    // 0x4ea9f0
    }
}

void Map_Scene_Base::makeButtons() {                 // 0x4a5420
    Layer *l = screen.layer(0);
    struct B { MapButton **p; const char *base; };
    auto make = [&](MapButton **p, const char *base) { *p = new MapButton(base); l->add(*p); };
    make(&indv, "glob_bu_indv");                     // +0x504
    make(&mcov, "glob_bu_mcov");
    make(&skrl, "glob_bu_skrl");
    make(&star, "glob_bu_star");
    make(&lost, "glob_bu_lost");
    make(&crd, "glob_bu_crd");                       // +0x518
    make(&pik, "glob_bu_pik");                       // +0x51c
    bool treasure = false;
    if (!Game::state().saves.hasTreasure(&treasure) || !treasure) {   // 0x4eae10
        pik->nono.setName("GLOB_BU_PIKnono_01");     // 0x424400
        pik->nono.load();                            // v15
    }
    make(&quit, "glob_bu_quit");                     // +0x520
    make(&sign, "glob_bu_sign");
    make(&tmap, "glob_bu_tmap");                     // +0x528
    if (!Game::state().saves.anyMapSegment()) {            // 0x4eabb0
        tmap->nono.setName("GLOB_BU_TMAPnono_01");
        tmap->nono.load();
    }
    make(&rtrn, "glob_bu_rtrn");                     // +0x52c
    make(&xspot, "GLOB_LY_Xspot_");                  // +0x530
}

void Map_Scene_Base::deleteButtons() {               // 0x4a5850 (v3(1) each; the pointers are left)
    MapButton **b[12] = {&indv, &mcov, &skrl, &star, &lost, &crd, &pik, &quit, &sign, &tmap, &rtrn, &xspot};
    for (MapButton **p : b) { delete *p; *p = nullptr; }   // port: cleared so a second leave can't free twice
}

// 0x4a6280: the map, its buttons and tokens; the X spot only with "Has X Spot" and no treasure yet.
void Map_Scene_Base::showButtons(bool b) {
    map1.visible = b;                                // +0xe8c
    MapButton *bs[11] = {indv, mcov, skrl, star, lost, crd, pik, quit, sign, tmap, rtrn};
    for (MapButton *x : bs) x->visible = b;
    bool spot = false, treasure = false;
    Game::state().saves.hasXSpot(&spot);                   // 0x4ea690 (when missing: left false)
    if (Game::state().saves.hasTreasure(&treasure)) {}     // 0x4eae10
    if (spot && !treasure) {
        xspot->visible = b;
        xspot->enabled = b;
    } else {
        xspot->visible = false;
        xspot->enabled = false;
    }
    tcd_indv.visible = tcd_star.visible = tcd_lost.visible = tcd_skrl.visible = tcd_mcov.visible = b;
}

// ---------------------------------------------------------------- MapTutorialCursor
MapTutorialCursor::MapTutorialCursor()               // (0x4a7ce0's inlined ctor)
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    normal = Game::state().cursor;                         // +0x5c
    active = Game::state().cursor_active;                  // +0x60
    static_cast<Animation *>(active)->start_time = frame_time();   // 0x403be0(0, 1)
    static_cast<Animation *>(active)->mode = 1;
    cur = normal;
}

MapTutorialCursor::~MapTutorialCursor() {}           // 0x4a9110

void MapTutorialCursor::draw() { cur->draw(); }      // v14 0x4a9170

// 0x4a9180: a fifth of the way in x and y (at least one pixel) toward the target; false when there.
bool MapTutorialCursor::move() {
    if (cur->x == tx && cur->y == ty) return false;
    int dx = tx - cur->x, dy = ty - cur->y;
    if (dx != 0) {
        int one = dx / (dx < 0 ? -dx : dx);
        dx /= 5;
        if (dx == 0) dx = one;
    }
    if (dy != 0) {
        int one = dy / (dy < 0 ? -dy : dy);
        dy /= 5;
        if (dy == 0) dy = one;
    }
    cur->x += dx;
    x = cur->x;
    cur->y += dy;
    y = cur->y;
    return true;
}

// ---------------------------------------------------------------- MapTutorial_Scene
MapTutorial_Scene::MapTutorial_Scene() : Map_Scene_Base("map_tutorial") {}   // 0x4a9340

MapTutorial_Scene::~MapTutorial_Scene() {            // 0x4a93a0
    for (Sound *s : lines) delete s;                 // port: the exe frees the array only (its leave freed them)
    lines.clear();
    delete cursor;
}

// v19 0x4a7ce0: Peter's lines, the cursor that takes the tour.
void MapTutorial_Scene::enter() {
    Map_Scene_Base::enter();
    static const char *const names[11] = {
        "LOST_VO_PETtnvmap", "LOST_VO_PETmaphlp", "LOST_VO_PETmaphl2", "LOST_VO_PETmaphl3", "LOST_VO_PETquit",
        "LOST_VO_PETnwgame", "LOST_VO_PETreturn", "LOST_VO_PETmphlid", "GLOB_FX_GSclick", "GLOB_VO_PETtogon",
        "GLOB_VO_PETpicka"};
    for (const char *n : names) lines.push_back(new Sound(n));   // 0x420210 + 0x430dd0
    for (Sound *s : lines) s->open();                // 0x41f9d0(0)
    hide_cursor();                                   // 0x4119e0
    cursor = new MapTutorialCursor();
    screen.layer(0)->add(cursor);
    cursor->cur->x = 160;                            // 0xa0
    cursor->x = 160;
    cursor->cur->y = 120;                            // 0x78
    cursor->y = 120;
    cursor->visible = true;
    keys.activate();                                 // +0x454
}

void MapTutorial_Scene::leave() {                    // v20 0x4a81e0
    keys.deactivate();
    Map_Scene_Base::leave();
    for (Sound *&s : lines) {
        s->close();                                  // 0x41fc50
        delete s;                                    // v3(1)
        s = nullptr;
    }
    lines.clear();
    if (cursor) {
        cursor->detach();                            // 0x412200
        delete cursor;
    }
    cursor = nullptr;
}

void MapTutorial_Scene::onKey() {                    // v23 0x4a81a0
    if (!g_key.down) return;
    if ((int)g_key.vk != Game::state().skip_key) return;   // +0x74
    Game::state().start_in_london = false;                 // +0x1818
    Game::state().tutorial_skipped = true;                 // +0x7bc
    change_scene("lost");                            // 0x41ddf0
}

// 0x4a8f10: each button the cursor is on lit, the others not.
void MapTutorial_Scene::highlight() {
    MapButton *bs[10] = {lost, mcov, skrl, star, indv, quit, sign, tmap, pik, rtrn};
    for (MapButton *b : bs) {
        if (b->hit(cursor->x, cursor->y)) b->showRov();
        else b->showIdle();
    }
}

void MapTutorial_Scene::moveTo(int px, int py) {
    cursor->tx = px;                                 // +0xa4
    cursor->ty = py;                                 // +0xa8
    while (cursor->move()) {                         // 0x4a9180
        highlight();
        wait(30);
    }
}

void MapTutorial_Scene::playWait(int i) {
    lines.at((size_t)i)->play();                     // (0x4a9230: out of range throws)
    while (lines.at((size_t)i)->isPlaying()) wait(30);   // 0x41fec0
}

// v10 0x4a82f0: Peter shows the five areas, then the buttons along the bottom, and goes on to the hideout.
void MapTutorial_Scene::run() {
    wait(1);
    lines.at(0)->play();                             // "tnvmap"
    moveTo(lost);
    moveTo(mcov);
    moveTo(skrl);
    moveTo(star);
    moveTo(indv);
    while (lines.at(0)->isPlaying()) wait(30);
    moveTo(lost);
    lines.at(2)->play();                             // "maphl2": the buttons blink
    while (lines.at(2)->isPlaying()) {
        bool on = frame_time() % 1000 < 500;
        MapButton *bs[5] = {pik, quit, sign, tmap, rtrn};
        for (MapButton *b : bs) {
            if (on) b->showRov();
            else b->showIdle();
        }
        wait(30);
    }
    highlight();
    lines.at(3)->play();                             // "maphl3": the return button blinks
    while (lines.at(3)->isPlaying()) {
        if (frame_time() % 1000 < 500) rtrn->showRov();
        else rtrn->showIdle();
        wait(30);
    }
    moveTo(227, 163);                                // 0xe3, 0xa3
    playWait(4);                                     // "quit"
    moveTo(163, 163);
    playWait(9);                                     // "togon"
    moveTo(97, 163);
    playWait(5);                                     // "nwgame"
    moveTo(28, 163);
    playWait(6);                                     // "return"
    moveTo(-22, 163);
    playWait(10);                                    // "picka"
    mouse.deactivate();                              // +0x420
    playWait(7);                                     // "mphlid"
    moveTo(28, 163);
    playWait(8);                                     // the click
    change_scene("lost");                            // 0x41ddf0
    for (;;) wait(30);
}

}  // namespace Game
