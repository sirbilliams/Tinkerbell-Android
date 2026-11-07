// The 2D game scenes' base, reconstructed from "Tinker Bell.exe" (see game2d.h).
#include "game2d.h"
#include "state.h"
#include "toolbar.h"
#include "raytrace_scene.h"
#include <stdio.h>
#include <string.h>

namespace Game {

using namespace Vision;

static const uint32_t kKey = 0xffff00ffu;            // 0x44ced0(0xff, 0, 0xff, 0xff): the colour key

// ---------------------------------------------------------------- CloudBank
CloudBank::CloudBank()
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, kKey),   // 0x411b90
      image("glob_ly_cloudbank_01", 0, 0, 0, 0, 0, 0, true) {   // 0x4179e0
    setZ(20);                                        // 0x411e70(0x14)
    mode = 2;                                        // +0x148
}

CloudBank::~CloudBank() {}                           // 0x4ace10

void CloudBank::draw() {                             // 0x4acdc0
    if (!shown) return;
    image.x = x;                                     // +0xf0 = +0x98 +0x58
    image.y = y;
    image.draw();
}

void CloudBank::load() {                             // 0x4ace90
    shown = false;
    image.load();
    shown = true;
    x = 0;
    y = 0;
}

void CloudBank::unload() {                           // 0x4acda0
    Graphic::unload();                               // 0x412020
    image.unload();
}

bool CloudBank::rise() {                             // 0x4acec0
    if (y > -50) {
        mode = 2;
        return false;
    }
    y += 20;
    return true;
}

bool CloudBank::fall() {                             // 0x4acee0
    if (y > 1000) {
        mode = 2;
        return false;
    }
    y += 20;
    return true;
}

void CloudBank::reset() {                            // 0x4acf00
    x = 0;
    y = 0;
    shown = true;
    mode = 1;
}

void CloudBank::cover() {                            // 0x4acf20
    y = -1000;
    x = 0;
    shown = true;
    mode = 0;
}

// ---------------------------------------------------------------- PlayerBadge
static Image *badge_image(const char *n) { return Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true); }

PlayerBadge::PlayerBadge(const char *hd)             // 0x4ad330
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, kKey) {
    if (!hd) hd = "GLOB_IF_PETheadsm_01";
    Saves &sv = Game::state().saves;
    int ch = 1, loc = 0, fc = 0;                     // the defaults when a key is missing
    if (!sv.getInt("Character", &ch)) ch = 1;        // 0x4e9710 (+0x3cc)
    if (!sv.getInt("Location", &loc)) loc = 0;
    if (!sv.getInt("Frame Color", &fc)) fc = 0;
    static const char *const chars[6] = {"GLOB_IF_HOKheadsm_01", "GLOB_IF_TNKlhedsm_01", "GLOB_IF_PLRiconsm_01",
                                         "GLOB_IF_WENheadsm_01", "GLOB_IF_TIGheadsm_01", "GLOB_IF_CUBheadsm_01"};
    static const char *const locs[6] = {"GLOB_IF_london_01", "GLOB_IF_skull_01", "GLOB_IF_indian_01",
                                        "GLOB_IF_mermaid_01", "GLOB_IF_starry_01", "GLOB_IF_lost_01"};
    static const char *const frames[6] = {"GLOB_IF_redframe_01", "GLOB_IF_blframe_01", "GLOB_IF_prframe_01",
                                          "GLOB_IF_orframe_01", "GLOB_IF_yelframe_01", "GLOB_IF_grframe_01"};
    if ((unsigned)ch <= 5) character = badge_image(chars[ch]);   // the jump tables 0x4ad8a8 / 0x4ad8c0 / 0x4ad8d8
    if ((unsigned)loc <= 5) location = badge_image(locs[loc]);
    if ((unsigned)fc <= 5) frame = badge_image(frames[fc]);
    head = badge_image(hd);
    // port: the exe loads all four unchecked (an out-of-range save value would crash it)
    if (frame) frame->load();                        // v15
    if (location) location->load();
    if (character) character->load();
    head->load();
}

PlayerBadge::~PlayerBadge() {                        // 0x4ad910
    for (Image **p : {&frame, &location, &character, &head}) {
        if (*p) {
            (*p)->unload();                          // v16
            delete *p;
        }
        *p = nullptr;
    }
}

void PlayerBadge::draw() {                           // 0x4ada10
    if (frame) frame->draw();                        // port: null checks as in the ctor
    if (location) location->draw();
    if (character) character->draw();
    head->draw();
}

// ---------------------------------------------------------------- ScrollGround
ScrollGround::ScrollGround(const char *a, const char *b)   // 0x4ac0e0
    : Graphic(GRAPHIC_ARGS), img1("", 0, 0, 0, 0, 0, 0, true), img2("", 0, 0, 0, 0, 0, 0, true) {
    strcpy(name1, a);
    strcpy(name2, b);
    speed = 30;                                      // +0xec
    x = 0;
    setZ(1000);                                      // 0x3e8
}

ScrollGround::~ScrollGround() {}                     // 0x4ac230

void ScrollGround::load() {                          // v15 0x4ac2c0
    left_ = right_ = false;
    row1 = 0;
    dx = dy = 0;
    accx = accy = 0;
    autoupdate = true;
    row2 = -960;                                     // 0xfffffc40
    mult = 2;
    img1.setName(name1);                             // 0x424400
    img1.load();
    img2.setName(name2);
    img2.load();
    last = frame_time();
}

void ScrollGround::unload() {                        // v16 0x4ac350
    Graphic::unload();
    img1.unload();
    img2.unload();
}

void ScrollGround::setDir(int d) {                   // 0x4ac380
    if (d == 0) { left_ = false; right_ = false; }
    else if (d == 1) { left_ = true; right_ = false; }
    else if (d == 2) { left_ = false; right_ = true; }
}

// v18 0x4ac3e0: the time since the last call in units of `speed` ms; sideways 3 a unit while a direction is
// held (else stopped), down `mult` a unit, both kept in fractions; the rows wrap at 960, x stays within 64.
void ScrollGround::update() {
    uint32_t now = frame_time();
    uint32_t el = now - last;
    last = now;
    double d = (double)(uint64_t)el / (double)speed; // fild qword, fidiv
    if (left_) accx = (float)(d * 3.0f + accx);      // 0x51329c
    else if (right_) accx = (float)(accx - d * 3.0f);
    else accx = 0;
    double f = d * (double)mult + accy;              // fimul, fadd
    accy = (float)f;                                 // fst (the sum stays on the stack)
    int ix = ftol(accx);
    dx = ix;                                         // +0x108
    int iy = ftol(f);
    dy = iy;                                         // +0x10c
    if (iy) {
        accy = (float)(f - (double)iy);
        row1 += iy;
        if (row1 >= 960) row1 = -960;
        row2 += iy;
        if (row2 >= 960) row2 = -960;
    }
    if (ix) {
        accx = (float)(accx - (double)ix);
        int old = x;
        x = dx + old;
        if (x > 64) x = 64;
        if (x < -64) x = -64;
        dx = x - old;
    }
}

void ScrollGround::draw() {                          // v14 0x4ac550
    if (autoupdate) update();                        // v18
    img1.x = x;
    img2.x = x;
    img1.y = row1 - img1.h / 2 + 240;
    img2.y = row2 - img2.h / 2 + 240;
    img1.draw();
    img2.draw();
}

// ---------------------------------------------------------------- Scoreboard
// 0x4ada80: the background, the digits "GLOB_IF_NUM0bg_01".."9" and the four places "GLOB_IF_btrgt_01".."04".
Scoreboard::Scoreboard(const char *b) : Graphic(GRAPHIC_ARGS), bg(b, 0, 0, 0, 0, 0, 0, true) {
    bg.load();                                       // v15
    char n[32];
    for (int i = 0; i < 10; i++) {
        snprintf(n, sizeof n, "GLOB_IF_NUM%dbg_01", i);
        Image *im = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);
        digits.push_back(im);
        im->load();
    }
    for (int i = 0; i < 4; i++) {
        snprintf(n, sizeof n, "GLOB_IF_btrgt_0%d", i + 1);
        Image *im = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);
        slots.push_back(im);
        im->load();
    }
}

Scoreboard::~Scoreboard() {                          // 0x4add80
    bg.unload();
    for (Image *im : digits) { im->unload(); delete im; }
    digits.clear();
    for (Image *im : slots) { im->unload(); delete im; }
    slots.clear();
}

// v14 0x4adf70: each score's tens (from 10) and units at its two places.
void Scoreboard::draw() {
    bg.draw();
    auto put = [this](int digit, int place) {
        Image *d = digits.at(digit);
        d->x = slots.at(place)->ox;
        d->y = slots.at(place)->oy;
        d->draw();
    };
    if (s1 >= 10) put(s1 / 10, 0);
    put(s1 % 10, 1);
    if (s2 >= 10) put(s2 / 10, 2);
    put(s2 % 10, 3);
}

// ---------------------------------------------------------------- Game2D_Scene
Game2D_Scene::Game2D_Scene(const char *nm)           // 0x4acf40
    : Scene(nm), screen(0, 0, 0, 0, 0, true, true) {}   // 0x41eea0

Game2D_Scene::~Game2D_Scene() {}                     // 0x4acfb0

// 0x4acfd0: the overlay screen (depth -10000), the number images, the map icon (z -1000), the cloud bank, the
// badge (z -1000) in the scene's screen; the fader (hidden) in the overlay, loaded and shown.
void Game2D_Scene::enter() {
    overlay = Gorilla::gnew<Screen>(0, 0, 0, 0, 0, true, true);
    overlay->depth = -10000;                         // +0x6c
    Game::state().openGlobal2d();                    // 0x4eca30
    map = Gorilla::gnew<NeverlandMapIcon>();         // 0x4ef8e0
    map->load();                                     // v15
    map->setZ(-1000);
    screen.layer(0)->add(map);                       // 0x41ece0, 0x41e930
    clouds = Gorilla::gnew<CloudBank>();
    clouds->load();
    clouds->setZ(-900);                              // 0xfffffc7c
    screen.layer(0)->add(clouds);
    badge = Gorilla::gnew<PlayerBadge>(nullptr);     // 0x4ad330
    badge->setZ(-1000);
    screen.layer(0)->add(badge);
    fader = Gorilla::gnew<SceneFader>();             // 0x4a9410
    overlay->layer(0)->add(fader);
    fader->visible = false;                          // +0x7c
    overlay->loadAll();                              // 0x41ec40
    overlay->activate();                             // v6
}

// v20 0x4ad270
void Game2D_Scene::leave() {
    if (clouds) clouds->unload();                    // v16
    delete clouds;                                   // +0xc
    clouds = nullptr;
    if (map) map->unload();
    delete map;
    map = nullptr;
    delete badge;
    badge = nullptr;
    if (overlay) {
        overlay->unloadAll();                        // 0x41ec00
        overlay->deactivate();                       // v7
    }
    delete overlay;
    overlay = nullptr;
    delete fader;                                    // (after its screen in the exe too)
    fader = nullptr;
    Game::state().closeGlobal2d();                   // 0x4ecba0
}

}  // namespace Game
