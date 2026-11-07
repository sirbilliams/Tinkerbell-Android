// The 2D game scenes' base, reconstructed from "Tinker Bell.exe" (see game2d.h).
#include "game2d.h"
#include "state.h"
#include "toolbar.h"
#include "raytrace_scene.h"

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
